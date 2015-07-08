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

ELIOT_BEGIN

static void Write(int fd, const char *buf, size_t size)
// ----------------------------------------------------------------------------
//   write() wrapper
// ----------------------------------------------------------------------------
{
    size_t left = size;
    while (left)
    {
        size_t s = write(fd, buf + size - left, left);
        if (s > 0)
            left -= s;
    }
}


void FlightRecorder::Dump(int fd, bool kill)
// ----------------------------------------------------------------------------
//   Dump the contents of the flight recorder to given stream
// ----------------------------------------------------------------------------
//   We use the lowest-possible sytem-level I/O facility to make it
//   easier to invoke Dump() from a variety of contexts
{
    using namespace std;
    static char buffer[512];

    // Write recorder time stamp
    time_t now = time(NULL);
    size_t size = snprintf(buffer, sizeof buffer,
                           "FLIGHT RECORDER DUMP AT %s\n",
                           asctime(localtime(&now)));
    Write(fd, buffer, size);

    // Can't have more events than the size of the buffer
    uint rindex = this->rindex;
    if (rindex + records.size() <= windex)
        rindex = windex - records.size() + 1;

    // Write all elements that remain to be shown
    while (rindex < windex)
    {
        Entry &e = records[rindex % records.size()];
        size = snprintf(buffer, sizeof buffer,
                        "%4d: %16s %8p ", windex - rindex, e.what, e.caller);

        if (e.label1[0])
            size += snprintf(buffer + size, sizeof buffer - size,
                             "%8s=%10p", e.label1, (void *) e.arg1);
        if (e.label2[0])
            size += snprintf(buffer + size, sizeof buffer - size,
                             "%8s=%10p", e.label2, (void *) e.arg2);
        if (e.label3[0])
            size += snprintf(buffer + size, sizeof buffer - size,
                             "%8s=%10p", e.label3, (void *) e.arg3);
        if (size < sizeof buffer)
            buffer[size++] = '\n';
        Write(fd, buffer, size);

        // Next step
        rindex++;
    }

    if (kill)
        this->rindex = rindex;
}


FlightRecorder * FlightRecorder::recorder = NULL;
ulong            FlightRecorder::enabled  = REC_ALWAYS|REC_CRITICAL|REC_DEBUG;

ELIOT_END

void recorder_dump()
// ----------------------------------------------------------------------------
//   Dump the recorder to standard error (for use in the debugger)
// ----------------------------------------------------------------------------
{
    ELIOT::FlightRecorder::SDump(2);
}
