// ****************************************************************************
//  opcodes.cpp                     (C) 1992-2009 Christophe de Dinechin (ddd)
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

#include "opcodes.h"
#include "basics.h"

XL_BEGIN

Tree *Invoke::Run(Context *context)
// ----------------------------------------------------------------------------
//    Run the child in the local context
// ----------------------------------------------------------------------------
{
    symbol_table save = locals.name_symbols;
    if (context != locals.Parent())
        return context->Error("Invalid context for '$1'", this);
    Tree *result = child->Run(&locals);
    locals.name_symbols = save;
    return result;
}


struct TreeMatch : Action
// ----------------------------------------------------------------------------
//   Check if two trees match in structure
// ----------------------------------------------------------------------------
{
    TreeMatch (Tree *t): test(t) {}
    Tree *DoInteger(Integer *what)
    {
        if (Integer *it = dynamic_cast<Integer *> (test))
            if (it->value == what->value)
                return what;
        return NULL;
    }
    Tree *DoReal(Real *what)
    {
        if (Real *rt = dynamic_cast<Real *> (test))
            if (rt->value == what->value)
                return what;
        return NULL;
    }
    Tree *DoText(Text *what)
    {
        if (Text *tt = dynamic_cast<Text *> (test))
            if (tt->value == what->value)
                return what;
        return NULL;
    }
    Tree *DoName(Name *what)
    {
        if (Name *nt = dynamic_cast<Name *> (test))
            if (nt->value == what->value)
                return what;
        return NULL;
    }

    Tree *DoBlock(Block *what)
    {
        // Test if we exactly match the block, i.e. the reference is a block
        if (Block *bt = dynamic_cast<Block *> (test))
        {
            if (bt->Opening() == what->Opening() &&
                bt->Closing() == what->Closing())
            {
                test = bt->child;
                Tree *br = what->child->Do(this);
                test = bt;
                if (br)
                    return br;
            }
        }
        return NULL;
    }
    Tree *DoInfix(Infix *what)
    {
        if (Infix *it = dynamic_cast<Infix *> (test))
        {
            // Check if we match the tree, e.g. A+B vs 2+3
            if (it->name == what->name)
            {
                test = it->left;
                Tree *lr = what->left->Do(this);
                test = it;
                if (!lr)
                    return NULL;
                test = it->right;
                Tree *rr = what->right->Do(this);
                test = it;
                if (!rr)
                    return NULL;
                return what;
            }
        }
        return NULL;
    }
    Tree *DoPrefix(Prefix *what)
    {
        if (Prefix *pt = dynamic_cast<Prefix *> (test))
        {
            // Check if we match the tree, e.g. f(A) vs. f(2)
            test = pt->left;
            Tree *lr = what->left->Do(this);
            test = pt;
            if (!lr)
                return NULL;
            test = pt->right;
            Tree *rr = what->right->Do(this);
            test = pt;
            if (!rr)
                return NULL;
            return what;
        }
        return NULL;
    }
    Tree *DoPostfix(Postfix *what)
    {
        if (Postfix *pt = dynamic_cast<Postfix *> (test))
        {
            // Check if we match the tree, e.g. A! vs 2!
            test = pt->right;
            Tree *rr = what->right->Do(this);
            test = pt;
            if (!rr)
                return NULL;
            test = pt->left;
            Tree *lr = what->left->Do(this);
            test = pt;
            if (!lr)
                return NULL;
            return what;
        }
        return NULL;
    }
    Tree *Do(Tree *what)
    {
        return NULL;
    }

    Tree *      test;
};


Tree *EqualityTest::Run (Context *context)
// ----------------------------------------------------------------------------
//   Check equality of two trees
// ----------------------------------------------------------------------------
{
    Tree *code = test->Run(context);
    Tree *ref = value->Run(context);
    condition = false;
    TreeMatch compareForEquality(ref);
    if (code->Do(compareForEquality))
        condition = true;
    return code;
}


Tree *TypeTest::Run (Context *context)
// ----------------------------------------------------------------------------
//   Check if the code being tested has the given type value
// ----------------------------------------------------------------------------
{
    Tree *code = test->Run(context);
    Tree *ref = type_value->Run(context);
    condition = false;
    if (TypeExpression *typeChecker = dynamic_cast<TypeExpression *>(ref))
        if (typeChecker->HasType(context, code))
            condition = true;
    return code;
}

XL_END

