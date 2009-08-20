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
#include "scanner.h" // for Positions

XL_BEGIN

// ============================================================================
// 
//   Table of all error messages
// 
// ============================================================================

void Errors::Error(text errMsg, ulong pos, Errors::Arguments &args)
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
    if (positions)
    {
        text  file, source;
        ulong line, column;
        positions->GetInfo(pos, &file, &line, &column, &source);
        fprintf(stderr, "%s:%lu: %s\n",
                file.c_str(), line, errMsg.c_str());
    }
    else
    {
        fprintf(stderr, "At offset %lu: %s\n", pos, errMsg.c_str());
    }
}


// ----------------------------------------------------------------------------
//   The general routine
// ----------------------------------------------------------------------------


void Errors::Error(text err, ulong pos)
// ----------------------------------------------------------------------------
//    Default error, no arguments
// ----------------------------------------------------------------------------
{
    Arguments args;
    Error(err, pos, args);
}
       

void Errors::Error(text err, ulong pos, text arg1)
// ----------------------------------------------------------------------------
//   Default error, one argument
// ----------------------------------------------------------------------------
{
    Arguments args;
    args.push_back(arg1);
    Error(err, pos, args);
}
       

void Errors::Error(text err, ulong pos, text arg1, text arg2)
// ----------------------------------------------------------------------------
//   Default error, one argument
// ----------------------------------------------------------------------------
{
    Arguments args;
    args.push_back(arg1);
    args.push_back(arg2);
    Error(err, pos, args);
}
       

void Errors::Error(text err, ulong pos, text arg1, text arg2, text arg3)
// ----------------------------------------------------------------------------
//   Default error, one argument
// ----------------------------------------------------------------------------
{
    Arguments args;
    args.push_back(arg1);
    args.push_back(arg2);
    args.push_back(arg3);
    Error(err, pos, args);
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
