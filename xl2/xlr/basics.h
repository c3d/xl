#ifndef BASICS_H
// ****************************************************************************
//  basics.h                        (C) 1992-2009 Christophe de Dinechin (ddd)
//                                                                 XL2 project
// ****************************************************************************
//
//   File Description:
//
//      Basic operations (arithmetic, etc)
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

#include "tree.h"
#include "context.h"
#include <cmath>

XL_BEGIN

// ============================================================================
//
//   Top level entry point
//
// ============================================================================

// Top-level entry point: enter all basic operations in the context
void EnterBasics(Context *context);

XL_END

#endif // BASICS_H
