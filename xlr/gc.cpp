// ****************************************************************************
//  gc.cpp                                                          XLR project
// ****************************************************************************
//
//   File Description:
//
//     Garbage collector and memory management
//
//     Garbage collectio in XL is based on reference counting.
//     The GCPtr class does the reference counting.
//     The rule is that as soon as you assign an object to a GCPtr,
//     it becomes "tracked". Objects created during a cycle and not assigned
//     to a GCPtr by the next cycle are an error, which is flagged
//     in debug mode.
//
//
//
// ****************************************************************************
// This document is released under the GNU General Public License, with the
// following clarification and exception.
//
// Linking this library statically or dynamically with other modules is making
// a combined work based on this library. Thus, the terms and conditions of the
// GNU General Public License cover the whole combination.
//
// As a special exception, the copyright holders of this library give you
// permission to link this library with independent modules to produce an
// executable, regardless of the license terms of these independent modules,
// and to copy and distribute the resulting executable under terms of your
// choice, provided that you also meet, for each linked independent module,
// the terms and conditions of the license of that module. An independent
// module is a module which is not derived from or based on this library.
// If you modify this library, you may extend this exception to your version
// of the library, but you are not obliged to do so. If you do not wish to
// do so, delete this exception statement from your version.
//
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#include "gc.h"
#include "options.h"
#include "flight_recorder.h"
#include "valgrind/memcheck.h"

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <pthread.h>

#ifdef CONFIG_MINGW // Windows: When getting in the way becomes an art form...
#include <malloc.h>
#endif // CONFIG_MINGW


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
static pthread_t collecting = NULL;


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
    RECORD(MEMORY, "New type allocator",
           tn, os, "this", (intptr_t) this);

    // Make sure we align everything on Chunk boundaries
    if ((alignedSize + sizeof (Chunk)) & CHUNKALIGN_MASK)
    {
        // Align total size up to 8-bytes boundaries
        uint totalSize = alignedSize + sizeof(Chunk);
        totalSize = (totalSize + CHUNKALIGN_MASK) & ~CHUNKALIGN_MASK;
        alignedSize = totalSize - sizeof(Chunk);
    }

    // Use the address of the garbage collector as signature
    gc = GarbageCollector::GC();

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
    RECORD(MEMORY, "Destroy type allocator", "this", (intptr_t) this);

    VALGRIND_DESTROY_MEMPOOL(this);

    for (Chunks::iterator c = chunks.begin(); c != chunks.end(); c++)
        free((void *) *c);
}


void *TypeAllocator::Allocate()
// ----------------------------------------------------------------------------
//   Allocate a chunk of the given size
// ----------------------------------------------------------------------------
{
    RECORD(MEMORY_DETAILS, "Allocate", "free", (intptr_t) freeList.Get());

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

            RECORD(MEMORY_DETAILS, "New Chunk", "addr", (intptr_t) allocated);

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
    return ret;
}


void TypeAllocator::Delete(void *ptr)
// ----------------------------------------------------------------------------
//   Free a chunk of the given size
// ----------------------------------------------------------------------------
{
    RECORD(MEMORY_DETAILS, "Delete", "ptr", (intptr_t) ptr);

    if (!ptr)
        return;

    Chunk_vp chunk = (Chunk_vp) ptr - 1;
    XL_ASSERT(IsGarbageCollected(ptr) && "Deleted pointer not managed by GC");
    XL_ASSERT(IsAllocated(ptr) && "Deleted GC pointer that was already freed");
    XL_ASSERT(!chunk->count && "Deleted pointer has live references");

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
    RECORD(MEMORY_DETAILS, "CheckLeaks");

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
    RECORD(MEMORY_DETAILS, "CheckLeaks done",
           "scanned", scannedCount, "collect", collected);
    return collected;
}


bool TypeAllocator::Sweep()
// ----------------------------------------------------------------------------
//    Remove all the things that we have pushed on the toDelete list
// ----------------------------------------------------------------------------
{
    RECORD(MEMORY_DETAILS, "Sweep");

    bool result = false;
    while (toDelete)
    {
        Chunk_vp next = LinkedListPopFront(toDelete);
        next->allocator = this;
        Finalize((void *) (next+1));
        result = true;
    }
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
#ifdef CONFIG_MINGW // Windows. Enough said
    result = __mingw_aligned_malloc(size, PTR_MASK+1);
    if (!result)
        throw std::bad_alloc();
#else // Real operating systems
    if (posix_memalign(&result, PTR_MASK+1, size))
        throw std::bad_alloc();
#endif // WINDOWS or real operating system
    return result;
}


void TypeAllocator::operator delete(void *ptr)
// ----------------------------------------------------------------------------
//    Matching deallocation
// ----------------------------------------------------------------------------
{
#ifdef CONFIG_MINGW // Aka MS-DOS NT.
    // Brain damaged?
    __mingw_aligned_free(ptr);
#else // No brain-damage
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
    RECORD(MEMORY_DETAILS, "Can delete",
           "addr", (intptr_t) obj, "ok", result);
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
    if (Atomic<pthread_t>::SetQ(collecting, NULL, self))
    {
        RECORD(MEMORY, "Garbage collection", "self", (intptr_t) self);

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
        IFTRACE(memory)
            PrintStatistics();

        // We are done, mark it so
        mustRun &= 0U;
        if (!Atomic<pthread_t>::SetQ(collecting, self, NULL))
        {
            XL_ASSERT(!"Someone else stole the collection lock?");
        }

        RECORD(MEMORY, "Garbage collection", "self", (intptr_t) self);

        return true;
    }
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

XL_END

void debuggc(void *ptr)
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
}
