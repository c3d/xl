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

enum ErrorNumber
// ----------------------------------------------------------------------------
//   Definition of error numbers
// ----------------------------------------------------------------------------
{
#define XL_ERROR(x, y)             x,
#include "errors.tbl"
    E_LAST
};


enum ErrorSeverity
// ----------------------------------------------------------------------------
//   Severity information
// ----------------------------------------------------------------------------
{
    severityError,
    severityWarning,
    severityInfo
};


typedef std::vector<std::string> ErrorArguments;


void Error(ErrorNumber err,
           text file, uint line,
           ErrorArguments args,
           ErrorSeverity severity);


inline void Error(ErrorNumber err, text file, uint line,
                  ErrorSeverity severity = severityError)
// ----------------------------------------------------------------------------
//    Default error, no arguments
// ----------------------------------------------------------------------------
{
    Error(err, file, line, ErrorArguments(), severity);
}
       

inline void Error(ErrorNumber err, text file, uint line,
                  text arg1,
                  ErrorSeverity severity = severityError)
// ----------------------------------------------------------------------------
//   Default error, one argument
// ----------------------------------------------------------------------------
{
    ErrorArguments args;
    args.push_back(arg1);
    Error(err, file, line, args, severity);
}
       

inline void Error(ErrorNumber err, text file, uint line,
                  text arg1, text arg2,
                  ErrorSeverity severity = severityError)
// ----------------------------------------------------------------------------
//   Default error, one argument
// ----------------------------------------------------------------------------
{
    ErrorArguments args;
    args.push_back(arg1);
    args.push_back(arg2);
    Error(err, file, line, args, severity);
}
       

inline void Error(ErrorNumber err, text file, uint line,
                  text arg1, text arg2, text arg3,
                  ErrorSeverity severity = severityError)
// ----------------------------------------------------------------------------
//   Default error, one argument
// ----------------------------------------------------------------------------
{
    ErrorArguments args;
    args.push_back(arg1);
    args.push_back(arg2);
    args.push_back(arg3);
    Error(err, file, line, args, severity);
}

XL_END

#endif /* XL_ERRORS_H */
