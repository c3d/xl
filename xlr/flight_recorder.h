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
#include <vector>

XL_BEGIN

// ============================================================================
// 
//    Higher-evel interface
// 
// ============================================================================

enum FlightRecorderChannels
// ----------------------------------------------------------------------------
//   Different channels that can be recorded
// ----------------------------------------------------------------------------
{
    // General enablers
    REC_ALWAYS             = 1<<0,
    REC_CRITICAL           = 1<<1,
    REC_DEBUG              = 1<<2,
    REC_INFO               = 1<<3,

    // Domain-specific enablers
    REC_MEMORY_DETAILS     = 1<<8,
    REC_COMPILER_DETAILS   = 1<<9,
    REC_EVAL_DETAILS       = 1<<10,
    REC_PRIMITIVES_DETAILS = 1<<11,

    // High-level enablers
    REC_MEMORY             = REC_DEBUG | REC_MEMORY_DETAILS,
    REC_COMPILER           = REC_DEBUG | REC_COMPILER_DETAILS,
    REC_EVAL               = REC_DEBUG | REC_EVAL_DETAILS,
    REC_PRIMITIVES         = REC_DEBUG | REC_PRIMITIVES_DETAILS,
};


struct FlightRecorder
// ----------------------------------------------------------------------------
//    Record events 
// ----------------------------------------------------------------------------
{
    struct Entry
    {
        Entry(kstring what = "", void *caller = NULL,
              kstring l1="", intptr_t a1=0,
              kstring l2="", intptr_t a2=0,
              kstring l3="", intptr_t a3=0):
            what(what), caller(caller),
            label1(l1), label2(l2), label3(l3),
            arg1(a1), arg2(a2), arg3(a3) {}
        kstring  what;
        void *   caller;
        kstring  label1, label2, label3;
        intptr_t arg1, arg2, arg3;
    };

    FlightRecorder(uint size=4096) : windex(0), rindex(0),
                                     records(size) {}

public:
    // Interface for a given recorder
    ulong Record(kstring what, void *caller = NULL,
                 kstring l1="", intptr_t a1=0,
                 kstring l2="", intptr_t a2=0,
                 kstring l3="", intptr_t a3=0)
    {
        Entry &e = records[windex++ % records.size()];
        e.what = what;
        e.caller = caller;
        e.label1 = l1;
        e.arg1 = a1;
        e.label2 = l2;
        e.arg2 = a2;
        e.label3 = l3;
        e.arg3 = a3;
        return enabled;
    }

    void Dump(int fd, bool consume = false);
    void Resize(uint size) { records.resize(size); }

public:
    // Static interface
    static void Initialize()   { if (!recorder) recorder = new FlightRecorder; }
    static ulong SRecord(kstring what, void *caller = NULL,
                         kstring l1="", intptr_t a1=0,
                         kstring l2="", intptr_t a2=0,
                         kstring l3="", intptr_t a3=0)
    {
        Initialize();
        return recorder->Record(what, caller, l1, a1, l2, a2, l3, a3);
    }
    static void SDump(int fd, bool kill=false) { recorder->Dump(fd,kill); }
    static void SResize(uint size) { recorder->Resize(size); }
    static void SFlags(ulong en) { enabled = en | REC_ALWAYS; }

public:
    uint               windex, rindex;
    std::vector<Entry> records;

    static ulong            enabled;

private:
    static FlightRecorder * recorder;
};


#define RECORD(cond, what, args...)                                     \
    ((XL::REC_##cond) & (XL::FlightRecorder::enabled | XL::REC_ALWAYS)  \
     && XL::FlightRecorder::SRecord(what,                               \
                                    __builtin_return_address(0),        \
                                    ##args))

XL_END

// For use within a debugger session
extern void recorder_dump();

#endif // FLIGHT_RECORDER_H
