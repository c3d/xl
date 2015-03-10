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

#include "base.h"
#include "tree.h"

#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

XL_BEGIN

struct Positions;


struct Error
// ----------------------------------------------------------------------------
//   Encapsulate a single error
// ----------------------------------------------------------------------------
{
    enum { UNKNOWN_POSITION = ~0UL, COMMAND_LINE=~1UL };

    Error (kstring m, ulong pos = UNKNOWN_POSITION);
    Error (kstring m, Tree *a);
    Error (kstring m, Tree *a, Tree *b);
    Error (kstring m, Tree *a, Tree *b, Tree *c);
    ~Error() {}

    // Adding arguments to an error message    
    Error &             Arg(int value) { return Arg(Integer::value_t(value)); }
    Error &             Arg(uint value){ return Arg(Integer::value_t(value)); }
    Error &             Arg(Integer::value_t value);
    Error &             Arg(Real::value_t value);
    Error &             Arg(Text::value_t t);
    Error &             Arg(Tree *arg);

    // Displaying the message
    void                Display();
    text                Position();
    text                Message();


public:
    text                message;
    std::vector<Tree *> arguments;
    ulong               position;
    ulong               indent;
};


struct Errors
// ----------------------------------------------------------------------------
//   Structure used to log errors and display them if necessary
// ----------------------------------------------------------------------------
{
    Errors();
    Errors(kstring m, ulong pos = Error::UNKNOWN_POSITION);
    Errors(kstring m, Tree *a);
    Errors(kstring m, Tree *a, Tree *b);
    Errors(kstring m, Tree *a, Tree *b, Tree *c);
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
Error &Ooops (kstring m, ulong pos = Error::UNKNOWN_POSITION);
Error &Ooops (kstring m, Tree *a);
Error &Ooops (kstring m, Tree *a, Tree *b);
Error &Ooops (kstring m, Tree *a, Tree *b, Tree *c);

// Formatting a tree for error reporting
text FormatTreeForError(Tree *tree);
text ShortTreeForm(Tree *tree, uint maxWidth = 60);

XL_END

#endif /* XL_ERRORS_H */
