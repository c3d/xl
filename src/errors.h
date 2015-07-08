#ifndef ELIOT_ERRORS_H
#define ELIOT_ERRORS_H
/* ************************************************************************* */
/*   Christophe de Dinechin                                  ELIOT  PROJECT  */
/*   errors.h             4                                      UTILITIES    */
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

ELIOT_BEGIN

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
    Error &             Arg(int x)      { return Arg(Integer::value_t(x)); }
    Error &             Arg(uint x)     { return Arg(Integer::value_t(x)); }
    Error &             Arg(long x)     { return Arg(Integer::value_t(x)); }
    Error &             Arg(ulong x)    { return Arg(Integer::value_t(x)); }
    Error &             Arg(Integer::value_t value);
    Error &             Arg(Real::value_t value);
    Error &             Arg(Text::value_t t);
    Error &             Arg(Tree *arg);

    // Displaying the message
    void                Display();
    text                Position();
    text                Message();

    // Converting to a prefix form for error evaluation
                        operator Tree *();
    

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
Error &Ooops (kstring m, TreePosition pos = Tree::NOWHERE);
Error &Ooops (kstring m, Tree *a);
Error &Ooops (kstring m, Tree *a, Tree *b);
Error &Ooops (kstring m, Tree *a, Tree *b, Tree *c);

// Formatting a tree for error reporting
text FormatTreeForError(Tree *tree);
text ShortTreeForm(Tree *tree, uint maxWidth = 60);

ELIOT_END

#endif /* ELIOT_ERRORS_H */
