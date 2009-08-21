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
    INFIX(",", ListHandler);

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
}



// ============================================================================
// 
//    class InfixStructureHandler
// 
// ============================================================================

Tree *ListHandler::Call(Context *context, Tree *args)
// ----------------------------------------------------------------------------
//   When processing A,B,C, return a similar list fA,fB,fC
// ----------------------------------------------------------------------------
{
    if (Infix *infix = dynamic_cast<Infix *> (args))
    {
        Tree *left = infix->left->Run(context);
        Tree *right = infix->right->Run(context);
        if (left)
            if (right)
                return new Infix(infix->name, left, right, infix->Position());
            else
                return left;
        return right;
    }
    else
    {
        return context->Error("Infix expected, got '$1'", args);
    }
}


Tree *LastInListHandler::Call(Context *context, Tree *args)
// ----------------------------------------------------------------------------
//   When processing blocks or A;B, return last value, e.g. B
// ----------------------------------------------------------------------------
{
    if (Infix *infix = dynamic_cast<Infix *> (args))
    {
        Tree *result = NULL;
        result = infix->left->Run(context);
        result = infix->right->Run(context);
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
        Tree *left = infix->left->Run(context);
        if (!left)
            return context->Error("No value to left of '$1'", args);
        Tree *right = infix->right->Run(context);
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
        Tree *left = infix->left->Run(context);
        if (!left)
            return context->Error("No value to left of '$1'", args);
        Tree *right = infix->right->Run(context);
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
        Tree *value = expr->Run(context);
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
//    fact 0 => 1
//    fact N => N * fact(N-1)

struct CollectVariables : Action
// ----------------------------------------------------------------------------
//   Collect the variables in a defined entity
// ----------------------------------------------------------------------------
{
    CollectVariables(Context *ctx): context(ctx) {}

    Tree *Do (Tree *what) { return what; }
    
    Tree *DoName(Name *what)
    {
        // Check if it already exists, if so return existing: A+A
        if (Tree *other = context->Name(what->value, false))
            return other;

        // Otherwise, enter it in the context
        context->EnterName(what->value, what);

        return what;
    }

    Context *           context;
};


struct CollectDefinition : Action
// ----------------------------------------------------------------------------
//   Collect the definitions in a tree
// ----------------------------------------------------------------------------
{
    CollectDefinition(Context *ctx, Tree *def):
        context(ctx), definition(def) {}

    Tree *Do (Tree *what) { return what; }
    
    // Specialization for the canonical nodes, default is to run them
    Tree *DoName(Name *what)
    {
        context->EnterName(what->value, definition);
        return what;
    }

    Tree *DoPrefix(Prefix *what)
    {
        // For a prefix, e.g. fact N, collect variables (N)
        if (Name *defined = dynamic_cast<Name *> (what->left))
        {
            Context locals(context);
            CollectVariables vars(&locals);
            vars.Do(what->right);
            context->EnterName(defined->value, definition);
        }
        else
        {
            // Not implemented yet
            return context->Error("Unimplemented: defining '$1'", what->left);
        }
        return what;
    }

    Tree *DoPostfix(Postfix *what)
    {
        // For a postfix, e.g. N!, collect variables (N)
        if (Name *defined = dynamic_cast<Name *> (what->right))
        {
            Context locals(context);
            CollectVariables vars(&locals);
            vars.Do(what->left);
            context->EnterName(defined->value, definition);
        }
        else
        {
            // Not implemented yet
            return context->Error("Unimplemented: defining '$1'", what->right);
        }
        return what;
    }

    Tree *DoInfix(Infix *what)
    {
        // For an infix, e.g. A+B, collect variables A and B
        Context locals(context);
        CollectVariables vars(&locals);
        vars.Do(what->left);
        vars.Do(what->right);
        context->EnterInfix(what->name, definition);
        return what;
    }

    Tree *DoBlock(Block *what)
    {
        // For a block, e.g. (A), collect variable A
        return context->Error("Unimplemented: defining block '$1'", what);
    }

    Context *           context;
    Tree *              definition;
};


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

        // Collect variables and store definition
        CollectDefinition define(context, definition);
        defined->Do(define);

        return defined;
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
//    Return the parse tree, not its value
// ----------------------------------------------------------------------------
{
    if (!args)
        return args;
    Tree *toEval = args->Run(context);
    if (!toEval)
        return context->Error("Unable to evaluate '$1'", args);
    return toEval->Run(context);
}

XL_END
