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
#include <string>
#include <vector>
#include "base.h"



enum XLErrorNumber
// ----------------------------------------------------------------------------
//   Definition of error numbers
// ----------------------------------------------------------------------------
{
#define XL_ERROR(x, y)             x,
#include "errors.tbl"
    E_LAST
};


enum XLErrorSeverity
// ----------------------------------------------------------------------------
//   Severity information
// ----------------------------------------------------------------------------
{
    severityError,
    severityWarning,
    severityInfo
};


typedef std::vector<std::string> XLErrorArguments;


void XLError(XLErrorNumber err,
             text file, uint line,
             XLErrorArguments args,
             XLErrorSeverity severity);


inline void XLError(XLErrorNumber err, text file, uint line,
                    XLErrorSeverity severity = severityError)
// ----------------------------------------------------------------------------
//    Default error, no arguments
// ----------------------------------------------------------------------------
{
    XLError(err, file, line, XLErrorArguments(), severity);
}
       

inline void XLError(XLErrorNumber err, text file, uint line,
                    text arg1,
                    XLErrorSeverity severity = severityError)
// ----------------------------------------------------------------------------
//   Default error, one argument
// ----------------------------------------------------------------------------
{
    XLErrorArguments args;
    args.push_back(arg1);
    XLError(err, file, line, args, severity);
}
       

inline void XLError(XLErrorNumber err, text file, uint line,
                    text arg1, text arg2,
                    XLErrorSeverity severity = severityError)
// ----------------------------------------------------------------------------
//   Default error, one argument
// ----------------------------------------------------------------------------
{
    XLErrorArguments args;
    args.push_back(arg1);
    args.push_back(arg2);
    XLError(err, file, line, args, severity);
}
       

inline void XLError(XLErrorNumber err, text file, uint line,
                    text arg1, text arg2, text arg3,
                    XLErrorSeverity severity = severityError)
// ----------------------------------------------------------------------------
//   Default error, one argument
// ----------------------------------------------------------------------------
{
    XLErrorArguments args;
    args.push_back(arg1);
    args.push_back(arg2);
    args.push_back(arg3);
    XLError(err, file, line, args, severity);
}

#endif /* XL_ERRORS_H */
