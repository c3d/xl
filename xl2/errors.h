#ifndef XL_ERRORS_H
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
// (C) 2003,2008,2019, Christophe de Dinechin <christophe@dinechin.org>
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
