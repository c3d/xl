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

#include "types.h"
#include "tree.h"
#include "runtime.h"
#include "errors.h"
#include "options.h"
#include <iostream>

XL_BEGIN

// ============================================================================
//
//   High-level type functions
//
// ============================================================================

Tree *ValueMatchesType(Context *ctx, Tree *type, Tree *value, bool convert)
// ----------------------------------------------------------------------------
//   Checks if a value matches a type, return value or NULL if no match
// ----------------------------------------------------------------------------
{
    // Check if we match some of the built-in leaf types
    if (type == integer_type)
        if (Integer *iv = value->AsInteger())
            return iv;
    if (type == real_type)
    {
        if (Real *rv = value->AsReal())
            return rv;
        if (convert)
            if (Integer *iv = value->AsInteger())
                return new Real(iv->value);
    }
    if (type == text_type)
        if (Text *tv = value->AsText())
            if (tv->opening == "\"" && tv->closing == "\"")
                return tv;
    if (type == character_type)
        if (Text *cv = value->AsText())
            if (cv->opening == "'" && cv->closing == "'")
                return cv;
    if (type == boolean_type)
        if (Name *nv = value->AsName())
            if (nv->value == "true" || nv->value == "false")
                return nv;
    if (type == tree_type || type == parse_tree_type)
        return value;
    if (type == symbol_type)
        if (Name *nv = value->AsName())
            return nv;
    if (type == name_type)
        if (Name *nv = value->AsName())
            if (nv->value.length() && isalpha(nv->value[0]))
                return nv;
    if (type == operator_type)
        if (Name *nv = value->AsName())
            if (nv->value.length() && !isalpha(nv->value[0]))
                return nv;
    if (type == infix_type)
        if (Infix *iv = value->AsInfix())
            return iv;
    if (type == prefix_type)
        if (Prefix *pv = value->AsPrefix())
            return pv;
    if (type == postfix_type)
        if (Postfix *pv = value->AsPostfix())
            return pv;
    if (type == block_type)
        if (Block *bv = value->AsBlock())
            return bv;

    // Check if we match constant values
    if (Integer *it = type->AsInteger())
        if (Integer *iv = value->AsInteger())
            if (iv->value == it->value)
                return iv;
    if (Real *rt = type->AsReal())
        if (Real *rv = value->AsReal())
            if (rv->value == rt->value)
                return rv;
    if (Text *tt = type->AsText())
        if (Text *tv = value->AsText())
            if (tv->value == tt->value &&
                tv->opening == tt->opening &&
                tv->closing == tt->closing)
                return tv;
    if (Name *nt = type->AsName())
        if (value == nt)
            return value;
    
    // Check if we match one of the constructed types
    if (Block *bt = type->AsBlock())
        return ValueMatchesType(ctx, bt->child, value, convert);
    if (Infix *it = type->AsInfix())
    {
        if (it->name == "|")
        {
            if (Tree *lfOK = ValueMatchesType(ctx, it->left, value, convert))
                return lfOK;
            if (Tree *rtOK = ValueMatchesType(ctx, it->right, value, convert))
                return rtOK;
        }
        else if (it->name == "->")
        {
            if (Infix *iv = value->AsInfix())
                if (iv->name == "->")
                {
                    // REVISIT: Compare function signatures
                    Ooops("Unimplemented: "
                          "signature comparison of $1 and $2",
                          value, type);
                    return iv;
                }
        }
    }
    if (Prefix *pt = type->AsPrefix())
    {
        if (Name *typeKeyword = pt->left->AsName())
        {
            if (typeKeyword->value == "type")
            {
                if (Block *block = pt->right->AsBlock())
                {
                    if (block->child)
                    {
                        // REVISIT: Match value with pattern
                        Ooops("Unimplemented: "
                              "testing $1 against pattern-based type $2",
                              value, type);
                        return value;
                    }
                }
            }
        }
    }

    // Failed to match type
    return NULL;
}


Tree *TypeCoversType(Context *ctx, Tree *type, Tree *test, bool convert)
// ----------------------------------------------------------------------------
//   Check if type 'test' is covered by 'type'
// ----------------------------------------------------------------------------
{
    // Quick exit when types are the same or the tree type is used
    if (type == test)
        return test;
    if (type == tree_type || type == parse_tree_type)
        return test;
    if (convert)
    {
        // REVISIT: Could we deduce this without knowing xl_real_cast?
        if (type == real_type && test == integer_type)
            return test;
    }

    // Check if test is constructed
    if (Infix *itst = test->AsInfix())
    {
        if (itst->name == "|")
        {
            // Does 'integer' cover 0 | 1 ? Yes if it covers both
            if (TypeCoversType(ctx, type, itst->left, convert) &&
                TypeCoversType(ctx, type, itst->right, convert))
                return test;
        }
        else if (itst->name == "->")
        {
            if (Infix *it = type->AsInfix())
            {
                if (it->name == "->")
                {
                    // REVISIT: Coverage of function types
                    Ooops("Unimplemented: "
                          "Coverage of function $1 by $2",
                          test, type);
                    return test;
                }
            }
        }
    }
    if (Block *btst = test->AsBlock())
        return TypeCoversType(ctx, type, btst->child, convert);

    // General case where the tested type is a value of the type
    if (test->IsConstant())
        if (ValueMatchesType(ctx, type, test, convert))
            return test;
    
    // Check if we match one of the constructed types
    if (Block *bt = type->AsBlock())
        return TypeCoversType(ctx, bt->child, test, convert);
    if (Infix *it = type->AsInfix())
    {
        if (it->name == "|")
        {
            if (Tree *lfOK = TypeCoversType(ctx, it->left, test, convert))
                return lfOK;
            if (Tree *rtOK = TypeCoversType(ctx, it->right, test, convert))
                return rtOK;
        }
        else if (it->name == "->")
        {
            if (Infix *iv = test->AsInfix())
                if (iv->name == "->")
                {
                    // REVISIT: Compare function signatures
                    Ooops("Unimplemented: "
                          "Signature comparison of $1 against $2",
                          test, type);
                    return iv;
                }
        }
    }
    if (Prefix *pt = type->AsPrefix())
    {
        if (Name *typeKeyword = pt->left->AsName())
        {
            if (typeKeyword->value == "type")
            {
                if (Block *block = pt->right->AsBlock())
                {
                    if (block->child)
                    {
                        // REVISIT: Match test with pattern
                        Ooops("Unimplemented: "
                              "Pattern type comparison of $1 against $2",
                              test, type);
                        return test;
                    }
                }
            }
        }
    }

    // Failed to match type
    return NULL;
}


Tree *TypeIntersectsType(Context *ctx, Tree *type, Tree *test, bool convert)
// ----------------------------------------------------------------------------
//   Check if type 'test' intersects 'type'
// ----------------------------------------------------------------------------
{
    // Quick exit when types are the same or the tree type is used
    if (type == test)
        return test;
    if (type == tree_type || type == parse_tree_type || test == tree_type)
        return test;
    if (convert)
    {
        if (type == real_type && test == integer_type)
            return test;
        if (test == real_type && type == integer_type)
            return test;
    }

    // Check if test is constructed
    if (Infix *itst = test->AsInfix())
    {
        if (itst->name == "|")
        {
            // Does 'integer' intersect 0 | 1 ? Yes if it intersects either
            if (TypeIntersectsType(ctx, type, itst->left, convert) ||
                TypeIntersectsType(ctx, type, itst->right, convert))
                return test;
        }
        else if (itst->name == "->")
        {
            if (Infix *it = type->AsInfix())
            {
                if (it->name == "->")
                {
                    // REVISIT: Coverage of function types
                    Ooops("Unimplemented: "
                          "Coverage of function $1 by $2",
                          test, type);
                    return test;
                }
            }
        }
    }
    if (Block *btst = test->AsBlock())
        return TypeIntersectsType(ctx, type, btst->child, convert);

    // General case where the tested type is a value of the type
    if (test->IsConstant())
        if (ValueMatchesType(ctx, type, test, convert))
            return test;
    
    // Check if we match one of the constructed types
    if (Block *bt = type->AsBlock())
        return TypeIntersectsType(ctx, bt->child, test, convert);
    if (Infix *it = type->AsInfix())
    {
        if (it->name == "|")
        {
            if (Tree *lfOK = TypeIntersectsType(ctx, it->left, test, convert))
                return lfOK;
            if (Tree *rtOK = TypeIntersectsType(ctx, it->right,test, convert))
                return rtOK;
        }
        else if (it->name == "->")
        {
            if (Infix *iv = test->AsInfix())
                if (iv->name == "->")
                {
                    // REVISIT: Compare function signatures
                    Ooops("Unimplemented: "
                          "Signature comparison of $1 against $2",
                          test, type);
                    return iv;
                }
        }
    }
    if (Prefix *pt = type->AsPrefix())
    {
        if (Name *typeKeyword = pt->left->AsName())
        {
            if (typeKeyword->value == "type")
            {
                if (Block *block = pt->right->AsBlock())
                {
                    if (block->child)
                    {
                        // REVISIT: Match test with pattern
                        Ooops("Unimplemented: "
                              "Pattern type comparison of $1 against $2",
                              test, type);
                        return test;
                    }
                }
            }
        }
    }

    // Failed to match type
    return NULL;
}


Tree *UnionType(Context *ctx, Tree *t1, Tree *t2)
// ----------------------------------------------------------------------------
//    Create the union of two types
// ----------------------------------------------------------------------------
{
    if (t1 == NULL)
        return t2;
    if (t2 == NULL)
        return t1;
    if (TypeCoversType(ctx, t1, t2, false))
        return t1;
    if (TypeCoversType(ctx, t2, t1, false))
        return t2;
    return new Infix("|", t1, t2);
}


Tree *CanonicalType(Context *ctx, Tree *value)
// ----------------------------------------------------------------------------
//   Return the canonical type for the given value
// ----------------------------------------------------------------------------
{
    Tree *type = tree_type;
    (void) ctx;
    switch (value->Kind())
    {
    case INTEGER:
    case REAL:
    case TEXT:          type = value; break;
    case NAME:          type = symbol_type; break;
    case INFIX:         type = infix_type; break;
    case PREFIX:        type = prefix_type; break;
    case POSTFIX:       type = postfix_type; break;
    case BLOCK:         type = block_type; break;
    }
    return type;    
}


Tree *StructuredType(Context *ctx, Tree *value)
// ----------------------------------------------------------------------------
//   Return the type of a structured value
// ----------------------------------------------------------------------------
{
    // First check if we already figured out the type for this
    Tree *type = value->Get<TypeInfo>();
    if (type)
        return type;

    // If there is no type, we need to be pessimistic
    type = tree_type;

    switch(value->Kind())
    {
    case INTEGER:
    case REAL:
    case TEXT:
        // Constants have themselves as type
        type = value;
        break;

    case NAME:
        // For names, we may be lucky and have a name in the value
        if (Tree *ref = ctx->Bound((Name *) value))
            if (ref != value)
                type = StructuredType(ctx, ref);
        break;

    case INFIX:
        if (Infix *infix = (Infix *) value)
        {
            Tree *lt = StructuredType(ctx, infix->left);
            Tree *rt = StructuredType(ctx, infix->right);
            type = new Infix(infix->name, lt, rt, infix->Position());
        }
        break;

    case PREFIX:
        type = prefix_type;
        break;

    case POSTFIX:
        type = postfix_type;
        break;

    case BLOCK:
        if (Block *block = (Block *) value)
            type = StructuredType(ctx, block->child);
        break;
    }

    // Memorize the type for next time
    if (type && type != tree_type && type != parse_tree_type)
    {
        IFTRACE(types)
            std::cerr << "Caching type " << type << " for " << value << '\n';
        value->Set<TypeInfo>(type);
    }

    return type;    
}

XL_END
