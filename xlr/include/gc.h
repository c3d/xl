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

#include "base.h"
#include <vector>
#include <map>
#include <set>
#include <cassert>
#include <stdint.h>
#include <typeinfo>

extern void debuggc(void *);

XL_BEGIN

// ============================================================================
//
//   Class declarations
//
// ============================================================================

struct GarbageCollector;

struct TypeAllocator
// ----------------------------------------------------------------------------
//   Structure allocating data for a single data type
// ----------------------------------------------------------------------------
{
    union Chunk
    {
        Chunk *         next;           // Next in free list
        TypeAllocator * allocator;      // Allocator for this object
        uintptr_t       bits;           // Allocation bits
    };
    typedef void (*mark_fn)(void *object);

public:
    TypeAllocator(kstring name, uint objectSize, mark_fn mark);
    virtual ~TypeAllocator();

    void *              Allocate();
    void                Delete(void *);
    virtual void        Finalize(void *);

    void                MarkRoots();
    void                Mark(void *inUse);
    void                Sweep();

    static TypeAllocator *ValidPointer(TypeAllocator *ptr);
    static TypeAllocator *AllocatorPointer(TypeAllocator *ptr);

    static uint         Acquire(void *ptr);
    static uint         Release(void *ptr);
    static bool         IsGarbageCollected(void *ptr);
    static bool         IsAllocated(void *ptr);
    static bool         InUse(void *ptr);

    void *operator new(size_t size);
    void operator delete(void *ptr);

public:
    enum ChunkBits
    {
        PTR_MASK        = 15,           // Special bits we take out of the ptr
        USE_MASK        = 7,            // Bits used as reference counter
        CHUNKALIGN_MASK = 7,            // Alignment for chunks
        ALLOCATED       = 0,            // Just allocated
        LOCKED_ROOT     = 7,            // Too many references to fit in mask
        IN_USE          = 8             // Set if already marked this time
    };

public:
    struct Listener
    {
        virtual void BeginCollection()          {}
        virtual bool CanDelete(void *)          { return true; }
        virtual void EndCollection()            {}
    };
    void AddListener(Listener *l) { listeners.insert(l); }
    bool CanDelete(void *object);

protected:
    GarbageCollector *  gc;
    kstring             name;
    std::vector<Chunk*> chunks;
    mark_fn             mark;
    std::set<Listener *>listeners;
    std::map<void*,uint>roots;
    Chunk *             freeList;
    uint                chunkSize;
    uint                objectSize;
    uint                alignedSize;
    uint                available;
    uint                allocatedCount;
    uint                freedCount;
    uint                totalCount;

    friend void ::debuggc(void *ptr);
    friend class GarbageCollector;

public:
    static void *       lowestAddress;
    static void *       highestAddress;
    static void *       lowestAllocatorAddress;
    static void *       highestAllocatorAddress;
} __attribute__((aligned(16)));


template <class Object, typename ValueType=void> struct GCPtr;


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

    static Allocator *  Singleton();
    static Object *     Allocate(size_t size);
    static void         Delete(Object *);
    virtual void        Finalize(void *object);
    void                RegisterPointers();
    static void         MarkObject(void *object);
    static bool         IsAllocated(void *ptr);
};


template<class Object, typename ValueType>
struct GCPtr
// ----------------------------------------------------------------------------
//   A root pointer to an object in a garbage-collected pool
// ----------------------------------------------------------------------------
{
    GCPtr(): pointer(0)                         { }
    GCPtr(Object *ptr): pointer(ptr)            { Acquire(); }
    GCPtr(Object &ptr): pointer(&ptr)           { Acquire(); }
    GCPtr(const GCPtr &ptr)
        : pointer(ptr.Pointer())                { Acquire(); }
    template<class U, typename V>
    GCPtr(const GCPtr<U,V> &p)
        : pointer((U*) p.Pointer())             { Acquire(); }
    ~GCPtr()                                    { Release(); }

    operator Object* () const                   { InUse(); return pointer; }
    const Object *ConstPointer() const          { InUse(); return pointer; }
    Object *Pointer() const                     { InUse(); return pointer; }
    Object *operator->() const                  { InUse(); return pointer; }
    Object& operator*() const                   { InUse(); return *pointer; }
    operator ValueType() const                  { return pointer; }

    GCPtr &operator= (const GCPtr &o)
    {
        if (o.ConstPointer() != pointer)
        {
            Release();
            pointer = (Object *) o.ConstPointer();
            Acquire();
        }
        return *this;
    }

    template<class U, typename V>
    GCPtr& operator=(const GCPtr<U,V> &o)
    {
        if (o.ConstPointer() != pointer)
        {
            Release();
            pointer = (U *) o.ConstPointer();
            Acquire();
        }
        return *this;
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

    bool        InUse() const   { return TypeAllocator::InUse(pointer); }

protected:
    uint        Acquire() const { return TypeAllocator::Acquire(pointer); }
    uint        Release() const { return TypeAllocator::Release(pointer); }

    Object *    pointer;
};


struct GarbageCollector
// ----------------------------------------------------------------------------
//    Structure registering all allocators
// ----------------------------------------------------------------------------
{
    GarbageCollector();
    ~GarbageCollector();

    void        Register(TypeAllocator *a);
    void        RunCollection(bool force=false, bool mark=true);
    void        MustRun()    { mustRun = true; }

    static GarbageCollector *   Singleton();
    static void                 Delete();
    static void                 Collect(bool force=false);
    static void                 CollectionNeeded() { Singleton()->MustRun(); }

protected:
    std::vector<TypeAllocator *> allocators;
    bool mustRun;
    static GarbageCollector *    gc;

    friend void ::debuggc(void *ptr);
};



// ============================================================================
//
//    Macros used to declare a garbage-collected type
//
// ============================================================================

// Define a garbage collected tree
// Intended usage : GARBAGE_COLLECT(Tree) { /* code to mark pointers */ }

#define GARBAGE_COLLECT_MARK(type)                                      \
    void *operator new(size_t size)                                     \
    {                                                                   \
        return XL::Allocator<type>::Allocate(size);                     \
    }                                                                   \
                                                                        \
    void operator delete(void *ptr)                                     \
    {                                                                   \
        XL::Allocator<type>::Delete((type *) ptr);                      \
    }                                                                   \
                                                                        \
    void Mark(XL::Allocator<type> &alloc)

#define GARBAGE_COLLECT(type)                           \
        GARBAGE_COLLECT_MARK(type) { (void) alloc; }



// ============================================================================
//
//   Inline functions for base allocator
//
// ============================================================================

inline TypeAllocator *TypeAllocator::ValidPointer(TypeAllocator *ptr)
// ----------------------------------------------------------------------------
//   Return a valid pointer from a possibly marked pointer
// ----------------------------------------------------------------------------
{
    TypeAllocator *result = (TypeAllocator *) (((uintptr_t) ptr) & ~PTR_MASK);
    assert(result && result->gc == GarbageCollector::Singleton());
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

        Chunk *chunk = (Chunk *) ptr - 1;
        TypeAllocator *alloc = AllocatorPointer(chunk->allocator);
        if (alloc >= lowestAllocatorAddress && alloc <= highestAllocatorAddress)
            if (alloc->gc == GarbageCollector::Singleton())
                return true;
    }
    return false;
}


inline uint TypeAllocator::Acquire(void *pointer)
// ----------------------------------------------------------------------------
//   Increase reference count for pointer and return it
// ----------------------------------------------------------------------------
{
    uint count = 0;
    if (IsGarbageCollected(pointer))
    {
        assert (((intptr_t) pointer & CHUNKALIGN_MASK) == 0);
        assert (IsAllocated(pointer));

        Chunk *chunk = ((Chunk *) pointer) - 1;
        count = chunk->bits & USE_MASK;
        if (count < LOCKED_ROOT)
        {
            chunk->bits = (chunk->bits & ~USE_MASK) | ++count;
        }
        if (count >= LOCKED_ROOT)
        {
            TypeAllocator *allocator = ValidPointer(chunk->allocator);
            count = LOCKED_ROOT + allocator->roots[pointer]++;
        }
    }
    return count;
}


inline uint TypeAllocator::Release(void *pointer)
// ----------------------------------------------------------------------------
//   Decrease reference count for pointer and return it
// ----------------------------------------------------------------------------
{
    uint count = 0;
    if (IsGarbageCollected(pointer))
    {
        assert (((intptr_t) pointer & CHUNKALIGN_MASK) == 0);
        assert (IsAllocated(pointer));

        Chunk *chunk = ((Chunk *) pointer) - 1;
        TypeAllocator *allocator = ValidPointer(chunk->allocator);
        count = chunk->bits & USE_MASK;
        assert(count);
        if (count < LOCKED_ROOT)
        {
            chunk->bits = (chunk->bits & ~USE_MASK) | --count;
            if (!count && !(chunk->bits & IN_USE))
                allocator->Finalize(pointer);
        }
        else
        {
            count = LOCKED_ROOT + --allocator->roots[pointer];
            if (count == LOCKED_ROOT)
                chunk->bits = (chunk->bits & ~USE_MASK) | --count;
        }
    }
    return count;
}


inline bool TypeAllocator::InUse(void *pointer)
// ----------------------------------------------------------------------------
//   Mark the current pointer as in use, to preserve in next GC cycle
// ----------------------------------------------------------------------------
{
    bool result = false;
    if (IsGarbageCollected(pointer))
    {
        assert (((intptr_t) pointer & CHUNKALIGN_MASK) == 0);
        Chunk *chunk = ((Chunk *) pointer) - 1;
        result = (chunk->bits & IN_USE) != 0;
        chunk->bits |= IN_USE;
    }
    return result;
}



// ============================================================================
//
//   Definitions for template Allocator
//
// ============================================================================

template<class Object> inline
Allocator<Object>::Allocator()
// ----------------------------------------------------------------------------
//   Create an allocator for the given size
// ----------------------------------------------------------------------------
    : TypeAllocator(typeid(Object).name(), sizeof (Object), MarkObject)
{}


template<class Object> inline
Allocator<Object> * Allocator<Object>::Singleton()
// ----------------------------------------------------------------------------
//    Return a singleton for the allocation class
// ----------------------------------------------------------------------------
{
    static Allocator *allocator = NULL;

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
    assert(size == Singleton()->TypeAllocator::objectSize);
    return (Object *) Singleton()->TypeAllocator::Allocate();
}


template<class Object> inline
void Allocator<Object>::Delete(Object *obj)
// ----------------------------------------------------------------------------
//   Allocate an object (invoked by operator new)
// ----------------------------------------------------------------------------
{
    Singleton()->TypeAllocator::Delete(obj);
}


template<class Object> inline
void Allocator<Object>::Finalize(void *obj)
// ----------------------------------------------------------------------------
//   Make sure that we properly call the destructor for the object
// ----------------------------------------------------------------------------
{
    if (CanDelete(obj))
    {
        Object *object = (Object *) obj;
        delete object;
    }
    else
    {
        Chunk *chunk = ((Chunk *) obj) - 1;
        chunk->bits |= IN_USE;
    }
}


template<class Object> inline
void Allocator<Object>::MarkObject(void *object)
// ----------------------------------------------------------------------------
//   Make sure that we properly call the destructor for the object
// ----------------------------------------------------------------------------
{
    if (object)
        ((Object *) object)->Mark(*Singleton());
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

        Chunk *chunk = (Chunk *) ptr - 1;
        TypeAllocator *alloc = AllocatorPointer(chunk->allocator);
        if (alloc == Singleton())
            return true;
    }
    return false;
}

XL_END

#endif // GC_H
