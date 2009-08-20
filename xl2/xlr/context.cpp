// ****************************************************************************
//  context.cpp                     (C) 1992-2003 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//     Description of an execution context
// 
// 
// 
// 
// 
// 
// 
// 
// ****************************************************************************
// This document is confidential.
// Do not redistribute without written permission
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

#include "context.h"

XLTree *XLContext::Find(text name)
// ----------------------------------------------------------------------------
//   Recursively find the name
// ----------------------------------------------------------------------------
{
    for (XLContext *s = this; s; s = s->Parent())
    {
        XLTree *found = s->Symbol(name);
        if (found)
            return found;
    }
    return NULL;
}


