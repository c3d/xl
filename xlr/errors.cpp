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
// This software is licensed under the GNU General Public License v3
// (C) 2003,2009-2012,2014,2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2003, Juan Carlos Arevalo Baeza <thejcab@sourceforge.net>
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

#include <sstream>
#include <iostream>
#include <stdio.h>
#include "errors.h"
#include "options.h"
#include "scanner.h" // for Positions
#include "context.h" // For error display
#include "tree.h"
#include "main.h"

XL_BEGIN

// ============================================================================
//
//   Encapsulating an error message
//
// ============================================================================

Error::Error(text m, ulong p)
// ----------------------------------------------------------------------------
//   Error without arguments
// ----------------------------------------------------------------------------
    : message(m), position(p), indent(0)
{}


Error::Error(text m, Tree *a)
// ----------------------------------------------------------------------------
//   Error with a tree argument
// ----------------------------------------------------------------------------
    : message(m), position(UNKNOWN_POSITION), indent(0)
{
    Arg(a);
}


Error::Error(text m, Tree *a, Tree *b)
// ----------------------------------------------------------------------------
//   Error with two tree arguments
// ----------------------------------------------------------------------------
    : message(m), position(UNKNOWN_POSITION), indent(0)
{
    Arg(a); Arg(b);
}


Error::Error(text m, Tree *a, Tree *b, Tree *c)
// ----------------------------------------------------------------------------
//   Error with three tree arguments
// ----------------------------------------------------------------------------
    : message(m), position(UNKNOWN_POSITION), indent(0)
{
    Arg(a); Arg(b); Arg(c);
}


Error &Error::Arg(text t)
// ----------------------------------------------------------------------------
//   Add an argument to the message, replacing $1, $2, ...
// ----------------------------------------------------------------------------
{
    arguments.push_back(t);
    return *this;
}


Error &Error::Arg(long value)
// ----------------------------------------------------------------------------
//   Add an argument to the message, replacing $1, $2, ...
// ----------------------------------------------------------------------------
{
    std::ostringstream out;
    out << value;
    arguments.push_back(out.str());
    return *this;
}


Error &Error::Arg(Tree *arg)
// ----------------------------------------------------------------------------
//   Add a tree argument, using its position if applicable
// ----------------------------------------------------------------------------
{
    if (arg && position == UNKNOWN_POSITION)
        position = arg->Position();
    arguments.push_back(text(*arg));
    return *this;
}


void Error::Display()
// ----------------------------------------------------------------------------
//   Display an error on the error output
// ----------------------------------------------------------------------------
{
    std::cerr << Position() << ": " << text(indent, ' ') << Message() << '\n';
}


text Error::Position()
// ----------------------------------------------------------------------------
//   Emit the position in a human-readable form
// ----------------------------------------------------------------------------
{
    switch (position)
    {
    case UNKNOWN_POSITION:      return "<Unknown position>";
    case COMMAND_LINE:          return "<Command line>";
    }

    text  file, source;
    ulong line, column;
    std::ostringstream out;
    MAIN->positions.GetInfo(position, &file, &line, &column, &source);
    out << file << ":" << line;
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
            result.replace(found, strlen(buffer), arguments[i]);
    }
    return result;
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


Errors::~Errors()
// ----------------------------------------------------------------------------
//   Display errors to top-levle handler
// ----------------------------------------------------------------------------
{
    assert (MAIN->errors == this);
    MAIN->errors = parent;

    if (errors.size() > context)
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
//   Return true if we swallowed errors
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



// ============================================================================
//
//   Quick error reporting functions
//
// ============================================================================

Error &Ooops (text m, ulong pos)
// ----------------------------------------------------------------------------
//   Report an error message without arguments
// ----------------------------------------------------------------------------
{
    return MAIN->errors->Log(Error(m, pos));
}


Error &Ooops (text m, Tree *a)
// ----------------------------------------------------------------------------
//   Report an error message with one tree argument
// ----------------------------------------------------------------------------
{
    return MAIN->errors->Log(Error(m, FormatTreeForError(a)));
}


Error &Ooops (text m, Tree *a, Tree *b)
// ----------------------------------------------------------------------------
//   Report an error message with two tree arguments
// ----------------------------------------------------------------------------
{
    return MAIN->errors->Log(Error(m,
                                   FormatTreeForError(a),
                                   FormatTreeForError(b)));
}


Error &Ooops (text m, Tree *a, Tree *b, Tree *c)
// ----------------------------------------------------------------------------
//   Report an error message with three tree arguments
// ----------------------------------------------------------------------------
{
    return MAIN->errors->Log(Error(m,
                                   FormatTreeForError(a),
                                   FormatTreeForError(b),
                                   FormatTreeForError(c)));
}


text ShortTreeForm(Tree *tree, uint maxWidth)
// ----------------------------------------------------------------------------
//   Format a tree for error reporting
// ----------------------------------------------------------------------------
{
    text t = *tree;
    size_t length = t.length();

    size_t first = t.find("\n");
    if (first != t.npos)
    {
        size_t last = t.rfind("\n");
        t.replace(first, last-first+1, "...");
        length = t.length();
    }

    if (length > maxWidth)
    {
        uint extra = length - maxWidth;
        first = maxWidth / 2;
        t.replace(first, extra+1, "...");
    }

    return t;
}


Text *FormatTreeForError(Tree *tree)
// ----------------------------------------------------------------------------
//   Format a tree for error reporting
// ----------------------------------------------------------------------------
{
    Text *result = tree->AsText();
    if (result == NULL)
    {
        text t = ShortTreeForm(tree);
        result = new Text(t, "'", "'", tree->Position());
    }
    return result;
}

XL_END



// ============================================================================
//
//    Runtime support (in global namespace)
//
// ============================================================================


void xl_assert_failed(kstring msg, kstring file, uint line)
// ----------------------------------------------------------------------------
//   Report an assertion failure
// ----------------------------------------------------------------------------
{
    fprintf(stderr, "%s:%u: Assertion failed: %s\n",
            file, line, msg);
    abort();
}
