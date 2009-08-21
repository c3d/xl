#ifndef BASICS_H
// ****************************************************************************
//  basics.h                        (C) 1992-2009 Christophe de Dinechin (ddd)
//                                                                 XL2 project
// ****************************************************************************
//
//   File Description:
//
//      Basic operations (arithmetic, etc)
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

#include "tree.h"
#include "opcodes.h"
#include <cmath>

XL_BEGIN

// ============================================================================
//
//   Top level entry point
//
// ============================================================================

// Top-level entry point: enter all basic operations in the scope
void EnterBasics(Context *context);



// ============================================================================
//
//    Special names
//
// ============================================================================

struct ReservedName : Name
// ----------------------------------------------------------------------------
//   Reserved names such as 'true', 'false', 'nil'
// ----------------------------------------------------------------------------
{
    ReservedName(text n) : Name(n) {}
    virtual Tree *      Run(Scope *scope)   { return this; }
};

extern ReservedName *true_name;
extern ReservedName *false_name;
extern ReservedName *nil_name;



// ============================================================================
// 
//    Base types
// 
// ============================================================================

struct TypeExpression : Native
// ----------------------------------------------------------------------------
//   A type used to identify all type expressions
// ----------------------------------------------------------------------------
//   The compiler can use a type expression to verify types
{
    Tree *TypeCheck(Scope *scope, Tree *args) { return NULL; }
};


struct BooleanType : TypeExpression
// ----------------------------------------------------------------------------
//    Check if argument can be interpreted as true or false
// ----------------------------------------------------------------------------
{
    Tree *TypeCheck(Scope *scope, Tree *args);
};


struct IntegerType : TypeExpression
// ----------------------------------------------------------------------------
//    Check if argument can be interpreted as an integer
// ----------------------------------------------------------------------------
{
    Tree *TypeCheck(Scope *scope, Tree *args);
};


struct RealType : TypeExpression
// ----------------------------------------------------------------------------
//    Check if argument can be interpreted as an integer
// ----------------------------------------------------------------------------
{
    Tree *TypeCheck(Scope *scope, Tree *args);
};


struct TextType : TypeExpression
// ----------------------------------------------------------------------------
//    Check if argument can be interpreted as an integer
// ----------------------------------------------------------------------------
{
    Tree *TypeCheck(Scope *scope, Tree *args);
};


struct CharacterType : TypeExpression
// ----------------------------------------------------------------------------
//    Check if argument can be interpreted as an integer
// ----------------------------------------------------------------------------
{
    Tree *TypeCheck(Scope *scope, Tree *args);
};


struct AnyType : TypeExpression
// ----------------------------------------------------------------------------
//    Don't actually check the argument...
// ----------------------------------------------------------------------------
{
    Tree *TypeCheck(Scope *scope, Tree *args);
};


struct InfixType : TypeExpression
// ----------------------------------------------------------------------------
//    Check if the argument is an infix
// ----------------------------------------------------------------------------
{
    Tree *TypeCheck(Scope *scope, Tree *args);
};


struct PrefixType : TypeExpression
// ----------------------------------------------------------------------------
//    Check if the argument is a prefix tree
// ----------------------------------------------------------------------------
{
    Tree *TypeCheck(Scope *scope, Tree *args);
};


struct PostfixType : TypeExpression
// ----------------------------------------------------------------------------
//    Check if the argument is a postfix tree
// ----------------------------------------------------------------------------
{
    Tree *TypeCheck(Scope *scope, Tree *args);
};


struct BlockType : TypeExpression
// ----------------------------------------------------------------------------
//    Check if the argument is a block tree
// ----------------------------------------------------------------------------
{
    Tree *TypeCheck(Scope *scope, Tree *args);
};

XL_END

#endif // BASICS_H
