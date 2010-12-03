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

Tree *ValueMatchesType(Tree *type, Tree *value, bool conversions)
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
        if (conversions)
        {
            if (Integer *iv = value->AsInteger())
                return iv;
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
    if (type == name_symbol_type)
        if (Name *nv = value->AsName())
            if (nv->value.length() && isalpha(nv->value[0]))
                return nv;
    if (type == operator_symbol_type)
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
        return ValueMatchesType(bt->child, value, conversions);
    if (Infix *it = type->AsInfix())
    {
        if (it->name == "|")
        {
            if (Tree *leftOK = ValueMatchesType(it->left, value, conversions))
                return leftOK;
            if (Tree *rightOK = ValueMatchesType(it->right, value, conversions))
                return rightOK;
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


Tree *TypeCoversType(Tree *type, Tree *test, bool conversions)
// ----------------------------------------------------------------------------
//   Check if type 'test' is covered by 'type'
// ----------------------------------------------------------------------------
{
    // Quick exit when types are the same or the tree type is used
    if (type == test)
        return test;
    if (type == tree_type)
        return test;
    if (conversions)
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
            if (TypeCoversType(type, itst->left, conversions) &&
                TypeCoversType(type, itst->right, conversions))
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
        return TypeCoversType(type, btst->child, conversions);

    // General case where the tested type is a value of the type
    if (test->IsConstant())
        if (ValueMatchesType(type, test, conversions))
            return test;

    // Check if we match one of the constructed types
    if (Block *bt = type->AsBlock())
        return TypeCoversType(bt->child, test, conversions);
    if (Infix *it = type->AsInfix())
    {
        if (it->name == "|")
        {
            if (Tree *leftOK = TypeCoversType(it->left, test, conversions))
                return leftOK;
            if (Tree *rightOK = TypeCoversType(it->right, test, conversions))
                return rightOK;
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


Tree *TypeIntersectsType(Tree *type, Tree *test, bool conversions)
// ----------------------------------------------------------------------------
//   Check if type 'test' intersects 'type'
// ----------------------------------------------------------------------------
{
    // Quick exit when types are the same or the tree type is used
    if (type == test)
        return test;
    if (type == tree_type || test == tree_type)
        return test;
    if (conversions)
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
            if (TypeIntersectsType(type, itst->left, conversions) ||
                TypeIntersectsType(type, itst->right, conversions))
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
        return TypeIntersectsType(type, btst->child, conversions);

    // General case where the tested type is a value of the type
    if (test->IsConstant())
        if (ValueMatchesType(type, test, conversions))
            return test;

    // Check if we match one of the constructed types
    if (Block *bt = type->AsBlock())
        return TypeIntersectsType(bt->child, test, conversions);
    if (Infix *it = type->AsInfix())
    {
        if (it->name == "|")
        {
            if (Tree *leftOK = TypeIntersectsType(it->left, test, conversions))
                return leftOK;
            if (Tree *rightOK = TypeIntersectsType(it->right,test,conversions))
                return rightOK;
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


Tree *UnionType(Tree *t1, Tree *t2)
// ----------------------------------------------------------------------------
//    Create the union of two types
// ----------------------------------------------------------------------------
{
    if (t1 == NULL)
        return t2;
    if (t2 == NULL)
        return t1;
    if (TypeCoversType(t1, t2, false))
        return t1;
    if (TypeCoversType(t2, t1, false))
        return t2;
    return new Infix("|", t1, t2);
}


Tree *CanonicalType(Tree *value)
// ----------------------------------------------------------------------------
//   Return the canonical type for the given value
// ----------------------------------------------------------------------------
{
    Tree *type = tree_type;
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


Tree *StructuredType(Tree *value)
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
        if (Symbols *syms = value->Symbols())
            if (Name *name = (Name *) value)
                if (Tree *ref = syms->Named(name->value, true))
                    if (ref != name)
                        type = StructuredType(ref);
        break;

    case INFIX:
        if (Infix *infix = (Infix *) value)
        {
            Tree *lt = StructuredType(infix->left);
            Tree *rt = StructuredType(infix->right);
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
            type = StructuredType(block->child);
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
        LocalSave<Tree_p> saveType(type);

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
        LocalSave<Tree_p> saveType(type);

        type = postfixType->right;
        Tree_p rightValue = NameMatch(what->right);
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
            LocalSave<Tree_p> saveType(type);

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
            LocalSave<Tree_p> saveType(type);

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
            LocalSave<Tree_p> saveType(type);

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
            LocalSave<Tree_p> saveType(type);
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


Tree *MatchType::Rewrites(Tree *what)
// ----------------------------------------------------------------------------
//   Check the various rewrites and see if there is one where types match
// ----------------------------------------------------------------------------
{
    // Compute the hash key for the form we have to match
    ulong formKey, testKey;
    RewriteKey formKeyHash;
    what->Do(formKeyHash);
    formKey = formKeyHash.Key();
    symbols_set visited;
    symbols_list lookups;

    // Build all the symbol tables that we are going to look into
    for (Symbols *s = symbols; s; s = s->Parent())
    {
        if (!visited.count(s))
        {
            lookups.push_back(s);
            visited.insert(s);
            symbols_set::iterator si;
            for (si = s->imported.begin(); si != s->imported.end(); si++)
            {
                if (!visited.count(*si))
                {
                    visited.insert(*si);
                    lookups.push_back(*si);
                }
            }
        }
    }

    // Iterate over all symbol tables listed above
    symbols_list::iterator li;
    for (li = lookups.begin(); li != lookups.end(); li++)
    {
        Symbols *s = *li;

        Rewrite *candidate = s->Rewrites();
        while (candidate)
        {
            // Compute the hash key for the 'from' of the current rewrite
            RewriteKey testKeyHash;
            candidate->from->Do(testKeyHash);
            testKey = testKeyHash.Key();

            // If we have an exact match for the keys, we may have a winner
            if (testKey == formKey)
            {
                // Create the invokation point
                Symbols args(symbols);
                ArgumentTypeMatch matchArgs(what, symbols,
                                            &args, candidate->symbols);
                Tree *argsTest = candidate->from->Do(matchArgs);

                // If we found something, type matched
                if (argsTest)
                    return what;
            } // Match test key

            // Otherwise, follow the hash chain for the next candidate
            if (candidate->hash.count(formKey) > 0)
                candidate = candidate->hash[formKey];
            else
                candidate = NULL;
        } // while(candidate)
    } // for(namespaces)

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
#undef INFIX
#undef PREFIX
#undef POSTFIX
#undef BLOCK
#undef NAME
#undef TYPE
#undef PARM
#undef DS
#undef RS
#undef DOC_RET
#undef DOC_GROUP
#undef DOC_SYNOPSIS
#undef DOC_DESCRIPTION
#undef DOC_MISC

#define DOC_MISC(misc)
#define DOC_RET(type, rdoc)
#define DOC_GROUP(grp)
#define DOC_SYNOPSIS(syno)
#define DOC_DESCRIPTION(desc)
#define INFIX(name, rtype, t1, symbol, t2, code,group, synopsis, desc, retdoc, misc)
#define PARM(symbol, type, pdoc)
#define PREFIX(name, rtype, symbol, parms, code,group, synopsis, desc, retdoc, misc)
#define POSTFIX(name, rtype, parms, symbol, code,group, synopsis, desc, retdoc, misc)
#define BLOCK(name, rtype, open, type, close, code,group, synopsis, desc, retdoc, misc)
#define NAME(symbol)
#define TYPE(symbol)                            \
            if (name == #symbol)                \
                type = symbol##_type;
#include "basics.tbl"
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



// ============================================================================
//
//    Argument matching - Test input arguments against parameters
//
// ============================================================================

Tree *ArgumentTypeMatch::Do(Tree *)
// ----------------------------------------------------------------------------
//   Default is to return failure
// ----------------------------------------------------------------------------
{
    return NULL;
}


Tree *ArgumentTypeMatch::DoInteger(Integer *what)
// ----------------------------------------------------------------------------
//   An integer argument matches the exact value
// ----------------------------------------------------------------------------
{
    // If the tested tree is a constant, it must be an integer with same value
    if (test->IsConstant())
    {
        Integer *it = test->AsInteger();
        if (!it)
            return NULL;
        if (it->value == what->value)
            return what;
    }

    return NULL;
}


Tree *ArgumentTypeMatch::DoReal(Real *what)
// ----------------------------------------------------------------------------
//   A real matches the exact value
// ----------------------------------------------------------------------------
{
    // If the tested tree is a constant, it must be an integer with same value
    if (test->IsConstant())
    {
        Real *rt = test->AsReal();
        if (!rt)
            return NULL;
        if (rt->value == what->value)
            return what;
    }

    return NULL;
}


Tree *ArgumentTypeMatch::DoText(Text *what)
// ----------------------------------------------------------------------------
//   A text matches the exact value
// ----------------------------------------------------------------------------
{
    // If the tested tree is a constant, it must be an integer with same value
    if (test->IsConstant())
    {
        Text *tt = test->AsText();
        if (!tt)
            return NULL;
        if (tt->value == what->value)
            return what;
    }

    return NULL;
}


Tree *ArgumentTypeMatch::DoName(Name *what)
// ----------------------------------------------------------------------------
//    Bind arguments to parameters being defined in the shape
// ----------------------------------------------------------------------------
{
    if (!defined)
    {
        // The first name we see must match exactly, e.g. 'sin' in 'sin X'
        defined = what;
        if (Name *nt = test->AsName())
            if (nt->value == what->value)
                return what;
        return NULL;
    }
    else
    {
        // Check if the name already exists, e.g. 'false' or 'A+A'
        // If it does, we generate a run-time check to verify equality
        if (Tree *existing = rewrite->Named(what->value))
        {
            // Check if the test is an identity
            if (Name *nt = test->AsName())
            {
                if (nt->code == xl_identity)
                {
                    if (nt->value == what->value)
                        return what;
                    if (Name *existingName = existing->AsName())
                        if (existingName->value == nt->value)
                            return what;
                    return NULL;
                }
            }
        }

        return what;
    }
}


Tree *ArgumentTypeMatch::DoBlock(Block *what)
// ----------------------------------------------------------------------------
//   Check if we match a block
// ----------------------------------------------------------------------------
{
    // Test if we exactly match the block, i.e. the reference is a block
    if (Block *bt = test->AsBlock())
    {
        if (bt->opening == what->opening &&
            bt->closing == what->closing)
        {
            test = bt->child;
            Tree *br = what->child->Do(this);
            test = bt;
            if (br)
                return br;
        }
    }

    // Otherwise, if the block is an indent or parenthese, optimize away
    if ((what->opening == "(" && what->closing == ")") ||
        (what->opening == Block::indent && what->closing == Block::unindent))
    {
        return what->child->Do(this);
    }

    return NULL;
}


Tree *ArgumentTypeMatch::DoInfix(Infix *what)
// ----------------------------------------------------------------------------
//   Check if we match an infix operator
// ----------------------------------------------------------------------------
{
    // Check if we match an infix tree like 'x,y' with a name like 'A'
    if (what->name != ":")
    {
        if (Name *name = test->AsName())
        {
            name = name;
        }
    }

    if (Infix *it = test->AsInfix())
    {
        // Check if we match the tree, e.g. A+B vs 2+3
        if (it->name == what->name)
        {
            if (!defined)
                defined = what;
            test = it->left;
            Tree *lr = what->left->Do(this);
            test = it;
            if (!lr)
                return NULL;
            test = it->right;
            Tree *rr = what->right->Do(this);
            test = it;
            if (!rr)
                return NULL;
            return what;
        }
    }

    // Check if we match a type, e.g. 2 vs. 'K : integer'
    if (what->name == ":")
    {
        // Check the variable name, e.g. K in example above
        Name *varName = what->left->AsName();
        if (!varName)
        {
            Ooops("Expected a name, got $1 ", what->left);
            return NULL;
        }

        // Check if the name already exists
        if (Tree *existing = rewrite->Named(varName->value))
        {
            Ooops("Name $1 already exists as $2", what->left, existing);
            return NULL;
        }

        return what;
    }

    // Otherwise, this is a mismatch
    return NULL;
}


Tree *ArgumentTypeMatch::DoPrefix(Prefix *what)
// ----------------------------------------------------------------------------
//   For prefix expressions, simply test left then right
// ----------------------------------------------------------------------------
{
    if (Prefix *pt = test->AsPrefix())
    {
        // Check if we match the tree, e.g. f(A) vs. f(2)
        // Note that we must test left first to define 'f' in above case
        Infix *defined_infix = defined->AsInfix();
        if (defined_infix)
            defined = NULL;

        test = pt->left;
        Tree *lr = what->left->Do(this);
        test = pt;
        if (!lr)
            return NULL;
        test = pt->right;
        Tree *rr = what->right->Do(this);
        test = pt;
        if (!rr)
            return NULL;
        if (!defined && defined_infix)
            defined = defined_infix;
        return what;
    }
    return NULL;
}


Tree *ArgumentTypeMatch::DoPostfix(Postfix *what)
// ----------------------------------------------------------------------------
//    For postfix expressions, simply test right, then left
// ----------------------------------------------------------------------------
{
    if (Postfix *pt = test->AsPostfix())
    {
        // Check if we match the tree, e.g. A! vs 2!
        // Note that ordering is reverse compared to prefix, so that
        // the 'defined' names is set correctly
        test = pt->right;
        Tree *rr = what->right->Do(this);
        test = pt;
        if (!rr)
            return NULL;
        test = pt->left;
        Tree *lr = what->left->Do(this);
        test = pt;
        if (!lr)
            return NULL;
        return what;
    }
    return NULL;
}

XL_END
