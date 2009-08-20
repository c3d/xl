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


struct Errors
// ----------------------------------------------------------------------------
//   Structure used to report errors
// ----------------------------------------------------------------------------
{
    Errors (Positions *pos): positions(pos) {}

    typedef std::vector<std::string> Arguments;
    void Error(text err, ulong pos, Arguments &args);
    void Error(text err, ulong pos);
    void Error(text err, ulong pos, text arg1);
    void Error(text err, ulong pos, text arg1, text arg2);
    void Error(text err, ulong pos, text arg1, text arg2, text arg3);

    Positions * positions;
};

XL_END

#endif /* XL_ERRORS_H */
