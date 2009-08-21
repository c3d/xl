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
//   Create a local scope, i.e. a frame for local variables
// ----------------------------------------------------------------------------
{
    Scope(tree_position pos = NOWHERE):
        Native(NULL, pos), parameterCount(0), values() {}
    Tree *              Run(Scope *scope);
    Tree *              Next() { return NULL; }
    Tree *              Error (text message,
                               Tree *a1=NULL, Tree *a2=NULL, Tree *a3=NULL);
    ulong               parameterCount;
    tree_list           values;
};


struct Invoke : Scope
// ----------------------------------------------------------------------------
//   Invoke a rewrite with some local variable
// ----------------------------------------------------------------------------
{
    Invoke(tree_position pos = NOWHERE):
        Scope(pos), child(NULL) {}
    Tree *              Run(Scope *scope);
    void                AddArgument(Tree *value);
    Tree *              child;
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


struct BranchTarget : Native
// ----------------------------------------------------------------------------
//   A branch target for conditionals
// ----------------------------------------------------------------------------
{
    BranchTarget(tree_position pos = NOWHERE): Native(NULL, pos) {}
};


struct FailedCall : Native
// ----------------------------------------------------------------------------
//    Indicate if we don't know how to deal with a call
// ----------------------------------------------------------------------------
{
    FailedCall(Tree *src, tree_position pos = NOWHERE):
        Native(NULL, pos), source(src) {}
    Tree *Run(Scope *scope)
    {
        return scope->Error("Don't know how to call '$1'", source);
    }
    Tree *source;
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


// ============================================================================
// 
//    Helpers for infix / prefix definition
// 
// ============================================================================
//    See how these are used in opcodes_declare.h and opcodes_define.h

longlong integer_arg(Scope *scope, ulong index);
double real_arg(Scope *scope, ulong index);
text text_arg(Scope *scope, ulong index);
bool boolean_arg(Scope *scope, ulong index);
Tree *anything_arg(Scope *scope, ulong index);
Tree *AddParameter(Tree *existing, Tree *append);

#define ANYTHING(index) scope->values[(index)]
#define INT(index)      integer_arg(scope, (index))
#define REAL(index)     real_arg(scope, (index))
#define TEXT(index)     text_arg(scope, (index))
#define BOOL(index)     boolean_arg(scope, (index))
#define RINT(val)       return new Integer((val), scope->Position())
#define RREAL(val)      return new Real((val), scope->Position())
#define RTEXT(val)      return new Text((val), scope->Position())
#define RBOOL(val)      return (val) ? true_name : false_name

typedef longlong integer_t;
typedef double real_t;
typedef text text_t;
typedef bool boolean_t;
typedef Tree *anything_t;

XL_END

#endif // OPCODES_H
