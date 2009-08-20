// ****************************************************************************
//   Christophe de Dinechin                                       XL2 PROJECT  
//   XL COMPILER: errors.cpp
// ****************************************************************************
// 
//   File Description:
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
// ****************************************************************************
// This document is distributed under the GNU General Public License
// See the enclosed COPYING file or http://www.gnu.org for information
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

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
    XL_ASSERT(err < E_LAST);
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


void xl_assert_failed(kstring msg, kstring file, uint line)
// ----------------------------------------------------------------------------
//   Report an assertion failure
// ----------------------------------------------------------------------------
{
    fprintf(stderr, "%s:%u: Assertion failed: %s\n",
            file, line, msg);
    abort();
}
