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
#include <iostream>
#include <fstream>
#include <iomanip>

XL_BEGIN

void FlightRecorder::Dump(std::ostream &out)
// ----------------------------------------------------------------------------
//   Dump the contents of the flight recorder to given stream
// ----------------------------------------------------------------------------
{
    using namespace std;

    if (rindex + records.size() <= windex)
        rindex = windex - records.size() + 1;

    time_t now = time(NULL);
    out << "FLIGHT RECORDER DUMP - " << asctime(localtime(&now)) << '\n';

    while (rindex < windex)
    {
        Entry &e = records[rindex % records.size()];
        out << setw(4) << windex - rindex << ' '
            << setw(16) << e.what << ' '
            << setw(18) << e.caller << ' ';
        if (e.label1[0])
            out << setw(8) << e.label1 << '=' << setw(8) << e.arg1 << ' ';
        if (e.label2[0])
            out << setw(8) << e.label2 << '=' << setw(8) << e.arg2 << ' ';
        if (e.label3[0])
            out << setw(8) << e.label3 << '=' << setw(8) << e.arg3;
        out << '\n';
        rindex++;
    }

    // Keep a reference to recorder_dump so that the linker preserves it
    // We need a convoluted expression so that GCC doesn't warn
    if ((void *) recorder_dump != (void *) 0x01)
        out.flush();
}

FlightRecorder *     FlightRecorder::recorder = NULL;

XL_END

void recorder_dump()
// ----------------------------------------------------------------------------
//   Dump the recorder to standard error (for use in the debugger)
// ----------------------------------------------------------------------------
{
    XL::FlightRecorder::SDump(std::cerr);
}
