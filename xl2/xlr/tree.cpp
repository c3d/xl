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
//   By default, we don't know how to evaluate a tree
// ----------------------------------------------------------------------------
{
    return context->Error("Don't know how to evaluate '$1'", this);
}


Tree *Tree::Call(Context *context, Tree *args)
// ----------------------------------------------------------------------------
//   By default, we don't know how to call a tree
// ----------------------------------------------------------------------------
{
    return context->Error("Don't know how to call '$1'", this);
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
        return context->Run(named);

    // Otherwise, this is an error to evaluate the name
    return context->Error("Name '$1' doesn't exist", this);
}


Tree *Name::Call(Context *context, Tree *args)
// ----------------------------------------------------------------------------
//    Call a name: call the associated value
// ----------------------------------------------------------------------------
{
    if (Tree *named = context->Name(value))
        return named->Call(context, args);

    // Otherwise, this is an error to try and call the name
    return context->Error("No match for call '$1 $2'", this, args);
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
    // Otherwise, simply execute the child (i.e. optimize away block)
    return context->Run(child);
}


Tree *Block::Call(Context *context, Tree *args)
// ----------------------------------------------------------------------------
//    Execute a block
// ----------------------------------------------------------------------------
{
    // If there is a block operation, execute that operation on child
    Tree *callee = Run(context);
    return callee->Call(context, args);
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
    // Call the left with the right as argument
    if (Tree *callee = context->Run(left))
        return callee->Call(context, right);

    // If there was no valid left, error out
    return context->Error("Don't know how to call '$1'", left);
}


Tree *Prefix::Call(Context *context, Tree *args)
// ----------------------------------------------------------------------------
//    Call a prefix node
// ----------------------------------------------------------------------------
{
    if (Tree *callee = Run(context))
        return callee->Call(context, args);

    // If there was no valid result, error out
    return context->Error("Don't know how to call prefix '$1'", right);
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
    // Call the right with the left as argument
    if (Tree *callee = context->Run(right))
        return callee->Call(context, left);

    // If there was no valid left, error out
    return context->Error("Don't know how to call '$1'", right);
}


Tree *Postfix::Call(Context *context, Tree *args)
// ----------------------------------------------------------------------------
//    Call a postfix node
// ----------------------------------------------------------------------------
{
    if (Tree *callee = Run(context))
        return callee->Call(context, args);

    // If there was no valid callee, error out
    return context->Error("Don't know how to call postfix '$1'", right);
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


Tree *Infix::Run(Context *context)
// ----------------------------------------------------------------------------
//    Execute an infix node
// ----------------------------------------------------------------------------
{
    return context->Error ("Cannot evaluate unknown infix '$1'", this);
}


Tree *Infix::Call(Context *context, Tree *args)
// ----------------------------------------------------------------------------
//    Call an infix node
// ----------------------------------------------------------------------------
{
    // Check if the result of evaluating ourselves is something we can call
    if (Tree *callee = Run(context))
        return callee->Call(context, args);

    return context->Error("Cannot call unknown infix '$1'", this);
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
//    Running a native node returns the native itself
// ----------------------------------------------------------------------------
{
    return context->Error("Uknown native operation '$1'", this);
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

XL_END
