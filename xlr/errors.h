#ifndef XL_ERRORS_H
#define XL_ERRORS_H
/* ************************************************************************* */
/*   Christophe de Dinechin                                     XL2 PROJECT  */
/*   XL COMPILER: errors.h                                      UTILITIES    */
/* ************************************************************************* */
/*                                                                           */
/*   File Description:                                                       */
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
/* ************************************************************************* */
/* This document is distributed under the GNU General Public License         */
/* See the enclosed COPYING file or http://www.gnu.org for information       */
/*  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>            */
/*  (C) 2010 Taodyne SAS                                                     */
/* ***************************************************************************/

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
    Errors(text m, ulong pos = Error::UNKNOWN_POSITION);
    Errors(text m, Tree *a);
    Errors(text m, Tree *a, Tree *b);
    Errors(text m, Tree *a, Tree *b, Tree *c);
    ~Errors();

    void                Clear();
    bool                Swallowed();
    void                Display();
    Error &             Log(const Error &e, bool context = false);
    Error &             Context(const Error &e) { return Log(e, true); }
    uint                Count()        { return errors.size() + count; }
    bool                HadErrors()    { return errors.size() > context; }

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
