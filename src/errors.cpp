// *****************************************************************************
// errors.cpp                                                         XL project
// *****************************************************************************
//
// File description:
//
//    Handling the compiler errors
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
// (C) 2003,2009-2012,2014-2019, Christophe de Dinechin <christophe@dinechin.org>
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

#include "errors.h"

#include "basics.h"
#include "context.h" // For error display
#include "main.h"
#include "options.h"
#include "scanner.h" // for Positions
#include "tree.h"

#include <iostream>
#include <sstream>
#include <stdio.h>
#include <unistd.h>

XL_BEGIN

// ============================================================================
//
//   Encapsulating an error message
//
// ============================================================================

Error::Error(kstring m, ulong p)
// ----------------------------------------------------------------------------
//   Error without arguments
// ----------------------------------------------------------------------------
    : message(m), position(p), indent(0)
{}


Error::Error(kstring m, Tree *a)
// ----------------------------------------------------------------------------
//   Error with a tree argument
// ----------------------------------------------------------------------------
    : message(m), position(Tree::NOWHERE), indent(0)
{
    Arg(a);
}


Error::Error(kstring m, Tree *a, Tree *b)
// ----------------------------------------------------------------------------
//   Error with two tree arguments
// ----------------------------------------------------------------------------
    : message(m), position(Tree::NOWHERE), indent(0)
{
    Arg(a); Arg(b);
}


Error::Error(kstring m, Tree *a, Tree *b, Tree *c)
// ----------------------------------------------------------------------------
//   Error with three tree arguments
// ----------------------------------------------------------------------------
    : message(m), position(Tree::NOWHERE), indent(0)
{
    Arg(a); Arg(b); Arg(c);
}


Error &Error::Arg(Natural::value_t value)
// ----------------------------------------------------------------------------
//   Add an argument to the message, replacing $1, $2, ...
// ----------------------------------------------------------------------------
{
    arguments.push_back(new Natural(value, position));
    return *this;
}


Error &Error::Arg(Real::value_t value)
// ----------------------------------------------------------------------------
//   Add an argument to the message, replacing $1, $2, ...
// ----------------------------------------------------------------------------
{
    arguments.push_back(new Real(value, position));
    return *this;
}


Error &Error::Arg(Text::value_t t, text delim)
// ----------------------------------------------------------------------------
//   Add an argument to the message, replacing $1, $2, ...
// ----------------------------------------------------------------------------
{
    arguments.push_back(new Text(t, delim, delim, position));
    return *this;
}


Error &Error::Arg(Text::value_t t, text open, text close)
// ----------------------------------------------------------------------------
//   Add an argument to the message, replacing $1, $2, ...
// ----------------------------------------------------------------------------
{
    arguments.push_back(new Text(t, open, close, position));
    return *this;
}


Error &Error::Arg(Tree *arg)
// ----------------------------------------------------------------------------
//   Add a tree argument, using its position if applicable
// ----------------------------------------------------------------------------
{
    if (arg && (long) position < 0)
        position = arg->Position();
    arguments.push_back(arg);
    return *this;
}


void Error::Display()
// ----------------------------------------------------------------------------
//   Display an error on the error output
// ----------------------------------------------------------------------------
{
    std::cerr << Position() << ": "
              << text(indent < 20 ? indent : 20, ' ')
              << Message() << '\n';
}


text Error::Position()
// ----------------------------------------------------------------------------
//   Emit the position in a human-readable form
// ----------------------------------------------------------------------------
{
    switch (position)
    {
    case Tree::UNKNOWN_POSITION:        return "<Unknown position>";
    case Tree::COMMAND_LINE:            return "<Command line>";
    case Tree::BUILTIN:                 return "<Builtin>";
    }

    text  file, source;
    ulong line, column;
    std::ostringstream out;
    MAIN->positions.GetInfo(position, &file, &line, &column, &source);
    out << file << ":" << line << ":" << column + 1;
    return out.str();
}


text Error::Message()
// ----------------------------------------------------------------------------
//    Return the error message for an error
// ----------------------------------------------------------------------------
{
    text result = message;
    for (uint i = 0; i < arguments.size(); i++)
    {
        char buffer[10];
        sprintf(buffer, "$%d", i+1);
        size_t found = result.find(buffer);
        if (found != result.npos)
            result.replace(found, strlen(buffer),
                           FormatTreeForError(arguments[i]));
    }
    return result;
}


Error::operator Tree *()
// ----------------------------------------------------------------------------
//    Return the error converted as a prefix form
// ----------------------------------------------------------------------------
{
    Tree *args = new Text(message, position);
    for (auto arg : arguments)
        args = new Infix(",", args, arg);
    return new Prefix(xl_error, args, position);
}



// ============================================================================
//
//   Table of all error messages
//
// ============================================================================

Errors::Errors()
// ----------------------------------------------------------------------------
//   Save errors from the top-level error handler
// ----------------------------------------------------------------------------
    : parent(MAIN->errors), count(0), context(0)
{
    MAIN->errors = this;
}


#define ERROR_OR_CONTEXT(e)                     \
    bool context = *m == ' ' && m++;            \
    Log(e, context);


Errors::Errors (kstring m, ulong pos)
// ----------------------------------------------------------------------------
//   Save errors from the top-level error handler
// ----------------------------------------------------------------------------
    : parent(MAIN->errors), count(0), context(0)
{
    MAIN->errors = this;
    ERROR_OR_CONTEXT(Error(m, pos));
}


Errors::Errors (kstring m, Tree *a)
// ----------------------------------------------------------------------------
//   Save errors from the top-level error handler
// ----------------------------------------------------------------------------
    : parent(MAIN->errors), count(0), context(0)
{
    MAIN->errors = this;
    ERROR_OR_CONTEXT(Error(m, a));
}


Errors::Errors (kstring m, Tree *a, Tree *b)
// ----------------------------------------------------------------------------
//   Save errors from the top-level error handler
// ----------------------------------------------------------------------------
    : parent(MAIN->errors), count(0), context(0)
{
    MAIN->errors = this;
    ERROR_OR_CONTEXT(Error(m, a, b));
}


Errors::Errors (kstring m, Tree *a, Tree *b, Tree *c)
// ----------------------------------------------------------------------------
//   Save errors from the top-level error handler
// ----------------------------------------------------------------------------
    : parent(MAIN->errors), count(0), context(0)
{
    MAIN->errors = this;
    ERROR_OR_CONTEXT(Error(m, a, b, c));
}


Errors::~Errors()
// ----------------------------------------------------------------------------
//   Display errors to top-level handler
// ----------------------------------------------------------------------------
{
    assert (MAIN->errors == this);
    MAIN->errors = parent;

    if (HadErrors())
        Display();
}


void Errors::Clear()
// ----------------------------------------------------------------------------
//   Clear error messages at the current level
// ----------------------------------------------------------------------------
{
    errors.clear();
    count = context = 0;
}


bool Errors::Swallowed()
// ----------------------------------------------------------------------------
//   Clear errors, and return true if there were errors before
// ----------------------------------------------------------------------------
{
    bool result = errors.size() > context;
    errors.clear();
    context = 0;
    return result;
}


void Errors::Display()
// ----------------------------------------------------------------------------
//   Display pending error messages
// ----------------------------------------------------------------------------
{
    if (parent)
    {
        parent->count += errors.size();
        if (context)
        {
            uint max = errors.size();
            for (uint i = context; i < max; i++)
                errors[i].indent++;
        }
        parent->errors.insert(parent->errors.end(),
                              errors.begin(), errors.end());
    }
    else
    {
        std::vector<Error>::iterator e;
        for (e = errors.begin(); e != errors.end(); e++)
            (*e).Display();
    }
}


Error &Errors::Log(const Error &e, bool isContext)
// ----------------------------------------------------------------------------
//   Log an error
// ----------------------------------------------------------------------------
{
    errors.push_back(e);
    if (isContext)
        context++;
    return errors.back();
}


Tree_p Errors::aborting;



// ============================================================================
//
//   Quick error reporting functions
//
// ============================================================================

Error &Ooops (kstring m, ulong pos)
// ----------------------------------------------------------------------------
//   Report an error message without arguments
// ----------------------------------------------------------------------------
{
    return MAIN->errors->Log(Error(m, pos));
}


Error &Ooops (kstring m, Tree *a)
// ----------------------------------------------------------------------------
//   Report an error message with one tree argument
// ----------------------------------------------------------------------------
{
    return MAIN->errors->Log(Error(m, a));
}


Error &Ooops (kstring m, Tree *a, Tree *b)
// ----------------------------------------------------------------------------
//   Report an error message with two tree arguments
// ----------------------------------------------------------------------------
{
    return MAIN->errors->Log(Error(m, a, b));
}


Error &Ooops (kstring m, Tree *a, Tree *b, Tree *c)
// ----------------------------------------------------------------------------
//   Report an error message with three tree arguments
// ----------------------------------------------------------------------------
{
    return MAIN->errors->Log(Error(m, a, b, c));
}


bool HadErrors()
// ----------------------------------------------------------------------------
//   Return true if we had errors
// ----------------------------------------------------------------------------
{
    return Errors::Aborting() || MAIN->errors->HadErrors();
}


text ShortTreeForm(Tree *tree, uint maxWidth)
// ----------------------------------------------------------------------------
//   Format a tree for error reporting
// ----------------------------------------------------------------------------
{
    text t = *tree;
    size_t length = t.length();

    if (length > maxWidth)
    {
        uint extra = length - maxWidth;
        size_t first = maxWidth / 2;
        t.replace(first, extra+1, "…");
    }

    size_t first = t.find("\n");
    while (first != t.npos)
    {
        t.replace(first, 1, "|");
        first = t.find("\n");
    }

    return t;
}


text FormatTreeForError(Tree *tree)
// ----------------------------------------------------------------------------
//   Format a tree for error reporting
// ----------------------------------------------------------------------------
{
    text result = ShortTreeForm(tree);
    if (tree && tree->Kind() >= KIND_LEAF_LAST)
        result = "[" + result + "]";
    return result;
}

XL_END



// ============================================================================
//
//    Runtime support (in global namespace)
//
// ============================================================================

RECORDER_DEFINE(xl_assert_error, 16, "XL assertions checks");

void xl_assert_failed(kstring kind, kstring msg, kstring file, unsigned line)
// ----------------------------------------------------------------------------
//   Report an assertion failure
// ----------------------------------------------------------------------------
{
    record(xl_assert_error, "%+s %+s failed (%+s:%u)", kind, msg, file, line);
    while (RECORDER_TRACE(xl_assert_error) == 1205)
        sleep(1);
    abort();
}
