// *****************************************************************************
// options.cpp                                                        XL project
// *****************************************************************************
//
// File description:
//
//     Processing of XL compiler options
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
// (C) 2011, Catherine Burvelle <catherine@taodyne.com>
// (C) 2003-2004,2009-2011,2013-2020, Christophe de Dinechin <cdedinechin@dxo.com>
// (C) 2010, Jérôme Forissier <jerome@taodyne.com>
// (C) 2003, Juan Carlos Arevalo Baeza <thejcab@sourceforge.net>
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <string>
#include <iostream>
#include "options.h"
#include "errors.h"



/* ========================================================================= */
/*                                                                           */
/*   The compiler options parsing                                            */
/*                                                                           */
/* ========================================================================= */

XLOptions::XLOptions():
/*---------------------------------------------------------------------------*/
/*  Set the default values for all options                                   */
/*---------------------------------------------------------------------------*/
#define OPTVAR(name, type, value)       name(value),
#define OPTION(name, descr, code)
#define TRACE(name)
#include "options.tbl"
    arg(0), argc(0), argv(NULL)
{}


static void Usage(char **argv)
// ----------------------------------------------------------------------------
//    Display usage information when an invalid name is given
// ----------------------------------------------------------------------------
{
    std::cerr << "Usage:\n";
    std::cerr << argv[0] << " <options> <source_file>\n";

#define OPTVAR(name, type, value)
#define OPTION(name, descr, code)                                       \
    std::cerr << "\t-" << #name ": " descr "\n";
#if DEBUG
#define TRACE(name)                 std::cerr << "\t-t" #name ": Trace " #name "\n";
#else
#define TRACE(name)
#endif
#include "options.tbl"
}


static bool OptionMatches(kstring &command_line, kstring optdescr)
// ----------------------------------------------------------------------------
//   Check if a given option matches the command line
// ----------------------------------------------------------------------------
// Single character options may accept argument as same or next parameter
{
    size_t len = strlen(optdescr);
    if (strncmp(command_line, optdescr, len) == 0)
    {
        command_line += len;
        return true;
    }
    return false;
}


static kstring OptionString(kstring &command_line,
                            int &arg, int argc, char **argv)
// ----------------------------------------------------------------------------
//   Check if we find an integer between low and high on the command line
// ----------------------------------------------------------------------------
{
    if (*command_line)
    {
        kstring result = command_line;
        command_line = "";
        return result;
    }
    arg += 1;
    if (arg  < argc)
    {
        command_line = "";
        return argv[arg];
    }
    XLError(E_OptNotIntegral, "<cmdline>", arg, command_line);
    return "";
}


static uint OptionInteger(kstring &command_line,
                          int &arg, int argc, char **argv,
                          uint low, uint high)
// ----------------------------------------------------------------------------
//   Check if we find an integer between low and high on the command line
// ----------------------------------------------------------------------------
{
    uint result = low;
    kstring old = command_line;
    if (*command_line)
    {
        if (isdigit(*command_line))
            result = strtol(command_line, (char**) &command_line, 10);
        else
            XLError(E_OptNotIntegral, "<cmdline>", arg, command_line);
    }
    else
    {
        arg += 1;
        if (arg  < argc && isdigit(argv[arg][0]))
            result = strtol(old = argv[arg], (char **) &command_line, 10);
        else
            XLError(E_OptNotIntegral, "<cmdline>", arg, command_line);
    }
    if (result < low || result > high)
    {
        char lowstr[15], highstr[15];
        sprintf(lowstr, "%d", low);
        sprintf(highstr, "%d", high);
        XLError(E_OptValueRange, "<cmdline>", arg, old, lowstr, highstr);
        if (result < low)
            result = low;
        else
            result = high;
    }
    return result;
}


text XLOptions::Parse(int argc, char **argv)
// ----------------------------------------------------------------------------
//   Start parsing options, return first non-option
// ----------------------------------------------------------------------------
{
    this->arg = 1;
    this->argc = argc;
    this->argv = argv;
    if (cstring envopt = getenv("XLOPTIONS"))
    {
        this->argv[0] = envopt;
        this->arg = 0;
    }
    return ParseNext();
}


text XLOptions::ParseNext()
// ----------------------------------------------------------------------------
//   Parse the command line, looking for known options, return first unknown
// ----------------------------------------------------------------------------
// Note: What we read here should be compatible with GCC parsing
{
    while (arg < argc)
    {
        if(argv[arg] && argv[arg][0] == '-' && argv[arg][1])
        {
            kstring argval = argv[arg] + 1;

#define OPTVAR(name, type, value)
#define OPTION(name, descr, code)                                       \
            if (OptionMatches(argval, #name))                           \
            {                                                           \
                code;                                                   \
                                                                        \
                if (*argval)                                            \
                    XLError(E_OptGarbage, "<cmdline>", arg, argval);    \
            }                                                           \
            else
#if MZ_DEBUG
#define TRACE(name)                                 \
            if (OptionMatches(argval, "t" #name))   \
                mz_traces |= 1 << MZ_TRACE_##name;  \
            else
#else
#define TRACE(name)
#endif
#define INTEGER(n, m)           OptionInteger(argval, arg, argc, argv, n, m)
#define STRING                  OptionString(argval, arg, argc, argv)
#include "options.tbl"
            {
                // Default: Output usage
                XLError(E_OptInvalid, "<cmdline>", arg, argval);
                Usage(argv);
            }
            arg++;
        }
        else
        {
            return text(argv[arg++]);
        }
    }
    return text("");
}


XLOptions gOptions;
/*---------------------------------------------------------------------------*/
/*  The global options used by all parts of the compiler                     */
/*---------------------------------------------------------------------------*/


ulong mz_traces = 0;
// ----------------------------------------------------------------------------
//   Bits for each trace
// ----------------------------------------------------------------------------
