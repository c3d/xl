// For GNUers only... this is really -*- C++ -*-
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

XL_BEGIN

class Positions;
class Tree;


struct Errors
// ----------------------------------------------------------------------------
//   Structure used to report errors
// ----------------------------------------------------------------------------
{
    Errors (Positions *pos): positions(pos) {}

    typedef std::vector<std::string> Arguments;
    text Error(text err, ulong pos, Arguments &args);
    text Error(text err, ulong pos);
    text Error(text err, ulong pos, text arg1);
    text Error(text err, ulong pos, text arg1, text arg2);
    text Error(text err, ulong pos, text arg1, text arg2, text arg3);
    text Error(text err, Tree *arg1);
    text Error(text err, Tree *arg1, Tree *arg2);
    text Error(text err, Tree *arg1, Tree *arg2, Tree *arg3);

    Positions *         positions;
};


struct Error
// ----------------------------------------------------------------------------
//   Encapsulate a single error
// ----------------------------------------------------------------------------
{
    Error (text message, Tree *arg1, Tree *arg2, Tree *arg3):
        message(message), arg1(arg1), arg2(arg2), arg3(arg3), handled(false) {}
    Error (const Error &o):
        message(o.message), arg1(o.arg1), arg2(o.arg2), arg3(o.arg3),
        handled(false) { ((Error *) &o)->handled = true; }
    ~Error() { if (!handled) Display(); }

    void        Display();
    text        Message();
    bool        Handled() { bool old = handled; handled = true; return old; }

    text        message;
    Tree *      arg1;
    Tree *      arg2;
    Tree *      arg3;
    bool        handled;
};

XL_END

#endif /* XL_ERRORS_H */
