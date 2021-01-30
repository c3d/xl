#ifndef XL_OPTIONS_H
#define XL_OPTIONS_H
// *****************************************************************************
// options.h                                                          XL project
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
// (C) 2003,2009-2012,2014-2017,2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2010, Jérôme Forissier <jerome@taodyne.com>
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

#include "base.h"
#include <string>
#include <vector>
#include <functional>
#include <recorder/recorder.h>

XL_BEGIN


struct Options;

struct Option
// ----------------------------------------------------------------------------
//    Representation of a command-line option
// ----------------------------------------------------------------------------
//    Option foo can be identified on command-line as -foo or --foo.
//    Only the first characters required to avoid ambiguity are required.
//    Arguments can either
//    - Follow the argument directly, e.g. -O3 or -tlabels
//    - Follow in the next argument, e.g. -Optim 3 or -trace labels
//    - Follow an '=' or ':' character, e.g. -Opt:3 or -tra=labels
{

    Option(kstring name, kstring help);
    virtual ~Option();
    virtual kstring     SkipPrefix(kstring command);
    virtual kstring     Matches(Options &);
    virtual void        Process(Options &);
    virtual void        Usage(int maxOptWidth, int maxWidth = 80);
    kstring             Name()  { return name; }
    kstring             Rename(kstring alias);

protected:
    friend struct Options;
    kstring             name;
    kstring             help;
    Option *            next;
    static Option *     options;
};


struct BooleanOption : Option
// ----------------------------------------------------------------------------
//   An option accepting boolean values
// ----------------------------------------------------------------------------
//   The argument can be either "true", "false", "yes", "no", "0" or "1"
//   Not having a matching argument means the options is interpreted
//   as "yes" and the argument is not consumed
//   The option name can be prefixed with "no" or "no-" to disabled it,
//   e.g. "-nodebug" or "--no-debug" or "-debug=no" all mean the same
//   Conversely, the option can be prefixed with "with"
{
    BooleanOption(kstring name, kstring help, bool value = false)
        : Option(name, help), value(value) {}
    virtual kstring     SkipPrefix(kstring command);
    virtual void        Process(Options &);
    operator            bool() { return value; }
public:
    bool                value;
};


struct NaturalOption : Option
// ----------------------------------------------------------------------------
//   An option accepting natural values
// ----------------------------------------------------------------------------
{
    NaturalOption(kstring name, kstring help,
                  uint64_t value = 0,
                  uint64_t min = 0,
                  uint64_t max = UINT64_MAX)
        : Option(name, help), value(value), min(min), max(max) {}
    virtual void        Process(Options &);
    operator            int64_t() { return value; }
public:
    uint64_t            value;
    uint64_t            min;
    uint64_t            max;
};


struct TextOption : Option
// ----------------------------------------------------------------------------
//   An option accepting text values
// ----------------------------------------------------------------------------
{
    TextOption(kstring name, kstring help, text value = "")
        : Option(name, help), value(value) {}
    virtual void        Process(Options &);
    operator            text() { return value; }
public:
    text                value;
};


struct CodeOption : Option
// ----------------------------------------------------------------------------
//   An option accepting text values
// ----------------------------------------------------------------------------
{
    typedef std::function<void(Option &opt, Options &opts)> Code;

    CodeOption(kstring name, kstring help, Code code)
        : Option(name, help), code(code) {}
    virtual void        Process(Options &);
public:
    Code code;
};


struct AliasOption : Option
// ----------------------------------------------------------------------------
//   An alias for another option
// ----------------------------------------------------------------------------
{
    AliasOption(kstring name, Option &alias):
        Option(name, "alias"), alias(alias) {}
    virtual kstring     SkipPrefix(kstring command);
    virtual kstring     Matches(Options &);
    virtual void        Process(Options &);
    virtual void        Usage(int maxOptWidth, int maxWidth = 80);

private:
    Option &            alias;
};


struct Options
// ----------------------------------------------------------------------------
//    Class managing options (parsing, isolating files)
// ----------------------------------------------------------------------------
{
    Options(int argc, char **argv);

    kstring             ParseFirst();
    kstring             ParseNext();
    void                Usage();
    kstring             Input();
    kstring             Command();
    bool                HasDirectArgument();
    bool                IsExact()               { return exact; }
    void                IsExact(bool e)         { exact = e; }
    kstring             Argument();

private:
    uint                arg;
    std::vector<kstring>args;
    kstring             selected;
    uint                length;
    bool                exact;
};

XL_END

RECORDER_DECLARE(options);

#endif /* XL_OPTIONS_H */
