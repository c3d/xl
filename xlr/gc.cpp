// *****************************************************************************
// gc.cpp                                                             XL project
// *****************************************************************************
//
// File description:
//
//     Garbage collector managing memory for us
//
//
//
//
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2010-2012,2019, Christophe de Dinechin <christophe@dinechin.org>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can r redistribute it and/or modify
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

#include "gc.h"
#include "options.h"
#include "flight_recorder.h"
#include "valgrind/memcheck.h"
#include <iostream>
#include <cstdio>
#include <cstdlib>

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
uint  TypeAllocator::finalizing = 0;

TypeAllocator::TypeAllocator(kstring tn, uint os)
// ----------------------------------------------------------------------------
//    Setup an empty allocator
// ----------------------------------------------------------------------------
    : gc(NULL), name(tn), chunks(), freeList(NULL), toDelete(NULL),
#ifdef XLR_GC_LIFO
      freeListTail(NULL),
#endif
      chunkSize(1022), objectSize(os), alignedSize(os), available(0),
      allocatedCount(0), freedCount(0), totalCount(0)
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
    gc = GarbageCollector::Singleton();

    // Register the allocator with the garbage collector
    gc->Register(this);

    // Make sure that we have the correct alignment
    assert(this == ValidPointer(this));

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

    std::vector<Chunk *>::iterator c;
    for (c = chunks.begin(); c != chunks.end(); c++)
        free(*c);
}


void *TypeAllocator::Allocate()
// ----------------------------------------------------------------------------
//   Allocate a chunk of the given size
// ----------------------------------------------------------------------------
{
    RECORD(MEMORY_DETAILS, "Allocate", "free", (intptr_t) freeList);

    Chunk *result = freeList;
    if (!result)
    {
        // Nothing free: allocate a big enough chunk
        size_t  itemSize  = alignedSize + sizeof(Chunk);
        size_t  allocSize = (chunkSize + 1) * itemSize;

        void   *allocated = malloc(allocSize);
        (void)VALGRIND_MAKE_MEM_NOACCESS(allocated, allocSize);

        RECORD(MEMORY_DETAILS, "New Chunk", "addr", (intptr_t) allocated);

        char   *chunkBase = (char *) allocated + alignedSize;
        chunks.push_back((Chunk *) allocated);
        for (uint i = 0; i < chunkSize; i++)
        {
            Chunk *ptr = (Chunk *) (chunkBase + i * itemSize);
            (void)VALGRIND_MAKE_MEM_UNDEFINED(&ptr->next, sizeof(ptr->next));
            ptr->next = result;
            result = ptr;
        }
        freeList = result;
        available += chunkSize;

        if (lowestAddress > allocated)
            lowestAddress = allocated;
        char *highMark = (char *) allocated + (chunkSize+1) * itemSize;
        if (highestAddress < (void *) highMark)
            highestAddress = highMark;
    }

    // REVISIT: Atomic operations here
    freeList = result->next;
#ifdef XLR_GC_LIFO
    if (!freeList)
        freeListTail = NULL;
#endif
    (void)VALGRIND_MAKE_MEM_UNDEFINED(result, sizeof(Chunk));
    result->allocator = this;
    result->bits |= IN_USE;     // In case a collection is running right now
    result->count = 0;
    if (--available < chunkSize * 0.9)
        GarbageCollector::CollectionNeeded();

    void *ret =  &result[1];
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

    Chunk *chunk = (Chunk *) ptr - 1;
    assert(IsGarbageCollected(ptr) || !"Deleted pointer is not managed by GC");
    assert(IsAllocated(ptr) || !"Deleted GC pointer that was already freed");
    assert(!chunk->count || !"Deleted pointer has live references");

    // Put the pointer back on the free list
#ifndef XLR_GC_LIFO
    chunk->next = freeList;
    freeList = chunk;
#else
    chunk->next = NULL;
    if (freeListTail)
        freeListTail->next = chunk;
    freeListTail = chunk;
    if (!freeList)
        freeList = chunk;
#endif
    available++;

#if 1
    // Scrub all the pointers
    uint32 *base = (uint32 *) ptr;
    uint32 *last = (uint32 *) (((char *) ptr) + alignedSize);
    (void)VALGRIND_MAKE_MEM_UNDEFINED(ptr, alignedSize);
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
    assert(!"No finalizer installed");
}


void TypeAllocator::Sweep()
// ----------------------------------------------------------------------------
//   Once we have marked everything, sweep what is not in use
// ----------------------------------------------------------------------------
{
    RECORD(MEMORY_DETAILS, "Sweep");

    allocatedCount = freedCount = totalCount = 0;
    std::vector<Chunk *>::iterator chunk;
    for (chunk = chunks.begin(); chunk != chunks.end(); chunk++)
    {
        char *chunkBase = (char *) *chunk + alignedSize;
        size_t  itemSize  = alignedSize + sizeof(Chunk);
        for (uint i = 0; i < chunkSize; i++)
        {
            Chunk *ptr = (Chunk *) (chunkBase + i * itemSize);
            totalCount++;
            if (AllocatorPointer(ptr->allocator) == this)
            {
                if (ptr->count)
                {
                    // Non-zero count, still alive somewhere
                    allocatedCount++;
                }
                else
                {
                    // Count is 0 : no longer referenced, may cascade free
                    // We cannot just recurse in delete here because stack
                    // space may be limited (#2488)
                    finalizing++;
                    DeleteLater(ptr);
                    DeleteAll();
                    finalizing--;
                }
            }
        }
    }

    RECORD(MEMORY_DETAILS, "Sweep Done", "freed", freedCount);
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
    std::set<Listener *>::iterator i;
    for (i = listeners.begin(); i != listeners.end(); i++)
        if (!(*i)->CanDelete(obj))
            result = false;
    RECORD(MEMORY_DETAILS, "Can delete",
           "addr", (intptr_t) obj, "ok", result);
    return result;
}




// ============================================================================
//
//   Allocator class
//
// ============================================================================

// Define the allocator singleton for each object type.
// Previously, Allocator::Singleton() had a static local 'allocator' variable,
// but we ended up with several instances on Windows. See #903.

#define DEFINE_ALLOC(_type) \
    struct _type;           \
    template<> Allocator<_type> * Allocator<_type>::allocator = NULL;

DEFINE_ALLOC(Tree);
DEFINE_ALLOC(Integer);
DEFINE_ALLOC(Real);
DEFINE_ALLOC(Text);
DEFINE_ALLOC(Name);
DEFINE_ALLOC(Block);
DEFINE_ALLOC(Prefix);
DEFINE_ALLOC(Postfix);
DEFINE_ALLOC(Infix);
DEFINE_ALLOC(Context);
DEFINE_ALLOC(Symbols);
DEFINE_ALLOC(TypeInference);
DEFINE_ALLOC(Rewrite);
DEFINE_ALLOC(RewriteCalls);

#undef DEFINE_ALLOC





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
    RunCollection(true);
    RunCollection(true);

    std::vector<TypeAllocator *>::iterator i;
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


void GarbageCollector::RunCollection(bool force)
// ----------------------------------------------------------------------------
//   Run garbage collection on all the allocators we own
// ----------------------------------------------------------------------------
{
    if (mustRun || force)
    {
        RECORD(MEMORY, "Garbage collection", "force", force);

        std::vector<TypeAllocator *>::iterator a;
        std::set<TypeAllocator::Listener *> listeners;
        std::set<TypeAllocator::Listener *>::iterator l;
        mustRun = false;
        running = true;

        // Build the listeners from all allocators
        for (a = allocators.begin(); a != allocators.end(); a++)
            for (l = (*a)->listeners.begin(); l != (*a)->listeners.end(); l++)
                listeners.insert(*l);

        // Notify all the listeners that we begin a collection
        for (l = listeners.begin(); l != listeners.end(); l++)
            (*l)->BeginCollection();

        // Sweep whatever is not referenced at this point in time
        for (a = allocators.begin(); a != allocators.end(); a++)
            (*a)->Sweep();

        // Cleanup pending purges to maximize the effect of garbage collection
        bool purging = true;
        while (purging)
        {
            purging = false;
            for (a = allocators.begin(); a != allocators.end(); a++)
                purging |= (*a)->DeleteAll();
        }

        // Notify all the listeners that we completed the collection
        for (l = listeners.begin(); l != listeners.end(); l++)
            (*l)->EndCollection();

        IFTRACE(memory)
        {
            uint tot = 0, alloc = 0, freed = 0;
            printf("%15s %8s %8s %8s\n", "NAME", "TOTAL", "ALLOC", "FREED");
            for (a = allocators.begin(); a != allocators.end(); a++)
            {
                TypeAllocator *ta = *a;
                printf("%15s %8u %8u %8u\n",
                       ta->name, ta->totalCount,
                       ta->allocatedCount, ta->freedCount);
                tot   += ta->totalCount     * ta->alignedSize;
                alloc += ta->allocatedCount * ta->alignedSize;
                freed += ta->freedCount     * ta->alignedSize;
            }
            printf("%15s %8s %8s %8s\n", "=====", "=====", "=====", "=====");
            printf("%15s %7uK %7uK %7uK\n",
                   "Kilobytes", tot >> 10, alloc >> 10, freed >> 10);
        }

        running = false;
        RECORD(MEMORY, "Garbage collection", "force", force);
    }
}


void GarbageCollector::Statistics(uint &tot, uint &alloc, uint &freed)
// ----------------------------------------------------------------------------
//   Get statistics about garbage collections
// ----------------------------------------------------------------------------
{
    tot = 0;
    alloc = 0;
    freed = 0;

    std::vector<TypeAllocator *>::iterator a;
    for (a = allocators.begin(); a != allocators.end(); a++)
    {
        TypeAllocator *ta = *a;
        tot   += ta->totalCount     * ta->alignedSize;
        alloc += ta->allocatedCount * ta->alignedSize;
        freed += ta->freedCount     * ta->alignedSize;
    }
}


GarbageCollector *GarbageCollector::gc = NULL;

GarbageCollector *GarbageCollector::Singleton()
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


void GarbageCollector::Collect(bool force)
// ----------------------------------------------------------------------------
//   Collect garbage in this garbage collector
// ----------------------------------------------------------------------------
{
    Singleton()->RunCollection(force);
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
        typedef TypeAllocator::Chunk Chunk;
        typedef TypeAllocator TA;

        Chunk *chunk = (Chunk *) ptr - 1;
        if ((uintptr_t) chunk & TA::CHUNKALIGN_MASK)
        {
            std::cerr << "WARNING: Pointer " << ptr << " is not aligned\n";
            chunk = (Chunk *) (((uintptr_t) chunk) & ~TA::CHUNKALIGN_MASK);
            std::cerr << "         Using " << chunk << " as chunk\n";
        }
        uintptr_t bits = chunk->bits;
        uintptr_t aligned = bits & ~TA::PTR_MASK;
        std::cerr << "Allocator bits: " << std::hex << bits
                  << " count=" << chunk->count << "\n";

        GarbageCollector *gc = GarbageCollector::Singleton();

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
            std::vector<TA::Chunk *>::iterator c;
            alloc = *a;
            uint itemBytes = alloc->alignedSize + sizeof(Chunk);
            uint chunkBytes = alloc->chunkSize * itemBytes;
            uint chunkIndex = 0;
            for (c = alloc->chunks.begin(); c != alloc->chunks.end(); c++)
            {
                char *start = (char *) *c;
                char *end = start + chunkBytes;
                chunkIndex++;
                if (ptr >= start && ptr <= end)
                {
                    if (!allocated)
                        std::cerr << "Free item in "
                                  << alloc << " (" << alloc->name << ") "
                                  << "chunk #" << chunkIndex << " at position ";
                    uint freeIndex = 0;
                    Chunk *prev = NULL;
                    for (Chunk *f = alloc->freeList; f; f = f->next)
                    {
                        freeIndex++;
                        if (f == chunk)
                        {
                            std::cerr << "#" << freeIndex
                                      << " after " << prev << " ";
                            found++;
                        }
                        prev = f;
                    }

                    if (!allocated || found)
                        std::cerr << "in free list\n";
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
