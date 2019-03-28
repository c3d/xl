// *****************************************************************************
// gc.cpp                                                             XL project
// *****************************************************************************
//
// File description:
//
//     Garbage collector and memory management
//
//     Garbage collection in XL is based on reference counting.
//     The GCPtr class does the reference counting.
//     The rule is that as soon as you assign an object to a GCPtr,
//     it becomes "tracked". Objects created during a cycle and not assigned
//     to a GCPtr by the next cycle are an error, which is flagged
//     in debug mode.
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2010-2012,2015-2019, Christophe de Dinechin <cdedinechin@dxo.com>
// (C) 2011-2012, Jérôme Forissier <jerome@taodyne.com>
// (C) 0,
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// XL is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with XL, in a file named COPYING.
// If not, see <https://www.gnu.org/licenses/>.
// *****************************************************************************

#include "config.h"
#include "gc.h"
#include "options.h"

#include <recorder/recorder.h>
#include <valgrind/memcheck.h>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <pthread.h>

// Windows/MinGW (ancient): When getting in the way becomes an art form...
#if !defined(HAVE_POSIX_MEMALIGN) && defined(HAVE_MINGW_ALIGNED_MALLOC)
#include <malloc.h>
#endif // HAVE_POSIX_MEMALIGN


RECORDER(memory, 256, "Memory related events and garbage collection");

XL_BEGIN

// ============================================================================
//
//    Allocator Base class
//
// ============================================================================

void *TypeAllocator::lowestAddress = (void *) ~0;
void *TypeAllocator::highestAddress = (void *) 0;
void *TypeAllocator::lowestAllocatorAddress = (void *) ~0;
void *TypeAllocator::highestAllocatorAddress = (void *) 0;
Atomic<uint> TypeAllocator::finalizing = 0;

// Identifier of the thread currently collecting if any
#define PTHREAD_NULL ((pthread_t) 0)
static pthread_t collecting = PTHREAD_NULL;

RECORDER_DEFINE(memory, 64, "Memory allocation and garbage collector");


TypeAllocator::TypeAllocator(kstring tn, uint os)
// ----------------------------------------------------------------------------
//    Setup an empty allocator
// ----------------------------------------------------------------------------
    : gc(NULL), name(tn), locked(0), lowestInUse(~0UL), highestInUse(0),
      chunks(), freeList(NULL), toDelete(NULL),
      available(0), freedCount(0),
      chunkSize(1022), objectSize(os), alignedSize(os),
      allocatedCount(0), scannedCount(0), collectedCount(0), totalCount(0)
{
    record(memory, "New type allocator %p name '%s' object size %u",
           this, tn, os);

    // Make sure we align everything on Chunk boundaries
    if ((alignedSize + sizeof (Chunk)) & CHUNKALIGN_MASK)
    {
        // Align total size up to 8-bytes boundaries
        uint totalSize = alignedSize + sizeof(Chunk);
        totalSize = (totalSize + CHUNKALIGN_MASK) & ~CHUNKALIGN_MASK;
        alignedSize = totalSize - sizeof(Chunk);
    }

    // Use the address of the garbage collector as signature
    gc = GarbageCollector::CreateSingleton();

    // Register the allocator with the garbage collector
    gc->Register(this);

    // Make sure that we have the correct alignment
    XL_ASSERT(this == ValidPointer(this));

    // Update allocator addresses
    if (lowestAllocatorAddress > this)
        lowestAllocatorAddress = (void *) this;
    char *highMark = (char *) this + sizeof(TypeAllocator);
    if (highestAllocatorAddress < (void *) highMark)
        highestAllocatorAddress = (void *) highMark;

    VALGRIND_CREATE_MEMPOOL(this, 0, 0);
}


TypeAllocator::~TypeAllocator()
// ----------------------------------------------------------------------------
//   Delete all the chunks we allocated
// ----------------------------------------------------------------------------
{
    record(memory, "Destroy type allocator %p '%+s'", this, this->name);

    VALGRIND_DESTROY_MEMPOOL(this);

    for (Chunks::iterator c = chunks.begin(); c != chunks.end(); c++)
        free((void *) *c);
}


void *TypeAllocator::Allocate()
// ----------------------------------------------------------------------------
//   Allocate a chunk of the given size
// ----------------------------------------------------------------------------
{
    record(memory, "Allocate in '%+s', free list %p",
           this->name, (void *) freeList.Get());

    Chunk_vp result;
    do
    {
        result = freeList;
        while (!result)
        {
            // Make sure only one thread allocates chunks
            uint wasLocked = locked++;
            if (wasLocked)
            {
                locked--;
                result = freeList;
                continue;
            }

            // Nothing free: allocate a big enough chunk
            size_t  itemSize  = alignedSize + sizeof(Chunk);
            size_t  allocSize = (chunkSize + 1) * itemSize;

            void   *allocated = malloc(allocSize);
            (void)VALGRIND_MAKE_MEM_NOACCESS(allocated, allocSize);

            record(memory, "New chunk %p in '%+s'", allocated, this->name);

            char *chunkBase = (char *) allocated + alignedSize;
            Chunk_vp last = (Chunk_vp) chunkBase;
            Chunk_vp free = result;
            for (uint i = 0; i < chunkSize; i++)
            {
                Chunk_vp ptr = (Chunk_vp) (chunkBase + i * itemSize);
                VALGRIND_MAKE_MEM_UNDEFINED(&ptr->next,sizeof(ptr->next));
                ptr->next = free;
                free = ptr;
            }

            // Update the chunks list
            chunks.push_back((Chunk *) allocated);
            available += chunkSize;
            if (lowestAddress > allocated)
                lowestAddress = allocated;
            char *highMark = (char *) allocated + (chunkSize+1) * itemSize;
            if (highestAddress < (void *) highMark)
                highestAddress = highMark;

            // Update the freelist
            while (!freeList.SetQ(result, free))
            {
                result = freeList;
                last->next = result;
            }

            // Unlock the chunks
            --locked;

            // Read back the free list
            result = freeList;
        }
    }
    while (!freeList.SetQ(result, result->next));

    VALGRIND_MAKE_MEM_UNDEFINED(result, sizeof(Chunk));
    result->allocator = this;
    result->bits |= IN_USE;     // Mark it as in use for current collection
    result->count = 0;
    UpdateInUseRange(result);
    allocatedCount++;
    if (--available < chunkSize * 0.9)
        gc->MustRun();

    void *ret =  (void *) &result[1];
    VALGRIND_MEMPOOL_ALLOC(this, ret, objectSize);

    record(memory, "Allocated %p from %+s", ret, name);
    return ret;
}


void TypeAllocator::Delete(void *ptr)
// ----------------------------------------------------------------------------
//   Free a chunk of the given size
// ----------------------------------------------------------------------------
{
    record(memory, "Delete %p in '%+s'", ptr, this->name);

    if (!ptr)
        return;

    Chunk_vp chunk = (Chunk_vp) ptr - 1;
    XL_ASSERT(IsGarbageCollected(ptr) &&
                 "Deleted pointer not managed by GC");
    XL_ASSERT(IsAllocated(ptr) &&
                 "Deleted GC pointer that was already freed");
    XL_ASSERT(!chunk->count &&
                 "Deleted pointer has live references");

    // Put the pointer back on the free list
    do
    {
        chunk->next = freeList;
    }
    while (!freeList.SetQ(chunk->next, chunk));
    available++;
    freedCount++;

#ifdef DEBUG
    // Scrub all the pointers
    uint32 *base = (uint32 *) ptr;
    uint32 *last = (uint32 *) (((char *) ptr) + alignedSize);
    VALGRIND_MAKE_MEM_UNDEFINED(ptr, alignedSize);
    for (uint *p = base; p < last; p++)
        *p = 0xDeadBeef;
#endif

    VALGRIND_MEMPOOL_FREE(this, ptr);
}


void TypeAllocator::Finalize(void *ptr)
// ----------------------------------------------------------------------------
//   We should never reach this one
// ----------------------------------------------------------------------------
{
    std::cerr << "No finalizer installed for " << ptr << "\n";
    XL_ASSERT(!"No finalizer installed");
}


void TypeAllocator::ScheduleDelete(TypeAllocator::Chunk_vp ptr)
// ----------------------------------------------------------------------------
//   Delete now if possible, or record that we will need to delete it later
// ----------------------------------------------------------------------------
{
    RECORD(memory, "Schedule delete %p (bits %lx)", (void *)(ptr+1), ptr->bits);
    if (ptr->bits & IN_USE)
    {
        UpdateInUseRange(ptr);
    }
    else
    {
        XL_ASSERT(ptr->count == 0 && "Deleting referenced object");
        TypeAllocator *allocator = ValidPointer(ptr->allocator);

        if (allocator->finalizing)
        {
            // Put it on the to-delete list to avoid deep recursion
            LinkedListInsert(allocator->toDelete, ptr);
        }
        else
        {
            // Delete current object immediately
            allocator->Finalize((void *) (ptr + 1));

            // Delete the children put on the toDelete list
            GarbageCollector::Sweep();
        }
    }
}


bool TypeAllocator::CheckLeakedPointers()
// ----------------------------------------------------------------------------
//   Check if any pointers were allocated and not captured between safe points
// ----------------------------------------------------------------------------
{
    record(memory, "CheckLeaks in '%+s'", name);

    char *lo = (char *) lowestInUse.Get();
    char *hi = (char *) highestInUse.Get();

    lowestInUse.Set((uintptr_t) lo, ~0UL);
    highestInUse.Set((uintptr_t) hi, 0UL);

    uint  collected = 0;
    totalCount = 0;
    for (Chunks::iterator chk = chunks.begin(); chk != chunks.end(); chk++)
    {
        char   *chunkBase = (char *) *chk + alignedSize;
        size_t  itemSize  = alignedSize + sizeof(Chunk);
        char   *chunkEnd = chunkBase + itemSize * chunkSize;
        totalCount += chunkSize;

        if (chunkBase <= hi && chunkEnd  >= lo)
        {
            char *start = (char *) chunkBase;
            char *end = (char *) chunkEnd;
            if (start < lo)
                start = lo;
            if (end > hi)
                end = hi;
            scannedCount += (end - start) / itemSize;

            for (char *addr = start; addr < end; addr += itemSize)
            {
                Chunk_vp ptr = (Chunk_vp) addr;
                if (AllocatorPointer(ptr->allocator) == this)
                {
                    Atomic<uintptr_t>::And(ptr->bits, ~(uintptr_t) IN_USE);
                    if (!ptr->count)
                    {
                        // It is dead, Jim
                        Finalize((void *) (ptr+1));
                        collected++;
                    }
                }
            }
        }
    }

    collectedCount += collected;
    record(memory, "CheckLeaks in '%+s' done, scanned %u, collected %u",
           name, scannedCount, collected);
    return collected;
}


bool TypeAllocator::Sweep()
// ----------------------------------------------------------------------------
//    Remove all the things that we have pushed on the toDelete list
// ----------------------------------------------------------------------------
{
    record(memory, "Sweep '%+s'", name);
    bool result = false;
    while (toDelete)
    {
        Chunk_vp next = LinkedListPopFront(toDelete);
        next->allocator = this;
        Finalize((void *) (next+1));
        result = true;
    }
    record(memory, "Swept '%+s' %+s objects deleted",
           name, result ? "with" : "without");
    return result;
}


void TypeAllocator::ResetStatistics()
// ----------------------------------------------------------------------------
//    Reset the statistics counters
// ----------------------------------------------------------------------------
{
    freedCount -= freedCount;
    allocatedCount = 0;
    scannedCount = 0;
    collectedCount = 0;
    totalCount = 0;
}


void *TypeAllocator::operator new(size_t size)
// ----------------------------------------------------------------------------
//   Force 16-byte alignment not guaranteed by regular operator new
// ----------------------------------------------------------------------------
{
    void *result = NULL;
#if defined(HAVE_POSIX_MEMALIGN)
    // Real operating systems
    if (posix_memalign(&result, PTR_MASK+1, size))
        throw std::bad_alloc();
#elif defined(HAVE_MINGW_ALIGNED_MALLOC)
    // Ancient versions of MinGW
    result = __mingw_aligned_malloc(size, PTR_MASK+1);
    if (!result)
        throw std::bad_alloc();
#else // don't align
#warning "Unknown platfom - No alignment"
    result = malloc(size);
    if (!result)
        throw std::bad_alloc();
#endif //
    return result;
}


void TypeAllocator::operator delete(void *ptr)
// ----------------------------------------------------------------------------
//    Matching deallocation
// ----------------------------------------------------------------------------
{
#if defined(HAVE_POSIX_MEMALIGN)
    // Normal system
    free(ptr);
#elif defined(HAVE_MINGW_ALIGNED_MALLOC)
    // Brain damaged OS?
    __mingw_aligned_free(ptr);
#else
    // Assume limited brain-damage
    free(ptr);
#endif // WINDOWS vs. rest of the world
}


bool TypeAllocator::CanDelete(void *obj)
// ----------------------------------------------------------------------------
//   Ask all the listeners if it's OK to delete the object
// ----------------------------------------------------------------------------
{
    bool result = true;
    Listeners::iterator i;
    for (i = listeners.begin(); i != listeners.end(); i++)
        if (!(*i)->CanDelete(obj))
            result = false;
    record(memory, "%+s delete %p in '%+s'", result ? "Can" : "Cannot",
           obj, name);
    return result;
}



// ============================================================================
//
//   Garbage Collector class
//
// ============================================================================

GarbageCollector::GarbageCollector()
// ----------------------------------------------------------------------------
//   Create the garbage collector
// ----------------------------------------------------------------------------
    : mustRun(false), running(false)
{}


GarbageCollector::~GarbageCollector()
// ----------------------------------------------------------------------------
//    Destroy the garbage collector
// ----------------------------------------------------------------------------
{
    MustRun();
    Collect();
    Collect();

    Allocators::iterator i;
    for (i = allocators.begin(); i != allocators.end(); i++)
        delete *i;

    // Make sure that destructors down the line won't try something silly
    TypeAllocator::lowestAddress = (void *) ~0;
    TypeAllocator::highestAddress = (void *) 0;
    TypeAllocator::lowestAllocatorAddress = (void *) ~0;
    TypeAllocator::highestAllocatorAddress = (void *) 0;

}


void GarbageCollector::Register(TypeAllocator *allocator)
// ----------------------------------------------------------------------------
//    Record each individual allocator
// ----------------------------------------------------------------------------
{
    allocators.push_back(allocator);
}


bool GarbageCollector::Sweep()
// ----------------------------------------------------------------------------
//    Cleanup all the pending deletions
// ----------------------------------------------------------------------------
{
    bool purging = false;
    Allocators &allocators = gc->allocators;
    for (Allocators::iterator a=allocators.begin(); a!=allocators.end(); a++)
        purging |= (*a)->Sweep();
    return purging;
}


bool GarbageCollector::Collect()
// ----------------------------------------------------------------------------
//   Run garbage collection on all the allocators we own
// ----------------------------------------------------------------------------
{
    pthread_t self = pthread_self();

    // If we get here, we are at a safe point.
    // Only one thread enters collecting, the others spin and wait
    if (Atomic<pthread_t>::SetQ(collecting, PTHREAD_NULL, self))
    {
        record(memory, "Garbage collection in thread %p", self);

        Allocators::iterator a;
        Listeners listeners;
        Listeners::iterator l;

        // Build the listeners from all allocators
        for (a = allocators.begin(); a != allocators.end(); a++)
            for (l = (*a)->listeners.begin(); l != (*a)->listeners.end(); l++)
                listeners.insert(*l);

        // Notify all the listeners that we begin a collection
        for (l = listeners.begin(); l != listeners.end(); l++)
            (*l)->BeginCollection();

        // Cleanup pending purges to maximize the effect of garbage collection
        bool sweeping = true;
        while (sweeping)
        {
            // Check if any object was allocated and not captured at this stage
            for (a = allocators.begin(); a != allocators.end(); a++)
                (*a)->CheckLeakedPointers();
            sweeping = Sweep();
        }

        // Notify all the listeners that we completed the collection
        for (l = listeners.begin(); l != listeners.end(); l++)
            (*l)->EndCollection();

        // Print statistics (inside lock, to increase race pressure)
        if (RECORDER_TRACE(memory))
            PrintStatistics();

        // We are done, mark it so
        mustRun &= 0U;
        if (!Atomic<pthread_t>::SetQ(collecting, self, PTHREAD_NULL))
        {
            XL_ASSERT(!"Someone else stole the collection lock?");
        }

        record(memory, "Finished garbage collection in thread %p", self);
        return true;
    }
    record(memory, "Garbage collection for thread %p was blocked", self);
    return false;
}


void GarbageCollector::PrintStatistics()
// ----------------------------------------------------------------------------
//    Print statistics about collection
// ----------------------------------------------------------------------------
{
    uint tot = 0, alloc = 0, avail = 0, freed = 0, scan = 0, collect = 0;
    printf("%24s %8s %8s %8s %8s %8s %8s\n",
           "NAME", "TOTAL", "AVAIL", "ALLOC", "FREED", "SCANNED", "COLLECT");

    Allocators::iterator a;
    for (a = allocators.begin(); a != allocators.end(); a++)
    {
        TypeAllocator *ta = *a;
        printf("%24s %8u %8u %8u %8u %8u %8u\n",
               ta->name, ta->totalCount,
               ta->available.Get(), ta->allocatedCount,
               ta->freedCount.Get(), ta->scannedCount, ta->collectedCount);
        tot     += ta->totalCount     * ta->alignedSize;
        alloc   += ta->allocatedCount * ta->alignedSize;
        avail   += ta->available      * ta->alignedSize;
        freed   += ta->freedCount     * ta->alignedSize;
        scan    += ta->scannedCount   * ta->alignedSize;
        collect += ta->collectedCount * ta->alignedSize;

        ta->ResetStatistics();
    }
    printf("%24s %8s %8s %8s %8s %8s %8s\n",
           "=====", "=====", "=====", "=====", "=====", "=====", "=====");
    printf("%24s %7uK %7uK %7uK %7uK %7uK %7uK\n",
           "Kilobytes",
           tot >> 10, avail >> 10, alloc >> 10,
           freed >> 10, scan >> 10, collect >> 10);
}


void GarbageCollector::Statistics(uint &total,
                                  uint &allocated, uint &available,
                                  uint &freed, uint &scanned, uint &collected)
// ----------------------------------------------------------------------------
//   Get statistics about garbage collections
// ----------------------------------------------------------------------------
{
    uint tot = 0, alloc = 0, avail = 0, free = 0, scan = 0, collect = 0;
    std::vector<TypeAllocator *>::iterator a;
    for (a = allocators.begin(); a != allocators.end(); a++)
    {
        TypeAllocator *ta = *a;
        tot     += ta->totalCount     * ta->alignedSize;
        alloc   += ta->allocatedCount * ta->alignedSize;
        avail   += ta->available      * ta->alignedSize;
        free    += ta->freedCount     * ta->alignedSize;
        scan    += ta->scannedCount   * ta->alignedSize;
        collect += ta->collectedCount * ta->alignedSize;

        ta->ResetStatistics();
    }

    total     = tot;
    allocated = alloc;
    available = avail;
    freed     = free;
    collected = collect;
}


GarbageCollector *GarbageCollector::gc = NULL;

GarbageCollector *GarbageCollector::CreateSingleton()
// ----------------------------------------------------------------------------
//   Return the garbage collector
// ----------------------------------------------------------------------------
{
    if (!gc)
        gc = new GarbageCollector;
    return gc;
}


void GarbageCollector::Delete()
// ----------------------------------------------------------------------------
//   Delete the garbage collector
// ----------------------------------------------------------------------------
{
    if (gc)
    {
        delete gc;
        gc = NULL;
    }
}


void *GarbageCollector::DebugPointer(void *ptr)
// ----------------------------------------------------------------------------
//   Show allocation information about the given pointer
// ----------------------------------------------------------------------------
{
    using namespace XL;
    if (TypeAllocator::IsGarbageCollected(ptr))
    {
        typedef TypeAllocator::Chunk    Chunk;
        typedef TypeAllocator::Chunks   Chunks;
        typedef TypeAllocator::Chunk_vp Chunk_vp;
        typedef TypeAllocator           TA;

        Chunk_vp chunk = (Chunk_vp) ptr - 1;
        if ((uintptr_t) (void *) chunk & TA::CHUNKALIGN_MASK)
        {
            std::cerr << "WARNING: Pointer " << ptr << " is not aligned\n";
            chunk = (Chunk_vp)
                (((uintptr_t) (void *) chunk) & ~TA::CHUNKALIGN_MASK);
            std::cerr << "         Using " << chunk << " as chunk\n";
        }
        uintptr_t bits = chunk->bits;
        uintptr_t aligned = bits & ~TA::PTR_MASK;
        std::cerr << "Allocator bits: " << std::hex << bits << std::dec
                  << " count=" << chunk->count << "\n";

        GarbageCollector *gc = GarbageCollector::GC();

        TA *alloc = (TA *) aligned;
        bool allocated = alloc->gc == gc;
        if (allocated)
        {
            std::cerr << "Allocated in " << alloc
                      << " (" << alloc->name << ")"
                      << " free=" << alloc->available
                      << " chunks=" << alloc->chunks.size()
                      << " size=" << alloc->chunkSize
                      << " item=" << alloc->objectSize
                      << " (" << alloc->alignedSize << ")"
                      << "\n";
        }

        // Need to walk the GC to see where we belong
        std::vector<TA *>::iterator a;
        uint found = 0;
        for (a = gc->allocators.begin(); a != gc->allocators.end(); a++)
        {
            Chunks::iterator c;
            alloc = *a;
            uint itemBytes = alloc->alignedSize + sizeof(Chunk);
            uint chunkBytes = (alloc->chunkSize+1) * itemBytes;
            uint chunkIndex = 0;
            for (c = alloc->chunks.begin(); c != alloc->chunks.end(); c++)
            {
                char *start = (char *) *c;
                char *end = start + chunkBytes;
                char *base = start + alloc->alignedSize + sizeof(Chunk);

                chunkIndex++;
                if (ptr >= start && ptr <= end)
                {
                    if (!allocated)
                        std::cerr << "Free item in "
                                  << alloc << " (" << alloc->name << ") "
                                  << "chunk #" << chunkIndex << " ";

                    uint freeIndex = 0;
                    Chunk_vp prev = NULL;
                    for (Chunk_vp f = alloc->freeList; f; f = f->next)
                    {
                        freeIndex++;
                        if (f == chunk)
                        {
                            std::cerr << " freelist #" << freeIndex
                                      << " after " << prev << " ";
                            found++;
                        }
                        prev = f;
                    }

                    freeIndex = 0;
                    prev = NULL;
                    for (Chunk_vp f = alloc->toDelete; f; f = f->next)
                    {
                        freeIndex++;
                        if (f == chunk)
                        {
                            std::cerr << " to-delete #" << freeIndex
                                      << " after " << prev << " ";
                            found++;
                        }
                        prev = f;
                    }

                    if (!allocated || found)
                        std::cerr << "\n";
                }

                for (char *addr = start; addr < end; addr += sizeof (void *))
                {
                    void **ref = (void **) addr;
                    if (*ref == ptr)
                    {
                        uint diff = addr-base;
                        uint index = diff / itemBytes;
                        char *obj = base + index * itemBytes;
                        uint offset = addr - obj;
                        std::cerr << "Referenced from " << ref
                                  << " at offset " << offset
                                  << " in item #" << index
                                  << " at addr " << (void *) obj
                                  << "\n";
                    }
                }
            }
        }

        // Check how many times we found the item
        if (allocated)
        {
            if (found)
                std::cerr << "*** Allocated item found " << found
                          << " time(s) in free list (DOUBLE PLUS UNGOOD)\n";
        }
        else if (found != 1)
        {
            if (!found)
                std::cerr << "*** Pointer probably not allocated by us\n";
            else
                std::cerr << "*** Damaged free list, item found " << found
                          << " times (MOSTLY UNFORTUNATE)\n";
        }
    }
    else
    {
        std::cerr << "Pointer " << ptr << " is not dynamically allocated\n";
    }
    return ptr;
}

XL_END


void *xldebug(void *ptr)
// ----------------------------------------------------------------------------
//   Debugger entry point to debug a garbage-collected pointer
// ----------------------------------------------------------------------------
{
    return XL::GarbageCollector::DebugPointer(ptr);
}
