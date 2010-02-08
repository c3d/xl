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
#include <iostream>
#include "tree.h"
#include "context.h"
#include "renderer.h"
#include "opcodes.h"
#include "options.h"
#include "sha1.h"
#include "hash.h"

XL_BEGIN

// ============================================================================
//
//    Class Tree
//
// ============================================================================

Tree::~Tree()
// ----------------------------------------------------------------------------
//   Delete the hash if we have one
// ----------------------------------------------------------------------------
{
    if (hash)
        delete hash;
}


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


Name::operator bool()
// ----------------------------------------------------------------------------
//    Conversion of a name to text
// ----------------------------------------------------------------------------
{
    if (this == xl_true)
        return true;
    else if (this == xl_false)
        return false;
    Error("Value '$1' is not a boolean value", this);
    return false;
}


void Tree::SetSymbols(Symbols *s)
// ----------------------------------------------------------------------------
//   Change the symbols table, make sure we don't overwrite stuff
// ----------------------------------------------------------------------------
{
    if (symbols && symbols != s)
    {
        std::cerr << "WARNING: Symbol overwritten on " << this << '\n';
        std::cerr << "Old symbols were:\n";
        debugsc(symbols);
        std::cerr << "New symbols were:\n";
        debugsc(s);
    }
    symbols = s;
    Context::context->active_symbols.insert(s);
}



// ============================================================================
// 
//   TreeRoot - Protect a tree against garbage collection
// 
// ============================================================================

TreeRoot::TreeRoot(Tree *t)
// ----------------------------------------------------------------------------
//   Record the root in the current context
// ----------------------------------------------------------------------------
    : tree(t)
{
    Context::context->roots.insert(this);
}


TreeRoot::TreeRoot(const TreeRoot &o)
// ----------------------------------------------------------------------------
//   Record the root in the current context
// ----------------------------------------------------------------------------
    : tree(o.tree)
{
    Context::context->roots.insert(this);
}


TreeRoot::~TreeRoot()
// ----------------------------------------------------------------------------
//   Remove a root from the context
// ----------------------------------------------------------------------------
{
    Context::context->roots.erase(this);
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
    return NULL;
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


text Block::indent   = "I+";
text Block::unindent = "I-";
text Text::textQuote = "\"";
text Text::charQuote = "'";


#include "sha1_ostream.h"

text sha1(Tree *t)
// ----------------------------------------------------------------------------
//    Compute the SHA-1 for a tree and return it
// ----------------------------------------------------------------------------
{
    text result;
    if (t)
    {
        TreeHashAction<> sha1;
        t->Do(sha1);
        std::ostringstream os;
        os << *t->hash;
        result = os.str();
    }
    return result;
}

XL_END


