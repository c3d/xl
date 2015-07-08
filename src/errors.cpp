// ****************************************************************************
//  errors.cpp                                                     Tao project 
// ****************************************************************************
//
//   File Description:
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
// ****************************************************************************
// This document is distributed under the GNU General Public License
// See the enclosed COPYING file or http://www.gnu.org for information
//  (C) 1992-2015 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010-2015 Taodyne SAS
// ****************************************************************************

#include "options.h"
#include "scanner.h" // for Positions
#include "context.h" // For error display
#include "tree.h"
#include "main.h"
#include "basics.h"

#include <sstream>
#include <iostream>
#include <stdio.h>
#include "errors.h"

ELIOT_BEGIN

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


Error &Error::Arg(Integer::value_t value)
// ----------------------------------------------------------------------------
//   Add an argument to the message, replacing $1, $2, ...
// ----------------------------------------------------------------------------
{
    arguments.push_back(new Integer(value, position));
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


Error &Error::Arg(Text::value_t t)
// ----------------------------------------------------------------------------
//   Add an argument to the message, replacing $1, $2, ...
// ----------------------------------------------------------------------------
{
    arguments.push_back(new Text(t, position));
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
    std::cerr << Position() << ": " << text(indent, ' ') << Message() << '\n';
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
    return new Prefix(eliot_error, new Text(Message(), position), position);
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
//   Display errors to top-levle handler
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


text FormatTreeForError(Tree *tree)
// ----------------------------------------------------------------------------
//   Format a tree for error reporting
// ----------------------------------------------------------------------------
{
    if (Text *t = tree->AsText())
        return "\"" + t->value + "\"";
    text result = ShortTreeForm(tree);
    return "'" + result + "'";
}

ELIOT_END



// ============================================================================
//
//    Runtime support (in global namespace)
//
// ============================================================================


void eliot_assert_failed(kstring msg, kstring file, uint line)
// ----------------------------------------------------------------------------
//   Report an assertion failure
// ----------------------------------------------------------------------------
{
    fprintf(stderr, "%s:%u: Assertion failed: %s\n",
            file, line, msg);
    abort();
}
