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
#include "context.h"

XL_BEGIN

// ============================================================================
// 
//    Native code
// 
// ============================================================================

struct Native : Tree
// ----------------------------------------------------------------------------
//   A native tree is intended to represent directly executable code
// ----------------------------------------------------------------------------
{
    Native(Tree *n=NULL, tree_position pos = NOWHERE): Tree(pos), next(n) {}
    virtual Tree *      Do(Action *action);
    virtual Tree *      Run(Scope *scope);
    virtual text        TypeName();
    virtual Tree *      Next()          { return next; }
    virtual Tree *      Append(Tree *tail);
    Tree *              next;   // Next opcode to run
};


struct Scope : Native
// ----------------------------------------------------------------------------
//   Create an execution context with local variables
// ----------------------------------------------------------------------------
{
    Scope(Context *c, tree_position pos = NOWHERE):
        Native(NULL, pos), locals(c), values() {}
    Tree *              Run(Scope *scope);
    Tree *              Next() { return NULL; }
    Tree *              Error (text message,
                               Tree *a1=NULL, Tree *a2=NULL, Tree *a3=NULL);
    Context *           locals;
    tree_list           values;
};


struct Variable : Native
// ----------------------------------------------------------------------------
//   A reference to some entity in a scope
// ----------------------------------------------------------------------------
{
    Variable(ulong vid, tree_position pos = NOWHERE):
        Native(NULL,pos), id(vid) {}
    Tree *              Run(Scope *scope);
    ulong               id;
};


struct Invoke : Native
// ----------------------------------------------------------------------------
//   Invoke a rewrite with some local variable
// ----------------------------------------------------------------------------
{
    Invoke(tree_position pos = NOWHERE):
        Native(NULL, pos), scope(s), child(NULL), arguments() {}
    Tree *              Run(Scope *scope);
    Tree *              child;
    tree_list           arguments;
};


struct BranchTarget : Native
// ----------------------------------------------------------------------------
//   A branch target for conditionals
// ----------------------------------------------------------------------------
{
    BranchTarget(tree_position pos = NOWHERE):
        Native(NULL, pos) {}
};
    

struct TreeTest : Native
// ----------------------------------------------------------------------------
//    Test a tree argument for a specific condition
// ----------------------------------------------------------------------------
{
    TreeTest (Tree *toTest, Tree *ift, Tree *iff, tree_position pos = NOWHERE):
        Native(ift, pos), test(toTest), iffalse(iff), condition(true) {}

    Tree *Run(Scope *scope)
    {
        Tree *code = test->Run(scope);
        return code;
    }
    Tree *Next() { return condition ? next : iffalse; }

    Tree *      test;
    Tree *      iffalse;
    bool        condition;
};


struct IntegerTest : TreeTest
// ----------------------------------------------------------------------------
//    Test if its arguments is a given integer value
// ----------------------------------------------------------------------------
{
    IntegerTest (Tree *code, longlong tv,
                 Tree *ift, Tree *iff,
                 tree_position pos = NOWHERE):
        TreeTest(code, ift, iff, pos), value(tv) {}

    Tree *Run(Scope *scope)
    {
        Tree *code = test->Run(scope);
        condition = false;
        if (Integer *iv = dynamic_cast<Integer *> (code))
            if (iv->value == value)
                condition = true;
        return code;
    }
    longlong value;
};


struct RealTest : TreeTest
// ----------------------------------------------------------------------------
//    Test if its arguments is a given real value
// ----------------------------------------------------------------------------
{
    RealTest (Tree *code, double tv,
              Tree *ift, Tree *iff,
              tree_position pos = NOWHERE):
        TreeTest(code, ift, iff, pos), value(tv) {}

    Tree *Run(Scope *scope)
    {
        Tree *code = test->Run(scope);
        condition = false;
        if (Real *iv = dynamic_cast<Real *> (code))
            if (iv->value == value)
                condition = true;
        return code;
    }
    double value;
};


struct TextTest : TreeTest
// ----------------------------------------------------------------------------
//    Test if its arguments is a given text value
// ----------------------------------------------------------------------------
{
    TextTest (Tree *code, text tv,
              Tree *ift, Tree *iff,
              tree_position pos = NOWHERE):
        TreeTest(code, ift, iff, pos), value(tv) {}

    Tree *Run(Scope *scope)
    {
        Tree *code = test->Run(scope);
        condition = false;
        if (Text *iv = dynamic_cast<Text *> (code))
            if (iv->value == value)
                condition = true;
        return code;
    }
    text value;
};


struct NameTest : TreeTest
// ----------------------------------------------------------------------------
//    Test if the argument matches the value of a given name
// ----------------------------------------------------------------------------
{
    NameTest (Tree *code, Name *tv,
              Tree *ift, Tree *iff,
              tree_position pos = NOWHERE):
        TreeTest(code, ift, iff, pos), value(tv) {}

    Tree *Run(Scope *scope)
    {
        Tree *code = test->Run(scope);
        condition = false;
        if (Name *iv = dynamic_cast<Name *> (code))
            if (iv->value == value->value)
                condition = true;
        return code;
    }
    Name *value;
};


struct EqualityTest : TreeTest
// ----------------------------------------------------------------------------
//    Test if the argument matches the value of another tree
// ----------------------------------------------------------------------------
{
    EqualityTest (Tree *code, Tree *tv,
                  Tree *ift, Tree *iff,
                  tree_position pos = NOWHERE):
        TreeTest(code, ift, iff, pos), value(tv) {}

    Tree *Run(Scope *scope);
    Tree *value;
};



struct TypeTest : TreeTest
// ----------------------------------------------------------------------------
//    Test if the argument matches the type given as input
// ----------------------------------------------------------------------------
{
    TypeTest (Tree *code, Tree *tv,
              Tree *ift, Tree *iff,
              tree_position pos = NOWHERE):
        TreeTest(code, ift, iff, pos), type_value(tv) {}

    Tree *Run(Scope *scope);
    Tree *type_value;
};




XL_END

#endif // OPCODES_H
