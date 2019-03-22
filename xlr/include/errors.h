#ifndef XL_ERRORS_H
#define XL_ERRORS_H
// *****************************************************************************
// errors.h                                                           XL project
// *****************************************************************************
//
// File description:
/*                                                                           */
/*      Type of errors that the compiler may generate                        */
/*                                                   l                       */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*                                                                           */
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2003,2008-2011,2014,2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2010, Jérôme Forissier <jerome@taodyne.com>
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

#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include "base.h"
#include "tree.h"

XL_BEGIN

struct Positions;


struct Error
// ----------------------------------------------------------------------------
//   Encapsulate a single error
// ----------------------------------------------------------------------------
{
    enum { UNKNOWN_POSITION = ~0UL, COMMAND_LINE=~1UL };

    Error (text m, ulong pos = UNKNOWN_POSITION);
    Error (text m, Tree *a);
    Error (text m, Tree *a, Tree *b);
    Error (text m, Tree *a, Tree *b, Tree *c);
    ~Error() {}

    // Adding arguments to an error message
    Error &             Arg(text t);
    Error &             Arg(long value);
    Error &             Arg(Tree *arg);

    // Displaying the message
    void                Display();
    text                Position();
    text                Message();


public:
    text                message;
    std::vector<text>   arguments;
    ulong               position;
    ulong               indent;
};


struct Errors
// ----------------------------------------------------------------------------
//   Structure used to log errors and display them if necessary
// ----------------------------------------------------------------------------
{
    Errors();
    ~Errors();

    void Clear();
    bool Swallowed();
    void Display();
    Error &Log(const Error &e, bool context = false);
    uint Count()        { return errors.size() + count; }
    bool HadErrors()    { return errors.size() > context; }

    std::vector<Error>  errors;
    Errors *            parent;
    ulong               count;
    ulong               context;
};


// Helper to quickly report errors
Error &Ooops (text m, ulong pos = Error::UNKNOWN_POSITION);
Error &Ooops (text m, Tree *a);
Error &Ooops (text m, Tree *a, Tree *b);
Error &Ooops (text m, Tree *a, Tree *b, Tree *c);

// Formatting a tree for error reporting
Text *FormatTreeForError(Tree *tree);
text ShortTreeForm(Tree *tree, uint maxWidth = 60);

XL_END

#endif /* XL_ERRORS_H */
