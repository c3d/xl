// ****************************************************************************
//  basics.cpp                      (C) 1992-2009 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//     Basic operations (arithmetic, ...)
// 
// 
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

#include "basics.h"
#include "context.h"
#include "renderer.h"
#include <iostream>


XL_BEGIN

// ============================================================================
// 
//    Top-level operation
// 
// ============================================================================

ReservedName *true_name = NULL;
ReservedName *false_name = NULL;
ReservedName *nil_name = NULL;

void EnterBasics(Context *c)
// ----------------------------------------------------------------------------
//   Enter all the basic operations defined in this file
// ----------------------------------------------------------------------------
{
#define INFIX(n,t)      \
    do { static t tmp; c->EnterInfix(n, &tmp); } while(0)
#define NAME(n,t)       \
    do { static t tmp(#n); c->EnterName(#n, &tmp); n##_name = &tmp; } while(0)
#define PREFIX(n,t)     \
    do { static t tmp; c->EnterName(n, &tmp); } while(0)

    INFIX("\n", LastInListHandler);
    INFIX(";", LastInListHandler);

    INFIX("+", BinaryAdd);
    INFIX("-", BinarySub);
    INFIX("*", BinaryMul);
    INFIX("/", BinaryDiv);
    INFIX("%", BinaryRemainder);
    INFIX("rem", BinaryRemainder);
    INFIX("<<", BinaryLeftShift);
    INFIX(">>", BinaryRightShift);
    INFIX("&", BinaryAnd);
    INFIX("and", BinaryAnd);
    INFIX("|", BinaryOr);
    INFIX("or", BinaryOr);
    INFIX("^", BinaryXor);
    INFIX("xor", BinaryXor);

    INFIX("<", BooleanLess);
    INFIX("<=", BooleanLessOrEqual);
    INFIX("=", BooleanEqual);
    INFIX("<>", BooleanDifferent);
    INFIX(">", BooleanGreater);
    INFIX(">=", BooleanGreaterOrEqual);

    INFIX(":=", Assignment);
    INFIX("->", Definition);

    NAME(nil, ReservedName);
    NAME(true, ReservedName);
    NAME(false, ReservedName);

    PREFIX("quote", ParseTree);
    PREFIX("eval", Evaluation);
    PREFIX("debug", DebugPrint);
    PREFIX("-", Negate);

    PREFIX("boolean", BooleanType);
    PREFIX("integer", IntegerType);
    PREFIX("real", RealType);
    PREFIX("text", TextType);
    PREFIX("character", CharacterType);
}



// ============================================================================
// 
//    class InfixStructureHandler
// 
// ============================================================================

Tree *LastInListHandler::Call(Context *context, Tree *args)
// ----------------------------------------------------------------------------
//   When processing blocks or A;B, return last value, e.g. B
// ----------------------------------------------------------------------------
{
    if (Infix *infix = dynamic_cast<Infix *> (args))
    {
        Tree *result = NULL;
        result = context->Eval(infix->left);
        result = context->Eval(infix->right);
        return result;
    }
    else
    {
        return context->Error("Infix expected, got '$1'", args);
    }
}



// ============================================================================
// 
//    class BinaryHandler
// 
// ============================================================================

Tree *BinaryHandler::Call(Context *context, Tree *args)
// ----------------------------------------------------------------------------
//   Assume an infix argument, deal with binary ops
// ----------------------------------------------------------------------------
{
    if (Infix *infix = dynamic_cast<Infix *> (args))
    {
        tree_position pos = args->Position();
        Tree *left = context->Eval(infix->left);
        if (!left)
            return context->Error("No value to left of '$1'", args);
        Tree *right = context->Eval(infix->right);
        if (!right)
            return context->Error("No value to right of '$1'", args);

        // Check if implementation is unhappy somehow
        try
        {
            // Check type of first argument
            if (Integer *li = dynamic_cast<Integer *> (left))
                if (Integer *ri = dynamic_cast<Integer *> (right))
                    return new Integer(DoInteger(li->value, ri->value), pos);

            if (Real *lr = dynamic_cast<Real *> (left))
                if (Real *rr = dynamic_cast<Real *> (right))
                    return new Real(DoReal(lr->value, rr->value), pos);

            if (Text *lt = dynamic_cast<Text *> (left))
                if (Text *rt = dynamic_cast<Text *> (right))
                    return new Text(DoText(lt->value, rt->value), pos);

            return context->Error("Incompatible types in '$1'", args);
        }
        catch(kstring msg)
        {
            return context->Error (msg, args);
        }
    }
    return context->Error("Infix expected, got '$1'", args);
}


longlong BinaryHandler::DoInteger(longlong left, longlong right)
// ----------------------------------------------------------------------------
//   Default is to report that operation is not supported
// ----------------------------------------------------------------------------
{
    throw "Operation '$1' not supported on integers";
}


double   BinaryHandler::DoReal(double left, double right)
// ----------------------------------------------------------------------------
//   Default is to report that operation is not supported
// ----------------------------------------------------------------------------
{
    throw "Operation '$1' not supported on real numbers";
}

text     BinaryHandler::DoText(text left, text right)
// ----------------------------------------------------------------------------
//   Default is to report that operation is not supported
// ----------------------------------------------------------------------------
{
    throw "Operation '$1' not supported on text";
}


Tree *Negate::Call(Context *context, Tree *args)
// ----------------------------------------------------------------------------
//   Negate its input argument
// ----------------------------------------------------------------------------
{
    Tree *value = context->Eval(args);
    tree_position pos = args->Position();

    if (Integer *li = dynamic_cast<Integer *> (value))
        return new Integer(-li->value, pos);
    if (Real *ri = dynamic_cast<Real *> (value))
        return new Real(-ri->value, pos);

    return context->Error("Invalid type for negation in '$1'", args);
}


// ============================================================================
// 
//    class BooleanHandler
// 
// ============================================================================

Tree *BooleanHandler::Call(Context *context, Tree *args)
// ----------------------------------------------------------------------------
//   Assume an infix argument, deal with binary boolean ops
// ----------------------------------------------------------------------------
{
    if (Infix *infix = dynamic_cast<Infix *> (args))
    {
        Tree *left = context->Run(infix->left);
        if (!left)
            return context->Error("No value to left of '$1'", args);
        Tree *right = context->Run(infix->right);
        if (!right)
            return context->Error("No value to right of '$1'", args);

        // Check if implementation is unhappy somehow
        try
        {
            // Check type of first argument
            if (Integer *li = dynamic_cast<Integer *> (left))
                if (Integer *ri = dynamic_cast<Integer *> (right))
                    return DoInteger(li->value, ri->value)
                        ? true_name : false_name;

            if (Real *lr = dynamic_cast<Real *> (left))
                if (Real *rr = dynamic_cast<Real *> (right))
                    return DoReal(lr->value, rr->value)
                        ? true_name : false_name;

            if (Text *lt = dynamic_cast<Text *> (left))
                if (Text *rt = dynamic_cast<Text *> (right))
                    return DoText(lt->value, rt->value)
                        ? true_name : false_name;

            return context->Error("Incompatible types in '$1'", args);
        }
        catch(kstring msg)
        {
            return context->Error (msg, args);
        }
    }
    return context->Error("Infix expected, got '$1'", args);
}


bool BooleanHandler::DoInteger(longlong left, longlong right)
// ----------------------------------------------------------------------------
//   Default is to report that operation is not supported
// ----------------------------------------------------------------------------
{
    throw "Operation '$1' not supported on integers";
}


bool BooleanHandler::DoReal(double left, double right)
// ----------------------------------------------------------------------------
//   Default is to report that operation is not supported
// ----------------------------------------------------------------------------
{
    throw "Operation '$1' not supported on real numbers";
}

bool BooleanHandler::DoText(text left, text right)
// ----------------------------------------------------------------------------
//   Default is to report that operation is not supported
// ----------------------------------------------------------------------------
{
    throw "Operation '$1' not supported on text";
}


// ============================================================================
// 
//    class Assignment
// 
// ============================================================================

Tree *Assignment::Call(Context *context, Tree *args)
// ----------------------------------------------------------------------------
//    Assign to the name on the left the value on the right
// ----------------------------------------------------------------------------
{
    if (Infix *infix = dynamic_cast<Infix *> (args))
    {
        Tree *expr = infix->right;
        Tree *value = context->Run(expr);
        if (!value)
            return context->Error("No value for '$1' in assignment", expr);

        Tree *target = infix->left;
        if (Name *name = dynamic_cast<Name *> (target))
            context->EnterName(name->value, value);
        else
            return context->Error("Cannot assign to non-name '$1'", target);

        return value;
    }
    return context->Error ("Invalid assignment '$1'", args);
}



// ============================================================================
// 
//    Definition
// 
// ============================================================================
//  This deals with XL definitions, such as:
//    fact 0 -> 1
//    fact N -> N * fact(N-1)

Tree *Definition::Call(Context *context, Tree *args)
// ----------------------------------------------------------------------------
//    Define the expression on the left to match expression on the right
// ----------------------------------------------------------------------------
{
    if (Infix *infix = dynamic_cast<Infix *> (args))
    {
        Tree *defined = infix->left;
        Tree *definition = infix->right;
        if (!defined || !definition)
            return context->Error("Definition '$1' is incomplete", args);

        context->Parent()->EnterRewrite(defined, definition);

        return nil_name;
    }
    return context->Error ("Invalid assignment '$1'", args);
}



// ============================================================================
// 
//    class Evaluation and Parse
// 
// ============================================================================

Tree *ParseTree::Call(Context *context, Tree *args)
// ----------------------------------------------------------------------------
//    Return the parse tree, not its value
// ----------------------------------------------------------------------------
{
    if (!args)
        return args;
    if (Block *block = dynamic_cast<Block *> (args))
        return block->child;
    return args;
}


Tree *Evaluation::Call(Context *context, Tree *args)
// ----------------------------------------------------------------------------
//    Evaluate the tree given by the argument
// ----------------------------------------------------------------------------
{
    if (!args)
        return args;
    return context->Run(args);
}


Tree *DebugPrint::Call(Context *context, Tree *args)
// ----------------------------------------------------------------------------
//   Print the tree given as argument
// ----------------------------------------------------------------------------
{
    std::cout << "DEBUG: " << args << "\n";
    return args;
}


// ============================================================================
// 
//    Type matching
// 
// ============================================================================

Tree *BooleanType::Call(Context *context, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a boolean value (true/false)
// ----------------------------------------------------------------------------
{
    if (value == true_name || value == false_name)
        return value;
    return NULL;
}


Tree *IntegerType::Call(Context *context, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as an integer
// ----------------------------------------------------------------------------
{
    if (Integer *it = dynamic_cast<Integer *>(context->Eval(value)))
        return it;
    return NULL;
}


Tree *RealType::Call(Context *context, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a real
// ----------------------------------------------------------------------------
{
    if (Real *rt = dynamic_cast<Real *>(context->Eval(value)))
        return rt;
    return NULL;
}


Tree *TextType::Call(Context *context, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a text
// ----------------------------------------------------------------------------
{
    if (Text *tt = dynamic_cast<Text *>(context->Eval(value)))
    {
        Quote q;
        if (tt->Opening() != q.Opening() || tt->Closing() != q.Closing())
            return tt;
    }
    return NULL;
}


Tree *CharacterType::Call(Context *context, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as an integer
// ----------------------------------------------------------------------------
{
    if (Text *tt = dynamic_cast<Text *>(context->Eval(value)))
    {
        Quote q;
        if (tt->Opening() == q.Opening() && tt->Closing() == q.Closing())
            return tt;
    }
    return NULL;
}

XL_END
