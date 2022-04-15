// ****************************************************************************
//  types.cpp                                                        XL project
// ****************************************************************************
//
//   File Description:
//
//    Common type analysis for XL programs
//
//    The Types class holds the implementation-independant portions of the type
//    analysis for XL, i.e. the part that is the same for the interpreter and
//    the compiler.
//
//
//
//
// ****************************************************************************
//   (C) 2020 Christophe de Dinechin <christophe@dinechin.org>
//   This software is licensed under the GNU General Public License v3
// ****************************************************************************
//   This file is part of XL.
//
//   XL is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   XL is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with XL.  If not, see <https://www.gnu.org/licenses/>.
// ****************************************************************************

#include "types.h"
#include "errors.h"
#include "builtins.h"
#include "rewrites.h"
#include "save.h"
#include "cdecls.h"


XL_BEGIN

Types::Types(Scope *scope)
// ----------------------------------------------------------------------------
//   Constructor for top-level type inferences
// ----------------------------------------------------------------------------
    : context(new Context(scope)),
      parent(nullptr),
      declaration(false)
{
    // Pre-assign some types
    types[xl_nil] = xl_nil;
    types[xl_true] = boolean_type;
    types[xl_false] = boolean_type;
    record(types, "Created Types %p for scope %t", this, scope);
}


Types::Types(Scope *scope, Types *parent)
// ----------------------------------------------------------------------------
//   Constructor for "child" type inferences, i.e. done within a parent
// ----------------------------------------------------------------------------
    : context(new Context(scope)), types(),
      parent(parent),
      declaration(false)
{
    context->CreateScope();
    scope = context->Symbols();
    record(types, "Created child Types %p scope %t", this, scope);
}


Types::~Types()
// ----------------------------------------------------------------------------
//    Destructor - Nothing to explicitly delete, but useful for debugging
// ----------------------------------------------------------------------------
{
    record(types, "Deleted Types %p", this);
}


Tree *Types::TypeAnalysis(Tree *program)
// ----------------------------------------------------------------------------
//   Perform all the steps of type inference on the given program
// ----------------------------------------------------------------------------
{
    // Once this is done, record all type information for the program
    record(types, "Type analysis for %t in %p", program, this);
    bool hasInstructions = context->ProcessDeclarations(program);
    Tree *result = hasInstructions ? Type(program) : declaration_type;
    record(types, "Type for %t in %p is %t", program, this, result);

    // Dump debug information if approriate
    if (RECORDER_TRACE(types))
        DumpAll();

    return result;
}


Types *Types::LocalTypes()
// ----------------------------------------------------------------------------
//   Return a Types structure for local processing
// ----------------------------------------------------------------------------
//   This is overloaded by implementations, e.g. CompilerTypes
{
    return new Types(context->Symbols(), this);
}


RewriteCalls *Types::NewRewriteCalls()
// ----------------------------------------------------------------------------
//   Return rewrite calls for this Types class
// ----------------------------------------------------------------------------
{
    return new RewriteCalls(this);
}


Tree *Types::Type(Tree *expr)
// ----------------------------------------------------------------------------
//   Return the type associated with a given expression
// ----------------------------------------------------------------------------
{
    // First check if we already computed the type for this expression
    Tree *type = KnownType(expr);
    if (type)
    {
        record(types, "In %p cached type for %t is %t", this, expr, type);
        return type;
    }

    // Apply the rules corresponding to each tree
    record(types, "In %p searching type for %t", this, expr);
    type = expr->Do(this);
    record(types, "In %p type for %t is %t initially", this, expr, type);
    type = AssignType(expr, type);
    record(types, "In %p type for %t is %t after assignment", this, expr, type);
    return type;
}


Tree *Types::ValueType(Tree *expr)
// ----------------------------------------------------------------------------
//   Return the type of an expression in a value context
// ----------------------------------------------------------------------------
{
    Save<bool> save(declaration, false);
    return Type(expr);
}


Tree *Types::DeclarationType(Tree *expr)
// ----------------------------------------------------------------------------
//   Return the type of an expression in a declaration context
// ----------------------------------------------------------------------------
{
    Save<bool> save(declaration, true);
    return Type(expr);
}


Tree *Types::BaseType(Tree *type, bool recurse)
// ----------------------------------------------------------------------------
//   Follow the chain of unifications and find base type
// ----------------------------------------------------------------------------
{
    for (Types *ts = this; ts; ts = recurse ? ts->parent : nullptr)
    {
        auto found = ts->unifications.find(type);
        if (found != ts->unifications.end())
            return found->second;
    }
    return type;
}


Tree *Types::KnownType(Tree *expr, bool recurse)
// ----------------------------------------------------------------------------
//   Check if there is a type already recorded for this expression
// ----------------------------------------------------------------------------
{
    for (Types *ts = this; ts; ts = recurse ? ts->parent : nullptr)
    {
        auto it = ts->types.find(expr);
        if (it != ts->types.end())
            return it->second;
    }
    return nullptr;
}



// ============================================================================
//
//    Tree::Do interface to
//
// ============================================================================

Tree *Types::Do(Natural *what)
// ----------------------------------------------------------------------------
//   Annotate an natural tree with its type - normally [natural]
// ----------------------------------------------------------------------------
{
    return DoConstant(what, natural_type, NATURAL);
}


Tree *Types::Do(Real *what)
// ----------------------------------------------------------------------------
//   Annotate a real tree with its type - normally [real]
// ----------------------------------------------------------------------------
{
    return DoConstant(what, real_type, REAL);
}


Tree *Types::Do(Text *what)
// ----------------------------------------------------------------------------
//   Annotate a text tree with its type - normally [text] or [character]
// ----------------------------------------------------------------------------
{
    Tree * type = what->IsCharacter() ? character_type : text_type;
    return DoConstant(what, type, TEXT);
}


Tree *Types::Do(Name *what)
// ----------------------------------------------------------------------------
//   Assign an unknown type to a name
// ----------------------------------------------------------------------------
{
    Tree *type = nullptr;
    record(types_ids, "In %p %+s name %t",
           this, declaration ? "declaring" : "processing", what);

    if (declaration)
    {
        type = PatternType(what);
        if (type)
            context->Define(what, what);
        return type;
    }

    Scope_p scope;
    Rewrite_p rw;
    Tree *body = context->Bound(what, true, &rw, &scope);
    if (body && body != what)
    {
        Tree *defined = PatternBase(rw->left);

        // Check if this is some built-in type
        if (defined != what)
        {
            if (scope != context->Symbols())
                captured[what] = defined;
            text label;
            if (RewriteCategory(rw, defined, label) == Decl::NORMAL)
                type = Type(body);
            else
                type = Type(defined);
            if (RewriteCalls *rc = TreeRewriteCalls(defined))
                rcalls[what] = rc;
            else if (RewriteCalls *rc = TreeRewriteCalls(body))
                rcalls[what] = rc;
        }
        else
        {
            type = Evaluate(what);
        }
    }
    else
    {
        type = Evaluate(what);
    }

    if (type && rw && rw->left != rw->right)
    {
        Tree *decl = rw->left;
        Tree *def = PatternBase(decl);
        if (def != what)
        {
            Tree *rwtype = DoRewrite(rw);
            if (!rwtype)
                return nullptr;
            type = AssignType(decl, type);
            if (def != decl)
                type = AssignType(def, type);
        }
    }

    return type;
}


Tree *Types::Do(Prefix *what)
// ----------------------------------------------------------------------------
//   Assign an unknown type to a prefix and then to its children
// ----------------------------------------------------------------------------
{
    // Deal with bizarre declarations
    if (Name *name = what->left->AsName())
    {
        if (name->value == "extern")
        {
            CDeclaration *cdecl = what->GetInfo<CDeclaration>();
            if (!cdecl)
            {
                Ooops("No C declaration for $1", what);
                return nullptr;
            }
            Tree *type = DoRewrite(cdecl->rewrite);
            return type;
        }
    }

    // For other cases, regular declaration
    return Evaluate(what);
}


Tree *Types::Do(Postfix *what)
// ----------------------------------------------------------------------------
//   Assign an unknown type to a postfix and then to its children
// ----------------------------------------------------------------------------
{
    // No special forms for postfix, try to look it up
    return Evaluate(what);
}


Tree *Types::Do(Infix *what)
// ----------------------------------------------------------------------------
//   Assign type to infix forms
// ----------------------------------------------------------------------------
//   We deal with the following special forms:
//   - [X;Y]: a statement sequence, type is the type of last statement
//   - [X:T] and [X as T]: a type declaration, assign the type T to X
//   - [X is Y]: A declaration, assign a type [type X => type Y]
{
    // For a sequence, both sub-expressions must succeed individually.
    // The type of the sequence is the type of the last statement
    if (IsSequence(what))
        return DoStatements(what, what->left, what->right);

    // Case of [X : T] : Set type of [X] to [T]
    if (IsTypeAnnotation(what))
        return DoTypeAnnotation(what);

    // Case of [X is Y]: Analysis if any will be done during evaluation
    if (IsDefinition(what))
        return DoRewrite(what);

    // For all other cases, evaluate the infix
    return Evaluate(what);
}


Tree *Types::Do(Block *what)
// ----------------------------------------------------------------------------
//   A block evaluates as its child
// ----------------------------------------------------------------------------
{
    Tree *type = Type(what->child);
    type = AssignType(what, type);
    return type;
}



// ============================================================================
//
//   Expression evaluation during type check
//
// ============================================================================

Tree *Types::DoConstant(Tree *what, Tree *base, kind k)
// ----------------------------------------------------------------------------
//   All constants have themselves as type, and evaluate normally
// ----------------------------------------------------------------------------
{
    // Check if we need to evaluate rewrites for this kind in the context
    // This would happen for something like [0 is 1; 0]
    Tree *type = Evaluate(what, true);

    // For constants, or if we failed to evaluate, assign basic type
    if (!type)
        type = base;

    return type;
}


Tree *Types::DoStatements(Tree *expr, Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//   Process sequences of statements
// ----------------------------------------------------------------------------
{
    // We always process left and right to find as many errors as possible
    Tree *lt = Type(left);
    Tree *rt = Type(right);

    // If there was any error, report it
    if (IsErrorType(lt) || IsErrorType(rt))
        return nullptr;

    // Return type of last non-declaration statement
    if (!IsRewriteType(rt))
        return rt;
    if (!IsRewriteType(lt))
        return lt;

    // If both left and right are rewrites, this is a declaration too
    return declaration_type;
}


Tree *Types::DoTypeAnnotation(Infix *rewrite)
// ----------------------------------------------------------------------------
//   All declarations evaluate as themselves
// ----------------------------------------------------------------------------
{
    record(types, "Type annotation %t", rewrite);
    Tree *value = rewrite->left;
    Tree *type = EvaluateType(rewrite->right);
    AssignType(value, type);
    Tree *base = PatternBase(value);
    if (base != value)
        AssignType(base, type);
    return declaration_type;
}


Tree *Types::DoRewrite(Infix *rewrite)
// ----------------------------------------------------------------------------
//   All declarations evaluate as themselves
// ----------------------------------------------------------------------------
{
    record(types, "Rewrite %t evaluates as declaration", rewrite);
    return declaration_type;
}


Tree *Types::TypeCoversType(Tree *wideType, Tree *narrowType)
// ----------------------------------------------------------------------------
//   Return the innermost type from wideType that covers narrowType
// ----------------------------------------------------------------------------
//   The input types are supposed to be base and evaluated types
//   REVISIT: We hard-code the name and processing of some type operat ors here
{
    // Direct match: stop here
    if (narrowType == wideType)
        return narrowType;

    // If there is a pattern, check if we can match the pattern
    if (Tree *pattern = IsPatternType(wideType))
    {
        if (pattern->AsName())
        {
            return narrowType;
        }
        if (Tree *npat = IsPatternType(narrowType))
        {
            if (Tree *t = PatternCoversPattern(pattern, npat, narrowType))
                return t;
        }
        if (Infix *typeAnnotation = IsTypeAnnotation(pattern))
        {
            Tree *declaredType = typeAnnotation->right;
            return TypeCoversType(declaredType, narrowType);
        }
        if (Infix *typeCondition = IsPatternCondition(pattern))
        {
            Ooops("Not implemented yet; Condition in pattern $1", pattern);
            return nullptr;
        }

        return nullptr;
    }

    // The tree type matches anything
    if (wideType == tree_type)
        return narrowType;

    // The infix type covers any infix type
    if (wideType == infix_type)
        if (Tree *pattern = IsPatternType(narrowType))
            if (pattern->AsInfix())
                return narrowType;

    // The prefix type covers any prefix type
    if (wideType == prefix_type)
        if (Tree *pattern = IsPatternType(narrowType))
            if (pattern->AsPrefix())
                return narrowType;

    // The postfix type covers any postfix type
    if (wideType == postfix_type)
        if (Tree *pattern = IsPatternType(narrowType))
            if (pattern->AsPostfix())
                return narrowType;

    // The block type covers any block type
    if (wideType == block_type)
        if (Tree *pattern = IsPatternType(narrowType))
            if (pattern->AsBlock())
                return narrowType;

    // Check natural types
    uint wideBits = 0;
    if (wideType == integer_type)       wideBits = 63;
    if (wideType == integer8_type)      wideBits = 7;
    if (wideType == integer16_type)     wideBits = 15;
    if (wideType == integer32_type)     wideBits = 31;
    if (wideType == integer64_type)     wideBits = 63;

    if (wideType == natural_type)       wideBits = 64;
    if (wideType == natural8_type)      wideBits = 8;
    if (wideType == natural16_type)     wideBits = 16;
    if (wideType == natural32_type)     wideBits = 32;
    if (wideType == natural64_type)     wideBits = 64;

    uint narrowBits = 0;
    if (narrowType == integer_type)     narrowBits = 63;
    if (narrowType == integer8_type)    narrowBits = 7;
    if (narrowType == integer16_type)   narrowBits = 15;
    if (narrowType == integer32_type)   narrowBits = 31;
    if (narrowType == integer64_type)   narrowBits = 63;

    if (narrowType == natural_type)     narrowBits = 64;
    if (narrowType == natural8_type)    narrowBits = 8;
    if (narrowType == natural16_type)   narrowBits = 16;
    if (narrowType == natural32_type)   narrowBits = 32;
    if (narrowType == natural64_type)   narrowBits = 64;

    if (wideBits)
    {
        // Check if we have [natural16] vs [matching 42]
        if (Tree *pattern = IsPatternType(narrowType))
        {
            if (Natural *ival = pattern->AsNatural())
            {
                if (wideBits & 1)
                {
                    longlong v = ival->value;
                    if ((v >> wideBits) == 0 || (v >> wideBits) + 1 == 0)
                        return narrowType;
                }
                else
                {
                    ulonglong v = (ulonglong) ival->value;
                    if ((v >> wideBits) == 0)
                        return narrowType;
                }
                return nullptr;
            }
        }

        // Check if we have an natural type against another
        if (narrowBits)
        {
            uint differentSigns = (narrowBits ^ wideBits) & 1;
            if (narrowBits < wideBits + differentSigns)
                return narrowType;
            else
                return nullptr;
        }
    }

    // Check real types
    wideBits = 0;
    if (wideType == real_type)          wideBits = 64;
    if (wideType == real16_type)        wideBits = 16;
    if (wideType == real32_type)        wideBits = 32;
    if (wideType == real64_type)        wideBits = 64;

    narrowBits = 0;
    if (narrowType == real_type)        narrowBits = 64;
    if (narrowType == real16_type)      narrowBits = 16;
    if (narrowType == real32_type)      narrowBits = 32;
    if (narrowType == real64_type)      narrowBits = 64;

    if (wideBits)
    {
        // Check if we have [natural16] vs [matching 42]
        if (Tree *pattern = IsPatternType(narrowType))
        {
            if (pattern->AsReal())
                return narrowType;
            return nullptr;
        }

        // Check if we have an natural type against another
        if (narrowBits)
        {
            if (narrowBits <= wideBits)
                return narrowType;
            else
                return nullptr;
        }
    }

    // Check text types
    if (wideType == text_type || wideType == character_type)
    {
        if (Tree *pattern = IsPatternType(narrowType))
        {
            if (Text *tval = pattern->AsText())
            {
                if (tval->IsCharacter() == (wideType == character_type))
                    return narrowType;
                else
                    return nullptr;
            }
        }
    }

    // Check name types
    if (wideType == name_type)
    {
        if (Tree *pattern = IsPatternType(narrowType))
        {
            if (Name *nval = pattern->AsName())
                return narrowType;
            else
                return nullptr;
        }
    }

    // Check infix operators for the wider type - REVISIT: HARDCODED
    if (Infix *infix = wideType->AsInfix())
    {
        if (infix->name == "or")
        {
            Tree *left = TypeCoversType(infix->left, narrowType);
            if (left)
                return left;
            Tree *right = TypeCoversType(infix->right, narrowType);
            if (right)
                return right;
        }
        else if (infix->name == "and")
        {
            if (Tree *left = TypeCoversType(infix->left, narrowType))
                if (Tree *right = TypeCoversType(infix->right, left))
                    return right;
        }
        else if (infix->name == "xor")
        {
            Tree *left = TypeCoversType(infix->left, narrowType);
            Tree *right = TypeCoversType(infix->right, narrowType);
            if (!left)
                return right;
            if (!right)
                return left;
            return nullptr;
        }
    }

    // Check some type prefix operators - REVISIT HARDCODED
    if (Prefix *prefix = wideType->AsPrefix())
    {
        if (Name *opcode = prefix->left->AsName())
        {
            if (opcode->value == "not")
            {
                Tree *right = TypeCoversType(prefix->right, narrowType);
                if (!right)
                    return narrowType;
            }
        }
    }

    // Check some type infix operators for the narrower type - REVISIT HARDCODED
    if (Infix *infix = narrowType->AsInfix())
    {
        if (infix->name == "or")
        {
            // For [T] to cover [A or B], it has to cover both [A] and [B]
            Tree *left = TypeCoversType(wideType, infix->left);
            Tree *right = TypeCoversType(wideType, infix->right);
            if (left && right)
                return narrowType;
            return nullptr;
        }
    }

    return nullptr;
}


Tree *Types::PatternCoversPattern(Tree *wide, Tree *narrow, Tree *type)
// ----------------------------------------------------------------------------
//   Check if a pattern covers a type, return narrower type
// ----------------------------------------------------------------------------
{
#define CONSTANT_PATTERN(Type)                                          \
    if (Type *nval = narrow->As<Type>())                                \
        if (nval->value == ((Type *) wide)->value)                      \
            return type;                                                \
    return nullptr

    switch(wide->Kind())
    {
    case NATURAL:       CONSTANT_PATTERN(Natural);
    case REAL:          CONSTANT_PATTERN(Real);
    case TEXT:          CONSTANT_PATTERN(Text);
    case NAME:          return narrow;

    case INFIX:
        if (Infix *typeAnnotation = IsTypeAnnotation(wide))
        {
            Tree *declaredType = typeAnnotation->right;
            return TypeCoversType(declaredType, type);
        }
        if (Infix *typeCondition = IsPatternCondition(wide))
        {
            Ooops("Not implemented yet; Condition in pattern $1", wide);
            return nullptr;
        }
        if (Infix *ni = narrow->AsInfix())
            if (Infix *wi = (Infix *) wide)
                if (ni->name == wi->name)
                    if (PatternCoversPattern(wi->left, ni->left, type))
                        if (PatternCoversPattern(wi->right, ni->right, type))
                            return type;
        return nullptr;
    case PREFIX:
        if (Prefix *np = narrow->AsPrefix())
            if (Prefix *wp = (Prefix *) wide)
                if (Name *nn = np->left->AsName())
                    if (Name *wn = wp->left->AsName())
                        if (nn->value == wn->value)
                            if (PatternCoversPattern(wp->right,np->right,type))
                                return type;
        return nullptr;
    case POSTFIX:
        if (Postfix *np = narrow->AsPostfix())
            if (Postfix *wp = (Postfix *) wide)
                if (Name *nn = np->right->AsName())
                    if (Name *wn = wp->right->AsName())
                        if (nn->value == wn->value)
                            if (PatternCoversPattern(wp->left,np->left, type))
                                return type;
        return nullptr;
    case BLOCK:
        return PatternCoversPattern(((Block *) wide)->child, narrow, type);
    }
#undef CONSTANT_PATTERN
    return nullptr;
}


// ============================================================================
//
//   Evaluation during type analysis
//
// ============================================================================

static Tree *lookupRewriteCalls(Scope *evalScope, Scope *sc,
                                Tree *what, Rewrite *entry, void *i)
// ----------------------------------------------------------------------------
//   Used to check if RewriteCalls pass
// ----------------------------------------------------------------------------
{
    RewriteCalls *rc = (RewriteCalls *) i;
    return rc->Check(sc, what, entry);
}


Tree *Types::Evaluate(Tree *what, bool mayFail)
// ----------------------------------------------------------------------------
//   Find candidates for the given expression and infer types from that
// ----------------------------------------------------------------------------
{
    record(types_calls, "In %p %+s %t",
           this, declaration ? "declaring" : "evaluating", what);
    if (declaration)
        return UnknownType(what->Position());

    // Test if we are already trying to evaluate this particular pattern
    // Need to assign a type name, which will be unified by the outer Evaluate()
    if (RewriteCalls_p existingRC = TreeRewriteCalls(what))
    {
        Tree *type = UnknownType(what->Position());
        type = AssignType(what, type);
        return type;
    }

    // Identify all candidate rewrites in the current context
    RewriteCalls_p rc = NewRewriteCalls();
    rcalls[what] = rc;
    uint count = 0;
    Errors errors;
    errors.Log (Error("Unable to evaluate $1:", what), true);
    context->Lookup(what, lookupRewriteCalls, rc);

    // If we have no candidate, this is a failure
    count = rc->Size();
    if (count == 0)
    {
        if (!mayFail)
            return PatternType(what);
        return nullptr;
    }

    errors.Clear();
    errors.Log(Error("The type of $1 is conflicting because", what), true);

    // The resulting type is the union of all candidates
    Tree *type = rc->Candidate(0)->type;
    for (uint i = 1; type && i < count; i++)
    {
        RewriteCandidate *call = rc->Candidate(i);
        Tree *ctype = call->type;
        type = Unify(type, ctype, what, call->rewrite->left);
    }
    type = AssignType(what, type);
    return type;
}



// ============================================================================
//
//   Type evaluation
//
// ============================================================================

Tree *Types::EvaluateType(Tree *what, bool mayFail)
// ----------------------------------------------------------------------------
//   Evaluate the type expression as a type
// ----------------------------------------------------------------------------
{
    TypeEvaluator typeEvaluator(this);
    Tree *type = what->Do(typeEvaluator);
    if (type && type != what)
        type = Join(what, type);
    return type;
}


Types::TypeEvaluator::TypeEvaluator(Types *types)
// ----------------------------------------------------------------------------
//   Constructor simply preserves the types
// ----------------------------------------------------------------------------
    : types(types)
{}


static Tree *lookupType(Scope *evalScope, Scope *sc,
                        Tree *what, Rewrite *entry, void *i)
// ----------------------------------------------------------------------------
//   Used to lookup type expressions
// ----------------------------------------------------------------------------
{
    if (Name *test = what->AsName())
        if (Name *decl = entry->left->AsName())
            if (test->value == decl->value)
                return decl;
    return nullptr;
}


Tree *Types::TypeEvaluator::Do(Tree *what)
// ----------------------------------------------------------------------------
//   Evaluate the tree in the given types and context
// ----------------------------------------------------------------------------
{
    record(types, "In %p evaluating type %t", this, what);
    Tree *candidate = types->context->Lookup(what, lookupType, this);
    record(types, "In %p evaluated type %t to %t", this, what, candidate);
    return candidate;
}


Tree *Types::TypeEvaluator::Do(Name *what)
// ----------------------------------------------------------------------------
//   Evaluate the tree in the given types and context
// ----------------------------------------------------------------------------
{
    record(types, "In %p evaluating type name %t", this, what);
    Tree *candidate = types->context->Bound(what);
    record(types, "In %p evaluated type name %t to %t", this, what, candidate);
    return candidate;
}


Tree *Types::TypeEvaluator::Do(Infix *what)
// ----------------------------------------------------------------------------
//   Evaluate the tree in the given types and context
// ----------------------------------------------------------------------------
{
    // The type of the sequence is the type of the last statement
    if (IsSequence(what) || IsTypeAnnotation(what) || IsDefinition(what))
    {
        Ooops("Malformed type expression $1", what);
        return nullptr;
    }

    // Fallback to normal evaluation
    return Do((Tree *) what);
}


Tree *Types::TypeEvaluator::Do(Prefix *what)
// ----------------------------------------------------------------------------
//   Evaluate the tree in the given types and context
// ----------------------------------------------------------------------------
{
    // Return pattern types as is
    if (Types::IsPatternType(what))
        return what;

    // Fallback to normal evaluation
    return Do((Tree *) what);
}



// ============================================================================
//
//   Type associations and unifications
//
// ============================================================================

Tree *Types::TypeError(Tree *t1, Tree *t2, Tree *x1, Tree *x2)
// ----------------------------------------------------------------------------
//   Show type matching errors
// ----------------------------------------------------------------------------
{
    // Find expressions that have the type
    for (auto &t : types)
    {
        if (x1 && x2)
            break;
        if (!x1 && t.second == t1)
            x1 = t.first;
        if (!x2 && t.second == t2)
            x2 = t.first;
    }

    if (x1 == x2)
    {
        if (x1)
            Ooops("Type of $1 cannot be both $2 and $3", x1, t1, t2);
        else
            Ooops("Cannot unify type $2 and $1", t1, t2);
    }
    else
    {
        if (x1)
            Ooops("Cannot unify type $2 of $1", x1, t1);
        else
            Ooops("Cannot unify type $1", t1);
        if (x2)
            Ooops("with type $2 of $1", x2, t2);
        else
            Ooops("with type $1", t2);
    }

    return nullptr;
}


Tree *Types::AssignType(Tree *expr, Tree *type, bool add)
// ----------------------------------------------------------------------------
//   Assign the given type to the expression
// ----------------------------------------------------------------------------
{
    // Check if what we want to assign is compatible with what is known
    if (Tree *known = KnownType(expr))
    {
        if (known == type)
            return type;
        type = Unify(known, type, expr);
    }
    types[expr] = type;
    return type;
}


Tree *Types::Unify(Tree *t1, Tree *t2, Tree *x1, Tree *x2)
// ----------------------------------------------------------------------------
//   Perform type unification between the two types
// ----------------------------------------------------------------------------
{
    // Success if t1 covers t2 or t2 covers t1
    record(types_unifications, "In %p unifying %t and %t", this, t1, t2);

    // Check if already unified
    if (t1 == t2)
        return t1;

    // Check if one of the sides had a type error
    if (!t1 || !t2)
        return nullptr;

    // Strip out blocks in type specification, i.e. [T] == [(T)]
    if (Block *b1 = t1->AsBlock())
        return Join(b1, Unify(b1->child, t2, x1, x2));
    if (Block *b2 = t2->AsBlock())
        return Join(b2, Unify(t1, b2->child, x1, x2));

    // Check if we have a unification for this type
    Tree *b1 = BaseType(t1);
    Tree *b2 = BaseType(t2);
    if (b1 != t1 || b2 != t2)
        // No join here, since already joined with base
        return Unify(b1, b2, x1, x2);

    // If either is a generic, unify with the other
    if (IsUnknownType(t1))
        return Join(t1, t2);
    if (IsUnknownType(t2))
        return Join(t2, t1);

    // Lookup type names, replace them with their value
    b1 = EvaluateType(t1, true);
    if (b1 && b1 != t1)
        return Unify(b1, t2, x1, x2);
    b2 = EvaluateType(t2, true);
    if (b2 && b2 != t2)
        return Unify(t1, b2, x1, x2);   // Join() happens in Evaluate()

    // Success if t1 covers t2 or t2 covers t1
    record(types_unifications, "In %p unify %t and %t", this, t1, t2);

    // Check cases of super-types
    if (Tree *t = TypeCoversType(t1, t2))
        return Join(t1, t);
    if (Tree *t = TypeCoversType(t2, t1))
        return Join(t1, t);

    // None of the above: fail
    return TypeError(t1, t2, x1, x2);
}


Tree *Types::Join(Tree *old, Tree *replacement)
// ----------------------------------------------------------------------------
//   Replace the old type with the new one
// ----------------------------------------------------------------------------
{
    // Deal with error cases
    if (!old || !replacement)
        return nullptr;

    // Go to the base type for the replacement
    Tree *replace = BaseType(replacement);
    if (old == replace)
        return old;

    // Store the unification
    record(types_unifications, "In %p join %t with %t (base for %t)",
           this, old, replace, replacement);
    auto it = unifications.find(old);
    if (it != unifications.end())
    {
        if (it->second == replace)
            return old;
        return TypeError(old, replace, it->first, it->first);
    }
    unifications[old] = replace;

    // Replace the type in the types map
    for (auto &t : types)
    {
        if (t.second)
        {
            Tree *joined = JoinedType(t.second, old, replace);
            if (joined != t.second)
            {
                Tree *original = t.second;
                t.second = joined;
                Join(original, joined);
            }
        }
    }

    // Replace the type in the 'unifications' map
    for (auto &u : unifications)
        if (u.second == old)
            u.second = replace;

    // Replace the type in the rewrite calls
    for (auto &r : rcalls)
        for (RewriteCandidate *rc : r.second->Candidates())
            if (rc->type == old)
                rc->type = replace;

    return replace;
}


Tree *Types::JoinedType(Tree *type, Tree *old, Tree *replace)
// ----------------------------------------------------------------------------
//   Build a type after joining, in case that's necessary
// ----------------------------------------------------------------------------
{
    record(types_joined, "In %p replace %t with %t in %t",
           this, old, replace, type);
    XL_REQUIRE (type != nullptr);
    XL_REQUIRE (old != nullptr);
    XL_REQUIRE (replace != nullptr);

    if (type == old || type == replace)
        return replace;

    switch (type->Kind())
    {
    case NATURAL:
    case REAL:
    case TEXT:
    case NAME:
        return type;

    case BLOCK:
    {
        Block *block = (Block *) type;
        Tree *child = JoinedType(block->child, old, replace);
        XL_ASSERT (child);
        if (child != block->child)
            return new Block(block, child);
        return block;
    }

    case PREFIX:
    {
        Prefix *prefix = (Prefix *) type;
        Tree *left = JoinedType(prefix->left, old, replace);
        Tree *right = JoinedType(prefix->right, old, replace);
        XL_ASSERT (left && right);
        if (left != prefix->left || right != prefix->right)
            return new Prefix(prefix, left, right);
        return prefix;
    }

    case POSTFIX:
    {
        Postfix *postfix = (Postfix *) type;
        Tree *left = JoinedType(postfix->left, old, replace);
        Tree *right = JoinedType(postfix->right, old, replace);
        XL_ASSERT (left && right);
        if (left != postfix->left || right != postfix->right)
            return new Postfix(postfix, left, right);
        return postfix;
    }

    case INFIX:
    {
        Infix *infix = (Infix *) type;
        if (infix->name != "=>"         ||
            infix->left != replace      ||
            infix->right != old)
        {
            Tree *left = JoinedType(infix->left, old, replace);
            Tree *right = JoinedType(infix->right, old, replace);
            XL_ASSERT (left && right);
            if (left != infix->left || right != infix->right)
                return new Infix(infix, left, right);
        }
        return infix;
    }

    }
    return nullptr; // Get rid of bogus GCC 8.3.1 warning "Control reaches end"
}


Tree *Types::UnionType(Tree *t1, Tree *t2)
// ----------------------------------------------------------------------------
//    Create the union of two types
// ----------------------------------------------------------------------------
{
    if (t1 == t2)
        return t1;
    if (!t1 || !t2)
        return nullptr;

    if (Tree *type = TypeCoversType(t1, t2))
        return type;
    if (Tree *type = TypeCoversType(t2, t1))
        return type;

    return new Infix("or", t1, t2, t1->Position());

}


Tree *Types::PatternType(Tree *expr)
// ----------------------------------------------------------------------------
//   Return the type of the expr as a [type X] expression
// ----------------------------------------------------------------------------
{
    // Check if we know a type for this expression, if so return it
    if (Tree *known = KnownType(expr))
        return known;

    Tree *type = expr;
    switch(expr->Kind())
    {
    case NATURAL:
        return natural_type;
    case REAL:
        return real_type;
    case TEXT:
        return ((Text *) expr)->IsCharacter() ? character_type : text_type;

    case NAME:
    {
         // Need to build type name by default
        type = nullptr;

        // For 'self', lookup original name
        Name *name = (Name *) expr;
        if (name->value == "self")
            if (Tree *declared = context->DeclaredPattern(expr))
                if (declared != expr)
                    type = PatternType(declared);
        break;
    }

    case BLOCK:
        type = PatternType(((Block *) expr)->child);
        break;

    case PREFIX:
        // Case of [X is C name] or [X is builtin Op]
        if (Prefix *prefix = type->AsPrefix())
        {
            if (Name *name = prefix->left->AsName())
            {
                if (name->value == "C" || name->value == "builtin")
                    type = nullptr;
            }
        }
        /* fall-through */

    case INFIX:
    case POSTFIX:
        if (type)
        {
            TreePosition pos = type->Position();
            type = MakeTypesExplicit(type);
            type = new Prefix(xl_matching, type, pos);
        }
        break;
    }

    // For other names, assign a new matching type, e.g. matching(X)
    if (!type)
        type = new Prefix(xl_matching, expr, expr->Position());

    // Otherwise, return [type X] and assign it to this expr
    types[expr] = type;
    return type;
}


Tree *Types::UnknownType(TreePosition pos)
// ----------------------------------------------------------------------------
//    Build an "unknown" type, e.g. #A
// ----------------------------------------------------------------------------
{
    ulong v = id++;
    text  name;
    do
    {
        name = char('A' + v % 26) + name;
        v /= 26;
    } while (v);
    Tree *type = new Name("#" + name, pos);
    return type;
}


Tree *Types::MakeTypesExplicit(Tree *expr)
// ----------------------------------------------------------------------------
//   Make the types explicit in a tree shape
// ----------------------------------------------------------------------------
//   For example, if we have [X,Y], based on current known types, we may
//   rewrite this as [X:#A, Y:natural].
{
    switch(expr->Kind())
    {
    case NATURAL:
    case REAL:
    case TEXT:
        return expr;

    case NAME:
    {
        // Replace name with reference type to minimize size of lookup tables
        if (Tree *def = context->DeclaredPattern(expr))
            if (Name *name = def->AsName())
                expr = name;

        Tree *type = Type(expr);
        Tree *result = new Infix(":", expr, type, expr->Position());
        return result;
    }

    case BLOCK:
    {
        Block *block = (Block *) expr;
        Tree *child = MakeTypesExplicit(block->child);
        if (child != block->child)
            block = new Block(block, child);
        return block;
    }

    case PREFIX:
    {
        Prefix *prefix = (Prefix *) expr;
        Tree *left = prefix->left->AsName()
            ? (Tree *) prefix->left
            : MakeTypesExplicit(prefix->left);
        Tree *right = MakeTypesExplicit(prefix->right);
        if (left != prefix->left || right != prefix->right)
            prefix = new Prefix(prefix, left, right);
        return prefix;
    }

    case POSTFIX:
    {
        Postfix *postfix = (Postfix *) expr;
        Tree *left = MakeTypesExplicit(postfix->left);
        Tree *right = postfix->right->AsName()
            ? (Tree *) postfix->right
            : MakeTypesExplicit(postfix->right);
        if (left != postfix->left || right != postfix->right)
            postfix = new Postfix(postfix, left, right);
        return postfix;
    }

    case INFIX:
    {
        Infix *infix = (Infix *) expr;
        if (IsTypeAnnotation(infix))
        {
            Tree *right = EvaluateType(infix->right);
            if (right != infix->right)
                infix = new Infix(infix, infix->left, right);
            return infix;
        }
        if (IsPatternCondition(infix))
        {
            Tree *left = MakeTypesExplicit(infix->left);
            if (left != infix->left)
                infix = new Infix(infix, left, infix->right);
            return infix;
        }
        Tree *left = MakeTypesExplicit(infix->left);
        Tree *right = MakeTypesExplicit(infix->right);
        if (left != infix->left || right != infix->right)
            infix = new Infix(infix, left, right);
        return infix;
    }
    }
    return nullptr; // Get rid of bogus GCC 8.3.1 warning "Control reaches end"
}



// ============================================================================
//
//   Test the kind of types we have here
//
// ============================================================================

Tree *Types::IsPatternType(Tree *type)
// ----------------------------------------------------------------------------
//   Check if type is a type pattern, i.e. [matching ( ... )]
// ----------------------------------------------------------------------------
{
    if (Prefix *pfx = type->AsPrefix())
    {
        if (Name *tname = pfx->left->AsName())
        {
            if (tname == xl_matching)
            {
                Tree *pattern = pfx->right;
                if (Block *block = pattern->AsBlock())
                    pattern = block->child;
                return pattern;
            }
        }
    }

    return nullptr;
}


Tree *Types::IsRewriteType(Tree *type)
// ----------------------------------------------------------------------------
//   Check if type is a rewrite type, i.e. something like [X => Y]
// ----------------------------------------------------------------------------
{
    if (type == declaration_type)
        return type;
    return nullptr;
}


Infix *Types::IsUnionType(Tree *type)
// ----------------------------------------------------------------------------
//   Check if type is a union type, i.e. something like [natural|real]
// ----------------------------------------------------------------------------
{
    if (Infix *infix = type->AsInfix())
        if (infix->name == "|")
            return infix;

    return nullptr;
}


RewriteCalls *Types::TreeRewriteCalls(Tree *what, bool recurse)
// ----------------------------------------------------------------------------
//   Check if we have rewrite calls for this tree
// ----------------------------------------------------------------------------
{
    for (Types *ts = this; ts; ts = recurse ? ts->parent : nullptr)
    {
        auto it = ts->rcalls.find(what);
        if (it != ts->rcalls.end())
        {
            RewriteCalls *rc = it->second;
            record(types_calls, "In %p calls for %t are %p (%u entries)",
                   ts, what, rc, rc->Size());
            return rc;
        }
    }
    return nullptr;;
}


void Types::TreeRewriteCalls(Tree *what, RewriteCalls *rc)
// ----------------------------------------------------------------------------
//   Check if we have rewrite calls for this tree
// ----------------------------------------------------------------------------
{
    XL_ASSERT(rcalls.find(what) == rcalls.end() &&
              "Duplicate insertion of RewriteCalls for the same tree");
    rcalls[what] = rc;
}



// ============================================================================
//
//    Static utilities to categorize kinds of rewrites
//
// ============================================================================

Types::Decl Types::RewriteCategory(RewriteCandidate *rc)
// ----------------------------------------------------------------------------
//   Check if the rewrite candidate is of a special kind
// ----------------------------------------------------------------------------
{
    return RewriteCategory(rc->rewrite, rc->defined, rc->defined_name);
}


Types::Decl Types::RewriteCategory(Rewrite *rw,
                                   Tree *defined,
                                   text &label)
// ----------------------------------------------------------------------------
//   Check if the declaration is of a special kind
// ----------------------------------------------------------------------------
{
    Types::Decl decl = Types::Decl::NORMAL;
    Tree *body = rw->right;

    if (Name *bodyname = body->AsName())
    {
        // Case of [sin X is C]: Use the name 'sin'
        if (bodyname->value == "C")
            if (Name *defname = defined->AsName())
                if (IsValidCName(defname, label))
                    decl = Types::Decl::C;
        // Case of [X, Y is self]: Mark as DATA
        if (bodyname->value == "self")
            decl = Types::Decl::DATA;
    }

    if (Prefix *prefix = body->AsPrefix())
    {
        if (Name *name = prefix->left->AsName())
        {
            // Case of [alloc X is C "_malloc"]: Use "_malloc"
            if (name->value == "C")
                if (IsValidCName(prefix->right, label))
                    decl = Types::Decl::C;
            // Case of [X+Y is builtin Add]: select BUILTIN type
            if (name->value == "builtin")
                decl = Types::Decl::BUILTIN;
        }
    }

    return decl;
}


bool Types::IsValidCName(Tree *tree, text &label)
// ----------------------------------------------------------------------------
//   Check if the name is valid for C
// ----------------------------------------------------------------------------
{
    uint len = 0;

    if (Name *name = tree->AsName())
    {
        label = name->value;
        len = label.length();
    }
    else if (Text *text = tree->AsText())
    {
        label = text->value;
        len = label.length();
    }

    if (len == 0)
    {
        Ooops("No valid C name in $1", tree);
        return false;
    }

    // We will NOT call functions beginning with _ (internal functions)
    for (uint i = 0; i < len; i++)
    {
        char c = label[i];
        if (!isalpha(c) && c != '_' && !(i && isdigit(c)))
        {
            Ooops("C name $1 contains invalid characters", tree);
            return false;
        }
    }
    return true;
}



// ============================================================================
//
//   Debug utilities
//
// ============================================================================

void Types::Dump()
// ----------------------------------------------------------------------------
//   Dump the list of types in this
// ----------------------------------------------------------------------------
{
    std::cout << "TYPES " << (void *) this << ":\n";
    for (auto &t : types)
    {
        Tree *value = t.first;
        Tree *type = t.second;
        Tree *base = BaseType(type);
        std::cout << ShortTreeForm(value)
                  << " (" << (void *) value << ")"
                  << "\t: " << type
                  << " (" << (void *) type << ")";
        if (base != type)
            std::cout << "\t= " << base << " (" << (void *) base << ")";
        std::cout << "\n";
    }

    std::cout << "\n\nCALLS " << (void *) this << ":\n";
    for (auto &t : rcalls)
    {
        Tree *expr = t.first;
        std::cout << ShortTreeForm(expr) << " (" << (void *) expr << ")\n";

        RewriteCalls *calls = t.second;
        calls->Dump();
    }
}


void Types::DumpAll()
// ----------------------------------------------------------------------------
//   Dump the list of types in this
// ----------------------------------------------------------------------------
{
    for (Types *types = this; types; types = types->parent)
        types->Dump();
}



// ============================================================================
//
//   Static variables
//
// ============================================================================

uint Types::id= 0;


XL_END

#ifndef INTERPRETER_ONLY
#include "compiler-types.h"
#endif

XL::Types *xldebug(XL::Types *ti)
// ----------------------------------------------------------------------------
//   Dump a type inference
// ----------------------------------------------------------------------------
{
    if (XL::Allocator<XL::Types>::IsAllocated(ti))
    {
        ti->DumpAll();
    }
#ifndef INTERPRETER_ONLY
    else if (XL::Allocator<XL::CompilerTypes>::IsAllocated(ti))
    {
        ti->DumpAll();
    }
#endif // INTERPRETER_ONLY
    else
    {
        std::cout << "Cowardly refusing to show bad Types pointer "
                  << (void *) ti << "\n";
    }
    return ti;
}


XL::Types *xldebug(XL::Types_p ti)
// ----------------------------------------------------------------------------
//   Dump a pointer to compiler types
// ----------------------------------------------------------------------------
{
    return xldebug((XL::Types *) ti);
}


RECORDER(types,                 64, "Type analysis");
RECORDER(types_ids,             64, "Assigned type identifiers");
RECORDER(types_unifications,    64, "Type unifications");
RECORDER(types_calls,           64, "Type deductions in rewrites (calls)");
RECORDER(types_boxing,          64, "Machine type boxing and unboxing");
RECORDER(types_joined,          64, "Joined types");
