// *****************************************************************************
// flight_recorder.cpp                                                XL project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2011,2013-2014,2019, Christophe de Dinechin <christophe@dinechin.org>
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

#include "flight_recorder.h"
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>

XL_BEGIN

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

#define AUTOFORMAT(x)                           \
        (((x) < 1000000 && (x) > -1000000)      \
         ? "%8s=%10" PRIdPTR " "                 \
         : "%8s=%#10" PRIxPTR " ")            

        if (e.label1[0])
            size += snprintf(buffer + size, sizeof buffer - size,
                             AUTOFORMAT(e.arg1), e.label1, e.arg1);
        if (e.label2[0])
            size += snprintf(buffer + size, sizeof buffer - size,
                             AUTOFORMAT(e.arg2), e.label2, e.arg2);
        if (e.label3[0])
            size += snprintf(buffer + size, sizeof buffer - size,
                             AUTOFORMAT(e.arg3), e.label3, e.arg3);
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

XL_END

void recorder_dump()
// ----------------------------------------------------------------------------
//   Dump the recorder to standard error (for use in the debugger)
// ----------------------------------------------------------------------------
{
    XL::FlightRecorder::SDump(2);
}
