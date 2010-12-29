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
//    Type allocation and unification algorithms (hacked Damas-Hilney-Milner)
//
// ============================================================================

bool TypeInference::DoInteger(Integer *what)
// ----------------------------------------------------------------------------
//   Annotate an integer tree with its value
// ----------------------------------------------------------------------------
{
    return AssignType(what, what);
}


bool TypeInference::DoReal(Real *what)
// ----------------------------------------------------------------------------
//   Annotate a real tree with its value
// ----------------------------------------------------------------------------
{
    return AssignType(what, what);
}


bool TypeInference::DoText(Text *what)
// ----------------------------------------------------------------------------
//   Annotate a text tree with its own value
// ----------------------------------------------------------------------------
{
    return AssignType(what, what);
}


bool TypeInference::DoName(Name *what)
// ----------------------------------------------------------------------------
//   Assign an unknown type to a name
// ----------------------------------------------------------------------------
{
    return AssignType(what);
}


bool TypeInference::DoPrefix(Prefix *what)
// ----------------------------------------------------------------------------
//   Assign an unknown type to a prefix and then to its children
// ----------------------------------------------------------------------------
{
    return AssignType(what) && what->left->Do(this) && what->right->Do(this);
}


bool TypeInference::DoPostfix(Postfix *what)
// ----------------------------------------------------------------------------
//   Assign an unknown type to a postfix and then to its children
// ----------------------------------------------------------------------------
{
    return AssignType(what) && what->right->Do(this) && what->left->Do(this);
}


bool TypeInference::DoInfix(Infix *what)
// ----------------------------------------------------------------------------
//   Special treatment for the special infix forms
// ----------------------------------------------------------------------------
{
    // Case of 'X : T' : Set type of X to T and unify X:T with X
    if (what->name == ":")
        return (AssignType(what->left, what->right) &&
                AssignType(what) &&
                Unify(what, what->left));

    // Case of 'X -> Y': Analyze type of X and Y, unify them, set type of result
    if (what->name == "->")
        return Rewrite(what);
 
    // For other cases, we assign types to left and right
    if (!(AssignType(what) && what->left->Do(this) && what->right->Do(this)))
        return false;

    // For a sequence, assign types for left and right, then unify
    // the type of the sequence with the type of the last statement
    if (what->name == "\n" || what->name == ";")
        return Unify(what, what->right);

    // Success
    return true;
}


bool TypeInference::DoBlock(Block *what)
// ----------------------------------------------------------------------------
//   Assign an unknown type to a block and then to its children
// ----------------------------------------------------------------------------
{
    return AssignType(what) && what->child->Do(this);
}


bool TypeInference::AssignType(Tree *expr, Tree *type)
// ----------------------------------------------------------------------------
//   Assign a type to a given tree
// ----------------------------------------------------------------------------
{
    // Check if we somehow already enterered that type - That is bad
    tree_map::iterator found = types.find(expr);
    if (found != types.end())
    {
        Ooops("Internal: Tree $1 assigned two types", expr);
        Ooops("   Previous type was $1", (*found).second);
        Ooops("   New type is $1", type);
        return false;
    }

    // Generate a unique type name if nothing is given
    if (type == NULL)
        type = NewTypeName(expr->Position());

    // Record the type for that tree
    types[expr] = type;

    // Success
    return true;
}


bool TypeInference::Rewrite(Infix *what)
// ----------------------------------------------------------------------------
//   Assign a type to a rewrite
// ----------------------------------------------------------------------------
{
    // Recursively assign types to tree elements on the left and right
    if (!(what->left->Do(this) && what->right->Do(this)))
        return false;

    // We need to be able to unify left and right
    if (!Unify(what->left, what->right))
        return false;

    // Create a new function type for the rewrite itself
    Tree *lt = Type(what->left);
    Tree *rt = Type(what->right);
    Infix *fntype = new Infix("->", lt, rt, what->Position());

    // And assign this type to the rewrite itself
    return AssignType(what, fntype);
}



bool TypeInference::Unify(Tree *expr1, Tree *expr2)
// ----------------------------------------------------------------------------
//   Indicates that the two trees must have identical types
// ----------------------------------------------------------------------------
{
    Tree *t1 = Type(expr1);
    Tree *t2 = Type(expr2);

    // If already unified, we are done
    if (t1 == t2)
        return true;

    return UnifyType(t1, t2);
}


bool TypeInference::UnifyType(Tree *t1, Tree *t2)
// ----------------------------------------------------------------------------
//   Unify two type forms
// ----------------------------------------------------------------------------
{
    // Make sure we have the canonical form
    t1 = Base(t1);
    t2 = Base(t2);
    if (t1 == t2)
        return true;            // Already unified

    // Strip out blocks
    if (Block *b1 = t1->AsBlock())
        if (UnifyType(b1->child, t2))
            return JoinTypes(b1, t2);
    if (Block *b2 = t2->AsBlock())
        if (UnifyType(t1, b2->child))
            return JoinTypes(t1, b2);

    // Check if we have similar structured types on both sides
    if (Infix *i1 = t1->AsInfix())
        if (Infix *i2 = t2->AsInfix())
            if (i1->name == i2->name)
                if (UnifyType(i1->left, i2->left) &&
                    UnifyType(i1->right, i2->right))
                    return JoinTypes(i1, i2);
    if (Prefix *p1 = t1->AsPrefix())
        if (Prefix *p2 = t2->AsPrefix())
            if (UnifyType(p1->left, p2->left) &&
                UnifyType(p1->right, p2->right))
                return JoinTypes(p1, p2);
    if (Postfix *p1 = t1->AsPostfix())
        if (Postfix *p2 = t2->AsPostfix())
            if (UnifyType(p1->left, p2->left) &&
                UnifyType(p1->right, p2->right))
                return JoinTypes(p1, p2);

    // Check that we can unify constants with associated built-in type
    if (Integer *i1 = t1->AsInteger())
    {
        if (Integer *i2 = t2->AsInteger())
            return i1->value == i2->value && JoinTypes(i1, i2);
        if (t2 == integer_type)
            return JoinTypes(t2, t1);
    }
    if (Integer *i2 = t2->AsInteger())
        if (t1 == integer_type)
            return JoinTypes(i2, t1);
    if (Real *i1 = t1->AsReal())
    {
        if (Real *i2 = t2->AsReal())
            return i1->value == i2->value && JoinTypes(i1, i2);
        if (t2 == real_type)
            return JoinTypes(t2, t1);
    }
    if (Real *i2 = t2->AsReal())
        if (t1 == real_type)
            return JoinTypes(i2, t1);
    if (Text *i1 = t1->AsText())
    {
        if (Text *i2 = t2->AsText())
            return i1->value == i2->value && JoinTypes(i1, i2);
        if (t2 == text_type)
            return JoinTypes(t2, t1);
    }
    if (Text *i2 = t2->AsText())
        if (t1 == text_type)
            return JoinTypes(i2, t1);

    // If either is a generic name, unify with the other
    if (IsGeneric(t1))
        return JoinTypes(t2, t1, true);
    if (IsGeneric(t2))
        return JoinTypes(t1, t2, true);
    
    // Try to unify with bound value if we find any
    Tree *bound1 = context->Bound(t1);
    if (bound1 && bound1 != t1)
    {
        if (UnifyType(bound1, t2))
            return JoinTypes(bound1, t2) && JoinTypes(t1, t2);
    }
    Tree *bound2 = context->Bound(t2);
    if (bound2 && bound2 != t2)
    {
        if (UnifyType(t1, bound2))
            return JoinTypes(t1, bound2) && JoinTypes(t1, t2);
    }

    // None of the above: fail
    Ooops ("Unable to unify $1", t1);
    Ooops ("with $1", t2);
    return false;
}


Tree *TypeInference::Type(Tree *expr)
// ----------------------------------------------------------------------------
//   Return the base type associated with a given expression
// ----------------------------------------------------------------------------
{
    Tree *type = types[expr];
    if (!type)
    {
        Ooops("Internal: $1 has no type", expr);
        type = integer_type;    // Avoid crashing...
    }
    return Base(type);
}


Tree *TypeInference::Base(Tree *type)
// ----------------------------------------------------------------------------
//   Return the base type for a given type, i.e. after all substitutions
// ----------------------------------------------------------------------------
{
    Tree *chain = type;

    // If we had some unification, find the reference type
    while (Tree *base = type->Get<TypeClass>())
        type = base;

    // Make all elements in chain point to correct type for performance
    while (chain != type)
    {
        TypeClass *typeclass = chain->GetInfo<TypeClass>();
        chain = typeclass->base;
        typeclass->base = type;
    }
    
    return type;
}


bool TypeInference::IsGeneric(Tree *type)
// ----------------------------------------------------------------------------
//   Check if a given type is a generated generic type name
// ----------------------------------------------------------------------------
{
    if (Name *name = type->AsName())
        return name->value.size() && name->value[0] == '#';
    return false;
}


bool TypeInference::IsTypeName(Tree *type)
// ----------------------------------------------------------------------------
//   Check if a given type is a 'true' type name, i.e. not generated
// ----------------------------------------------------------------------------
{
    if (Name *name = type->AsName())
        return name->value.size() && name->value[0] != '#';
    return false;
}


bool TypeInference::JoinTypes(Tree *base, Tree *other, bool knownGood)
// ----------------------------------------------------------------------------
//   Use 'base' as the prototype for the other type
// ----------------------------------------------------------------------------
{
    if (!knownGood)
    {
        // If we have a type name, prefer that to a more complex form
        // in order to keep error messages more readable
        if (IsTypeName(other))
            std::swap(other, base);

        // If what we want to use as a base is a generic and other isn't, swap
        else if (IsGeneric(base))
            std::swap(other, base);
    }

    // Make sure that built-in types always come out on top
    if (other == boolean_type ||
        other == integer_type ||
        other == real_type ||
        other == text_type ||
        other == character_type ||
        other == tree_type ||
        other == source_type ||
        other == code_type ||
        other == lazy_type ||
        other == XL::value_type ||
        other == symbol_type ||
        other == name_type ||
        other == operator_type ||
        other == infix_type ||
        other == prefix_type ||
        other == postfix_type ||
        other == block_type)
        std::swap(other, base);

    if (Tree *already = other->Get<TypeClass>())
    {
        Ooops("Internal: Type $1 already unified to another type", other);
        Ooops("   The previous type was $1", already);
        return false;
    }
    other->Set<TypeClass>(base);
    return true;
}


Name * TypeInference::NewTypeName(TreePosition pos)
// ----------------------------------------------------------------------------
//   Automatically generate new type names
// ----------------------------------------------------------------------------
{
    ulong v = id++;
    text  name = "";
    do
    {
        name = char('A' + v % 26) + name;
        v /= 26;
    } while (v);
    return new Name("#" + name, pos);
}



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
        {
            if (Integer *iv = value->AsInteger())
            {
                Tree *result = new Real(iv->value);
                if (ctx->keepSource)
                    xl_set_source(result, value);
                return result;
            }
        }
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
    if (type == tree_type)
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
    if (type == tree_type)
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
    if (type == tree_type || test == tree_type)
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
    if (type && type != tree_type)
    {
        IFTRACE(types)
            std::cerr << "Caching type " << type << " for " << value << '\n';
        value->Set<TypeInfo>(type);
    }

    return type;
}

XL_END
