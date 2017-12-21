// ****************************************************************************
//   Christophe de Dinechin                                        XL2 PROJECT
//   XL COMPILER: options.cpp
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <iterator>
#include "options.h"
#include "errors.h"
#include "renderer.h"
#include "flight_recorder.h"


XL_BEGIN

/* ========================================================================= */
/*                                                                           */
/*   The compiler options parsing                                            */
/*                                                                           */
/* ========================================================================= */

Options *Options::options = NULL;

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
    if (kstring envopt = getenv("XLOPT"))
    {
        RECORD(INFO, "Options from XLOPT", envopt);

        // Split space-separated input options and prepend them to args[]
        text envtext = envopt;
        std::istringstream input(envtext);
        std::copy(std::istream_iterator<text> (input),
                  std::istream_iterator<text> (),
                  std::back_inserter< std::vector<text> > (args));
    }

    // Add options from the command-line
    RECORD(INFO, "Options list", "Count", argc);
    for (int a = 1; a < argc; a++)
    {
        RECORD(INFO, "Option", "Index", a, argv[a]);
        args.push_back(argv[a]);
    }
}


static void Usage(kstring appName)
// ----------------------------------------------------------------------------
//    Display usage information when an invalid name is given
// ----------------------------------------------------------------------------
{
    std::cerr << "Usage:\n";
    std::cerr << appName << " <options> <source_file>\n"
              << "   (option names can be shortened if not ambiguous)\n";

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


template <class TextArray>
static inline bool OptionMatches(Options &options,
                                 kstring command_line,
                                 const TextArray &optdescr)
// ----------------------------------------------------------------------------
//   Check if a given option matches the command line
// ----------------------------------------------------------------------------
// Single character options may accept argument as same or next parameter
{
    const unsigned size = sizeof(optdescr) - 1;
    bool matches = true;
    for (unsigned i = 0; matches && i < size && command_line[i]; i++)
        matches = command_line[i] == optdescr[i];
    if (matches)
    {
        options.argt = command_line + size;
        return true;
    }
    return false;
}


static kstring OptionString(Options &opt)
// ----------------------------------------------------------------------------
//   Check if we find an integer between low and high on the command line
// ----------------------------------------------------------------------------
{
    if (*opt.argt == 0)
    {
        if (opt.arg + 1 < opt.args.size())
        {
            opt.arg++;
            opt.argt = opt.args[opt.arg].c_str();
        }
        if (*opt.argt == 0)
            Ooops("Option #$1 does not exist", Error::COMMAND_LINE)
                .Arg(opt.arg);
    }
    return opt.argt;
}


static ulong OptionInteger(Options &opt, ulong low, ulong high)
// ----------------------------------------------------------------------------
//   Check if we find an integer between low and high on the command line
// ----------------------------------------------------------------------------
{
    uint result = low;
    kstring val = OptionString(opt);
    if (isdigit(*val))
        result = strtol(val, (char**) &val, 10);
    else
        Ooops("Option #$1 ($2) is not an integer value", Error::COMMAND_LINE)
            .Arg(opt.arg).Arg(val);
    if (*val)
        Ooops("Garbage $1 after integer value", Error::COMMAND_LINE)
            .Arg(val);

    if (result < low || result > high)
    {
        char lowstr[15], highstr[15];
        sprintf(lowstr, "%lu", low);
        sprintf(highstr, "%lu", high);
        Ooops("Option $1 is out of range $2..$3", Error::COMMAND_LINE)
            .Arg(opt.args[opt.arg].c_str()).Arg(lowstr).Arg(highstr);
        if (result < low)
            result = low;
        else
            result = high;
    }
    return result;
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
    enum
    {
        OPTION_INVALID,
#define OPTVAR(name, type, value)
#define OPTION(name, descr, code)       OPTION_##name,
#include "options.tbl"
        OPTION_COUNT
    } selected = OPTION_INVALID;
    static const char *option_name[] = {
        "invalid",
#define OPTVAR(name, descr, code)
#define OPTION(name, descr, code)       #name,
#include "options.tbl"
    };

    while (arg < args.size())
    {
        if(args[arg].length() > 1 && args[arg][0] == '-')
        {
            kstring option = args[arg].c_str();
            kstring argval = option + 1;

            RECORD(INFO, "Parse option", "Index", arg, option);

            selected = OPTION_INVALID;

#if XL_DEBUG
            if (argval[0] == 't')
            {
                kstring trace_name = argval + 1;
                Traces::enable(trace_name);
                arg++;
                continue;
            }
#endif
            // Pass LLVM options as is (they are caught in compiler init)
            if (strncmp(argval, "llvm", 4) == 0)
            {
                arg++;
                continue;
            }

#define OPTVAR(name, type, value)
#define OPTION(name, descr, code)                                       \
            if (OptionMatches(*this, argval, #name))                    \
            {                                                           \
                if (selected != OPTION_INVALID)                         \
                    Ooops("Ambiguous option $1, "                       \
                          "selected " #name " instead of $2",           \
                          Error::COMMAND_LINE)                          \
                        .Arg(argval)                                    \
                        .Arg(option_name[selected]);                    \
                selected = OPTION_##name;                               \
            }
#include "options.tbl"

            switch(selected)
            {
#define OPTVAR(name, type, value)
#define OPTION(name, descr, code)                                       \
                case OPTION_##name:                                     \
                {                                                       \
                    code;                                               \
                }                                                       \
                break;
#define INTEGER(n, m)           OptionInteger(*this, n, m)
#define STRING                  OptionString(*this)
#include "options.tbl"

            default:
                // Default: Output usage
                Ooops("Unknown option $1 ignored", Error::COMMAND_LINE)
                    .Arg(option);
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
