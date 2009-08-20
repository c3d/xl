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

XL_BEGIN

// ============================================================================
// 
//   Table of all error messages
// 
// ============================================================================

void Error(text errMsg, ulong pos,
           ErrorArguments args = ErrorArguments(),
           ErrorSeverity severity = severityError)
// ----------------------------------------------------------------------------
//   Emit an error message
// ----------------------------------------------------------------------------
{
    for (uint i = 0; i < args.size(); i++)
    {
        char buffer[10];
        sprintf(buffer, "$%d", i+1);
        size_t found = errMsg.find(buffer);
        if (found)
            errMsg.replace(found, strlen(buffer), args[i]);
    }
    fprintf(stderr, "%lu: %s\n", pos, errMsg.c_str());
}

XL_END


void xl_assert_failed(kstring msg, kstring file, uint line)
// ----------------------------------------------------------------------------
//   Report an assertion failure
// ----------------------------------------------------------------------------
{
    fprintf(stderr, "%s:%u: Assertion failed: %s\n",
            file, line, msg);
    abort();
}
