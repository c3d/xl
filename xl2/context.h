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
typedef std::map<text, int>             priority_table;
typedef std::map<text, text>            comment_table;


class XLContext
// ----------------------------------------------------------------------------
//   This is the execution environment for all trees
// ----------------------------------------------------------------------------
{
public:
    XLContext():
        parent(NULL), priority(0),
        default_priority(0),
        statement_priority(100), function_priority(200) {}
    XLContext(XLContext *p):
        parent(p), priority(0) {}

public:
    XLContext *         Parent() { return parent; }
    
    // Managing priorities
    int                 InfixPriority(text n);
    void                SetInfixPriority(text n, int p);
    int                 PrefixPriority(text n);
    void                SetPrefixPriority(text n, int p);

    // Symbol table for values
    void                Enter(text name, XLTree *v) { symbols[name] = v; }
    XLTree *            Symbol(text name) { return symbols[name]; }
    XLTree *            Find(text name);


    // Convenience notation to define priorities
    XLContext &         operator / (kstring opname)
    {
        SetInfixPriority(opname, priority);
        return *this;
    }
    XLContext &         operator / (int prio)
    {
        priority = prio;
        return *this;
    }
    XLContext &         operator | (kstring opname)
    {
        SetPrefixPriority(opname, priority);
        return *this;
    }
    XLContext &         operator | (int prio)
    {
        priority = prio;
        return *this;
    }
    XLContext &         Comment(text Begin, text End)
    {
        comments[Begin] = End;
        return *this;
    }
    bool                IsComment(text Begin, text &end);

private:
    XLContext *         parent;
    priority_table      infix_priority;
    priority_table      prefix_priority;
    comment_table       comments;
    symbol_table        symbols;
    int                 priority;

public:
    int                 statement_priority;
    int                 function_priority;
    int                 default_priority;
};


extern XLContext gContext;

#endif // CONTEXT_H
