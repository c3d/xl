// ****************************************************************************
//  tree.cpp                        (C) 1992-2003 Christophe de Dinechin (ddd)
//                                                            XL2 project
// ****************************************************************************
//
//   File Description:
//
//     Implementation of the parse tree elements
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

#include <sstream>
#include <cassert>
#include "tree.h"
#include "context.h"
#include "renderer.h"

XL_BEGIN

// ============================================================================
//
//    Class Tree
//
// ============================================================================

void *Tree::operator new(size_t sz)
// ----------------------------------------------------------------------------
//    Record the tree in the garbage collector
// ----------------------------------------------------------------------------
{
    void *result = ::operator new(sz);
    if (Context::context)
        Context::context->Mark((Tree *) result);
    return result;
}


Tree::operator text()
// ----------------------------------------------------------------------------
//   Conversion of a tree to text
// ----------------------------------------------------------------------------
{
    std::ostringstream out;
    out << this;
    return out.str();
}



// ============================================================================
// 
//   Actions on a tree
// 
// ============================================================================

Tree *Tree::Do(Action *action)
// ----------------------------------------------------------------------------
//   Perform an action on the tree 
// ----------------------------------------------------------------------------
{
    switch(Kind())
    {
    case INTEGER:       return action->DoInteger((Integer *) this);
    case REAL:          return action->DoReal((Real *) this);
    case TEXT:          return action->DoText((Text *) this);
    case NAME:          return action->DoName((Name *) this);
    case BLOCK:         return action->DoBlock((Block *) this);
    case PREFIX:        return action->DoPrefix((Prefix *) this);
    case POSTFIX:       return action->DoPostfix((Postfix *) this);
    case INFIX:         return action->DoInfix((Infix *) this);
    default:            assert(!"Unexpected tree kind");
    }
}


Tree *Action::DoInteger(Integer *what)
// ----------------------------------------------------------------------------
//   Default is simply to invoke 'Do'
// ----------------------------------------------------------------------------
{
    return Do(what);
}


Tree *Action::DoReal(Real *what)
// ----------------------------------------------------------------------------
//   Default is simply to invoke 'Do'
// ----------------------------------------------------------------------------
{
    return Do(what);
}


Tree *Action::DoText(Text *what)
// ----------------------------------------------------------------------------
//   Default is simply to invoke 'Do'
// ----------------------------------------------------------------------------
{
    return Do(what);
}


Tree *Action::DoName(Name *what)
// ----------------------------------------------------------------------------
//   Default is simply to invoke 'Do'
// ----------------------------------------------------------------------------
{
    return Do(what);
}


Tree *Action::DoBlock(Block *what)
// ----------------------------------------------------------------------------
//    Default is to firm perform action on block's child, then on self
// ----------------------------------------------------------------------------
{
    what->child = what->child->Do(this);
    return Do(what);
}


Tree *Action::DoPrefix(Prefix *what)
// ----------------------------------------------------------------------------
//   Default is to run the action on the left, then on right
// ----------------------------------------------------------------------------
{
    what->left = what->left->Do(this);
    what->right = what->right->Do(this);
    return Do(what);
}


Tree *Action::DoPostfix(Postfix *what)
// ----------------------------------------------------------------------------
//   Default is to run the action on the right, then on the left
// ----------------------------------------------------------------------------
{
    what->right = what->right->Do(this);
    what->left = what->left->Do(this);
    return Do(what);
}


Tree *Action::DoInfix(Infix *what)
// ----------------------------------------------------------------------------
//   Default is to run the action on children first
// ----------------------------------------------------------------------------
{
    what->left = what->left->Do(this);
    what->right = what->right->Do(this);
    return Do(what);
}


text Block::indent   = "\t+";
text Block::unindent = "\t-";
text Text::textQuote = "\"";
text Text::charQuote = "'";



// ============================================================================
// 
//    Tree shape equality comparison
// 
// ============================================================================

struct TreeMatch : Action
// ----------------------------------------------------------------------------
//   Check if two trees match in structure
// ----------------------------------------------------------------------------
{
    TreeMatch (Tree *t): test(t) {}
    Tree *DoInteger(Integer *what)
    {
        if (Integer *it = test->AsInteger())
            if (it->value == what->value)
                return what;
        return NULL;
    }
    Tree *DoReal(Real *what)
    {
        if (Real *rt = test->AsReal())
            if (rt->value == what->value)
                return what;
        return NULL;
    }
    Tree *DoText(Text *what)
    {
        if (Text *tt = test->AsText())
            if (tt->value == what->value)
                return what;
        return NULL;
    }
    Tree *DoName(Name *what)
    {
        if (Name *nt = test->AsName())
            if (nt->value == what->value)
                return what;
        return NULL;
    }

    Tree *DoBlock(Block *what)
    {
        // Test if we exactly match the block, i.e. the reference is a block
        if (Block *bt = test->AsBlock())
        {
            if (bt->opening == what->opening &&
                bt->closing == what->closing)
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
        if (Infix *it = test->AsInfix())
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
        if (Prefix *pt = test->AsPrefix())
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
        if (Postfix *pt = test->AsPostfix())
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


XL_END


// ============================================================================
// 
//    Runtime functions that can be invoked from the compiled code
// 
// ============================================================================

extern "C"
{
    using namespace XL;


    Tree *xl_identity(Tree *what)
    // ------------------------------------------------------------------------
    //   Return the input tree unchanged
    // ------------------------------------------------------------------------
    {
        return what;
    }


    Tree *xl_evaluate(Tree *what)
    // ------------------------------------------------------------------------
    //   Compile the tree if necessary, then evaluate it
    // ------------------------------------------------------------------------
    // This is similar to Context::Run, but we save stack space for recursion
    {
        if (!what)
            return what;

        if (!what->code)
            what = Context::context->Compile(what);

        assert(what->code);
        Tree *result = what->code(what);
        return result;
    }


    bool xl_same_text(Text *what, const char *ref)
    // ------------------------------------------------------------------------
    //   Compile the tree if necessary, then evaluate it
    // ------------------------------------------------------------------------
    {
        assert(what && what->Kind() == TEXT);
        return what->value == text(ref);
    }


    bool xl_same_shape(Tree *left, Tree *right)
    // ------------------------------------------------------------------------
    //   Check equality of two trees
    // ------------------------------------------------------------------------
    {
        XL::TreeMatch compareForEquality(right);
        if (left->Do(compareForEquality))
            return true;
        return false;
    }


    bool xl_type_check(Tree *value, Tree *type)
    // ------------------------------------------------------------------------
    //   Check if value has the type of 'type'
    // ------------------------------------------------------------------------
    {
        if (!type->code)
            return false;
        Tree *afterTypeCast = type->code(value);
        if (afterTypeCast != value)
            return false;
        return true;
    }

} // extern "C"
