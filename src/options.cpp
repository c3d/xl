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
// This software is licensed under the GNU General Public License v3
// (C) 2011, Catherine Burvelle <catherine@taodyne.com>
// (C) 2003-2004,2009-2011,2013-2019, Christophe de Dinechin <cdedinechin@dxo.com>
// (C) 2010, Jérôme Forissier <jerome@taodyne.com>
// (C) 2003, Juan Carlos Arevalo Baeza <thejcab@sourceforge.net>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can redistribute it and/or modify
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

#include "options.h"
#include "errors.h"
#include "renderer.h"
#include "main.h"
#include "save.h"

#include <recorder/recorder.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <ctype.h>
#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <sys/ioctl.h>
#include <unistd.h>



RECORDER_DEFINE(options,        64, "Compiler options");
RECORDER_DEFINE(option_parsing, 16, "Processing compiler options");

XL_BEGIN
// ============================================================================
//
//    Options for this module
//
// ============================================================================

CodeOption      help("help",
                     "Show usage for the program and list available options",
                     [](Option &opt, Options &opts)
                     {
                         opts.Usage();
                         exit(0);
                     });



// ============================================================================
//
//   Option class
//
// ============================================================================

Option *Option::options = nullptr;


Option::Option(kstring name, kstring help)
// ----------------------------------------------------------------------------
//    Constructor adds option to linked list of options
// ----------------------------------------------------------------------------
    : name(name), help(help), next(options)
{
    options = this;
}


Option::~Option()
// ----------------------------------------------------------------------------
//    Destructor removes option from the list
// ----------------------------------------------------------------------------
{
    if (options == this)
    {
        options = next;
        return;
    }

    Option *last = nullptr;
    for (Option *opt = options; opt; opt = opt->next)
    {
        if (opt == this)
        {
            last->next = this->next;
            return;
        }
        last = opt;
    }
}


static inline bool commandArg(char c)
// ----------------------------------------------------------------------------
//   Return true for characters that indicate an arg on the command-line
// ----------------------------------------------------------------------------
{
    return c == ':' || c == '=';
}


static inline bool commandSep(char c)
// ----------------------------------------------------------------------------
//   Return true for characters that end a command on the command-line
// ----------------------------------------------------------------------------
{
    return !c;
}


static inline bool commandEnd(char c)
// ----------------------------------------------------------------------------
//   Return true for characters that end a command on the command-line
// ----------------------------------------------------------------------------
{
    return commandSep(c) || commandArg(c);
}


kstring Option::SkipPrefix(kstring command)
// ----------------------------------------------------------------------------
//   For most options, there are no acceptable prefix
// ----------------------------------------------------------------------------
{
    return command;
}


kstring Option::Matches(Options &opts)
// ----------------------------------------------------------------------------
//   Return number of characters matched, 0 if mismatch
// ----------------------------------------------------------------------------
{
    kstring command = opts.Command();
    command = SkipPrefix(command);

    // Match as many characters as possible
    kstring match = name;
    while (*match && !commandEnd(*command) && *match == *command)
    {
        ++match;
        ++command;
    }

    // Return 0 if there is a mismatch
    if (*match && !commandEnd(*command))
    {
        record(option_parsing,
               "Mismatch after %u characters, where [%+s] != [%+s]",
               match - name, match, command);
        return nullptr;
    }

    // Update the argument text pointer
    record(option_parsing,
           "Matched %u characters of [%+s], remaining is [%+s]",
           match - name, name, command);
    return command;
}


void Option::Process(Options &opts)
// ----------------------------------------------------------------------------
//   Process a command line - By default there should be no argument
// ----------------------------------------------------------------------------
{
    record(option_parsing,
           "Default processing for option [%+s], input [%+s]",
           name, opts.Input());
}


void Option::Usage(int maxOptWidth, int maxWidth)
// ----------------------------------------------------------------------------
//   Print usage information for the option
// ----------------------------------------------------------------------------
{
    fprintf(stderr, "-%*s : ", -maxOptWidth, name);

    kstring hp = help;
    int available = maxWidth - maxOptWidth - 4;
    if (available < 20)
        available = INT_MAX;
    while (*hp)
    {
        if ((int) strlen(hp) <= available)
        {
            fprintf(stderr, "%s\n", hp);
            break;
        }

        int cut = available;
        for (int i = 0; i < available; i++)
            if (isspace(hp[i]))
                cut = i + 1;
        fprintf(stderr, "%.*s\n%*s", cut, hp, maxOptWidth + 4, "");
        hp += cut;
    }
}


kstring Option::Rename(kstring alias)
// ----------------------------------------------------------------------------
//   Run the given code under some alternate name, then restore it
// ----------------------------------------------------------------------------
{
    kstring old = name;
    name = alias;
    return old;
}



// ============================================================================
//
//    Boolean option
//
// ============================================================================
//
// Boolean options are somewhat special, because they are designed so that
// they can be used as flags, i.e. without an argument.
// Consider option 'sjhow' for example. You can use one of the following:
//   xl -show file.xl
//   xl -with-show file.xl
//   xl -no-show file.xl
//   xl -show=yes file.xl
//   xl -show:no file.xl
// However, you cannot put the argument after a space, i.e. the following
// is not accepted, since it would be ambiguous with the first form above.
//   xl -show yes file.xl
//

static inline kstring prefixMatch(kstring command,
                                  kstring match,
                                  int ref,
                                  int &value)
// ----------------------------------------------------------------------------
//    Check if we match some prefix, in which case set value to ref
// ----------------------------------------------------------------------------
{
    bool matched = true;
    kstring start = command;
    while (matched && *match)
        matched = tolower(*command++) == tolower(*match++);
    if (!matched)
        return start;
    if (*command == '-')
        command++;
    value = ref;
    return command;
}


static inline int prefixBooleanValue(kstring &command)
// ----------------------------------------------------------------------------
//   Return a value matching
// ----------------------------------------------------------------------------
{
    int value = -1;
    command = prefixMatch(command, "no",        false, value);
    command = prefixMatch(command, "without",   false, value);
    command = prefixMatch(command, "with",      true,  value);
    command = prefixMatch(command, "disable",   false, value);
    command = prefixMatch(command, "enable",    true,  value);
    return value;
}


kstring BooleanOption::SkipPrefix(kstring command)
// ----------------------------------------------------------------------------
//   Deal with prefix "no" or "no-" and "with-"
// ----------------------------------------------------------------------------
{
    (void) prefixBooleanValue(command);
    return command;
}


void BooleanOption::Process(Options &opts)
// ----------------------------------------------------------------------------
//   Process a command line - By default there should be no argument
// ----------------------------------------------------------------------------
{
    kstring command = opts.Command();
    int opt = prefixBooleanValue(command);
    record(option_parsing, "Boolean option [%+s] prefix is %d", command, opt);
    if (opt < 0)
    {
        if (opts.HasDirectArgument())
        {
            kstring arg = opts.Argument();
            prefixMatch(arg, "0",     false, opt);
            prefixMatch(arg, "no",    false, opt);
            prefixMatch(arg, "false", false, opt);
            prefixMatch(arg, "1",     true,  opt);
            prefixMatch(arg, "yes",   true,  opt);
            prefixMatch(arg, "true",  true,  opt);
            record(option_parsing,
                   "Boolean option [%+s] postfix %d", command, opt);
            if (opt < 0)
                Ooops("Invalid value $1 for $2 option", Tree::COMMAND_LINE)
                    .Arg(arg)
                    .Arg(name);
        }
        else
        {
            opt = true;
        }
    }
    if (opt >= 0)
        value = (bool) opt;
}



// ============================================================================
//
//    Integer and text option classes
//
// ============================================================================

void IntegerOption::Process(Options &opts)
// ----------------------------------------------------------------------------
//   Find integer value and check if the range matches
// ----------------------------------------------------------------------------
{
    kstring arg = opts.Argument();
    record(option_parsing,
           "Integer option [%+s] in %llu..%llu, current %llu",
           arg, min, max, value);
    char *end = (char *) arg;
    value = strtoll(arg, &end, 0);
    if (end == arg || *end != 0)
        Ooops("Invalid numerical value $1 for option $2", Tree::COMMAND_LINE)
            .Arg(arg)
            .Arg(name);
    if (value < min || value > max)
        Ooops("Value $1 for option $2 is outside of $3..$4", Tree::COMMAND_LINE)
            .Arg(value)
            .Arg(name)
            .Arg(min)
            .Arg(max);
}


void TextOption::Process(Options &opts)
// ----------------------------------------------------------------------------
//   Find text value and stores it
// ----------------------------------------------------------------------------
{
    kstring arg = opts.Argument();
    record(option_parsing, "Text option [%+s] current [%s]", arg, value);
    value = arg;
}


void CodeOption::Process(Options &opts)
// ----------------------------------------------------------------------------
//   Pass the arguments to the code
// ----------------------------------------------------------------------------
{
    record(option_parsing, "Code option [%+s]", name);
    code(*this, opts);
}



// ============================================================================
//
//    AliasOption
//
// ============================================================================

kstring AliasOption::SkipPrefix(kstring command)
// ----------------------------------------------------------------------------
//    Forward to the base option
// ----------------------------------------------------------------------------
{
    kstring old = alias.Rename(name);
    kstring result = alias.SkipPrefix(command);
    alias.Rename(old);
    return result;
}


kstring AliasOption::Matches(Options &opts)
// ----------------------------------------------------------------------------
//    Forward to the base option after changing the name
// ----------------------------------------------------------------------------
{
    kstring old = alias.Rename(name);
    kstring result = alias.Matches(opts);
    alias.Rename(old);
    return result;
}


void AliasOption::Process(Options &opts)
// ----------------------------------------------------------------------------
//   Forward to the base option after changing the name
// ----------------------------------------------------------------------------
{
    kstring old = alias.Rename(name);
    alias.Process(opts);
    alias.Rename(old);
}


void AliasOption::Usage(int maxOptWidth, int maxWidth)
// ----------------------------------------------------------------------------
//   Print the alias help string
// ----------------------------------------------------------------------------
{
    fprintf(stderr, "-%*s : Alias for %s\n", -maxOptWidth, name, alias.Name());
}



// ============================================================================
//
//    Compiler options parsing
//
// ============================================================================

Options::Options(int argc, char **argv)
// ----------------------------------------------------------------------------
//    Process all the given options
// ----------------------------------------------------------------------------
    : arg(1), args()
{
    // Store name of the program
    args.push_back(argv[0]);
    record(options, "Program name: %+s", argv[0]);

    // Check if some options are given from environment
    if (char *envtext = getenv("XL_OPT"))
    {
        record(options, "Options from XLOPT: %+s", envtext);

        // Split space-separated input options and prepend them to args[]
        size_t len = strlen(envtext);
        char *ptr = envtext;
        bool space = false;
        for (size_t i = 0; i < len; i++)
        {
            if (envtext[i] == ' ')
            {
                envtext[i] = 0;
                record(options, "New option from XLOPT: %+s", ptr);
                args.push_back(ptr);
                space = true;
            }
            else if (space)
            {
                ptr = envtext + i;
                space = false;
            }
        }
        args.push_back(ptr);
    }
    else
    {
        record(options, "Environment variable XL_OPT is not set");
    }

    // Add options from the command-line
    record(options, "Command line has %d options", argc-1);
    for (int a = 1; a < argc; a++)
    {
        record(options, "  Option #%d is '%+s'", a, argv[a]);
        args.push_back(argv[a]);
    }
}


void Options::Usage(void)
// ----------------------------------------------------------------------------
//    Display usage information when an invalid name is given
// ----------------------------------------------------------------------------
{
    kstring appName = args[0];
    fprintf(stderr, "Usage: %s <options> <source_file>\n\n", appName);
    fprintf(stderr, "Option names can be shortened if unambiguous.\n\n");

    int maxNameLen = 0;
    std::vector<Option *> sorted;
    for (Option *opt = Option::options; opt; opt = opt->next)
    {
        int len = strlen(opt->name);
        if (maxNameLen < len)
            maxNameLen = len;
        sorted.push_back(opt);
    }
    std::sort(sorted.begin(), sorted.end(), [](Option *left, Option *right)
              {
                  return strcasecmp(left->Name(), right->Name()) < 0;
              });

#ifdef TIOCGSIZE
    struct ttysize ts;
    ioctl(STDOUT_FILENO, TIOCGSIZE, &ts);
    int cols = ts.ts_cols;
    // int lines = ts.ts_lines;
#elif defined(TIOCGWINSZ)
    struct winsize ts;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ts);
    int cols = ts.ws_col;
    // int lines = ts.ws_row;
#endif /* TIOCGSIZE */

    for (Option *opt : sorted)
        opt->Usage(maxNameLen, cols);

#ifdef XL_DEBUG
    fprintf(stderr,
            "You can get the available recorder traces with -trace help\n");
#endif // XL_DEBUG
}


kstring Options::Input()
// ----------------------------------------------------------------------------
//   Return the start of the input argumment
// ----------------------------------------------------------------------------
{
    XL_ASSERT(arg < args.size());
    kstring input = args[arg];
    return input;
}


kstring Options::Command()
// ----------------------------------------------------------------------------
//   Return the start of the command, i.e. after '-' or '--'
// ----------------------------------------------------------------------------
{
    kstring command = Input();
    if (*command != '-')
        return nullptr;
    ++command;
    if (*command == '-')
        ++command;
    return command;
}


bool Options::HasDirectArgument()
// ----------------------------------------------------------------------------
//   Return true if there is a direct argument, i.e. with '=' or ':'
// ----------------------------------------------------------------------------
{
    XL_ASSERT(arg < args.size());
    kstring argp = args[arg] + length;
    return commandArg(*argp);
}


kstring Options::Argument()
// ----------------------------------------------------------------------------
//    Return the next argument and consume it
// ----------------------------------------------------------------------------
{
    XL_ASSERT(arg < args.size());
    kstring result = args[arg] + length;
    length = 0;
    if (commandArg(*result))
        return result + 1;
    if (commandSep(*result))
    {
        ++arg;
        if (arg >= args.size())
        {
            Ooops("Command-line option $1 needs argument", Tree::COMMAND_LINE)
                .Arg(selected);
            return "";
        }
        result = args[arg];
    }
    return result;
}


kstring Options::ParseFirst()
// ----------------------------------------------------------------------------
//   Start parsing options, return first non-option
// ----------------------------------------------------------------------------
{
    // Parse next option
    arg = 1;
    return ParseNext();
}


kstring Options::ParseNext()
// ----------------------------------------------------------------------------
//   Parse the command line, looking for known options, return first unknown
// ----------------------------------------------------------------------------
// Note: What we read here should be compatible with GCC parsing
{
    while (arg < args.size())
    {
        kstring input = Input();
        if (*input != '-')
        {
            ++arg;
            return input;
        }

        Option *best = nullptr;
        for (Option *opt = Option::options; opt; opt = opt->next)
        {
            record(option_parsing, "Matching #%d '%+s' against '%+s'",
                   arg, input, opt->Name());
            kstring match = opt->Matches(*this);
            if (match)
            {
                if (best)
                {
                    Ooops("Ambiguous command-line option $1 could be $2 or $3",
                          Tree::COMMAND_LINE)
                        .Arg(input)
                        .Arg(opt->Name())
                        .Arg(best->Name());
                }
                else
                {
                    best = opt;
                    length = match - input;
                    selected = opt->Name();
                }
            }
        }

        record(option_parsing, "Best option choice is [%+s] (%p)",
               best ? best->Name() : "nonexistent", best);
        if (!best)
        {
            Ooops("Command-line option $1 does not exist", Tree::COMMAND_LINE)
                .Arg(input);
            break;
        }
        best->Process(*this);
        if (length && !commandSep(input[length]))
        {

            Ooops("Option $1 does not expect argument $2", Tree::COMMAND_LINE)
                .Arg(input)
                .Arg(input + length);
            break;
        }
        ++arg;
    }
    return "";
}

XL_END
