#ifndef CTRANS_H
#define CTRANS_H
// ****************************************************************************
//  ctrans.h                        (C) 1992-2003 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//     XL to C translator
// 
//     This is a very bare-bones translator, designed only to allow
//     low-cost bootstrap of the XL compiler
// 
// 
// 
// 
// 
// ****************************************************************************
// This program is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html for details
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

#include "base.h"
#include "context.h"

extern void XLInitCTrans();
extern void XL2C(XLTree *tree);

#endif // CTRANS_H
