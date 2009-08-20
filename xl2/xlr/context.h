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
#include <set>
#include <vector>
#include "base.h"

XL_BEGIN

struct Tree;
struct Context;
struct Errors;

typedef std::map<text, Tree *>  symbol_table;
typedef std::vector<Tree *>     evaluation_stack;
typedef std::set<Tree *>        active_set;


struct Context
// ----------------------------------------------------------------------------
//   This is the execution context in which we evaluate trees
// ----------------------------------------------------------------------------
{
    // Constructors and destructors
    Context(Errors &err):
        errors(err), parent(NULL), gc_threshold(200), error_handler(NULL) {}
    Context(Context *p):
        errors(p->errors), parent(p), gc_threshold(200), error_handler(NULL) {}

    // Context properties
    Context *           Parent()                { return parent; }
    Tree *              ErrorHandler();
    void                SetErrorHandler(Tree *e) { error_handler = e; }
    
    // Garbage collection
    void                Root(Tree *t)           { roots.insert(t); }
    void                Mark(Tree *t)            { active.insert(t); }
    void                CollectGarbage();

    // Evaluation of trees
    Tree *              Run (Tree *what);
    Tree *              Error (text message, Tree *args = NULL);
    uint                Push (Tree *tos);
    Tree *              Pop(void);
    Tree *              Peek(uint depth = 1);
    uint                Depth(void)             { return stack.size(); }

    // Symbol management
    Tree *              Name (text name, bool deep = true);
    Tree *              Infix (text name, bool deep = true);
    Tree *              Prefix (text name, bool deep = true);
    Tree *              Postfix (text name, bool deep = true);
    Tree *              Block (text opening, bool deep = true);

    // Entering symbols in the symbol table
    void                EnterName (text name, Tree *value);
    void                EnterInfix (text name, Tree *value);
    void                EnterPrefix (text name, Tree *value);
    void                EnterPostfix (text name, Tree *value);
    void                EnterBlock (text name, Tree *value);

public:
    static ulong        gc_increment;
    static ulong        gc_growth_percent;

private:
    Errors &            errors;
    Context *           parent;
    symbol_table        name_symbols;
    symbol_table        infix_symbols;
    symbol_table        prefix_symbols;
    symbol_table        postfix_symbols;
    symbol_table        block_symbols;
    active_set          active;
    active_set          roots;
    evaluation_stack    stack;
    ulong               gc_threshold;
    Tree *              error_handler;
};

XL_END

#endif // CONTEXT_H
