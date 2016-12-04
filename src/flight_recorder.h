#ifndef FLIGHT_RECORDER_H
#define FLIGHT_RECORDER_H
// ****************************************************************************
//  flight_recorder.h                                             Tao project
// ****************************************************************************
//
//   File Description:
//
//     Record information about what's going on in the application
//
//
//
//
//
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
#include "ring.h"
#include <vector>
#include <iostream>

ELFE_BEGIN

// ============================================================================
//
//    Higher-evel interface
//
// ============================================================================

struct FlightRecorderBase
// ----------------------------------------------------------------------------
//    A base for all flight recorders, to store them in a linked list
// ----------------------------------------------------------------------------
{
    // The following struct is where we keep the data.
    // Should be a power-of-two size on all regular architectures
    struct Entry
    {
        kstring     what;
        intptr_t    order;
        intptr_t    timestamp;
        void *      caller;
        intptr_t    args[4];
    };

public:
    FlightRecorderBase(): next(NULL) {}

    virtual kstring             Name()                      = 0;
    virtual unsigned            Size()                      = 0;
    virtual unsigned            Readable()                  = 0;
    virtual unsigned            Writeable()                 = 0;
    virtual bool                Read(Entry &entry)          = 0;
    virtual unsigned            Write(const Entry &entry)   = 0;
    void                        Link();
    FlightRecorderBase *        Next()  { return next; }

    typedef std::ostream        ostream;
    ostream &                   Dump(ostream &out);

public:
    static FlightRecorderBase * Head()  { return head; }
    static ostream &            DumpAll(ostream &out, kstring pattern = "");
    static intptr_t             Order();
    static intptr_t             Now();
    static void *               Here();

    static void                 Block()     { blocked++; }
    static void                 Unblock()   { blocked--; }
    static bool                 Blocked()   { return blocked > 0; }

private:
    static Atomic<FlightRecorderBase *> head;
    static Atomic<intptr_t>     order;
    static Atomic<unsigned>     blocked;

private:
    FlightRecorderBase *        next;

    template <class Link>
    friend void LinkedListInsert(Atomic<Link> &list, Link link);
};


inline std::ostream &operator<< (std::ostream &out, FlightRecorderBase &fr)
// ----------------------------------------------------------------------------
//   Dump a single flight recorder entry on the given ostream
// ----------------------------------------------------------------------------
{
    return fr.Dump(out);
}


template <unsigned RecSize = 128>
struct FlightRecorder : private FlightRecorderBase,
                        private Ring<FlightRecorderBase::Entry, RecSize>
// ----------------------------------------------------------------------------
//    Record events in a circular buffer, up to Size events recorded
// ----------------------------------------------------------------------------
{
    typedef FlightRecorderBase  List;
    typedef List::Entry         Entry;
    typedef Ring<Entry, RecSize>Base;

    FlightRecorder(kstring name): FlightRecorderBase(), Base(name) {}

    virtual kstring             Name()              { return Base::Name(); }
    virtual unsigned            Size()              { return Base::size; }
    virtual unsigned            Readable()          { return Base::Readable(); }
    virtual unsigned            Writeable()         { return Base::Writable(); }
    virtual bool                Read(Entry &e)      { return Base::Read(e); }
    virtual unsigned            Write(const Entry&e){ return Base::Write(e); }

public:
    struct Arg
    {
        Arg(float f):           value(f2i(f))               {}
        Arg(double d):          value(f2i(d))               {}
        template<class T>
        Arg(T t):               value(intptr_t(t))          {}

        operator intptr_t()     { return value; }

    private:
        template <typename float_type>
        static intptr_t f2i(float_type f)
        {
            if (sizeof(float) == sizeof(intptr_t))
            {
                union { float f; intptr_t i; } u;
                u.f = f;
                return u.i;
            }
            else
            {
                union { double d; intptr_t i; } u;
                u.d = f;
                return u.i;
            }
        }

    private:
        intptr_t                value;
    };

    void Record(kstring what, Arg a1 = 0, Arg a2 = 0, Arg a3 = 0, Arg a4 = 0)
    {
        if (List::Blocked())
            return;
        Entry e = {
            what, List::Order(), List::Now(), List::Here(),
            { a1, a2, a3, a4 }
        };
        unsigned writeIndex = Base::Write(e);
        if (!writeIndex)
            List::Link();
    }

    void operator()(kstring what, Arg a1=0, Arg a2=0, Arg a3=0, Arg a4=0)
    {
        Record(what, a1, a2, a3, a4);
    }
};



// ============================================================================
//
//   Inline functions for FlightRecorderBase
//
// ============================================================================

inline void FlightRecorderBase::Link()
// ----------------------------------------------------------------------------
//    Link a flight recorder that was activated into the list
// ----------------------------------------------------------------------------
{
    LinkedListInsert(head, this);
}


inline intptr_t FlightRecorderBase::Order()
// ----------------------------------------------------------------------------
//   Generate a unique sequence number for ordering entries in recorders
// ----------------------------------------------------------------------------
{
    return order++;
}


inline void *FlightRecorderBase::Here()
// ----------------------------------------------------------------------------
//   Get the location where Here() is called from
// ----------------------------------------------------------------------------
{
    return (void *) (__builtin_return_address(0));
}


// ============================================================================
//
//    Available recorders
//
// ============================================================================

extern FlightRecorder<>  ERROR_RECORD;
extern FlightRecorder<>  DEBUG_RECORD;
extern FlightRecorder<>  OPTIONS_RECORD;
extern FlightRecorder<>  MEMORY_RECORD;
extern FlightRecorder<>  COMPILER_RECORD;
extern FlightRecorder<>  EVAL_RECORD;
extern FlightRecorder<>  PRIMITIVES_RECORD;

ELFE_END

// For use within a debugger session
extern "C" void recorder_dump();
extern "C" void recorder_dump_one(kstring select);

#endif // FLIGHT_RECORDER_H
