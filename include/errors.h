#ifndef XL_ERRORS_H
#define XL_ERRORS_H
// *****************************************************************************
// errors.h                                                           XL project
// *****************************************************************************
//
// File description:
//
//      Type of errors that the compiler may generate
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
// (C) 2003,2008-2011,2014-2019, Christophe de Dinechin <christophe@dinechin.org>
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
#include "tree.h"

#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <type_traits>

XL_BEGIN

struct Positions;


struct Error
// ----------------------------------------------------------------------------
//   Encapsulate a single error
// ----------------------------------------------------------------------------
{
    Error (kstring m, TreePosition pos = Tree::NOWHERE);
    Error (kstring m, Tree *a);
    Error (kstring m, Tree *a, Tree *b);
    Error (kstring m, Tree *a, Tree *b, Tree *c);
    ~Error() {}

    // Adding arguments to an error message
    Error &             Arg(Natural::value_t value);
    Error &             Arg(Real::value_t value);
    Error &             Arg(Text::value_t t, text delim="\"");
    Error &             Arg(Text::value_t t, text open, text close);
    Error &             Arg(Tree *arg);
    template <typename num, typename =
              typename std::enable_if<std::is_integral<num>::value>::type>
    Error &             Arg(num x) { return Arg(Natural::value_t(x)); }

    // Displaying the message
    void                Display();
    text                Position();
    text                Message();

    // Converting to a prefix form for error evaluation
                        operator Tree *();
                        operator Tree_p() { return (Tree *) (*this); }
    bool                operator ==(const Error &other) const;

public:
    text                message;
    std::vector<Tree_p> arguments;
    ulong               position;
    ulong               indent;
};


struct Errors
// ----------------------------------------------------------------------------
//   Structure used to log errors and display them if necessary
// ----------------------------------------------------------------------------
{
    Errors();
    Errors(kstring m, TreePosition pos = Tree::NOWHERE);
    Errors(kstring m, Tree *a);
    Errors(kstring m, Tree *a, Tree *b);
    Errors(kstring m, Tree *a, Tree *b, Tree *c);
    ~Errors();

    void                Clear();
    bool                Swallowed();
    void                Display();
    Tree_p              AsErrorTree();
    Error &             Log(const Error &e, bool context = false);
    Error &             Context(const Error &e) { return Log(e, true); }
    uint                Count()         { return errors.size() + count; }
    bool                HadErrors()     { return errors.size() > context; }
    static Tree_p       Aborting()      { return aborting; }
    static void         Abort(Error &e) { if (!aborting) aborting = e; }

    std::vector<Error>  errors;
    Errors *            parent;
    ulong               count;
    ulong               context;
    static Tree_p       aborting;
};


// Helper to quickly report errors
Error &Ooops (kstring m, TreePosition pos = Tree::NOWHERE);
Error &Ooops (kstring m, Tree *a);
Error &Ooops (kstring m, Tree *a, Tree *b);
Error &Ooops (kstring m, Tree *a, Tree *b, Tree *c);


// Check if errors were reported
bool   HadErrors();
Tree_p LastErrorAsErrorTree();

// Formatting a tree for error reporting
text FormatTreeForError(Tree *tree);
text ShortTreeForm(Tree *tree, uint maxWidth = 60);

XL_END

#endif /* XL_ERRORS_H */
