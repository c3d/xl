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
// This software is property of Taodyne SAS - Confidential
// Ce logiciel est la propriété de Taodyne SAS - Confidentiel
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#include "flight_recorder.h"
#include <stdio.h>
#include <time.h>
#include <fcntl.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

XL_BEGIN

void FlightRecorder::Dump(int fd)
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
    write(fd, buffer, size);

    // Can't have more events than the size of the buffer
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
                             "%8s=%8" PRIdPTR " ", e.label1, e.arg1);
        if (e.label2[0])
            size += snprintf(buffer + size, sizeof buffer - size,
                             "%8s=%8" PRIdPTR " ", e.label2, e.arg2);
        if (e.label3[0])
            size += snprintf(buffer + size, sizeof buffer - size,
                             "%8s=%8" PRIdPTR " ", e.label3, e.arg3);
        if (size < sizeof buffer)
            buffer[size++] = '\n';
        write(fd, buffer, size);

        // Next step
        rindex++;
    }
}

FlightRecorder *     FlightRecorder::recorder = NULL;

XL_END

void recorder_dump()
// ----------------------------------------------------------------------------
//   Dump the recorder to standard error (for use in the debugger)
// ----------------------------------------------------------------------------
{
    XL::FlightRecorder::SDump(2);
}
