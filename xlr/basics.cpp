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
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
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
#include "main.h"


XL_BEGIN

// ============================================================================
// 
//    Top-level operation
// 
// ============================================================================

#include "opcodes_declare.h"
#include "basics.tbl"


void EnterBasics()
// ----------------------------------------------------------------------------
//   Enter all the basic operations defined in basics.tbl
// ----------------------------------------------------------------------------
{
    Compiler *compiler = MAIN->compiler;
    Context *context = MAIN->context;
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
