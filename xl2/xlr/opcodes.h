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

struct OpCode : Native
// ----------------------------------------------------------------------------
//   An opcode tree is intended to represent directly executable code
// ----------------------------------------------------------------------------
{
    OpCode(Tree *next=NULL, tree_position pos = NOWHERE): Native(next,pos) {}
};


struct Named : OpCode
// ----------------------------------------------------------------------------
//   A reference to some entity
// ----------------------------------------------------------------------------
{
    Named(Tree *v=NULL, tree_position pos = NOWHERE):
        OpCode(NULL,pos), value(v) {}
    Tree *              Run(Context *context) { return this; }
    Tree *              value;
};


struct Invoke : OpCode
// ----------------------------------------------------------------------------
//   Invoke a rewrite with some local variable
// ----------------------------------------------------------------------------
{
    Invoke(Context *p, tree_position pos = NOWHERE):
        OpCode(NULL, pos), child(NULL), locals(p) {}
    Tree *              Run(Context *context);
    Tree *              child;
    Context             locals;
};


struct BranchTarget : OpCode
// ----------------------------------------------------------------------------
//   A branch target for conditionals
// ----------------------------------------------------------------------------
{
    BranchTarget(tree_position pos = NOWHERE):
        OpCode(NULL, pos) {}
};
    

struct TreeTest : OpCode
// ----------------------------------------------------------------------------
//    Test a tree argument for a specific condition
// ----------------------------------------------------------------------------
{
    TreeTest (Tree *toTest, Tree *ift, Tree *iff, tree_position pos = NOWHERE):
        OpCode(ift, pos), test(toTest), iffalse(iff), condition(true) {}

    Tree *Run(Context *context)
    {
        Tree *code = test->Run(context);
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

    Tree *Run(Context *context)
    {
        Tree *code = test->Run(context);
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

    Tree *Run(Context *context)
    {
        Tree *code = test->Run(context);
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

    Tree *Run(Context *context)
    {
        Tree *code = test->Run(context);
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

    Tree *Run(Context *context)
    {
        Tree *code = test->Run(context);
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

    Tree *Run(Context *context);
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

    Tree *Run(Context *context);
    Tree *type_value;
};




XL_END

#endif // OPCODES_H
