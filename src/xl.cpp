// *****************************************************************************
// xl.cpp                                                             XL project
// *****************************************************************************
//
// File description:
//
//    Main entry point of the XL runtime and compiler
//
//
//
//
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2019, Christophe de Dinechin <christophe@dinechin.org>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License,
// or (at your option) any later version.
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

#include "main.h"
#include "builtins.h"
#include <recorder/recorder.h>

#if HAVE_SBRK
#include <unistd.h>
#endif // HAVE_SBRK


int main(int argc, char **argv)
// ----------------------------------------------------------------------------
//   Parse the command line and run the compiler phases
// ----------------------------------------------------------------------------
{
    recorder_dump_on_common_signals(0, 0);
    record(main, "XL Compiler version %+s starting", XL_VERSION);

#if HAVE_SBRK
    char *low_water = (char *) sbrk(0);
#endif

    XL::path_list bin { XL_BIN,
                        "/usr/local/bin", "/bin", "/usr/bin" };
    XL::path_list lib { "../lib/xl", "../lib",
                        XL_LIB,
                        "/usr/local/lib/xl", "/lib/xl", "/usr/lib/xl"  };
    XL::path_list path { };
    XL::Main main(argc, argv, path, bin, lib, "xl");

    // Load files and run code
    XL::Tree_p result = main.LoadFiles();

    // Show errors if we had any
    int rc = main.HadErrors() > 0;
    if (rc)
    {
        main.errors->Display();
        main.errors->Clear();
    }
    else
    {
        result = main.Show(std::cout, result);
    }

    IFTRACE2(memory, gc_statistics)
        XL::GarbageCollector::GC()->PrintStatistics();

#if HAVE_SBRK
    record(main, "Total memory usage %ldK\n",
           long ((char *) sbrk(1) - low_water) / 1024);
#endif

    RECORD(main, "Compiler exit code %d", rc);

    if (RECORDER_TWEAK(dump_on_exit))
        recorder_dump();

    return rc;
}
