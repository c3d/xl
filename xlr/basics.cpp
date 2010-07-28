// ****************************************************************************
//  basics.cpp                      (C) 1992-2009 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//     Basic operations (arithmetic, ...)
// 
// 
// 
// 
// 
// 
// 
// 
// ****************************************************************************
// This document is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

#include <iostream>
#include <sstream>
#include <ctime>
#include <cstdio>

#include "basics.h"
#include "context.h"
#include "renderer.h"
#include "opcodes.h"
#include "compiler.h"
#include "options.h"
#include "runtime.h"
#include "types.h"


XL_BEGIN

// ============================================================================
// 
//    Top-level operation
// 
// ============================================================================

#include "opcodes_declare.h"
#include "basics.tbl"


void EnterBasics(Context *c)
// ----------------------------------------------------------------------------
//   Enter all the basic operations defined in basics.tbl
// ----------------------------------------------------------------------------
{
    Compiler *compiler = c->compiler;
#include "opcodes_define.h"
#include "basics.tbl"
}

void DeleteBasics()
// ----------------------------------------------------------------------------
//   Delete all the global operations defined in basics.tbl
// ----------------------------------------------------------------------------
{
#include "opcodes_delete.h"
#include "basics.tbl"
}

XL_END
