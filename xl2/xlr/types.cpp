// ****************************************************************************
//  types.cpp                                                       XLR project
// ****************************************************************************
//
//   File Description:
//
//     The type system in XL
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
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#define DEFINE_TYPE(n)                                  \
    XL::Name n##_type_implementation(#n);               \
    XL::Tree *n##_type = &n##_type_implementation

#include "types.h"
#include "tree.h"

XL_BEGIN

// ============================================================================
//
//    Infer types by scanning source code for type information
//
// ============================================================================

Tree *InferTypes::Do (Tree *what)
// ----------------------------------------------------------------------------
//   Infer the type of some arbitrary tree
// ----------------------------------------------------------------------------
{
    // Otherwise, we don't know how to deal with it
    return Ooops("Cannot infer the type of '$1'", what);
}


Tree *InferTypes::DoInteger(Integer *what)
// ----------------------------------------------------------------------------
//   Return the integer type
// ----------------------------------------------------------------------------
{
    what->Set<TypeInfo> (integer_type);
    return integer_type;
}


Tree *InferTypes::DoReal(Real *what)
// ----------------------------------------------------------------------------
//   Return the real type
// ----------------------------------------------------------------------------
{
    what->Set<TypeInfo> (real_type);
    return real_type;
}


Tree *InferTypes::DoText(Text *what)
// ----------------------------------------------------------------------------
//   Return text or character type
// ----------------------------------------------------------------------------
{
    Tree *type = what->opening == "'" ? character_type : text_type;
    what->Set<TypeInfo> (type);
    return type;
}


Tree *InferTypes::DoName(Name *what)
// ----------------------------------------------------------------------------
//   Return the type of the value of the name
// ----------------------------------------------------------------------------
{
    if (Tree *value = symbols->Named(what->value))
    {
        if (Tree *type = value->Get<TypeInfo>())
            return type;
        return Ooops("Unknown type for '$1'", what);
    }
    return Ooops("Unknown name '$1'", what);
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



// ============================================================================
//
//    Check if a value matches a type
//
// ============================================================================

Tree *MatchType::Do (Tree *what)
// ----------------------------------------------------------------------------
//   If we get there, this is not a type match
// ----------------------------------------------------------------------------
{
    return MatchStructuredType(what);
}


Tree *MatchType::DoInteger(Integer *what)
// ----------------------------------------------------------------------------
//   An integer values matches the same value, the integer or real types
// ----------------------------------------------------------------------------
{
    // If the type is itself an integer, the value must be the same
    if (Integer *it = what->AsInteger())
        return it->value == what->value ? what : NULL;

    // An integer value directly matches with integer type
    Normalize();
    if (type == integer_type)
        return what;

    // If the type we match against is real, add a conversion
    if (type == real_type)
        return new Prefix(new Name("real"), what);

    // Otherwise, check union types or fail
    return MatchStructuredType(what);
}


Tree *MatchType::DoReal(Real *what)
// ----------------------------------------------------------------------------
//   A real value matches the same value or real types
// ----------------------------------------------------------------------------
{
    // If the type is itself a real, the value must be the same
    if (Real *rt = what->AsReal())
        return rt->value == what->value ? what : NULL;

    // A real value matches with the real type
    Normalize();
    if (type == real_type)
        return what;

    // Otherwise, check union types or fail
    return MatchStructuredType(what);
}


Tree *MatchType::DoText(Text *what)
// ----------------------------------------------------------------------------
//   Text values match text or character type
// ----------------------------------------------------------------------------
{
    // If the type is itself a text litteral, we must match value exactly
    if (Text *tt = what->AsText())
        return (tt->value == what->value     &&
                tt->opening == what->opening &&
                tt->closing == what->closing
                ? what
                : NULL);

    // A text value matches either the text or character type
    Tree *textType = (what->opening == "'" && what->closing == "'"
                      ? character_type
                      : text_type);
    Normalize();
    if (type == textType)
        return what;

    // Otherwise, check union types or fail
    return MatchStructuredType(what);
}


Tree *MatchType::DoName(Name *what)
// ----------------------------------------------------------------------------
//   A name matches if the value matches or if we expect a symbol
// ----------------------------------------------------------------------------
{
    Normalize();
    if (type == symbol_type)
        return what;
    if (Tree *value = symbols->Named(what->value))
        return value->Do(this);
    return NULL;
}


Tree *MatchType::DoPrefix(Prefix *what)
// ----------------------------------------------------------------------------
//   Check if we have a similar form, or if this matches rewrites
// ----------------------------------------------------------------------------
{
    // Check if we match a prefix with the same shape
    if (Prefix *prefixType = type->AsPrefix())
    {
        LocalSave<Tree *> saveType = type;

        type = prefixType->left;
        Tree *leftValue = NameMatch(what->left);
        if (leftValue)
        {
            type = prefixType->right;
            Tree *rightValue = what->right->Do(this);
            if (rightValue)
                return what;
        }
    }

    // Check if the given type is 'prefix'
    Normalize();
    if (type == prefix_type)
        return what;

    // Otherwise check rewrites
    return Rewrites(what);
}


Tree *MatchType::DoPostfix(Postfix *what)
// ----------------------------------------------------------------------------
//   Check if we have a similar form, or if this matches rewrites
// ----------------------------------------------------------------------------
{
    // Check if we match a postfix with the same shape
    if (Postfix *postfixType = type->AsPostfix())
    {
        LocalSave<Tree *> saveType = type;

        type = postfixType->right;
        Tree *rightValue = NameMatch(what->right);
        if (rightValue)
        {
            type = postfixType->left;
            Tree *leftValue = what->left->Do(this);
            if (leftValue)
                return what;
        }
    }

    // Check if the given type is 'postfix'
    Normalize();
    if (type == postfix_type)
        return what;

    // Otherwise check rewrites
    return Rewrites(what);
}


Tree *MatchType::DoInfix(Infix *what)
// ----------------------------------------------------------------------------
//   Check if we have a similar form, or if it matches rewrites
// ----------------------------------------------------------------------------
{
    // Check if we match an infix with the same shape
    if (Infix *infixType = type->AsInfix())
    {
        if (infixType->name == what->name)
        {
            LocalSave<Tree *> saveType = type;

            type = infixType->left;
            Tree *leftValue = what->left->Do(this);
            if (leftValue)
            {
                type = infixType->right;
                Tree *rightValue = what->right->Do(this);
                if (rightValue)
                    return what;
            }
        }
    }

    // Check if the given type is 'infix'
    Normalize();
    if (type == infix_type)
        return what;

    // Otherwise check rewrites
    return Rewrites(what);
}


Tree *MatchType::DoBlock(Block *what)
// ----------------------------------------------------------------------------
//   Check if the evaluated value matches, otherwise check rewrites
// ----------------------------------------------------------------------------
{
    // Check if we match a block with the same shape
    if (Block *blockType = type->AsBlock())
    {
        if (blockType->opening == what->opening &&
            blockType->closing == what->closing)
        {
            LocalSave<Tree *> saveType = type;

            type = blockType->child;
            Tree *childValue = what->child->Do(this);
            if (childValue)
                return what;
        }
    }

    // Check if the given type is 'block'
    Normalize();
    if (type == block_type)
        return what;

    // Check if the value is (X), in which case we test X
    if (what->opening == "(" && what->closing == ")")
        if (Tree *childValue = what->child->Do(this))
            return childValue;

    // Otherwise check rewrites
    return Rewrites(what);
}


Tree *MatchType::MatchStructuredType(Tree *what, Tree *kind)
// ----------------------------------------------------------------------------
//   Check structured types like T1|T2 or (T)
// ----------------------------------------------------------------------------
{
    // If this is some union type, i.e. T1 | T2 matches T1 values or T2 values
    if (Infix *infixType = type->AsInfix())
    {
        if (infixType->name == "|")
        {
            LocalSave<Tree *> saveType = type;

            type = infixType->left;
            Tree *leftValue = what->Do(this);
            if (leftValue)
                return leftValue;

            type = infixType->right;
            Tree *rightValue = what->Do(this);
            if (rightValue)
                return rightValue;
        }
    }

    // If this is a block, type (T) matches the same as T
    else if (Block *blockType = type->AsBlock())
    {
        if (blockType->opening == "(" && blockType->closing == ")")
        {
            LocalSave<Tree *> saveType = type;
            type = blockType->child;
            Tree *childValue = what->Do(this);
            if (childValue)
                return childValue;
        }
    }

    // Check if the type is 'tree' or a given structure type
    else if (Name *namedType = type->AsName())
    {
        if (namedType->value == "tree" || namedType == kind)
            return what;
    }

    return NULL;
}


Tree * MatchType::Rewrites(Tree *what)
// ----------------------------------------------------------------------------
//   Build code selecting among rewrites in current context
// ----------------------------------------------------------------------------
{
    return NULL;
}


Tree *MatchType::Normalize()
// ----------------------------------------------------------------------------
//    Find the normalized type for built-ins, e.g. integer->integer_type
// ----------------------------------------------------------------------------
{
    if (Name *named = type->AsName())
    {
        text name = named->value;

        // Look it up in the symbol table (takes precedence over builtins)
        if (Tree *value = symbols->Named(name))
        {
            type = value;
        }
        else
        {
            // Check built-in types
#undef DEFINE_TYPE
#define DEFINE_TYPE(t)                          \
            if (name == #t)                     \
                type = t##_type;
#include "types.h"
        }
    }

    return type;
}


Tree *MatchType::NameMatch(Tree *what)
// ----------------------------------------------------------------------------
//    Check if either we have a matching name, or a regular match
// ----------------------------------------------------------------------------
{
    if (Name *name = what->AsName())
    {
        if (Name *typedName = type->AsName())
        {
            if (typedName->value == name->value)
                return what;
        }
    }

    // Otherwise, check normal rules for types
    return what->Do(this);
}

XL_END

