#ifndef CONTEXT_H
#define CONTEXT_H
// ****************************************************************************
//  context.h                       (C) 1992-2003 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//     The execution environment for XL
// 
// 
// 
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

#include <map>
#include "base.h"

struct XLTree;
class XLContext;

typedef std::map<text, XLTree *>        symbol_table;

class XLContext
// ----------------------------------------------------------------------------
//   This is the execution context in which we evaluate trees
// ----------------------------------------------------------------------------
{
public:
    XLContext(): parent(NULL) {}
    XLContext(XLContext *p): parent(p) {}

public:
    XLContext *         Parent() { return parent; }
    
    // Symbol table for values
    void                Enter(text name, XLTree *v) { symbols[name] = v; }
    XLTree *            Symbol(text name) { return symbols[name]; }
    XLTree *            Find(text name);


private:
    XLContext *         parent;
    symbol_table        symbols;
};

#endif // CONTEXT_H
