// *****************************************************************************
// basics.cpp                                                         XL project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2009-2012,2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2010,2012, Jérôme Forissier <jerome@taodyne.com>
// (C) 2010, Lionel Schaffhauser <lionel@taodyne.com>
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
                symbols->priority = rp->value;
            else if (Integer *ip = prefix->right->AsInteger())
                symbols->priority = ip->value;
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
