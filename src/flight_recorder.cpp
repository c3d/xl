// ****************************************************************************
//  flight_recorder.cpp                                           Tao project
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

#include "flight_recorder.h"
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/time.h>

ELFE_BEGIN

FlightRecorder<>  ERROR_RECORD("Errors");
FlightRecorder<>  DEBUG_RECORD("Debug");
FlightRecorder<>  OPTIONS_RECORD("Options");
FlightRecorder<>  MEMORY_RECORD("Memory");
FlightRecorder<>  COMPILER_RECORD("Compiler");
FlightRecorder<>  EVAL_RECORD("Evaluation");
FlightRecorder<>  PRIMITIVES_RECORD("Primitives");


Atomic<FlightRecorderBase *> FlightRecorderBase::head = NULL;
Atomic<intptr_t>             FlightRecorderBase::order = 0;
Atomic<unsigned>             FlightRecorderBase::blocked = 0;

intptr_t FlightRecorderBase::Now()
// ----------------------------------------------------------------------------
//    Give a high-resolution timer for the flight recorder
// ----------------------------------------------------------------------------
{
    struct timeval t;
    gettimeofday(&t, NULL);
    intptr_t tick = 
        (sizeof(uintptr_t) == 8)
        ? t.tv_sec * 1000000ULL + t.tv_usec
        : t.tv_sec * 1000ULL + t.tv_usec / 1000; // Wraps around in 49 days
    static intptr_t initialTick = 0;
    if (!initialTick)
        initialTick = tick;
    return tick - initialTick;
}


void FlightRecorderBase::Link()
// ----------------------------------------------------------------------------
//    Link a flight recorder that was activated into the list
// ----------------------------------------------------------------------------
{
    LinkedListInsert(head, this);
}


std::ostream &FlightRecorderBase::Dump(ostream &out)
// ----------------------------------------------------------------------------
//   Dump current flight recorder into given stream
// ----------------------------------------------------------------------------
{
    static char buffer[1024];
    static char format_buffer[32];

    Block();

    kstring name = Name();
    out << "DUMPING " << name
        << " SIZE " << Size()
        << ", " << Readable() << " ENTRIES\n";
        
    Entry entry;
    while (Readable() > 0)
    {
        // The read function may return 0 if we had to catch-up
        if (Read(entry))
        {
            if (sizeof(uintptr_t) == 8)
            {
                // Time stamp in us, show in seconds
                snprintf(buffer, sizeof(buffer),
                         "%lu [%lu.%06lu:%p] %s: ",
                         entry.order,
                         entry.timestamp / 1000000,
                         entry.timestamp % 1000000,
                         entry.caller,
                         name);
                out << buffer;
            }
            else
            {
                // Time stamp  in ms, show in seconds
                snprintf(buffer, sizeof(buffer),
                         "%lu [%lu.%03lu:%p] %s: ",
                         (ulong) entry.order,
                         (ulong) entry.timestamp / 1000,
                         (ulong) entry.timestamp % 1000,
                         entry.caller,
                         name);
            }

            const char *fmt = entry.what;
            char *dst = buffer;
            unsigned argIndex = 0;

            // Apply formatting. This complicated loop is because
            // we need to detect floating-point formats, which are passed
            // differently on many architectures such as x86 or ARM
            // (passed in different registers), and so we need to cast here.
            kstring end = buffer + sizeof buffer;
            while (dst < end)
            {
                char c = *fmt++;
                if (c != '%')
                {
                    *dst++ = c;
                    if (!c)
                        break;
                }
                else
                {
                    char *fmtCopy = format_buffer;
                    int floatingPoint = 0;
                    int done = 0;
                    *fmtCopy++ = c;
                    char *fmt_end = format_buffer + sizeof format_buffer - 1;
                    while (!done && fmt < fmt_end)
                    {
                        c = *fmt++;
                        *fmtCopy++ = c;
                        switch(c)
                        {
                        case 'f': case 'F':  // Floating point formatting
                        case 'g': case 'G':
                        case 'e': case 'E':
                        case 'a': case 'A':
                            floatingPoint = 1;
                            // Fall through here on purpose
                        case 'b':           // Integer formatting
                        case 'c': case 'C':
                        case 's': case 'S':
                        case 'd': case 'D':
                        case 'i':
                        case 'o': case 'O':
                        case 'u': case 'U':
                        case 'x':
                        case 'X':
                        case 'p':
                        case '%':
                        case 'n':           // Does not make sense here, but hey
                        case 0:             // End of string
                            done = 1;
                            break;
                        case '0' ... '9':
                        case '+':
                        case '-':
                        case '.':
                        case 'l': case 'L':
                        case 'h':
                        case 'j':
                        case 't':
                        case 'z':
                        case 'q':
                        case 'v':
                            break;
                        }
                    }
                    if (!c)
                        break;
                    *fmtCopy++ = 0;
                    if (floatingPoint)
                    {
                        double floatArg;
                        if (sizeof(intptr_t) == sizeof(float))
                        {
                            union { float f; intptr_t i; } u;
                            u.i = entry.args[argIndex++];
                            floatArg = u.f;
                        }
                        else
                        {
                            union { double d; intptr_t i; } u;
                            u.i = entry.args[argIndex++];
                            floatArg = u.d;
                        }
                        dst += snprintf(dst, end-dst, format_buffer, floatArg);
                    }
                    else
                    {
                        intptr_t intArg = entry.args[argIndex++];
                        dst += snprintf(dst, end-dst, format_buffer, intArg);
                    }
                }
            }
            out << buffer << "\n";
        }
        else
        {
            // Indicate we skipped some entries
            out << "... " << Readable() << " more entries\n";
        }
    }
    Unblock();

    return out;
}


std::ostream &FlightRecorderBase::DumpAll(ostream &out, kstring pattern)
// ----------------------------------------------------------------------------
//   Dump all recorders matching the pattern
// ----------------------------------------------------------------------------
{
    Block();

    time_t now = time(NULL);
    out << "FLIGHT RECORDER DUMP " << pattern
        << " AT " << asctime(localtime(&now));
    
    for (FlightRecorderBase *rec = Head(); rec; rec = rec->Next())
        rec->Dump(out);
    Unblock();
    return out;
}

ELFE_END

void recorder_dump()
// ----------------------------------------------------------------------------
//   Dump the recorder to standard error (for use in the debugger)
// ----------------------------------------------------------------------------
{
    recorder_dump_one("");
}



void recorder_dump_one(kstring select)
// ----------------------------------------------------------------------------
//   Dump one specific recorder to standard error
// ----------------------------------------------------------------------------
{
    ELFE::FlightRecorderBase::DumpAll(std::cout, select);
}
