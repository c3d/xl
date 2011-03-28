// ****************************************************************************
//  gc.cpp                                                          XLR project
// ****************************************************************************
//
//   File Description:
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
// ****************************************************************************
// This document is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#include "gc.h"
#include "options.h"
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


TypeAllocator::TypeAllocator(kstring tn, uint os, mark_fn mark)
// ----------------------------------------------------------------------------
//    Setup an empty allocator
// ----------------------------------------------------------------------------
    : gc(NULL), name(tn), chunks(), mark(mark), freeList(NULL),
      chunkSize(1022), objectSize(os), alignedSize(os), available(0),
      allocatedCount(0), freedCount(0), totalCount(0)
{
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
}


TypeAllocator::~TypeAllocator()
// ----------------------------------------------------------------------------
//   Delete all the chunks we allocated
// ----------------------------------------------------------------------------
{
    std::vector<Chunk *>::iterator c;
    for (c = chunks.begin(); c != chunks.end(); c++)
        free(*c);
}


void *TypeAllocator::Allocate()
// ----------------------------------------------------------------------------
//   Allocate a chunk of the given size
// ----------------------------------------------------------------------------
{
    Chunk *result = freeList;
    if (!result)
    {
        // Nothing free: allocate a big enough chunk
        size_t  itemSize  = alignedSize + sizeof(Chunk);
        void   *allocated = malloc((chunkSize + 1) * itemSize);
        char   *chunkBase = (char *) allocated + alignedSize;
        chunks.push_back((Chunk *) allocated);
        for (uint i = 0; i < chunkSize; i++)
        {
            Chunk *ptr = (Chunk *) (chunkBase + i * itemSize);
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
    result->allocator = this;
    result->bits |= IN_USE;     // In case a collection is running right now
    if (--available < chunkSize / 4)
        GarbageCollector::CollectionNeeded();

    return &result[1];
}


void TypeAllocator::Delete(void *ptr)
// ----------------------------------------------------------------------------
//   Free a chunk of the given size
// ----------------------------------------------------------------------------
{
    if (!ptr)
        return;

    Chunk *chunk = (Chunk *) ptr - 1;
    assert(IsGarbageCollected(ptr) || !"Deleted pointer is not managed by GC");
    assert(IsAllocated(ptr) || !"Deleted GC pointer that was already freed");
    assert(!(chunk->bits & USE_MASK) || !"Deleted pointer has live references");

    // Put the pointer back on the free list
    chunk->next = freeList;
    freeList = chunk;
    available++;

#if 1
    // Scrub all the pointers
    uint32 *base = (uint32 *) ptr;
    uint32 *last = (uint32 *) (((char *) ptr) + alignedSize);
    for (uint *p = base; p < last; p++)
        *p = 0xDeadBeef;
#endif
}


void TypeAllocator::Finalize(void *ptr)
// ----------------------------------------------------------------------------
//   We should never reach this one
// ----------------------------------------------------------------------------
{
    std::cerr << "No finalizer installed for " << ptr << "\n";
    assert(!"No finalizer installed");
}


void TypeAllocator::MarkRoots()
// ----------------------------------------------------------------------------
//    Loop on all objects that have a reference count above 1
// ----------------------------------------------------------------------------
{
    std::vector<Chunk *>::iterator chunk;
    allocatedCount = freedCount = totalCount = 0;
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
                allocatedCount++;
                if ((ptr->bits & IN_USE) == 0)
                    if ((ptr->bits & USE_MASK) > 0)
                        Mark(ptr+1);
            }
        }
    }
}


void TypeAllocator::Mark(void *data)
// ----------------------------------------------------------------------------
//   Loop on all allocated items and mark which ones are in use
// ----------------------------------------------------------------------------
{
    if (!data)
        return;

    // Find chunk pointer
    Chunk *inUse = ((Chunk *) data) - 1;

    // We should only look at allocated items, otherwise oops...
    if (inUse->bits & IN_USE)
        return;

    // We'd better be in the right allocator
    assert(ValidPointer(inUse->allocator) == this);

    // We had not marked that one yet, mark it now
    inUse->bits |= IN_USE;

    // Mark all pointers in this item
    mark(data);
}


void TypeAllocator::Sweep()
// ----------------------------------------------------------------------------
//   Once we have marked everything, sweep what is not in use
// ----------------------------------------------------------------------------
{
    std::vector<Chunk *>::iterator chunk;
    for (chunk = chunks.begin(); chunk != chunks.end(); chunk++)
    {
        char *chunkBase = (char *) *chunk + alignedSize;
        size_t  itemSize  = alignedSize + sizeof(Chunk);
        for (uint i = 0; i < chunkSize; i++)
        {
            Chunk *ptr = (Chunk *) (chunkBase + i * itemSize);
            if (AllocatorPointer(ptr->allocator) == this)
            {
                if (ptr->bits & IN_USE)
                {
                    ptr->bits &= ~IN_USE;
                }
                else if ((ptr->bits & USE_MASK) == 0)
                {
                    Finalize(ptr+1);
                    freedCount++;
                }
            }
        }
    }
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
    : mustRun(false)
{}


GarbageCollector::~GarbageCollector()
// ----------------------------------------------------------------------------
//    Destroy the garbage collector
// ----------------------------------------------------------------------------
{
    RunCollection(true, false);
    RunCollection(true, false);

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


void GarbageCollector::RunCollection(bool force, bool mark)
// ----------------------------------------------------------------------------
//   Run garbage collection on all the allocators we own
// ----------------------------------------------------------------------------
{
    if (mustRun || force || !mark)
    {
        std::vector<TypeAllocator *>::iterator a;
        std::set<TypeAllocator::Listener *> listeners;
        std::set<TypeAllocator::Listener *>::iterator l;
        mustRun = false;

        // Build the listeners from all allocators
        for (a = allocators.begin(); a != allocators.end(); a++)
            for (l = (*a)->listeners.begin(); l != (*a)->listeners.end(); l++)
                listeners.insert(*l);

        // Notify all the listeners that we begin a collection
        for (l = listeners.begin(); l != listeners.end(); l++)
            (*l)->BeginCollection();

        // Mark roots in all the allocators
        if (mark)
            for (a = allocators.begin(); a != allocators.end(); a++)
                (*a)->MarkRoots();

        // Then sweep whatever was not referenced
        for (a = allocators.begin(); a != allocators.end(); a++)
            (*a)->Sweep();

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
        std::cerr << "Allocator bits: " << std::hex << bits << "\n";

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
