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
#include "options.h"
#include "runtime.h"
#include "symbols.h"
#include "types.h"
#include "main.h"
#include "hash.h"


XL_BEGIN

// ============================================================================
// 
//    Top-level operation
// 
// ============================================================================

#include "opcodes_declare.h"
#include "basics.tbl"


Tree *xl_process_import(Symbols *symbols, Tree *source, bool execute)
// ----------------------------------------------------------------------------
//   Standard connector for 'import' statements
// ----------------------------------------------------------------------------
{
    if (Prefix *prefix = source->AsPrefix())
    {
        if (Text *name = prefix->right->AsText())
        {
            source->SetSymbols(symbols);
            return xl_import(MAIN->context, source, name->value, execute);
        }
    }
    return NULL;
}


Tree *xl_process_load(Symbols *symbols, Tree *source, bool execute)
// ----------------------------------------------------------------------------
//   Standard connector for 'load' statements
// ----------------------------------------------------------------------------
{
    execute = false;            // 'load' statement doesn't execute loaded code
    return xl_process_import(symbols, source, execute);
}


void EnterBasics()
// ----------------------------------------------------------------------------
//   Enter all the basic operations defined in basics.tbl
// ----------------------------------------------------------------------------
{
    Context *context = MAIN->context;
#include "opcodes_define.h"
#include "basics.tbl"
    xl_enter_declarator("load", xl_process_load);
    xl_enter_declarator("import", xl_process_import);
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
