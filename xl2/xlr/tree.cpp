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
#include <typeinfo>
#include "tree.h"
#include "opcodes.h"
#include "context.h"
#include "renderer.h"

XL_BEGIN

// ============================================================================
//
//    Class Tree
//
// ============================================================================

tree_position Tree::NOWHERE = ~0UL;

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


Tree *Tree::Run(Scope *scope)
// ----------------------------------------------------------------------------
//   By default, we don't know how to evaluate a tree
// ----------------------------------------------------------------------------
{
    return scope->Error("Don't know how to evaluate '$1'", this);
}


Tree *Tree::Do(Action *action)
// ----------------------------------------------------------------------------
//   Perform an action on the tree and its descendents
// ----------------------------------------------------------------------------
{
    return action->Do(this);
}


void Tree::DoData(Action *action)
// ----------------------------------------------------------------------------
//   For data actions, run on all trees referenced by this tree
// ----------------------------------------------------------------------------
{
    // Loop on all associated data
    for (tree_data::iterator i = data.begin(); i != data.end(); i++)
        (*i).second = (*i).second->Do(action);
}


Tree *Tree::Normalize()
// ----------------------------------------------------------------------------
//   The basic tree types are normalized by definition
// ----------------------------------------------------------------------------
{
    // Verify that we don't use the default 'Normalize' for unholy trees
    XL_ASSERT(dynamic_cast<Integer *> (this)  ||
              dynamic_cast<Real *> (this)     ||
              dynamic_cast<Text *> (this)     ||
              dynamic_cast<Name *> (this)     ||
              dynamic_cast<Block *> (this)    ||
              dynamic_cast<Prefix *> (this)   ||
              dynamic_cast<Infix *> (this));

    return this;
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
//   Class Integer
// 
// ============================================================================

Tree *Action::DoInteger(Integer *what)
// ----------------------------------------------------------------------------
//   Default is simply to invoke 'Do'
// ----------------------------------------------------------------------------
{
    return Do(what);
}


Tree *Integer::Do(Action *action)
// ----------------------------------------------------------------------------
//   Call specialized Integer routine in the action
// ----------------------------------------------------------------------------
{
    return action->DoInteger(this);
}



// ============================================================================
// 
//   Class Real
// 
// ============================================================================

Tree *Action::DoReal(Real *what)
// ----------------------------------------------------------------------------
//   Default is simply to invoke 'Do'
// ----------------------------------------------------------------------------
{
    return Do(what);
}


Tree *Real::Do(Action *action)
// ----------------------------------------------------------------------------
//   Call specialized Real routine in the action
// ----------------------------------------------------------------------------
{
    return action->DoReal(this);
}



// ============================================================================
// 
//   Class Text
// 
// ============================================================================

Tree *Action::DoText(Text *what)
// ----------------------------------------------------------------------------
//   Default is simply to invoke 'Do'
// ----------------------------------------------------------------------------
{
    return Do(what);
}


Tree *Text::Do(Action *action)
// ----------------------------------------------------------------------------
//   Call specialized Text routine in the action
// ----------------------------------------------------------------------------
{
    return action->DoText(this);
}



// ============================================================================
// 
//   Class Name
// 
// ============================================================================

Tree *Action::DoName(Name *what)
// ----------------------------------------------------------------------------
//   Default is simply to invoke 'Do'
// ----------------------------------------------------------------------------
{
    return Do(what);
}


Tree *Name::Do(Action *action)
// ----------------------------------------------------------------------------
//   Call specialized Name routine in the action
// ----------------------------------------------------------------------------
{
    return action->DoName(this);
}


Tree *Name::Run(Scope *scope)
// ----------------------------------------------------------------------------
//    Evaluate a name
// ----------------------------------------------------------------------------
{
    // If not found at compile-time, this is an error to evaluate the name
    return scope->Error("Name '$1' doesn't exist", this);
}



// ============================================================================
// 
//   Class Block
// 
// ============================================================================

Tree *Action::DoBlock(Block *what)
// ----------------------------------------------------------------------------
//    Default is to firm perform action on block's child, then on self
// ----------------------------------------------------------------------------
{
    what->child = what->child->Do(this);
    return Do(what);
}


Tree *Block::Do(Action *action)
// ----------------------------------------------------------------------------
//   Run the action on the child first
// ----------------------------------------------------------------------------
{
    return action->DoBlock(this);
}


Tree *Block::Run(Scope *scope)
// ----------------------------------------------------------------------------
//    Execute a block
// ----------------------------------------------------------------------------
{
    // If the block was not identified at compile time, this is an error
    return scope->Error ("Unidentified block '$1'", this);
}


Block *Block::MakeBlock(Tree *child, text open, text close, tree_position pos)
// ----------------------------------------------------------------------------
//   Create the right type of block based on open and close
// ----------------------------------------------------------------------------
{
    static Block indent(NULL);
    static Parentheses paren(NULL);
    static Brackets brackets(NULL);
    static Curly curly(NULL);
    if (open == indent.Opening() && close == indent.Closing())
        return new Block(child, pos);
    else if (open == paren.Opening() && close == paren.Closing())
        return new Parentheses(child, pos);
    else if (open == brackets.Opening() && close == brackets.Closing())
        return new Brackets(child, pos);
    else if (open == curly.Opening() && close == curly.Closing())
        return new Curly(child, pos);
    return new DelimitedBlock(child, open, close, pos);
}



// ============================================================================
// 
//   Class Prefix
// 
// ============================================================================

Tree *Action::DoPrefix(Prefix *what)
// ----------------------------------------------------------------------------
//   Default is to run the action on the left, then on right
// ----------------------------------------------------------------------------
{
    what->left = what->left->Do(this);
    what->right = what->right->Do(this);
    return Do(what);
}


Tree *Prefix::Do(Action *action)
// ----------------------------------------------------------------------------
//   Run the action on the left and right children first
// ----------------------------------------------------------------------------
{
    return action->DoPrefix(this);
}


Tree *Prefix::Run(Scope *scope)
// ----------------------------------------------------------------------------
//    Execute a prefix node
// ----------------------------------------------------------------------------
{
    // If not found at compile-time, this is an error
    return scope->Error("Don't know how to call '$1'", left);
}



// ============================================================================
// 
//   Class Postfix
// 
// ============================================================================

Tree *Action::DoPostfix(Postfix *what)
// ----------------------------------------------------------------------------
//   Default is to run the action on the right, then on the left
// ----------------------------------------------------------------------------
{
    what->right = what->right->Do(this);
    what->left = what->left->Do(this);
    return Do(what);
}


Tree *Postfix::Do(Action *action)
// ----------------------------------------------------------------------------
//   Run the action on the left and right children first
// ----------------------------------------------------------------------------
{
    return action->DoPostfix(this);
}


Tree *Postfix::Run(Scope *scope)
// ----------------------------------------------------------------------------
//    Execute a postfix node
// ----------------------------------------------------------------------------
{
    // If the postfix was not identified at compile-time, this is an error
    return scope->Error("Don't know how to call '$1'", right);
}



// ============================================================================
// 
//    Class Infix 
// 
// ============================================================================

Tree *Action::DoInfix(Infix *what)
// ----------------------------------------------------------------------------
//   Default is to run the action on children first
// ----------------------------------------------------------------------------
{
    what->left = what->left->Do(this);
    what->right = what->right->Do(this);
    return Do(what);
}


Tree *Infix::Do(Action *action)
// ----------------------------------------------------------------------------
//   Run the action on the children first
// ----------------------------------------------------------------------------
{
    return action->DoInfix(this);
}


Tree *Infix::Run(Scope *scope)
// ----------------------------------------------------------------------------
//    Execute an infix node
// ----------------------------------------------------------------------------
{
    return scope->Error ("Cannot evaluate unknown infix '$1'", this);
}

XL_END
