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

XL_BEGIN

Tree *InferTypes::Do (Tree *what)
// ----------------------------------------------------------------------------
//   For a default tree, return an error for the moment
// ----------------------------------------------------------------------------
{
    return context->Error("Infer type not implemented for '$1'", what);
}


Tree *InferTypes::DoInteger(Integer *what)
// ----------------------------------------------------------------------------
//   Return the integer type
// ----------------------------------------------------------------------------
{
    return integer_type;
}


Tree *InferTypes::DoReal(Real *what)
// ----------------------------------------------------------------------------
//   Return the real type
// ----------------------------------------------------------------------------
{
    return real_type;
}


Tree *InferTypes::DoText(Text *what)
// ----------------------------------------------------------------------------
//   Return text or character type
// ----------------------------------------------------------------------------
{
    static Quote quote;
    if (what->Opening() == quote.Opening())
        return character_type;
    return text_type;
}


Tree *InferTypes::DoName(Name *what)
// ----------------------------------------------------------------------------
//   Return the type of the value of the name
// ----------------------------------------------------------------------------
{
    Tree *value = context->Name(what->value);
    if (value)
        return value->Do(this);
    return context->Error("Unknown type for '$1'", what);
}


Tree *InferTypes::DoPrefix(Prefix *what)
// ----------------------------------------------------------------------------
// 
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

