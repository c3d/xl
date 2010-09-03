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
/* ****************************************************************************
   * File       : $RCSFile$
   * Revision   : $Revision$
   * Date       : $Date$
   ************************************************************************* */

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

    // Converting to an error tree
    operator Tree *     ();
    operator Tree_p     ()      { return Tree_p((Tree *) *this); }
    Tree *operator ->   ()      { return (Tree *) *this; }

    text                message;
    std::vector<text>   arguments;
    ulong               position;
};


struct Errors
// ----------------------------------------------------------------------------
//   Structure used to log errors and display them if necessary
// ----------------------------------------------------------------------------
{
    Errors();
    ~Errors();

    void Clear();
    void Display();
    Error &Log(const Error &e);
    uint Count()        { return errors.size() + count; }

    std::vector<Error>  errors;
    Errors *            parent;
    ulong               count;
};


// Helper to quickly report errors
Error &Ooops (text m, ulong pos = Error::UNKNOWN_POSITION);
Error &Ooops (text m, Tree *a);
Error &Ooops (text m, Tree *a, Tree *b);
Error &Ooops (text m, Tree *a, Tree *b, Tree *c);

XL_END

#endif /* XL_ERRORS_H */
