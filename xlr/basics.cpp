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
// This document is released under the GNU General Public License, with the
// following clarification and exception.
//
// Linking this library statically or dynamically with other modules is making
// a combined work based on this library. Thus, the terms and conditions of the
// GNU General Public License cover the whole combination.
//
// As a special exception, the copyright holders of this library give you
// permission to link this library with independent modules to produce an
// executable, regardless of the license terms of these independent modules,
// and to copy and distribute the resulting executable under terms of your
// choice, provided that you also meet, for each linked independent module,
// the terms and conditions of the license of that module. An independent
// module is a module which is not derived from or based on this library.
// If you modify this library, you may extend this exception to your version
// of the library, but you are not obliged to do so. If you do not wish to
// do so, delete this exception statement from your version.
//
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


Tree *xl_process_import(Symbols *symbols, Tree *source, phase_t phase)
// ----------------------------------------------------------------------------
//   Standard connector for 'import' statements
// ----------------------------------------------------------------------------
{
    if (Prefix *prefix = source->AsPrefix())
    {
        if (Text *name = prefix->right->AsText())
        {
            source->SetSymbols(symbols);
            return xl_import(MAIN->context, source, name->value, phase);
        }
    }
    return NULL;
}


Tree *xl_process_load(Symbols *symbols, Tree *source, phase_t phase)
// ----------------------------------------------------------------------------
//   Standard connector for 'load' statements
// ----------------------------------------------------------------------------
{
    phase = DECLARATION_PHASE;
    return xl_process_import(symbols, source, phase);
}


Tree *xl_process_override_priority(Symbols *symbols, Tree *self, phase_t phase)
// ----------------------------------------------------------------------------
//   Declaration-phase for overriding a priority
// ----------------------------------------------------------------------------
{
    if (phase == DECLARATION_PHASE)
    {
        self->SetSymbols(symbols);
        if (Prefix *prefix = self->AsPrefix())
        {
            if (Real *rp = prefix->right->AsReal())
            {
                std::cerr << "DECL REAL PRIORITY(" << symbols
                          << ":" << symbols->name << ")="
                          << rp->value << "\n";
                symbols->priority = rp->value;
            }
            else if (Integer *ip = prefix->right->AsInteger())
            {
                std::cerr << "DECL INTEGER PRIORITY(" << symbols
                          << ":" << symbols->name << ")="
                          << ip->value << "\n";
                symbols->priority = ip->value;
            }
        }
    }
    return NULL;
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
    xl_enter_declarator("override_priority", xl_process_override_priority);

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
