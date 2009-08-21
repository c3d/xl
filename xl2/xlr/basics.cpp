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

Tree *LastInListHandler::Call(Scope *scope, Tree *args)
// ----------------------------------------------------------------------------
//   When processing blocks or A;B, return last value, e.g. B
// ----------------------------------------------------------------------------
{
    if (Infix *infix = dynamic_cast<Infix *> (args))
    {
        Tree *result = NULL;
        result = scope->Run(infix->left);
        result = scope->Run(infix->right);
        return result;
    }
    else
    {
        return scope->Error("Infix expected, got '$1'", args);
    }
}



// ============================================================================
// 
//    class BinaryHandler
// 
// ============================================================================

Tree *BinaryHandler::Call(Scope *scope, Tree *args)
// ----------------------------------------------------------------------------
//   Assume an infix argument, deal with binary ops
// ----------------------------------------------------------------------------
{
    if (Infix *infix = dynamic_cast<Infix *> (args))
    {
        tree_position pos = args->Position();
        Tree *left = scope->Run(infix->left);
        if (!left)
            return scope->Error("No value to left of '$1'", args);
        Tree *right = scope->Run(infix->right);
        if (!right)
            return scope->Error("No value to right of '$1'", args);

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

            return scope->Error("Incompatible types in '$1'", args);
        }
        catch(kstring msg)
        {
            return scope->Error (msg, args);
        }
    }
    return scope->Error("Infix expected, got '$1'", args);
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


Tree *Negate::Call(Scope *scope, Tree *args)
// ----------------------------------------------------------------------------
//   Negate its input argument
// ----------------------------------------------------------------------------
{
    Tree *value = scope->Run(args);
    tree_position pos = args->Position();

    if (Integer *li = dynamic_cast<Integer *> (value))
        return new Integer(-li->value, pos);
    if (Real *ri = dynamic_cast<Real *> (value))
        return new Real(-ri->value, pos);

    return scope->Error("Invalid type for negation in '$1'", args);
}


// ============================================================================
// 
//    class BooleanHandler
// 
// ============================================================================

Tree *BooleanHandler::Call(Scope *scope, Tree *args)
// ----------------------------------------------------------------------------
//   Assume an infix argument, deal with binary boolean ops
// ----------------------------------------------------------------------------
{
    if (Infix *infix = dynamic_cast<Infix *> (args))
    {
        Tree *left = scope->Run(infix->left);
        if (!left)
            return scope->Error("No value to left of '$1'", args);
        Tree *right = scope->Run(infix->right);
        if (!right)
            return scope->Error("No value to right of '$1'", args);

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

            return scope->Error("Incompatible types in '$1'", args);
        }
        catch(kstring msg)
        {
            return scope->Error (msg, args);
        }
    }
    return scope->Error("Infix expected, got '$1'", args);
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

Tree *Assignment::Call(Scope *scope, Tree *args)
// ----------------------------------------------------------------------------
//    Assign to the name on the left the value on the right
// ----------------------------------------------------------------------------
{
    if (Infix *infix = dynamic_cast<Infix *> (args))
    {
        Tree *expr = infix->right;
        Tree *value = scope->LazyRun(expr);
        if (!value)
            return scope->Error("No value for '$1' in assignment", expr);

        Tree *target = infix->left;
        if (Name *name = dynamic_cast<Name *> (target))
            scope->EnterName(name->value, value);
        else
            return scope->Error("Cannot assign to non-name '$1'", target);

        return value;
    }
    return scope->Error ("Invalid assignment '$1'", args);
}



// ============================================================================
// 
//    Definition
// 
// ============================================================================
//  This deals with XL definitions, such as:
//    fact 0 -> 1
//    fact N -> N * fact(N-1)

Tree *Definition::Call(Scope *scope, Tree *args)
// ----------------------------------------------------------------------------
//    Define the expression on the left to match expression on the right
// ----------------------------------------------------------------------------
{
    if (Infix *infix = dynamic_cast<Infix *> (args))
    {
        Tree *defined = infix->left;
        Tree *definition = infix->right;
        if (!defined || !definition)
            return scope->Error("Definition '$1' is incomplete", args);

        scope->EnterRewrite(defined, definition);

        return nil_name;
    }
    return scope->Error ("Invalid assignment '$1'", args);
}



// ============================================================================
// 
//    class Evaluation and Parse
// 
// ============================================================================

Tree *ParseTree::Call(Scope *scope, Tree *args)
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


Tree *Evaluation::Call(Scope *scope, Tree *args)
// ----------------------------------------------------------------------------
//    Evaluate the tree given by the argument
// ----------------------------------------------------------------------------
{
    if (!args)
        return args;
    return scope->Run(args);
}


Tree *DebugPrint::Call(Scope *scope, Tree *args)
// ----------------------------------------------------------------------------
//   Print the tree given as argument
// ----------------------------------------------------------------------------
{
    std::cerr << "DEBUG: " << args << "\n";
    return args;
}


// ============================================================================
// 
//    Type matching
// 
// ============================================================================

Tree *BooleanType::TypeCheck(Scope *scope, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a boolean value (true/false)
// ----------------------------------------------------------------------------
{
    if (value == true_name || value == false_name)
        return value;
    return NULL;
}


Tree *IntegerType::TypeCheck(Scope *scope, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as an integer
// ----------------------------------------------------------------------------
{
    if (Integer *it = dynamic_cast<Integer *>(scope->Run(value)))
        return it;
    return NULL;
}


Tree *RealType::TypeCheck(Scope *scope, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a real
// ----------------------------------------------------------------------------
{
    if (Real *rt = dynamic_cast<Real *>(scope->Run(value)))
        return rt;
    return NULL;
}


Tree *TextType::TypeCheck(Scope *scope, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a text
// ----------------------------------------------------------------------------
{
    if (Text *tt = dynamic_cast<Text *>(scope->Run(value)))
    {
        Quote q;
        if (tt->Opening() != q.Opening() || tt->Closing() != q.Closing())
            return tt;
    }
    return NULL;
}


Tree *CharacterType::TypeCheck(Scope *scope, Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as an integer
// ----------------------------------------------------------------------------
{
    if (Text *tt = dynamic_cast<Text *>(scope->Run(value)))
    {
        Quote q;
        if (tt->Opening() == q.Opening() && tt->Closing() == q.Closing())
            return tt;
    }
    return NULL;
}

XL_END
