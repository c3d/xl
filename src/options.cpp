// ****************************************************************************
//   Christophe de Dinechin                                      XL PROJECT
//   options.cpp
// ****************************************************************************
//
//   File Description:
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
// ****************************************************************************
// This document is distributed under the GNU General Public License
// See the enclosed COPYING file or http://www.gnu.org for information
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#include "options.h"
#include "errors.h"
#include "renderer.h"
#include "recorder.h"
#include "main.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <iterator>


XL_BEGIN

/* ========================================================================= */
/*                                                                           */
/*   The compiler options parsing                                            */
/*                                                                           */
/* ========================================================================= */

Options *Options::options = NULL;
RECORDER(options, 128, "Options passed to the compiler");

Options::Options(int argc, char **argv):
/*---------------------------------------------------------------------------*/
/*  Set the default values for all options                                   */
/*---------------------------------------------------------------------------*/
#define OPTVAR(name, type, value)       name(value),
#define OPTION(name, descr, code)
#include "options.tbl"
    arg(1), args()
{
    // Store name of the program
    args.push_back(argv[0]);

    // Check if some options are given from environment
    if (kstring envopt = getenv("XL_OPT"))
    {
        RECORD(options, "Environment variable XL_OPT='%s'", envopt);

        // Split space-separated input options and prepend them to args[]
        text envtext = envopt;
        std::istringstream input(envtext);
        std::copy(std::istream_iterator<text> (input),
                  std::istream_iterator<text> (),
                  std::back_inserter< std::vector<text> > (args));
    }
    else
    {
        RECORD(options, "Environment variable XL_OPT is not set");
    }

    // Add options from the command-line
    RECORD(options, "Command line has %d options", argc-1);
    for (int a = 1; a < argc; a++)
    {
        RECORD(options, "  Option #%d is '%s'", a, argv[a]);
        args.push_back(argv[a]);
    }
}


static void Usage(kstring appName)
// ----------------------------------------------------------------------------
//    Display usage information when an invalid name is given
// ----------------------------------------------------------------------------
{
    std::cerr << "Usage:\n";
    std::cerr << appName << " <options> <source_file>\n";

#define OPTVAR(name, type, value)
#define OPTION(name, descr, code)                                       \
    std::cerr << "\t-" << #name ": " descr "\n";
#include "options.tbl"
#if XL_DEBUG
    std::set<std::string> names = XL::Traces::names();
    if (names.size())
    {
        std::cerr << "\t-t<name>: Enable trace <name>. ";
        std::cerr << "Valid trace names are:\n";
        std::cerr << "\t          ";
        std::set<std::string>::iterator it;
        for (it = names.begin(); it != names.end(); it++)
            std::cerr << (*it) << " ";
        std::cerr << "\n";
    }
#endif
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


static kstring OptionString(kstring &command_line, Options &opt)
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
    opt.arg += 1;
    if (opt.arg  < opt.args.size())
    {
        command_line = "";
        return opt.args[opt.arg].c_str();
    }
    Ooops("Option #$1 does not exist", Tree::COMMAND_LINE)
        .Arg(opt.arg);
    return "";
}


static ulong OptionInteger(kstring &command_line, Options &opt,
                           ulong low, ulong high)
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
            Ooops("Option $1 is not an integer value", Tree::COMMAND_LINE)
                .Arg(command_line);
    }
    else
    {
        opt.arg += 1;
        if (opt.arg  < opt.args.size() && isdigit(opt.args[opt.arg][0]))
            result = strtol(old = opt.args[opt.arg].c_str(),
                            (char **) &command_line, 10);
        else
            Ooops("Option $1 is not an integer value", Tree::COMMAND_LINE)
                .Arg(old);
    }
    if (result < low || result > high)
    {
        char lowstr[15], highstr[15];
        sprintf(lowstr, "%lu", low);
        sprintf(highstr, "%lu", high);
        Ooops("Option $1 is out of range $2..$3", Tree::COMMAND_LINE)
            .Arg(result).Arg(low).Arg(high);
        if (result < low)
            result = low;
        else
            result = high;
    }
    return result;
}


static void PassOptionToLLVM(kstring &command_line)
// ----------------------------------------------------------------------------
//   An option beginning with -llvm is passed to llvm as is
// ----------------------------------------------------------------------------
{
    while (*command_line++) /* Skip */;
}


text Options::ParseFirst(bool consumeFile)
// ----------------------------------------------------------------------------
//   Start parsing options, return first non-option
// ----------------------------------------------------------------------------
{
    // Parse next option
    arg = 1;
    return ParseNext(consumeFile);
}


text Options::ParseNext(bool consumeFiles)
// ----------------------------------------------------------------------------
//   Parse the command line, looking for known options, return first unknown
// ----------------------------------------------------------------------------
// Note: What we read here should be compatible with GCC parsing
{
    while (arg < args.size())
    {
        if(args[arg].length() > 1 && args[arg][0] == '-')
        {
            kstring option = args[arg].c_str() + 1;
            kstring argval = option;

            RECORD(options, "Parse option #%d, '%s'", arg, option);

#if XL_DEBUG
            if (argval[0] == 't')
            {
                kstring trace_name = argval + 1;
                Traces::enable(trace_name);
            }
            else
#endif

#define OPTVAR(name, type, value)
#define OPTION(name, descr, code)                                       \
            if (OptionMatches(argval, #name))                           \
            {                                                           \
                code;                                                   \
                                                                        \
                if (*argval)                                            \
                    Ooops("Garbage found after option -$1 : $2",        \
                          Tree::COMMAND_LINE).Arg(#name).Arg(option);   \
            }                                                           \
            else
#define INTEGER(n, m)           OptionInteger(argval, *this, n, m)
#define STRING                  OptionString(argval, *this)
#include "options.tbl"
            {
                // Default: Output usage
                Ooops("Unknown option $1 ignored", Tree::COMMAND_LINE)
                    .Arg(argval);
                Usage(args[0].c_str());
            }
            arg++;
        }
        else
        {
            text fileName = args[arg];
            if (consumeFiles)
            {
                arg++;
                files.push_back(fileName);
            }
            return fileName;
        }
    }
    return text("");
}

XL_END
ulong xl_traces = 0;
// ----------------------------------------------------------------------------
//   Bits for each trace
// ----------------------------------------------------------------------------
