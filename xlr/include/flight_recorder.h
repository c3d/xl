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
// This software is property of Taodyne SAS - Confidential
// Ce logiciel est la propriété de Taodyne SAS - Confidentiel
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
                                     enabled(REC_CRITICAL | REC_DEBUG),
                                     records(size) {}

public:
    // Interface for a given recorder
    void Record(ulong when,
                kstring what, void *caller = NULL,
                kstring l1="", intptr_t a1=0,
                kstring l2="", intptr_t a2=0,
                kstring l3="", intptr_t a3=0)
    {
        if (when & (enabled | REC_ALWAYS))
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
        }
    }

    void Dump(int fd, bool consume = false);
    void Resize(uint size) { records.resize(size); }

public:
    // Static interface
    static void Initialize()   { if (!recorder) recorder = new FlightRecorder; }
    static void SRecord(ulong when,
                        kstring what, void *caller = NULL,
                        kstring l1="", intptr_t a1=0,
                        kstring l2="", intptr_t a2=0,
                        kstring l3="", intptr_t a3=0)
    {
        recorder->Record(when, what, caller, l1, a1, l2, a2, l3, a3);
    }
    static void SDump(int fd, bool kill=false) { recorder->Dump(fd,kill); }
    static void SResize(uint size) { recorder->Resize(size); }


public:
    uint        windex, rindex;
    ulonglong   enabled;
    std::vector<Entry>  records;

private:
    static FlightRecorder *     recorder;
};


#define RECORD(cond, what, args...)                             \
    (XL::FlightRecorder::SRecord(XL::REC_##cond, what,          \
                                 __builtin_return_address(0),   \
                                 ##args))

XL_END

// For use within a debugger session
extern void recorder_dump();

#endif // FLIGHT_RECORDER_H
