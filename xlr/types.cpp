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
// This document is released under the GNU General Public License, with the
// following clarification and exception.
//
// Linking this library statically or dynamically with other modules is making
// a combined work based on this library. Thus, the terms and conditions of the
// GNU General Public License cover the whole combination.
//
// As a special exception, the copyright holders of this library give you
// permission to link this library with independent modules to produce an
// executable, regardless of the license terms of these independent modules,
// and to copy and distribute the resulting executable under terms of your
// choice, provided that you also meet, for each linked independent module,
// the terms and conditions of the license of that module. An independent
// module is a module which is not derived from or based on this library.
// If you modify this library, you may extend this exception to your version
// of the library, but you are not obliged to do so. If you do not wish to
// do so, delete this exception statement from your version.
//
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#include "types.h"
#include "tree.h"
#include "runtime.h"
#include "errors.h"
#include "options.h"
#include "save.h"
#include "args.h"
#include "cdecls.h"
#include "renderer.h"
#include "basics.h"

#include <iostream>

XL_BEGIN

// ============================================================================
//
//    Type allocation and unification algorithms (hacked Damas-Hilney-Milner)
//
// ============================================================================

ulong Types::id = 0;

Types::Types(Context *context)
// ----------------------------------------------------------------------------
//   Constructor for top-level type inferences
// ----------------------------------------------------------------------------
    : context(context),
      types(),
      unifications(),
      rcalls(),
      left(NULL), right(NULL),
      prototyping(false), matching(false)
{}


Types::Types(Context *context, Types *parent)
// ----------------------------------------------------------------------------
//   Constructor for "child" type inferences, i.e. done within a parent
// ----------------------------------------------------------------------------
    : context(context),
      types(parent->types),
      unifications(parent->unifications),
      rcalls(parent->rcalls),
      left(parent->left), right(parent->right),
      prototyping(false), matching(false)
{}


Types::~Types()
// ----------------------------------------------------------------------------
//    Destructor - Nothing to explicitly delete, but useful for debugging
// ----------------------------------------------------------------------------
{}


bool Types::TypeCheck(Tree *program)
// ----------------------------------------------------------------------------
//   Perform all the steps of type inference on the given program
// ----------------------------------------------------------------------------
{
    // Once this is done, record all type information for the program
    bool result = program->Do(this);

    // Dump debug information if approriate
    IFTRACE(typecheck)
    {
        std::cout << "TYPE CHECK FOR " << ShortTreeForm(program) << "\n";
        std::cout << "TYPES:\n"; debugt(this);
        std::cout << "UNIFICATIONS:\n"; debugu(this);
    }
    IFTRACE(types)
    {
        std::cout << "CALLS FOR " << ShortTreeForm(program) << ":\n";
        debugr(this);
    }

    return result;
}


Tree *Types::Type(Tree *expr)
// ----------------------------------------------------------------------------
//   Return the base type associated with a given expression
// ----------------------------------------------------------------------------
{
    Tree *type = types[expr];
    if (!type)
    {
        if (expr->Kind() == NAME)
        {
            if (expr == xl_true || expr == xl_false)
                AssignType(expr, boolean_type);
            else
                AssignType(expr);
        }
        else if (!expr->Do(this))
        {
            Ooops("Unable to assign type to $1", expr);
            if (!types[expr])
                AssignType(expr);
        }
        type = types[expr];
    }
    return Base(type);
}



bool Types::DoInteger(Integer *what)
// ----------------------------------------------------------------------------
//   Annotate an integer tree with its value
// ----------------------------------------------------------------------------
{
    return DoConstant(what);
}


bool Types::DoReal(Real *what)
// ----------------------------------------------------------------------------
//   Annotate a real tree with its value
// ----------------------------------------------------------------------------
{
    return DoConstant(what);
}


bool Types::DoText(Text *what)
// ----------------------------------------------------------------------------
//   Annotate a text tree with its own value
// ----------------------------------------------------------------------------
{
    return DoConstant(what);
}


bool Types::DoConstant(Tree *what)
// ----------------------------------------------------------------------------
//   All constants have themselves as type, and evaluate normally
// ----------------------------------------------------------------------------
{
    Tree *canon = CanonicalType(what);
    bool result = AssignType(what, canon);
    result = Evaluate(what);
    return result;
}


bool Types::DoName(Name *what)
// ----------------------------------------------------------------------------
//   Assign an unknown type to a name
// ----------------------------------------------------------------------------
{
    if (!AssignType(what))
        return false;
    return Evaluate(what);
}


bool Types::DoPrefix(Prefix *what)
// ----------------------------------------------------------------------------
//   Assign an unknown type to a prefix and then to its children
// ----------------------------------------------------------------------------
{
    if (!AssignType(what))
        return false;

    // Skip bizarre declarations
    if (Name *name = what->left->AsName())
    {
        if (name->value == "data")
            return AssignType(what, declaration_type) && Data(what->right);
        else if (name->value == "extern")
            return AssignType(what, declaration_type) && Extern(what->right);
    }

    // What really matters is if we can evaluate the top-level expression
    return Evaluate(what);
}


bool Types::DoPostfix(Postfix *what)
// ----------------------------------------------------------------------------
//   Assign an unknown type to a postfix and then to its children
// ----------------------------------------------------------------------------
{
    if (!AssignType(what))
        return false;

    // What really matters is if we can evaluate the top-level expression
    return Evaluate(what);
}


bool Types::DoInfix(Infix *what)
// ----------------------------------------------------------------------------
//   Special treatment for the special infix forms
// ----------------------------------------------------------------------------
{
    // For a sequence, both sub-expressions must succeed individually.
    // The type of the sequence is the type of the last statement
    if (what->name == "\n" || what->name == ";")
    {
        // Assign types to left and right
        if (!AssignType(what))
            return false;
        return Statements(what, what->left, what->right);
    }

    // Case of 'X : T' : Set type of X to T and unify X:T with X
    if (what->name == ":" || what->name == "as")
        return (AssignType(what->left, what->right) &&
                what->left->Do(this) &&
                AssignType(what) &&
                UnifyExpressionTypes(what, what->left));

    // Case of 'X -> Y': Analyze type of X and Y, unify them, set type of result
    if (what->name == "->")
        return Rewrite(what);

    // For other cases, we assign types to left and right
    if (!AssignType(what))
        return false;

    // Success depends on successful evaluation of the complete form
    return Evaluate(what);
}


bool Types::DoBlock(Block *what)
// ----------------------------------------------------------------------------
//   A block has the same type as its children, except if child alone fails
// ----------------------------------------------------------------------------
{
    // Assign a type to the block
    if (!AssignType(what))
        return false;

    // If child succeeds, the block and its child have the same type
    if (what->child->Do(this))
        return UnifyExpressionTypes(what, what->child);

    // Otherwise, try to find a matching form
    return Evaluate(what);
}


bool Types::AssignType(Tree *expr, Tree *type)
// ----------------------------------------------------------------------------
//   Assign a type to a given tree
// ----------------------------------------------------------------------------
{
    // Check if we already have a type
    if (Tree *existing = types[expr])
    {
        // If no type given, that's it
        if (!type || existing == type)
            return true;

        // We have two types specified for that entity, need to unify
        return Unify(existing, type, expr, expr);
    }

    // Generate a unique type name if nothing is given
    if (type == NULL)
    {
        if (expr == xl_true || expr == xl_false)
            type = boolean_type;
        else
            type = NewTypeName(expr->Position());
    }

    // Record the type for that tree
    types[expr] = type;

    // Success
    return true;
}


bool Types::Rewrite(Infix *what)
// ----------------------------------------------------------------------------
//   Assign a type to a rewrite
// ----------------------------------------------------------------------------
{
    // Create a context for the rewrite parameters
    Context *childContext = new Context(context);
    Save<Context_p> saveContext(context, childContext);

    // Assign types on the left of the rewrite
    Save<bool> proto(prototyping, true);
    if (!what->left->Do(this))
    {
        Ooops("Malformed rewrite pattern $1", what->left);
        return false;
    }

    // The rewrite itself is an infix (in case we have to manage it)
    Tree *formType = Type(what->left);
    Tree *valueType = Type(what->right);
    if (!AssignType(what, declaration_type))
        return false;

    // We need to be able to unify pattern and definition types
    if (!Unify(valueType, formType, what->right, what->left))
        return false;

    // The type of the definition is a pattern type, perform unification
    if (Infix *infix = what->left->AsInfix())
    {
        if (infix->name == ":" || what->name == "as")
        {
            // Explicit type declaration
            if (!Unify(valueType, infix->right, what->right, infix->right))
                return false;
        }
    }

    // Well done, success!
    return true;
}


bool Types::Data(Tree *what)
// ----------------------------------------------------------------------------
//   Use the structure type associated to the data form
// ----------------------------------------------------------------------------
{
    return AssignType(what, CanonicalType(what));
}


bool Types::Extern(Tree *what)
// ----------------------------------------------------------------------------
//   Recover the transformed rewrite and enter that
// ----------------------------------------------------------------------------
{
    CDeclaration *cdecl = what->GetInfo<CDeclaration>();
    if (!cdecl)
        return false;
    return Rewrite(cdecl->rewrite);
}


bool Types::Statements(Tree *expr, Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//   Return the type of a combo statement, skipping declarations
// ----------------------------------------------------------------------------
{
    if (!left->Do(this))
        return false;
    if (!right->Do(this))
        return false;

    // Check if right term is a declaration, otherwise return that
    Tree *t2 = Type(right);
    if (t2 != declaration_type)
        return t2;
    return Type(left);
}


static Tree *lookupRewriteCalls(Scope *evalScope, Scope *sc,
                                Tree *what, Infix *entry, void *i)
// ----------------------------------------------------------------------------
//   Used to check if RewriteCalls pass
// ----------------------------------------------------------------------------
{
    RewriteCalls *rc = (RewriteCalls *) i;
    return rc->Check(sc, what, entry);
}


bool Types::Evaluate(Tree *what)
// ----------------------------------------------------------------------------
//   Find candidates for the given expression and infer types from that
// ----------------------------------------------------------------------------
{
    // We don't evaluate expressions while prototyping a pattern
    if (prototyping)
        return true;

    // Record if we are matching patterns
    bool matchingPattern = matching;
    matching = false;

    // Look directly inside blocks
    while (Block *block = what->AsBlock())
        what = block->child;

    // Test if we are already trying to evaluate this particular form
    rcall_map::iterator found = rcalls.find(what);
    bool recursive = found != rcalls.end();
    if (recursive)
        return true;

    // Identify all candidate rewrites in the current context
    RewriteCalls_p rc = new RewriteCalls(this);
    rcalls[what] = rc;
    uint count = 0;
    Errors errors;
    errors.Log (Error("Unable to evaluate '$1':", what), true);
    context->Lookup(what, lookupRewriteCalls, rc);
        
    // If we have no candidate, this is a failure
    count = rc->candidates.size();
    if (count == 0)
    {
        if (what->IsConstant())
        {
            Tree *wtype = Type(what);
            return Unify(wtype, what, what, what);
        }

        if (matchingPattern && what->Kind() > KIND_LEAF_LAST)
        {
            Tree *wtype = Type(what);
            return Unify(wtype, what, what, what);
        }
        Ooops("No form matches $1", what);
        return false;
    }
    errors.Clear();
    errors.Log(Error("Unable to check types in $1 because", what), true);

    // The resulting type is the union of all candidates
    Tree *type = Base(rc->candidates[0].type);
    Tree *wtype = Type(what);
    for (uint i = 1; i < count; i++)
    {
        Tree *ctype = rc->candidates[i].type;
        ctype = Base(ctype);
        if (IsGeneric(ctype) && IsGeneric(wtype))
        {
            // foo:#A rewritten as bar:#B and another type
            // Join types instead of performing a union
            if (!Join(ctype, type))
                return false;
            if (!Join(wtype, type))
                return false;
            continue;
        }
        type = UnionType(context, type, ctype);
    }

    // Perform type unification
    return Unify(type, wtype, what, what, DECLARATION);
}


bool Types::UnifyExpressionTypes(Tree *expr1, Tree *expr2)
// ----------------------------------------------------------------------------
//   Indicates that the two trees must have identical types
// ----------------------------------------------------------------------------
{
    Tree *t1 = Type(expr1);
    Tree *t2 = Type(expr2);

    // If already unified, we are done
    if (t1 == t2)
        return true;

    return Unify(t1, t2, expr1, expr2);
}


bool Types::Unify(Tree *t1, Tree *t2,
                  Tree *x1, Tree *x2,
                  unify_mode mode)
// ----------------------------------------------------------------------------
//   Unification with expressions
// ----------------------------------------------------------------------------
{
    Save<Tree_p> saveLeft(left, x1);
    Save<Tree_p> saveRight(right, x2);
    return Unify(t1, t2, mode);
}


bool Types::Unify(Tree *t1, Tree *t2, unify_mode mode)
// ----------------------------------------------------------------------------
//   Unify two type forms
// ----------------------------------------------------------------------------
//  A type form in XL can be:
//   - A type name              integer
//   - A generic type name      #ABC
//   - A litteral value         0       1.5             "Hello"
//   - A block for precedence   (real)
//   - The type of a pattern    type (X:integer, Y:integer)
//
// Unification happens almost as "usual" for Algorithm W, except for how
// we deal with XL "shape-based" type constructors, e.g. type(P)
{
    // Make sure we have the canonical form
    t1 = Base(t1);
    t2 = Base(t2);
    if (t1 == t2)
        return true;            // Already unified

    // Strip out blocks in type specification
    if (Block *b1 = t1->AsBlock())
        if (Unify(b1->child, t2))
            return Join(b1, t2);
    if (Block *b2 = t2->AsBlock())
        if (Unify(t1, b2->child))
            return Join(t1, b2);

    // Lookup type names, replace them with their value
    t1 = LookupTypeName(t1);
    t2 = LookupTypeName(t2);
    if (t1 == t2)
        return true;            // This may have been enough for unifiation

    // If either is a generic, unify with the other
    if (IsGeneric(t1))
        return Join(t1, t2);
    if (IsGeneric(t2))
        return Join(t1, t2);

    // In declaration mode, we have success if t2 covers t1
    if (mode == DECLARATION && TypeCoversType(context, t2, t1, false))
        return true;

    // If we have a type name at this stage, this is a failure
    if (IsTypeName(t1))
    {
        if (JoinConstant((Name *) t1, t2))
            return true;

        return TypeError(t1, t2);
    }
    if (IsTypeName(t2))
    {
        if (JoinConstant((Name *) t2, t1))
            return true;
        return TypeError(t1, t2);
    }

    // Check prefix constructor types
    if (Tree *pat1 = TypePattern(t1))
    {
        // If we have two type patterns, they must be structurally identical
        if (Tree *pat2 = TypePattern(t2))
        {
            if (UnifyPatterns(pat1, pat2))
                return Join(t1, t2);
            return TypeError(t1, t2);
        }

        // Match a type pattern with another value
        return UnifyPatternAndValue(pat1, t2);
    }
    if (Tree *pat2 = TypePattern(t2))
        return UnifyPatternAndValue(pat2, t1);

    // None of the above: fail
    return TypeError(t1, t2);
}


Tree *Types::Base(Tree *type)
// ----------------------------------------------------------------------------
//   Return the base type for a given type, i.e. after all substitutions
// ----------------------------------------------------------------------------
{
    Tree *chain = type;

    // If we had some unification, find the reference type
    tree_map::iterator found = unifications.find(type);
    while (found != unifications.end())
    {
        type = (*found).second;
        found = unifications.find(type);
        assert(type != chain || !"Circularity in unification chain");
    }

    // Make all elements in chain point to correct type for performance
    while (chain != type)
    {
        Tree_p &u = unifications[chain];
        chain = u;
        u = type;
    }

    return type;
}


Tree *Types::TypePattern(Tree *type)
// ----------------------------------------------------------------------------
//   Check if type is a type pattern, i.e. type ( ... )
// ----------------------------------------------------------------------------
{
    if (Prefix *pfx = type->AsPrefix())
        if (Name *tname = pfx->left->AsName())
            if (tname->value == "type")
                return pfx->right;

    return NULL;
}


bool Types::Join(Tree *base, Tree *other, bool knownGood)
// ----------------------------------------------------------------------------
//   Use 'base' as the prototype for the other type
// ----------------------------------------------------------------------------
{
    if (!knownGood)
    {
        // If we have a type name, prefer that to a more complex form
        // in order to keep error messages more readable
        if (IsTypeName(other) && !IsTypeName(base))
            std::swap(other, base);

        // If what we want to use as a base is a generic and other isn't, swap
        // (otherwise we could later unify through that variable)
        else if (IsGeneric(base))
            std::swap(other, base);
    }

    // Connext the base type classes
    base = Base(base);
    other = Base(other);
    if (other != base)
        unifications[other] = base;
    return true;
}


bool Types::JoinConstant(Name *type, Tree *cst)
// ----------------------------------------------------------------------------
//    Join a constant with a type name
// ----------------------------------------------------------------------------
{
    // Check if we match against some sized type, otherwise force type
    switch (cst->Kind())
    {
    case INTEGER:
        if (type == integer_type   || type == unsigned_type   ||
            type == integer8_type  || type == unsigned8_type  ||
            type == integer16_type || type == unsigned16_type ||
            type == integer32_type || type == unsigned32_type ||
            type == integer64_type || type == unsigned64_type)
            return Join(type, cst, true);
        return Unify(integer_type, type) && Join(cst, integer_type);

    case REAL:
        if (type == real_type   ||
            type == real64_type || 
            type == real32_type)
            return Join(type, cst, true);
        return Unify(real_type, type) && Join(cst, real_type);

    case TEXT:
    {
        Text *text = (Text *) cst;
        if (text->IsCharacter())
        {
            if (type == character_type)
                return Join(type, cst, true);
            return Join(type, character_type) && Join(cst, character_type);
        }

        if (type == text_type)
            return Join(type, cst, true);
        return Unify(text_type, type) && Join(cst, text_type);
    }
    default:
    {
        Tree *canon = CanonicalType(cst);
        return type == canon;
    }
    }
    return false;
}


bool Types::UnifyPatterns(Tree *t1, Tree *t2)
// ----------------------------------------------------------------------------
//   Check if two patterns describe the same tree shape
// ----------------------------------------------------------------------------
{
    if (t1 == t2)
        return true;

    switch(t1->Kind())
    {
    case INTEGER:
        if (Integer *x1 = t1->AsInteger())
            if (Integer *x2 = t2->AsInteger())
                return x1->value == x2->value;
        return false;

    case REAL:
        if (Real *x1 = t1->AsReal())
            if (Real *x2 = t2->AsReal())
                return x1->value == x2->value;
        return false;

    case TEXT:
        if (Text *x1 = t1->AsText())
            if (Text *x2 = t2->AsText())
                return x1->value == x2->value;
        return false;

    case NAME:
        // We don't attempt to allow renames. Names must match, it's simpler.
        if (Name *x1 = t1->AsName())
            if (Name *x2 = t2->AsName())
                return x1->value == x2->value;
        return false;

    case INFIX:
        if (Infix *x1 = t1->AsInfix())
            if (Infix *x2 = t2->AsInfix())
                return
                    x1->name == x2->name &&
                    UnifyPatterns(x1->left, x2->left) &&
                    UnifyPatterns(x1->right, x2->right);

        return false;

    case PREFIX:
        if (Prefix *x1 = t1->AsPrefix())
            if (Prefix *x2 = t2->AsPrefix())
                return
                    UnifyPatterns(x1->left, x2->left) &&
                    UnifyPatterns(x1->right, x2->right);

        return false;

    case POSTFIX:
        if (Postfix *x1 = t1->AsPostfix())
            if (Postfix *x2 = t2->AsPostfix())
                return
                    UnifyPatterns(x1->left, x2->left) &&
                    UnifyPatterns(x1->right, x2->right);

        return false;

    case BLOCK:
        if (Block *x1 = t1->AsBlock())
            if (Block *x2 = t2->AsBlock())
                return
                    x1->opening == x2->opening &&
                    x1->closing == x2->closing &&
                    UnifyPatterns(x1->child, x2->child);

        return false;

    }

    return false;
}


bool Types::UnifyPatternAndValue(Tree *pat, Tree *val)
// ----------------------------------------------------------------------------
//   Check if two patterns describe the same tree shape
// ----------------------------------------------------------------------------
{
    switch(pat->Kind())
    {
    case INTEGER:
        if (Integer *x1 = pat->AsInteger())
            if (Integer *x2 = val->AsInteger())
                return x1->value == x2->value;
        return false;

    case REAL:
        if (Real *x1 = pat->AsReal())
            if (Real *x2 = val->AsReal())
                return x1->value == x2->value;
        return false;

    case TEXT:
        if (Text *x1 = pat->AsText())
            if (Text *x2 = val->AsText())
                return x1->value == x2->value;
        return false;

    case NAME:
        // A name at that stage is a variable, so we match
        // PROBLEM: matching X+X will match twice?
        return UnifyExpressionTypes(pat, val);

    case INFIX:
        if (Infix *x1 = pat->AsInfix())
        {
            // Check if the pattern is a type declaration
            if (x1->name == ":")
                return Unify(x1->right, val);

            if (Infix *x2 = val->AsInfix())
                return
                    x1->name == x2->name &&
                    UnifyPatternAndValue(x1->left, x2->left) &&
                    UnifyPatternAndValue(x1->right, x2->right);
        }

        return false;

    case PREFIX:
        if (Prefix *x1 = pat->AsPrefix())
            if (Prefix *x2 = val->AsPrefix())
                return
                    UnifyPatterns(x1->left, x2->left) &&
                    UnifyPatternAndValue(x1->right, x2->right);

        return false;

    case POSTFIX:
        if (Postfix *x1 = pat->AsPostfix())
            if (Postfix *x2 = val->AsPostfix())
                return
                    UnifyPatternAndValue(x1->left, x2->left) &&
                    UnifyPatterns(x1->right, x2->right);

        return false;

    case BLOCK:
        if (Block *x1 = pat->AsBlock())
            if (Block *x2 = val->AsBlock())
                return
                    x1->opening == x2->opening &&
                    x1->closing == x2->closing &&
                    UnifyPatternAndValue(x1->child, x2->child);

        return false;

    }

    return false;
}


bool Types::Commit(Types *child)
// ----------------------------------------------------------------------------
//   Commit all the inferences from 'child' into the current
// ----------------------------------------------------------------------------
{
    rcall_map &rmap = rcalls;
    for (rcall_map::iterator r = rmap.begin(); r != rmap.end(); r++)
    {
        Tree *expr = (*r).first;
        Tree *type = child->Type(expr);
        if (!AssignType(expr, type))
            return false;
    }

    return true;
}


Name * Types::NewTypeName(TreePosition pos)
// ----------------------------------------------------------------------------
//   Automatically generate new type names
// ----------------------------------------------------------------------------
{
    ulong v = id++;
    text  name;
    do
    {
        name = char('A' + v % 26) + name;
        v /= 26;
    } while (v);
    return new Name("#" + name, pos);
}


Tree *Types::LookupTypeName(Tree *type)
// ----------------------------------------------------------------------------
//   If we have a type name, lookup its definition
// ----------------------------------------------------------------------------
{
    if (Name *name = type->AsName())
    {
        // Don't lookup type variables (generic names such as #A)
        if (IsGeneric(name->value))
            return name;

        // Check if we have a type definition. If so, use it
        Tree *definition = context->Bound(name);
        if (definition && definition != name)
        {
            Join(definition, name);
            return Base(definition);
        }
    }

    // Otherwise, simply return input type
    return type;
}


bool Types::TypeError(Tree *t1, Tree *t2)
// ----------------------------------------------------------------------------
//   Show type matching errors
// ----------------------------------------------------------------------------
{
    assert(left && right);

    if (left == right)
    {
        Ooops("Type of $1 cannot be both $2 and $3", left, t1, t2);
    }
    else
    {
        Ooops("Cannot unify type $2 of $1", left, t1);
        Ooops("with type $2 of $1", right, t2);
    }
    return false;
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
                return result;
            }
        }
    }
    if (type == text_type)
        if (Text *tv = value->AsText())
            if (tv->IsText())
                return tv;
    if (type == character_type)
        if (Text *cv = value->AsText())
            if (cv->IsCharacter())
                return cv;
    if (type == boolean_type)
        if (Name *nv = value->AsName())
            if (nv->IsBoolean())
                return nv;
    if (IsTreeType(type))
        return value;
    if (type == symbol_type)
        if (Name *nv = value->AsName())
            return nv;
    if (type == name_type)
        if (Name *nv = value->AsName())
            if (nv->IsName())
                return nv;
    if (type == operator_type)
        if (Name *nv = value->AsName())
            if (nv->IsOperator())
                return nv;
    if (type == declaration_type)
        if (Infix *iv = value->AsInfix())
            if (iv->IsDeclaration())
                return iv;
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
    if (IsTreeType(type))
        return test;

    // Numerical conversion
    if (convert)
    {
        if (type == real_type && test == integer_type)
            return test;
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
    if (IsTreeType(type) || IsTreeType(test))
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
    return tree_type;
}


Tree *CanonicalType(Text *value)
// ----------------------------------------------------------------------------
//    Return the canonical type for a text value
// ----------------------------------------------------------------------------
{
    if (value->IsCharacter())
        return character_type;
    return text_type;
}


Tree *CanonicalType(Name *value)
// ----------------------------------------------------------------------------
//    Return the canonical type for a name value
// ----------------------------------------------------------------------------
{
    if (value->IsBoolean())
        return boolean_type;
    if (value->IsOperator())
        return operator_type;
    if (value->IsName())
        return name_type;
    return symbol_type;         // Only occurs for empty names today
}


Tree *CanonicalType(Infix *value)
// ----------------------------------------------------------------------------
//   Return a canonical type for an infix value
// ----------------------------------------------------------------------------
{
    if (value->IsDeclaration())
        return declaration_type;
    return infix_type;
}


Tree *CanonicalType(Tree *value)
// ----------------------------------------------------------------------------
//   Return the canonical type for the given value
// ----------------------------------------------------------------------------
{
    Tree *type = tree_type;
    switch (value->Kind())
    {
    case INTEGER:       type = integer_type;                    break;
    case REAL:          type = real_type;                       break;
    case TEXT:          type = CanonicalType((Text *) value);   break;
    case NAME:          type = CanonicalType((Name *) value);   break;
    case INFIX:         type = CanonicalType((Infix *) value);  break;
    case PREFIX:        type = prefix_type;                     break;
    case POSTFIX:       type = postfix_type;                    break;
    case BLOCK:         type = block_type;                      break;
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
    if (type && !IsTreeType(type))
    {
        IFTRACE(types)
            std::cerr << "Caching type " << type << " for " << value << '\n';
        value->Set<TypeInfo>(type);
    }

    return type;
}

XL_END


void debugt(XL::Types *ti)
// ----------------------------------------------------------------------------
//   Dump a type inference
// ----------------------------------------------------------------------------
{
    if (!XL::Allocator<XL::Types>::IsAllocated(ti))
    {
        std::cout << "Cowardly refusing to show bad Types pointer "
                  << (void *) ti << "\n";
        return;
    }

    using namespace XL;
    uint i = 0;

    tree_map &map = ti->types;
    for (tree_map::iterator t = map.begin(); t != map.end(); t++)
    {
        Tree *value = (*t).first;
        Tree *type = (*t).second;
        Tree *base = ti->Base(type);
        std::cout << "#" << ++i
                  << "\t" << ShortTreeForm(value)
                  << "\t: " << type;
        if (base != type)
            std::cout << "\t= " << base;
        std::cout << "\n";
    }
}


void debugu(XL::Types *ti)
// ----------------------------------------------------------------------------
//   Dump type unifications in a given inference system
// ----------------------------------------------------------------------------
{
    if (!XL::Allocator<XL::Types>::IsAllocated(ti))
    {
        std::cout << "Cowardly refusing to show bad Types pointer "
                  << (void *) ti << "\n";
        return;
    }

    using namespace XL;
    uint i = 0;

    tree_map &map = ti->unifications;
    for (tree_map::iterator t = map.begin(); t != map.end(); t++)
    {
        Tree *value = (*t).first;
        Tree *type = (*t).second;
        std::cout << "#" << ++i
                  << "\t" << ShortTreeForm(value)
                  << "\t= " << type
                  << "\t= " << ti->Base(type) << "\n";
    }
}



void debugr(XL::Types *ti)
// ----------------------------------------------------------------------------
//   Dump rewrite calls associated with each tree in this type inference system
// ----------------------------------------------------------------------------
{
    if (!XL::Allocator<XL::Types>::IsAllocated(ti))
    {
        std::cout << "Cowardly refusing to show bad Types pointer "
                  << (void *) ti << "\n";
        return;
    }

    using namespace XL;
    uint i = 0;

    rcall_map &map = ti->rcalls;
    for (rcall_map::iterator t = map.begin(); t != map.end(); t++)
    {
        Tree *expr = (*t).first;
        std::cout << "#" << ++i << "\t" << ShortTreeForm(expr) << "\n";

        uint j = 0;
        RewriteCalls *calls = (*t).second;
        RewriteCandidates &rc = calls->candidates;
        for (RewriteCandidates::iterator r = rc.begin(); r != rc.end(); r++)
        {
            std::cout << "\t#" << ++j
                      << "\t" << (*r).rewrite->left
                      << "\t: " << (*r).type << "\n";

            RewriteConditions &rt = (*r).conditions;
            for (RewriteConditions::iterator t = rt.begin(); t != rt.end(); t++)
                std::cout << "\t\tWhen " << ShortTreeForm((*t).value)
                          << "\t= " << ShortTreeForm((*t).test) << "\n";

            RewriteBindings &rb = (*r).bindings;
            for (RewriteBindings::iterator b = rb.begin(); b != rb.end(); b++)
            {
                std::cout << "\t\t" << (*b).name;
                std::cout << "\t= " << ShortTreeForm((*b).value) << "\n";
            }
        }
    }
}
