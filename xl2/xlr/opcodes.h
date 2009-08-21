#ifndef OPCODES_H
#define OPCODES_H
// ****************************************************************************
//  opcodes.h                       (C) 1992-2009 Christophe de Dinechin (ddd)
//                                                                 XL2 project
// ****************************************************************************
//
//   File Description:
//
//    Opcodes are native trees generated as part of compilation/optimization
//    to speed up execution. They represent a step in the evaluation of
//    the code.
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

#include "tree.h"

XL_BEGIN

struct OpCode : Native
// ----------------------------------------------------------------------------
//   An opcode tree is intended to represent directly executable code
// ----------------------------------------------------------------------------
{
    OpCode(tree_position pos = NOWHERE): Native(pos) {}
    virtual Tree *      Do(Action *action);
    virtual Tree *      Run(Context *context);
    virtual text        TypeName();
};

XL_END

#endif // OPCODES_H
