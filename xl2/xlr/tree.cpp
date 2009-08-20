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


Tree *Tree::Run(Context *context)
// ----------------------------------------------------------------------------
//   The default when executing a tree is to return it
// ----------------------------------------------------------------------------
{
    return this;
}


Tree *Tree::Call(Context *context, Tree *args)
// ----------------------------------------------------------------------------
//   The default when executing a tree with arguments (invoke)
// ----------------------------------------------------------------------------
{
    // By default, we don't know how to invoke a tree
    return context->Error("Don't know how to evaluate '$1'", this);
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


Tree *Name::Run(Context *context)
// ----------------------------------------------------------------------------
//    Evaluate a name
// ----------------------------------------------------------------------------
{
    if (Tree *named = context->Name(value))
        return named;

    // Otherwise, this is an error to evaluate the name
    return context->Error("Name '$1' doesn't exist", this);
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


Tree *Block::Run(Context *context)
// ----------------------------------------------------------------------------
//    Execute a block
// ----------------------------------------------------------------------------
{
    // If there is a block operation, execute that operation on child
    Tree *blockOp = context->Block(Opening());
    if (blockOp)
        return blockOp->Call(context, child);

    // Otherwise, simply execute the child (i.e. optimize away block)
    return child->Run(context);
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


Tree *Prefix::Run(Context *context)
// ----------------------------------------------------------------------------
//    Execute a prefix node
// ----------------------------------------------------------------------------
{
    // If the name denotes a known prefix, then execute, e.g. sin X
    if (Name *name = dynamic_cast<Name *> (left))
        if (Tree *prefixOp = context->Prefix(name->value))
            return prefixOp->Call(context, right);

    // Evaluate left, e.g. in A[5] (3), evaluate A[5]
    Tree *callee = left->Run(context);
    if (callee)
        return callee->Call(context, right);

    return context->Error("Don't know how to call '$1'", left);
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


Tree *Postfix::Run(Context *context)
// ----------------------------------------------------------------------------
//    Execute a postfix node
// ----------------------------------------------------------------------------
{
    // If the name denotes a known prefix, then execute, e.g. sin X
    if (Name *name = dynamic_cast<Name *> (right))
        if (Tree *postfixOp = context->Postfix(name->value))
            return postfixOp->Call(context, left);

    // Evaluate right
    Tree *callee = right->Run(context);
    if (callee)
        return callee->Call(context, left);

    return context->Error("Don't know how to call '$1'", right);
}



// ============================================================================
// 
//    Class Infix 
// 
// ============================================================================

Infix::Infix(text n, Tree *left, Tree *right, tree_position pos)
// ----------------------------------------------------------------------------
//   Constructor optimizing the tree structure
// ----------------------------------------------------------------------------
    : Tree(pos), name(n), list()
{
    if (Infix *li = dynamic_cast<Infix *> (left))
    {
        if (li->name == name)
        {
            list.insert(list.end(), li->list.begin(), li->list.end());
            left = NULL;
        }
    }
    if (left)
        list.push_back(left);

    if (Infix *ri = dynamic_cast<Infix *> (right))
    {
        if (ri->name == name)
        {
            list.insert(list.end(), ri->list.begin(), ri->list.end());
            right = NULL;
        }
    }
    if (right)
        list.push_back(right);
}


Tree *Action::DoInfix(Infix *what)
// ----------------------------------------------------------------------------
//   Default is to run the action on children first
// ----------------------------------------------------------------------------
{
    tree_list::iterator i;
    for (i = what->list.begin(); i != what->list.end(); i++)
        *i = (*i)->Do(this);
    return Do(what);
}


Tree *Infix::Do(Action *action)
// ----------------------------------------------------------------------------
//   Run the action on the children first
// ----------------------------------------------------------------------------
{
    return action->DoInfix(this);
}


Tree *Infix::Run(Context *context)
// ----------------------------------------------------------------------------
//    Execute an infix node
// ----------------------------------------------------------------------------
{
    // If the name denotes a known infix, then execute, e.g. A+B
    if (Tree *infixOp = context->Infix(name))
        return infixOp->Call(context, this);

    return context->Error("Uknown infix operation '$1'", this);
}


Tree *Infix::Left()
// ----------------------------------------------------------------------------
//   Return the left of an infix if it exists
// ----------------------------------------------------------------------------
{
    if (list.size())
        return list[0];
    return NULL;
}


Tree *Infix::Right()
// ----------------------------------------------------------------------------
//   Return the right of an infix if it exists
// ----------------------------------------------------------------------------
{
    if (list.size() == 2)
        return list[1];
    return NULL;
}



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


Tree *Native::Run(Context *context)
// ----------------------------------------------------------------------------
//    Execute a infix node
// ----------------------------------------------------------------------------
{
    return context->Error("Uknown native operation '$1'", this);
}


text Native::Name()
// ----------------------------------------------------------------------------
//   The name of a native tree is its type
// ----------------------------------------------------------------------------
{
    return typeid(*this).name();
}



// ============================================================================
//
//   Output: Show a possibly parenthesized rendering of the tree
//
// ============================================================================

XL_END
