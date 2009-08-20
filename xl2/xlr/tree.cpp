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

#include "tree.h"
#include "context.h"

XL_BEGIN

// ============================================================================
//
//    Class Tree
//
// ============================================================================

tree_position Tree::NOWHERE = ~0UL;

Tree *Tree::Run(Context *context)
// ----------------------------------------------------------------------------
//   Execute a tree.
// ----------------------------------------------------------------------------
//   [ ] -> [ this ]
{
    context->Push(this);
    return this;
}


Tree *Tree::Do(Action &action)
// ----------------------------------------------------------------------------
//   Perform an action on the tree and its descendents
// ----------------------------------------------------------------------------
{
    return action.Run(this);
}


Tree *Tree::Mark(Action &action)
// ----------------------------------------------------------------------------
//   Mark all trees referenced by this tree
// ----------------------------------------------------------------------------
{
    // Loop on all associated data
    for (tree_data::iterator i = data.begin(); i != data.end(); i++)
        (*i).second = action.Run((*i).second);

    // And finish with self
    return Do(action);
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



// ============================================================================
// 
//   Class Name
// 
// ============================================================================

Tree *Name::Run(Context *context)
// ----------------------------------------------------------------------------
//    Evaluate a name
// ----------------------------------------------------------------------------
//    [ ] -> [ named-value ]
{
    if (Tree *named = context->Name(value))
    {
        // We have a tree with that name in the context, put that on TOS
        context->Mark(named->Run(context));

        // Make sure we lookup the name every time
        return this;
    }

    // Otherwise, this is an error to evaluate the name
    return context->Error("Name '$1' doesn't exist", this);
}



// ============================================================================
// 
//   Class Block
// 
// ============================================================================

Tree *Block::Do(Action &action)
// ----------------------------------------------------------------------------
//   Run the action on the child first
// ----------------------------------------------------------------------------
{
    child = action.Run(child);
    return Tree::Do(action);
}


Tree *Block::Run(Context *context)
// ----------------------------------------------------------------------------
//    Execute a block
// ----------------------------------------------------------------------------
{
    // If there is a block operation, execute that operation on child
    Tree *blockOp = context->Block(Opening());
    if (blockOp)
    {
        // Leave child unevaluated for block operation to process
        context->Push(this);
        context->Push(child);
        return blockOp->Run(context);
    }

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

Tree *Prefix::Do(Action &action)
// ----------------------------------------------------------------------------
//   Run the action on the left and right children first
// ----------------------------------------------------------------------------
{
    left = action.Run(left);
    right = action.Run(right);
    return Tree::Do(action);
}


Tree *Prefix::Run(Context *context)
// ----------------------------------------------------------------------------
//    Execute a prefix node
// ----------------------------------------------------------------------------
{
    // If the name denotes a known prefix, then execute, e.g. sin X
    if (Name *name = dynamic_cast<Name *> (left))
    {
        if (Tree *prefixOp = context->Prefix(name->value))
        {
            context->Push(this);
            context->Push(right);
            return prefixOp->Run(context);
        }
    }

   // Evaluate left, e.g. in A[5] (3), evaluate A[5]
    left = left->Run(context);

    // Retrieve TOS, i.e. evaluated value of A[5]
    Tree *leftVal = context->Pop();

    // If leftVal is left, we don't know what to do with it.
    if (leftVal == left)
        return context->Error("Uknown prefix operation '$1'", this);

    // Otherwise, evaluate with that result
    context->Push(this);
    context->Push(right);
    return leftVal->Run(context);
}



// ============================================================================
// 
//   Class Postfix
// 
// ============================================================================

Tree *Postfix::Do(Action &action)
// ----------------------------------------------------------------------------
//   Run the action on the left and right children first
// ----------------------------------------------------------------------------
{
    left = action.Run(left);
    right = action.Run(right);
    return Tree::Do(action);
}


Tree *Postfix::Run(Context *context)
// ----------------------------------------------------------------------------
//    Execute a postfix node
// ----------------------------------------------------------------------------
{
    // If the name denotes a known postfix, then execute, e.g. sin X
    if (Name *name = dynamic_cast<Name *> (left))
    {
        if (Tree *postfixOp = context->Postfix(name->value))
        {
            context->Push(this);
            context->Push(left);
            return postfixOp->Run(context);
        }
    }

    return context->Error("Uknown postfix operation '$1'", this);
}



// ============================================================================
// 
//    Class Infix 
// 
// ============================================================================

Tree *Infix::Do(Action &action)
// ----------------------------------------------------------------------------
//   Run the action on the children first
// ----------------------------------------------------------------------------
{
    tree_list::iterator i;
    for (i = list.begin(); i != list.end(); i++)
        *i = action.Run(*i);
    return Tree::Do(action);
}


Tree *Infix::Run(Context *context)
// ----------------------------------------------------------------------------
//    Execute an infix node
// ----------------------------------------------------------------------------
{
    // If the name denotes a known infix, then execute, e.g. A+B
    if (Tree *infixOp = context->Infix(name))
    {
        tree_list::iterator i;
        bool firstOne = true;
        Tree *result = this;
        context->Push(this);
        for (i = list.begin(); i != list.end(); i++)
        {
            context->Push(*i);
            if (!firstOne)
            {
                result = infixOp->Run(context);
                context->Mark(result);
            }
            firstOne = true;
        }
        return result;
    }

    return context->Error("Uknown infix operation '$1'", this);
}



// ============================================================================
// 
//    Class Native
// 
// ============================================================================

Tree *Native::Do(Action &action)
// ----------------------------------------------------------------------------
//   For native nodes, default actions will do
// ----------------------------------------------------------------------------
{
    return Tree::Do(action);
}


Tree *Native::Run(Context *context)
// ----------------------------------------------------------------------------
//    Execute a infix node
// ----------------------------------------------------------------------------
{
    return context->Error("Uknown native operation '$1'", this);
}



// ============================================================================
//
//    Output: Show a possibly parenthesized rendering of the tree
//
// ============================================================================

XL_END
