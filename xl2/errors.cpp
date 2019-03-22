// *****************************************************************************
// errors.cpp                                                         XL project
// *****************************************************************************
//
// File description:
//
//    Handling the compiler errors
//
//
//
//
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2003,2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2003, Juan Carlos Arevalo Baeza <thejcab@sourceforge.net>
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

#include <stdio.h>
#include "errors.h"
#include "options.h"



// ============================================================================
//
//   Table of all error messages
//
// ============================================================================

kstring ErrorMessages[E_LAST] =
// ----------------------------------------------------------------------------
//    Internal table of all error messages
// ----------------------------------------------------------------------------
{
#define XL_ERROR(x, y)     y,
#include "errors.tbl"
};


void XLError(XLErrorNumber err, text file, uint line,
             XLErrorArguments args = XLErrorArguments(),
             XLErrorSeverity severity = severityError)
// ----------------------------------------------------------------------------
//   Emit an error message
// ----------------------------------------------------------------------------
{
    MZ_ASSERT(err < E_LAST);
    text errMsg = ErrorMessages[err];
    for (uint i = 0; i < args.size(); i++)
    {
        char buffer[10];
        sprintf(buffer, "$%d", i+1);
        size_t found = errMsg.find(buffer);
        if (found)
            errMsg.replace(found, strlen(buffer), args[i]);
    }
    fprintf(stderr, "%s:%d: %s\n", file.c_str(), line, errMsg.c_str());
}


void mz_assert_failed(kstring msg, kstring file, uint line)
// ----------------------------------------------------------------------------
//   Report an assertion failure
// ----------------------------------------------------------------------------
{
    fprintf(stderr, "%s:%u: Assertion failed: %s\n",
            file, line, msg);
    abort();
}
