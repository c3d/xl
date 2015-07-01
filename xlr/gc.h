#ifndef GC_H
#define GC_H
// ****************************************************************************
//  gc.h                                                            Tao project
// ****************************************************************************
//
//   File Description:
//
//     Garbage collector managing memory for us
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

#include "base.h"
#include "atomic.h"

#include <vector>
#include <map>
#include <set>
#include <stdint.h>
#include <typeinfo>

extern void debuggc(void *);

XL_BEGIN

struct GarbageCollector;
template <class Object, typename ValueType=void> struct GCPtr;



// ****************************************************************************
//
//   Type Allocator - Manage allocation for a given type
//
// ****************************************************************************

struct TypeAllocator
// ----------------------------------------------------------------------------
//   Structure allocating data for a single data type
// ----------------------------------------------------------------------------
{
    struct Chunk
    {
        union
        {
            volatile Chunk *next;           // Next in free list
            TypeAllocator * allocator;      // Allocator for this object
            uintptr_t       bits;           // Allocation bits
        };
        uint                count;          // Ref count
    };
    typedef volatile Chunk *Chunk_vp;
    typedef std::vector<Chunk_vp> Chunks;

public:
    TypeAllocator(kstring name, uint objectSize);
    virtual ~TypeAllocator();

    void *              Allocate();
    void                Delete(void *);
    virtual void        Finalize(void *);

    static TypeAllocator *ValidPointer(TypeAllocator *ptr);
    static TypeAllocator *AllocatorPointer(TypeAllocator *ptr);

    static void         Acquire(void *ptr);
    static void         Release(void *ptr);
    static uint         RefCount(void *ptr);
    static bool         IsGarbageCollected(void *ptr);
    static bool         IsAllocated(void *ptr);
    static void *       InUse(void *ptr);
    static void         UpdateInUseRange(Chunk_vp chunk);
    static void         ScheduleDelete(Chunk_vp);
    bool                CheckLeakedPointers();
    bool                Sweep();
    void                ResetStatistics();

    void *operator new(size_t size);
    void operator delete(void *ptr);

public:
    enum ChunkBits
    {
        PTR_MASK        = 15,           // Special bits we take out of the ptr
        CHUNKALIGN_MASK = 7,            // Alignment for chunks
        ALLOCATED       = 0,            // Just allocated
        IN_USE          = 1             // Set if already marked this time
    };

public:
    struct Listener
    {
        virtual void BeginCollection()          {}
        virtual bool CanDelete(void *)          { return true; }
        virtual void EndCollection()            {}
    };
    typedef std::set<Listener *> Listeners;
    void AddListener(Listener *l) { listeners.insert(l); }
    bool CanDelete(void *object);

protected:
    GarbageCollector *  gc;
    kstring             name;
    Atomic<uint>        locked;
    Atomic<uintptr_t>   lowestInUse;
    Atomic<uintptr_t>   highestInUse;
    Chunks              chunks;
    Listeners           listeners;
    Atomic<Chunk_vp>    freeList;
    Atomic<Chunk_vp>    toDelete;
    Atomic<uint>        available;
    Atomic<uint>        freedCount;

    uint                chunkSize;
    uint                objectSize;
    uint                alignedSize;
    uint                allocatedCount;
    uint                scannedCount;
    uint                collectedCount;
    uint                totalCount;

    friend void ::debuggc(void *ptr);
    friend struct GarbageCollector;

public:
    static void *       lowestAddress;
    static void *       highestAddress;
    static void *       lowestAllocatorAddress;
    static void *       highestAllocatorAddress;
    static Atomic<uint> finalizing;
} __attribute__((aligned(16)));


template <class Object>
struct Allocator : TypeAllocator
// ----------------------------------------------------------------------------
//    Allocate objects for a given object type
// ----------------------------------------------------------------------------
{
    typedef Object  object_t;
    typedef Object *ptr_t;

public:
    Allocator();

    static Allocator *  CreateSingleton();
    static Allocator *  Singleton()             { return allocator; }
    static Object *     Allocate(size_t size);
    static void         Delete(Object *);
    virtual void        Finalize(void *object);
    static bool         IsAllocated(void *ptr);

private:
    static Allocator *  allocator;
};



// ****************************************************************************
//
//   Garbage collection root pointer
//
// ****************************************************************************

template<class Object, typename ValueType>
struct GCPtr
// ----------------------------------------------------------------------------
//   A root pointer to an object in a garbage-collected pool
// ----------------------------------------------------------------------------
//   This class is made a bit more complex by thread safety.
//   It is supposed to work correctly when two threads assign to
//   the same GCPtr at the same time. This is managed by 'Assign'
{
    typedef TypeAllocator TA;

    GCPtr(): pointer(0)                         { }
    GCPtr(Object *ptr): pointer(ptr)            { TA::Acquire(pointer); }
    GCPtr(Object &ptr): pointer(&ptr)           { TA::Acquire(pointer); }
    GCPtr(const GCPtr &ptr)
        : pointer(ptr.Pointer())                { TA::Acquire(pointer); }
    template<class U, typename V>
    GCPtr(const GCPtr<U,V> &p)
        : pointer((U*) p.Pointer())             { TA::Acquire(pointer); }
    ~GCPtr()                                    { TA::Release(pointer); }


    // ------------------------------------------------------------------------
    // Thread safety section
    // ------------------------------------------------------------------------

    operator Object* () const
    {
        // If an object 'escapes' a GCPtr, it is marked as 'in use'
        // That way, it survives until captured by another GCPtr
        // We pass what we think is the current pointer at the moment
        // to make sure what is marked as 'in use' is also what escapes
        // in the current thread.
        return (Object *) TA::InUse(pointer);
    }

    // These operators are not marking the pointer as escaping.
    // If you use them, the resulting pointer becomes possibly invalid as soon
    // as this GCPtr is destroyed
    const Object *ConstPointer() const          { return pointer; }
    Object *Pointer() const                     { return pointer; }
    Object *operator->() const                  { return pointer; }
    Object& operator*() const                   { return *pointer; }

    // Two threads may be assigning to this GCPtr at the same time,
    // e.g. if we update a same Tree child from two different threads.
    GCPtr& Assign(Object *oldVal, Object *newVal)
    {
        while (!Atomic<Object *>::SetQ(pointer, oldVal, newVal))
            oldVal = pointer;
        if (newVal != oldVal)
        {
            TA::Acquire(newVal);
            TA::Release(oldVal);
        }
        return *this;
    }

    GCPtr &operator= (const GCPtr &o)
    {
        return Assign(pointer, (Object *) o.ConstPointer());
    }
            
    template<class U, typename V>
    GCPtr& operator=(const GCPtr<U,V> &o)
    {
        return Assign(pointer, (Object *) o.ConstPointer());
    }

#define DEFINE_CMP(CMP)                                 \
    template<class U, typename V>                       \
    bool operator CMP(const GCPtr<U,V> &o) const        \
    {                                                   \
        return pointer CMP o.ConstPointer();            \
    }

    DEFINE_CMP(==)
    DEFINE_CMP(!=)
    DEFINE_CMP(<)
    DEFINE_CMP(<=)
    DEFINE_CMP(>)
    DEFINE_CMP(>=)

protected:
    Object *    pointer;
};



// ****************************************************************************
// 
//    The GarbageCollector class
// 
// ****************************************************************************

struct GarbageCollector
// ----------------------------------------------------------------------------
//    Structure registering all allocators
// ----------------------------------------------------------------------------
{
    GarbageCollector();
    ~GarbageCollector();

    static GarbageCollector *   GC()            { return gc; }
    static GarbageCollector *   CreateSingleton();
    static void                 Delete();

    static void                 MustRun()       { gc->mustRun |= 1U; }
    static bool                 Running()       { return gc->running; }
    static bool                 SafePoint();
    static bool                 Sweep();
    
    void                        Statistics(uint &totalBytes,
                                           uint &allocBytes,
                                           uint &availableBytes,
                                           uint &freedBytes,
                                           uint &scannedBytes,
                                           uint &collectedBytes);
    void                        PrintStatistics();
    void                        Register(TypeAllocator *a);

private:
    // Collection happens at SafePoint, you can't trigger it manually.
    bool                        Collect();

private:
    typedef std::vector<TypeAllocator *> Allocators;
    typedef TypeAllocator::Listeners     Listeners;

    static GarbageCollector *   gc;

    Allocators                  allocators;
    Atomic<uint>                mustRun;
    Atomic<uint>                running;

    friend void ::debuggc(void *ptr);
};



// ============================================================================
//
//    Macros used to declare a garbage-collected type
//
// ============================================================================

// Define a garbage collected tree

#define GARBAGE_COLLECT(type)                                           \
/* ------------------------------------------------------------ */      \
/*  Declare a garbage-collected type and the related Mark       */      \
/* ------------------------------------------------------------ */      \
    void *operator new(size_t size)                                     \
    {                                                                   \
        return XL::Allocator<type>::Allocate(size);                     \
    }                                                                   \
                                                                        \
    void operator delete(void *ptr)                                     \
    {                                                                   \
        XL::Allocator<type>::Delete((type *) ptr);                      \
    }



// ============================================================================
//
//   Inline functions for TypeAllocator
//
// ============================================================================

inline TypeAllocator *TypeAllocator::ValidPointer(TypeAllocator *ptr)
// ----------------------------------------------------------------------------
//   Return a valid pointer from a possibly marked pointer
// ----------------------------------------------------------------------------
{
    TypeAllocator *result = (TypeAllocator *) (((uintptr_t) ptr) & ~PTR_MASK);
    XL_ASSERT(result && result->gc == GarbageCollector::GC());
    return result;
}


inline TypeAllocator *TypeAllocator::AllocatorPointer(TypeAllocator *ptr)
// ----------------------------------------------------------------------------
//   Return a valid pointer from a possibly marked pointer
// ----------------------------------------------------------------------------
{
    TypeAllocator *result = (TypeAllocator *) (((uintptr_t) ptr) & ~PTR_MASK);
    return result;
}


inline bool TypeAllocator::IsGarbageCollected(void *ptr)
// ----------------------------------------------------------------------------
//   Tell if a pointer is managed by the garbage collector
// ----------------------------------------------------------------------------
{
    return ptr >= lowestAddress && ptr <= highestAddress;
}


inline bool TypeAllocator::IsAllocated(void *ptr)
// ----------------------------------------------------------------------------
//   Tell if a pointer is allocated by the garbage collector (not free)
// ----------------------------------------------------------------------------
{
    if (IsGarbageCollected(ptr))
    {
        if ((uintptr_t) ptr & CHUNKALIGN_MASK)
            return false;

        Chunk_vp chunk = (Chunk_vp) ptr - 1;
        TypeAllocator *alloc = AllocatorPointer(chunk->allocator);
        if (alloc >= lowestAllocatorAddress && alloc <= highestAllocatorAddress)
            if (alloc->gc == GarbageCollector::GC())
                return true;
    }
    return false;
}


inline void TypeAllocator::Acquire(void *pointer)
// ----------------------------------------------------------------------------
//   Increase reference count for pointer and return it
// ----------------------------------------------------------------------------
{
    if (IsGarbageCollected(pointer))
    {
        XL_ASSERT (((intptr_t) pointer & CHUNKALIGN_MASK) == 0);
        XL_ASSERT (IsAllocated(pointer));

        Chunk_vp chunk = ((Chunk_vp) pointer) - 1;
        ++chunk->count;
    }
}


inline void TypeAllocator::Release(void *pointer)
// ----------------------------------------------------------------------------
//   Decrease reference count for pointer and return it
// ----------------------------------------------------------------------------
{
    if (IsGarbageCollected(pointer))
    {
        XL_ASSERT (((intptr_t) pointer & CHUNKALIGN_MASK) == 0);
        XL_ASSERT (IsAllocated(pointer));

        Chunk_vp chunk = ((Chunk_vp) pointer) - 1;
        XL_ASSERT(chunk->count);
        uint count = --chunk->count;
        if (!count)
            ScheduleDelete(chunk);
    }
}


inline uint TypeAllocator::RefCount(void *pointer)
// ----------------------------------------------------------------------------
//   Return reference count for given pointer
// ----------------------------------------------------------------------------
{
    XL_ASSERT (((intptr_t) pointer & CHUNKALIGN_MASK) == 0);
    if (IsAllocated(pointer))
    {
        Chunk_vp chunk = ((Chunk_vp) pointer) - 1;
        return chunk->count;
    }
    return ~0U;
}


inline void *TypeAllocator::InUse(void *pointer)
// ----------------------------------------------------------------------------
//   Mark the current pointer as in use, to preserve in next GC cycle
// ----------------------------------------------------------------------------
{
    if (IsGarbageCollected(pointer))
    {
        XL_ASSERT (((intptr_t) pointer & CHUNKALIGN_MASK) == 0);
        Chunk_vp chunk = ((Chunk_vp) pointer) - 1;
        uint bits = Atomic<uintptr_t>::Or(chunk->bits, IN_USE);
        if (!chunk->count && (~bits & IN_USE))
            UpdateInUseRange(chunk);
    }
    return pointer;
}


inline void TypeAllocator::UpdateInUseRange(Chunk_vp chunk)
// ----------------------------------------------------------------------------
//    Update the range of in-use pointers when in-use bit is set
// ----------------------------------------------------------------------------
{
    TypeAllocator *allocator = ValidPointer(chunk->allocator);
    allocator->lowestInUse.Minimize((uintptr_t) chunk);
    allocator->highestInUse.Maximize((uintptr_t) (chunk + 1));
}



// ============================================================================
//
//   Inline functions for template Allocator
//
// ============================================================================

template<class Object> inline
Allocator<Object>::Allocator()
// ----------------------------------------------------------------------------
//   Create an allocator for the given size
// ----------------------------------------------------------------------------
    : TypeAllocator(typeid(Object).name(), sizeof (Object))
{}


template<class Object> inline
Allocator<Object> * Allocator<Object>::CreateSingleton()
// ----------------------------------------------------------------------------
//    Return a singleton for the allocation class
// ----------------------------------------------------------------------------
{
    if (!allocator)
        // Create the singleton
        allocator = new Allocator;

    return allocator;
}


template<class Object> inline
Object *Allocator<Object>::Allocate(size_t size)
// ----------------------------------------------------------------------------
//   Allocate an object (invoked by operator new)
// ----------------------------------------------------------------------------
{
    XL_ASSERT(size == allocator->TypeAllocator::objectSize); (void) size;
    return (Object *) allocator->TypeAllocator::Allocate();
}


template<class Object> inline
void Allocator<Object>::Delete(Object *obj)
// ----------------------------------------------------------------------------
//   Allocate an object (invoked by operator delete)
// ----------------------------------------------------------------------------
{
    allocator->TypeAllocator::Delete(obj);
}


template<class Object> inline
void Allocator<Object>::Finalize(void *obj)
// ----------------------------------------------------------------------------
//   Make sure that we properly call the destructor for the object
// ----------------------------------------------------------------------------
{
    if (CanDelete(obj))
    {
        finalizing++;
        Object *object = (Object *) obj;
        delete object;
        finalizing--;
    }
    else
    {
        InUse(obj);
    }
}


template <class Object> inline
bool Allocator<Object>::IsAllocated(void *ptr)
// ----------------------------------------------------------------------------
//   Tell if a pointer is allocated in this particular pool
// ----------------------------------------------------------------------------
{
    if (IsGarbageCollected(ptr))
    {
        if ((uintptr_t) ptr & CHUNKALIGN_MASK)
            return false;

        Chunk_vp chunk = (Chunk_vp) ptr - 1;
        TypeAllocator *alloc = AllocatorPointer(chunk->allocator);
        if (alloc == allocator)
            return true;
    }
    return false;
}



// ============================================================================
//
//    Inline functions for the GarbageCollector class
//
// ============================================================================

inline bool GarbageCollector::SafePoint()
// ----------------------------------------------------------------------------
//    Check if we need to run the garbage collector, and if so run it
// ----------------------------------------------------------------------------
//    When calling this function, the current thread should not have any
//    allocation "in flight", i.e. not recorded using a root pointer
//    This looks for pointers that were allocated since the last
//    safe point and not assigned to any GCPtr yet.
{
    if (gc->mustRun)
        return gc->Collect();
    return false;
}

XL_END

#endif // GC_H
