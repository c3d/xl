// ****************************************************************************
//  types.cpp                       (C) 1992-2009 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//     The type system in interpreted XL
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

#include "types.h"
#include "tree.h"
#include "opcodes.h"

XL_BEGIN

Tree *InferTypes::Do (Tree *what)
// ----------------------------------------------------------------------------
//   Infer the type of some arbitrary tree
// ----------------------------------------------------------------------------
{
    // Otherwise, we don't know how to deal with it
    return context->Error("Cannot infer the type of '$1'", what);
}


Tree *InferTypes::DoInteger(Integer *what)
// ----------------------------------------------------------------------------
//   Return the integer type
// ----------------------------------------------------------------------------
{
    types[what] = integer_type;
    return integer_type;
}


Tree *InferTypes::DoReal(Real *what)
// ----------------------------------------------------------------------------
//   Return the real type
// ----------------------------------------------------------------------------
{
    types[what] = real_type;
    return real_type;
}


Tree *InferTypes::DoText(Text *what)
// ----------------------------------------------------------------------------
//   Return text or character type
// ----------------------------------------------------------------------------
{
    static Quote quote;
    Tree *t = what->Opening() == quote.Opening() ? character_type : text_type;
    types[what] = t;
    return t;
}


Tree *InferTypes::DoName(Name *what)
// ----------------------------------------------------------------------------
//   Return the type of the value of the name
// ----------------------------------------------------------------------------
{
    if (Tree *value = context->Name(what->value))
    {
        if (Tree *t = types[value])
            return t;
        return context->Error("Unknown type for '$1'", what);
    }
    return context->Error("Unknown name '$1'", what);
}


Tree *InferTypes::DoPrefix(Prefix *what)
// ----------------------------------------------------------------------------
//   Infer all the possible types for a prefix expression
// ----------------------------------------------------------------------------
{
    return what;
}


Tree *InferTypes::DoPostfix(Postfix *what)
// ----------------------------------------------------------------------------
// 
// ----------------------------------------------------------------------------
{
    return what;
}


Tree *InferTypes::DoInfix(Infix *what)
// ----------------------------------------------------------------------------
// 
// ----------------------------------------------------------------------------
{
    return what;
}


Tree *InferTypes::DoBlock(Block *what)
// ----------------------------------------------------------------------------
// 
// ----------------------------------------------------------------------------
{
    return what;
}


Tree *InferTypes::DoNative(Native *what)
// ----------------------------------------------------------------------------
// 
// ----------------------------------------------------------------------------
{
    return what;
}


Name integer_type_implementation("integer");
Name real_type_implementation("real");
Name text_type_implementation("text");
Name character_type_implementation("character");
Name boolean_type_implementation("boolean");
Name nil_type_implementation("nil");


Tree * integer_type = &integer_type_implementation;
Tree * real_type = &real_type_implementation;
Tree * text_type = &text_type_implementation;
Tree * character_type = &character_type_implementation;
Tree * boolean_type = &boolean_type_implementation;
Tree * nil_type = &nil_type_implementation;

XL_END

