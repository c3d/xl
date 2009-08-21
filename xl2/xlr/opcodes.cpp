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

// ============================================================================
// 
//    Class Native
// 
// ============================================================================

Tree *Action::DoNative(Native *what)
// ----------------------------------------------------------------------------
//   Default is simply to invoke 'Do'
// ----------------------------------------------------------------------------
{
    return Do(what);
}


Tree *Native::Do(Action *action)
// ----------------------------------------------------------------------------
//   For native nodes, default actions will do
// ----------------------------------------------------------------------------
{
    return action->DoNative(this);
}


Tree *Native::Run(Scope *scope)
// ----------------------------------------------------------------------------
//    Running a native node returns the native itself
// ----------------------------------------------------------------------------
{
    return this;
}


text Native::TypeName()
// ----------------------------------------------------------------------------
//   The name of a native tree is its type
// ----------------------------------------------------------------------------
{
    return typeid(*this).name();
}


Tree *Native::Append(Tree *tail)
// ----------------------------------------------------------------------------
//   Append another tree to a native tree
// ----------------------------------------------------------------------------
{
    // Find end of opcode chain. If end is not native, optimize away
    Native *prev = this;
    while (Native *next = dynamic_cast<Native *> (prev->next))
        prev = next;

    // String right opcode at end
    prev->next = tail;

    return this;
}


Tree *Scope::Run(Scope *scope)
// ----------------------------------------------------------------------------
//    Enter a scope, capture caller's variables
// ----------------------------------------------------------------------------
{
    tree_list saveFrame = values;
    ulong i, max = scope->values.size();
    if (max > values.size())
        return scope->Error("Internal error: Frame size error '$1'", this);

    // Copy input frame
    for (i = 0; i < max; i++)
        values[i] = scope->values[i];

    // Run 'next'
    Tree *result = next->Run(this);

    // Restore old values
    for (i = 0; i < max; i++)
        values[i] = saveFrame[i];

    return result;
}


Tree * Variable::Run(Scope *scope)
// ----------------------------------------------------------------------------
//   Return the variable at given index in the scope
// ----------------------------------------------------------------------------
{
    if (id < scope->values.size())
    {
        Tree *value = scope->values[id];
        if (value)
            return value->Run(scope);
    }
    return scope->Error("Unbound variable '$1'", this);
}


Tree *Invoke::Run(Scope *scope)
// ----------------------------------------------------------------------------
//    Run the child in the local arguments scope
// ----------------------------------------------------------------------------
{
    return child->Run(this);
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


Tree *EqualityTest::Run (Scope *scope)
// ----------------------------------------------------------------------------
//   Check equality of two trees
// ----------------------------------------------------------------------------
{
    Tree *code = test->Run(scope);
    Tree *ref = value->Run(scope);
    condition = false;
    TreeMatch compareForEquality(ref);
    if (code->Do(compareForEquality))
        condition = true;
    return code;
}


Tree *TypeTest::Run (Scope *scope)
// ----------------------------------------------------------------------------
//   Check if the code being tested has the given type value
// ----------------------------------------------------------------------------
{
    Tree *code = test->Run(scope);
    Tree *ref = type_value->Run(scope);
    condition = false;
    if (TypeExpression *typeChecker = dynamic_cast<TypeExpression *>(ref))
        if (typeChecker->HasType(scope, code))
            condition = true;
    return code;
}

XL_END

