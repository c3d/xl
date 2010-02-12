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

#include <sstream>
#include <iostream>
#include <stdio.h>
#include "errors.h"
#include "options.h"
#include "scanner.h" // for Positions
#include "context.h" // For error display
#include "tree.h"

XL_BEGIN

// ============================================================================
// 
//   Table of all error messages
// 
// ============================================================================

text Errors::Error(text errMsg, ulong pos, Errors::Arguments &args)
// ----------------------------------------------------------------------------
//   Emit an error message
// ----------------------------------------------------------------------------
{
    std::ostringstream out;

    for (uint i = 0; i < args.size(); i++)
    {
        char buffer[10];
        sprintf(buffer, "$%d", i+1);
        size_t found = errMsg.find(buffer);
        if (found != errMsg.npos)
            errMsg.replace(found, strlen(buffer), args[i]);
    }
    if (positions)
    {
        text  file, source;
        ulong line, column;
        positions->GetInfo(pos, &file, &line, &column, &source);
        out << file << ":" << line << ": " << errMsg;
    }
    else
    {
        out << "At offset " << pos << ": " << errMsg;
    }
    return out.str();
}


text Errors::Error(text err, ulong pos)
// ----------------------------------------------------------------------------
//    Default error, no arguments
// ----------------------------------------------------------------------------
{
    Arguments args;
    return Error(err, pos, args);
}
       

text Errors::Error(text err, ulong pos, text arg1)
// ----------------------------------------------------------------------------
//   Default error, one argument
// ----------------------------------------------------------------------------
{
    Arguments args;
    args.push_back(arg1);
    return Error(err, pos, args);
}
       

text Errors::Error(text err, ulong pos, text arg1, text arg2)
// ----------------------------------------------------------------------------
//   Default error, one argument
// ----------------------------------------------------------------------------
{
    Arguments args;
    args.push_back(arg1);
    args.push_back(arg2);
    return Error(err, pos, args);
}
       

text Errors::Error(text err, ulong pos, text arg1, text arg2, text arg3)
// ----------------------------------------------------------------------------
//   Default error, one argument
// ----------------------------------------------------------------------------
{
    Arguments args;
    args.push_back(arg1);
    args.push_back(arg2);
    args.push_back(arg3);
    return Error(err, pos, args);
}


text Errors::Error(text err, Tree *arg1)
// ----------------------------------------------------------------------------
//   Emit an error at a tree position
// ----------------------------------------------------------------------------
{
    return Error (err, arg1->Position(), text(*arg1));
}


text Errors::Error(text err, Tree *arg1, Tree *arg2)
// ----------------------------------------------------------------------------
//   Emit an error at a tree position
// ----------------------------------------------------------------------------
{
    return Error (err, arg1->Position(), text(*arg1), text(*arg2));
}


text Errors::Error(text err, Tree *arg1, Tree *arg2, Tree *arg3)
// ----------------------------------------------------------------------------
//   Emit an error at a tree position
// ----------------------------------------------------------------------------
{
    return Error (err, arg1->Position(), text(*arg1), text(*arg2), text(*arg3));
}



// ============================================================================
// 
//   Display an error
// 
// ============================================================================

void Error::Display()
// ----------------------------------------------------------------------------
//   Display an error on the error output
// ----------------------------------------------------------------------------
{
    std:: cerr << Message() << '\n';
}


text Error::Message()
// ----------------------------------------------------------------------------
//    Return the error message for an error
// ----------------------------------------------------------------------------
{
    Errors &errs = Context::context->errors;
    handled = true;
    return errs.Error(message, arg1, arg2, arg3);
}

XL_END



// ============================================================================
// 
//    Runtime support (in global namespace)
// 
// ============================================================================


void xl_assert_failed(kstring msg, kstring file, uint line)
// ----------------------------------------------------------------------------
//   Report an assertion failure
// ----------------------------------------------------------------------------
{
    fprintf(stderr, "%s:%u: Assertion failed: %s\n",
            file, line, msg);
    abort();
}
