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
    do { static t tmp; c->EnterPrefix(n, &tmp); } while(0)

    INFIX("\n", InfixStructureHandler);
    INFIX(";", InfixStructureHandler);
    INFIX(",", InfixStructureHandler);

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
    INFIX("=>", Definition);

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

Tree *InfixStructureHandler::Call(Context *context, Tree *args)
// ----------------------------------------------------------------------------
//   Evaluate args, which is suppsed to be an infix
// ----------------------------------------------------------------------------
{
    if (Infix *infix = dynamic_cast<Infix *> (args))
    {
        tree_list results;
        tree_list::iterator i;
        for (i = infix->list.begin(); i != infix->list.end(); i++)
            if (Tree *item = (*i)->Run(context))
                results.push_back(item);
        switch (results.size())
        {
        case 0: return NULL;
        case 1: return results[0];
        default: return new Infix(infix->name, results, infix->Position());
        }
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
        if (infix->list.size() < 2)
            return context->Error("Expected two arguments in '$1", args);

        tree_list::iterator i    = infix->list.begin();
        Tree *              item = (*i)->Run(context);

        if (!item)
            return context->Error("No value to left of '$1'", args);

        // Check if implementation is unhappy somehow
        try
        {
            // Check type of first argument
            if (Integer *integer = dynamic_cast<Integer *> (item))
            {
                longlong result = integer->value;
                for (i++; i != infix->list.end(); i++)
                    if (Integer*r=dynamic_cast<Integer*> ((*i)->Run(context)))
                        result = DoInteger(result, r->value);
                    else
                        return context->Error("'$1' is not an integer", *i);
                return new Integer(result, args->Position());
            }
            else if (Real *real = dynamic_cast<Real *> (item))
            {
                double result = real->value;
                for (i++; i != infix->list.end(); i++)
                    if (Real*r=dynamic_cast<Real*> ((*i)->Run(context)))
                        result = DoReal(result, r->value);
                    else
                        return context->Error("'$1' is not a real", *i);
                return new Real(result, args->Position());
            }
            else if (Text *txt = dynamic_cast<Text *> (item))
            {
                text result = txt->value;
                for (i++; i != infix->list.end(); i++)
                    if (Text*r=dynamic_cast<Text*> ((*i)->Run(context)))
                        result = DoText(result, r->value);
                    else
                        return context->Error("'$1' is not a text", *i);
                return new Text(result, args->Position());
            }
            return context->Error("Unimplemented operation '$1'", args);
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
        if (infix->list.size() != 2)
            return context->Error("Expected two arguments in '$1", args);

        Tree *left = infix->list[0]->Run(context);
        if (!left)
            return context->Error("No value to left of '$1'", args);
        Tree *right = infix->list[1]->Run(context);

        // Check if implementation is unhappy somehow
        try
        {
            bool result = false;

            // Check type of first argument
            if (Integer *il = dynamic_cast<Integer *> (left))
            {
                if (Integer *ir = dynamic_cast<Integer*> (right))
                    result = DoInteger(il->value, ir->value);
                else
                    return context->Error("'$1' is not an integer", right);
            }
            else if (Real *il = dynamic_cast<Real *> (left))
            {
                if (Real *ir = dynamic_cast<Real*> (right))
                    result = DoReal(il->value, ir->value);
                else
                    return context->Error("'$1' is not a real number", right);
            }
            else if (Text *il = dynamic_cast<Text *> (left))
            {
                if (Text *ir = dynamic_cast<Text*> (right))
                    result = DoText(il->value, ir->value);
                else
                    return context->Error("'$1' is not a text", right);
            }
            else
                return context->Error("Unimplemented operation '$1'", args);

            if (result)
                return true_name;
            return false_name;
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
        if (infix->list.size() < 2)
            return context->Error("Assignment '$1' without arguments", args);

        Tree *assigned = infix->list.back();
        Tree *value = assigned->Run(context);
        if (!value)
            return context->Error("No value for '$1' in assignment", assigned);

        tree_list::iterator begin = infix->list.begin();
        tree_list::iterator end = infix->list.end();

        end--;
        for (tree_list::iterator i = begin; i != end; i++)
            if (Name *name = dynamic_cast<Name *> (*i))
                context->EnterName(name->value, value);
            else
                return context->Error("Cannot assign to non-name '$1'", *i);
        return value;
    }
    return context->Error ("Invalid assignment '$1'", args);
}



// ============================================================================
// 
//    class Definition
// 
// ============================================================================

Tree *Definition::Call(Context *context, Tree *args)
// ----------------------------------------------------------------------------
//    Define the expression on the left to match expression on the right
// ----------------------------------------------------------------------------
{
    if (Infix *infix = dynamic_cast<Infix *> (args))
    {
        if (infix->list.size() < 2)
            return context->Error("Definition '$1' is empty", args);

        Tree *defined = infix->list.back();
        if (!defined)
            return context->Error("No value for '$1' in definition", defined);

        tree_list::iterator begin = infix->list.begin();
        tree_list::iterator end = infix->list.end();

        end--;
        for (tree_list::iterator i = begin; i != end; i++)
            if (Name *name = dynamic_cast<Name *> (*i))
                context->EnterName(name->value, defined);
            else
                return context->Error("Cannot define non-name '$1'", *i);
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
