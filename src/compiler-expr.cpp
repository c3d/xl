// *****************************************************************************
// compiler-expr.cpp                                                  XL project
// *****************************************************************************
//
// File description:
//
//    Compilation of XL expressions ("expression reduction")
//
//
//
//
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2010-2012,2014-2020, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2012, Jérôme Forissier <jerome@taodyne.com>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License,
// or (at your option) any later version.
//
// XL is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with XL, in a file named COPYING.
// If not, see <https://www.gnu.org/licenses/>.
// *****************************************************************************

#include "compiler-expr.h"
#include "compiler-unit.h"
#include "compiler-rewrites.h"
#include "compiler-types.h"
#include "context.h"
#include "save.h"
#include "basics.h"
#include "errors.h"
#include "renderer.h"
#include "llvm-crap.h"

#include <vector>


RECORDER(compiler_expr,   128, "Expression reduction (compilation of calls)");
RECORDER(boxed_assign,    128, "Boxed assignment/read debug flow");

XL_BEGIN


static bool RewriteBodyReferencesName(Tree *tree, const text &nm)
// ----------------------------------------------------------------------------
//   True if the rewrite RHS references this binding name as an identifier.
// ----------------------------------------------------------------------------
//   Pattern-only bindings must not be evaluated before the RHS (e.g. the
//   then-branch of [if [[false]] then TrueBody is false] is not executed).
{
    if (!tree)
        return false;
    if (Name *n = tree->AsName())
        return n->value == nm;
    if (Block *b = tree->AsBlock())
        return RewriteBodyReferencesName(b->child, nm);
    if (Prefix *p = tree->AsPrefix())
        return RewriteBodyReferencesName(p->left, nm) ||
               RewriteBodyReferencesName(p->right, nm);
    if (Infix *ix = tree->AsInfix())
        return RewriteBodyReferencesName(ix->left, nm) ||
               RewriteBodyReferencesName(ix->right, nm);
    if (Postfix *px = tree->AsPostfix())
        return RewriteBodyReferencesName(px->left, nm) ||
               RewriteBodyReferencesName(px->right, nm);
    return false;
}


static bool ArgWantsLazyThickClosure(Tree *arg)
// ----------------------------------------------------------------------------
//   True if a NORMAL rewrite argument should be passed as a thick closure.
// ----------------------------------------------------------------------------
//   Deferred bodies stay lazy. Plain names are handled separately by remapping
//   them to their lazy-binding source before using any eager known value.
{
    if (!arg)
        return false;
    if (Block *block = arg->AsBlock())
        return block->IsIndent() || block->IsBraces();
    if (Infix *infix = arg->AsInfix())
        return IsSequence(infix) || IsDefinition(infix);
    return false;
}


static Tree *LazyCompositeExpansionForName(lazy_binding_map &lazyBindings,
                                           Context *          ctx,
                                           Name *             n)
// ----------------------------------------------------------------------------
//   Lazy NORMAL binding tree for n when it is composite (!IsLeaf after peel).
// ----------------------------------------------------------------------------
//   Uses lazyBindingSource only; ordinary local bindings must not be peeled
//   through Context::Bound() because locals like [I] may be backed by mutable
//   assignments such as [I := I + 1].
{
    if (!n)
        return nullptr;
    Tree *exp = nullptr;
    auto lit = lazyBindings.find(n->value);
    if (lit != lazyBindings.end())
        exp = lit->second;
    if (!exp)
        return nullptr;
    Context_g peelCtx = ctx;
    while (Tree *inner = exp->IsClosure(&peelCtx))
        exp = inner;
    if (!exp || exp->IsLeaf())
        return nullptr;
    record(closures, "Lazy composite expansion %t => %t", n, exp);
    return exp;
}


static bool ApplyLazyBindingPeel(lazy_binding_map & lazyBindings,
                                 CompilerTypes *    types,
                                 Context *          context,
                                 Tree *             call,
                                 Tree *&            rewriteCall,
                                 CompilerRewriteCalls *&rc)
// ----------------------------------------------------------------------------
//   Replace call with shaped tree when a bound Name hides composite structure.
// ----------------------------------------------------------------------------
//   XL encodes calls as PREFIX / INFIX / POSTFIX trees; operand slots that are
//   still bare Names may need expansion so Lookup sees the argument shape. No
//   operator names from lazyBindingSource—only composite expansions for Names.
{
    auto accept = [&](Tree *peeled) -> bool
    {
        record(closures, "Try peeled call %t from %t", peeled, call);
        CompilerRewriteCalls *rc2 = types->TreeRewriteCalls(peeled);
        if (!rc2)
        {
            types->Type(peeled);
            rc2 = types->TreeRewriteCalls(peeled);
        }
        if (rc2)
        {
            rewriteCall = peeled;
            rc = rc2;
            record(closures, "Accepted peeled call %t with %u candidates",
                   rewriteCall, rc2->Size());
            return true;
        }
        record(closures, "Rejected peeled call %t (no rewrite calls)", peeled);
        return false;
    };

    if (Prefix *p = call->AsPrefix())
    {
        Name *op = p->left->AsName();
        Name *argn = p->right->AsName();
        if (!op || !argn)
            return false;
        if (Tree *exp = LazyCompositeExpansionForName(lazyBindings,
                                                       context,
                                                       argn))
        {
            types->Type(exp);
            return accept(new Prefix(op, exp, p->Position()));
        }
        return false;
    }

    if (Postfix *px = call->AsPostfix())
    {
        Name *argn = px->left->AsName();
        Name *op = px->right->AsName();
        if (!op || !argn)
            return false;
        if (Tree *exp = LazyCompositeExpansionForName(lazyBindings,
                                                       context,
                                                       argn))
        {
            types->Type(exp);
            return accept(new Postfix(exp, op, px->Position()));
        }
        return false;
    }

    if (Infix *ix = call->AsInfix())
    {
        Tree *eL = nullptr;
        Tree *eR = nullptr;
        if (Name *nl = ix->left->AsName())
            eL = LazyCompositeExpansionForName(lazyBindings, context, nl);
        if (Name *nr = ix->right->AsName())
            eR = LazyCompositeExpansionForName(lazyBindings, context, nr);
        if (!eL && !eR)
            return false;
        if (eL)
            types->Type(eL);
        if (eR)
            types->Type(eR);
        if (eL && eR)
            return accept(new Infix(ix->name, eL, eR, ix->Position()));
        if (eL)
            return accept(new Infix(ix->name, eL, ix->right, ix->Position()));
        return accept(new Infix(ix->name, ix->left, eR, ix->Position()));
    }

    return false;
}


static JIT::Value_p UnusedRewriteBindingValue(JITBlock &code, JIT::Type_p ty)
// ----------------------------------------------------------------------------
//   Placeholder value for a binding not referenced by the rewrite RHS.
// ----------------------------------------------------------------------------
//   The compiled body must not load this argument; type matches the signature.
{
    return code.NullConstant(ty);
}


static JIT::Value_p UnwrapThickIfNeeded(CompilerFunction &fn,
                                        CompilerUnit &   unit,
                                        JITBlock &       code,
                                        Tree *           forExpr,
                                        JIT::Value_p     v)
// ----------------------------------------------------------------------------
//   Force-evaluate thick lazy parameters when a bound name is read.
// ----------------------------------------------------------------------------
{
    if (!v)
        return v;
    JIT::Type_p t = code.Type(v);
    if (unit.IsClosureType(t))
    {
        record(closures, "Unwrap thick lazy value %v for %t type %T",
               v, forExpr, t);
        return fn.InvokeAndAutoboxThickClosure(forExpr, v, t);
    }
    return v;
}


void CompilerExpression::SeedKnownCompositeLeaves(Tree *expansion,
                                                  JIT::Value_p boxed,
                                                  JIT::Type_p boxedTy,
                                                  uint &index)
// ----------------------------------------------------------------------------
//   Seed peeled leaf values from a known composite argument
// ----------------------------------------------------------------------------
{
    switch(expansion->Kind())
    {
    case NATURAL:
    case REAL:
    case TEXT:
    case NAME:
        function.values[expansion] =
            function.code.StructLoad(boxedTy, boxed, index++, "peel");
        return;

    case BLOCK:
        SeedKnownCompositeLeaves(((Block *) expansion)->child,
                                 boxed, boxedTy, index);
        return;

    case PREFIX:
    {
        Prefix *prefix = (Prefix *) expansion;
        if (prefix->left->Kind() != NAME)
            SeedKnownCompositeLeaves(prefix->left, boxed, boxedTy, index);
        SeedKnownCompositeLeaves(prefix->right, boxed, boxedTy, index);
        return;
    }

    case POSTFIX:
    {
        Postfix *postfix = (Postfix *) expansion;
        if (postfix->right->Kind() != NAME)
            SeedKnownCompositeLeaves(postfix->right, boxed, boxedTy, index);
        SeedKnownCompositeLeaves(postfix->left, boxed, boxedTy, index);
        return;
    }

    case INFIX:
    {
        Infix *infix = (Infix *) expansion;
        if (IsTypeAnnotation(infix) || IsPatternCondition(infix))
        {
            SeedKnownCompositeLeaves(infix->left, boxed, boxedTy, index);
            return;
        }
        SeedKnownCompositeLeaves(infix->left, boxed, boxedTy, index);
        SeedKnownCompositeLeaves(infix->right, boxed, boxedTy, index);
        return;
    }
    }
}


void CompilerExpression::SeedKnownCompositeLeaves(Name *sourceName,
                                                  Tree *expansion)
// ----------------------------------------------------------------------------
//   Prime peeled leaves when the source name already has a boxed value
// ----------------------------------------------------------------------------
{
    if (!sourceName || !expansion || expansion->IsLeaf())
        return;

    JIT::Value_p boxed = function.Known(sourceName,
                                        CompilerFunction::knowValues |
                                        CompilerFunction::knowLocals);
    if (!boxed)
        return;

    JIT::Type_p boxedTy = function.code.Type(boxed);
    if (!boxedTy ||
        !JIT::IsStructType(boxedTy) ||
        function.compiler.IsTreePointerType(boxedTy) ||
        function.unit.IsClosureType(boxedTy))
        return;

    uint index = 0;
    SeedKnownCompositeLeaves(expansion, boxed, boxedTy, index);
    record(closures, "Seed peeled values %t => %t from %v",
           sourceName, expansion, boxed);
}


void CompilerExpression::SeedPeeledKnownValues(Tree *call, Tree *rewriteCall)
// ----------------------------------------------------------------------------
//   Seed peeled call leaves from known composite arguments
// ----------------------------------------------------------------------------
{
    if (!call || !rewriteCall || call == rewriteCall)
        return;

    if (Prefix *from = call->AsPrefix())
        if (Prefix *to = rewriteCall->AsPrefix())
            if (Name *arg = from->right->AsName())
                SeedKnownCompositeLeaves(arg, to->right);

    if (Postfix *from = call->AsPostfix())
        if (Postfix *to = rewriteCall->AsPostfix())
            if (Name *arg = from->left->AsName())
                SeedKnownCompositeLeaves(arg, to->left);

    if (Infix *from = call->AsInfix())
        if (Infix *to = rewriteCall->AsInfix())
        {
            if (Name *left = from->left->AsName())
                SeedKnownCompositeLeaves(left, to->left);
            if (Name *right = from->right->AsName())
                SeedKnownCompositeLeaves(right, to->right);
        }
}


static Tree *BindingArgFromCall(Tree *call, Rewrite *rw, Name *param)
// ----------------------------------------------------------------------------
//   Recover a pattern parameter from a call-shaped binding value
// ----------------------------------------------------------------------------
{
    if (!call || !rw || !param)
        return nullptr;
    Tree *pat = rw->left;
    for (;;)
    {
        Tree *old = pat;
        if (Infix *w = pat->AsInfix())
            if (IsPatternCondition(w) || IsTypeAnnotation(w))
                pat = w->left;
        if (Block *b = pat->AsBlock())
            pat = b->child;
        if (pat == old)
            break;
    }
    if (Prefix *pc = call->AsPrefix())
    {
        if (Prefix *pp = pat->AsPrefix())
            if (Name *pname = pp->right->AsName())
                if (pname->value == param->value)
                    return pc->right;
        if (Infix *ip = pat->AsInfix())
            if (Name *op = ip->left->AsName())
                if (Name *cop = pc->left->AsName())
                    if (op->value == cop->value)
                        if (Name *pname = ip->right->AsName())
                            if (pname->value == param->value)
                                return pc->right;
    }
    if (Infix *ic = call->AsInfix())
    {
        if (Infix *ip = pat->AsInfix())
        {
            if (Name *pname = ip->right->AsName())
                if (pname->value == param->value)
                    return ic->right;
            if (Name *pname = ip->left->AsName())
                if (pname->value == param->value)
                    return ic->left;
        }
    }
    return nullptr;
}


static void RemoveDeadBindingArgFromCall(Tree *&arg, Tree *call,
                                         Rewrite *rw, Name *param)
// ----------------------------------------------------------------------------
//   Replace a call-shaped binding argument with the matching parameter slot
// ----------------------------------------------------------------------------
{
    if (!arg || !call || !rw || !param)
        return;
    if (arg == call)
        if (Tree *fromCall = BindingArgFromCall(call, rw, param))
            arg = fromCall;
}


void CompilerExpression::SeedCandidateBindings(CompilerRewriteCandidate *cand)
// ----------------------------------------------------------------------------
//   Bind pattern parameters while testing rewrite candidate conditions
// ----------------------------------------------------------------------------
{
    if (!cand)
        return;
    for (RewriteBinding &b : cand->bindings)
    {
        if (!b.name || !b.value)
            continue;
        if (function.Known(b.name, CompilerFunction::knowValues))
            continue;
        JIT::Value_p value = nullptr;
        JIT::Type_p  paramTy = nullptr;
        auto         tyIt = function.paramTypes.find(b.name);
        if (tyIt != function.paramTypes.end())
            paramTy = tyIt->second;
        if (Natural *n = b.value->AsNatural())
        {
            if (paramTy == function.compiler.naturalTy)
                value = function.code.IntegerConstant(function.compiler.naturalTy,
                                                      int64_t(n->value));
            else
                value = function.ConstantNatural(n);
        }
        else if (Real *r = b.value->AsReal())
        {
            if (paramTy == function.compiler.realTy)
                value = function.code.FloatConstant(function.compiler.realTy,
                                                    r->value);
            else
                value = function.ConstantReal(r);
        }
        else if (Text *t = b.value->AsText())
            value = function.ConstantText(t);
        else if (Name *vn = b.value->AsName())
            value = function.Known(vn, CompilerFunction::knowValues |
                                        CompilerFunction::knowLocals);
        if (value)
            function.values[b.name] = value;
        record(compiler_expr, "Seed binding %t = %t as %v",
               b.name, b.value, value);
    }
}


JIT::Value_p CompilerExpression::BindingArgValue(Tree *arg)
// ----------------------------------------------------------------------------
//   Evaluate simple binding argument shapes without re-entering calls
// ----------------------------------------------------------------------------
{
    if (Infix *ix = arg->AsInfix())
    {
        if (ix->name == "-")
        {
            if (Name *n = ix->left->AsName())
            {
                JIT::Value_p lhs = function.Known(n,
                                                  CompilerFunction::knowValues |
                                                  CompilerFunction::knowLocals);
                if (lhs)
                {
                    JIT::Value_p rhs = Value(ix->right);
                    if (rhs)
                        return function.code.Sub(lhs, rhs);
                }
            }
        }
    }
    return nullptr;
}


// ============================================================================
//
//    Compile an expression
//
// ============================================================================

CompilerExpression::CompilerExpression(CompilerFunction &function,
                                       bool               tail)
// ----------------------------------------------------------------------------
//   Constructor for a compiler expression
// ----------------------------------------------------------------------------
    : CompilerExpression(function, function.FunctionContext(), tail)
{}


CompilerExpression::CompilerExpression(CompilerFunction &function,
                                       Context          *context)
// ----------------------------------------------------------------------------
//   Constructor for a compiler expression
// ----------------------------------------------------------------------------
    : CompilerExpression(function, context, false)
{}


CompilerExpression::CompilerExpression(CompilerFunction &function,
                                       Context          *context,
                                       bool               tail)
// ----------------------------------------------------------------------------
//   Constructor for a compiler expression
// ----------------------------------------------------------------------------
    : function(function),
      computed(),
      context(context),
      inTailPosition(tail)
{}


JIT::Value_p CompilerExpression::Evaluate(Tree *expr, bool force)
// ----------------------------------------------------------------------------
//   For top-level expressions, make sure we evaluate closures
// ----------------------------------------------------------------------------
{
    JIT::Value_p result = expr->Do(this);
    if (result)
    {
        CompilerUnit &unit = function.unit;
        JITBlock &code = function.code;
        JIT::Type_p mtype = code.Type(result);
        function.ValueMachineType(expr, mtype);
        if (force && unit.IsClosureType(mtype))
        {
            result = function.InvokeThickClosure(expr, result, mtype);
            mtype = code.Type(result);
            function.ValueMachineType(expr, mtype);
        }
    }
    return result;
}


JIT::Value_p CompilerExpression::Do(Natural *what)
// ----------------------------------------------------------------------------
//   Compile an natural constant
// ----------------------------------------------------------------------------
{
    Compiler &compiler = function.compiler;
    JITBlock &code = function.code;
    return code.IntegerConstant(compiler.naturalTy, int64_t(what->value));
}


JIT::Value_p CompilerExpression::Do(Real *what)
// ----------------------------------------------------------------------------
//   Compile a real constant
// ----------------------------------------------------------------------------
{
    Compiler &compiler = function.compiler;
    JITBlock &code = function.code;
    return code.FloatConstant(compiler.realTy, what->value);
}


JIT::Value_p CompilerExpression::Do(Text *what)
// ----------------------------------------------------------------------------
//   Compile a text constant
// ----------------------------------------------------------------------------
{
    Compiler &compiler = function.compiler;
    JITBlock &code = function.code;
    if (what->IsCharacter())
    {
        char c = what->value.length() ? what->value[0] : 0;
        return code.IntegerConstant(compiler.characterTy, c);
    }
    return code.TextConstant(compiler.charPtrTy, what->value);
}


JIT::Value_p CompilerExpression::Do(Name *what)
// ----------------------------------------------------------------------------
//   Compile a name
// ----------------------------------------------------------------------------
{
    CompilerUnit &unit     = function.unit;
    JITBlock     &code     = function.code;
    Scope_g       where;
    Rewrite_g     rewrite;
    Tree         *existing = context->Bound(what, true, &rewrite, &where);
    record(boxed_assign, "Do(Name) %t existing %t rewrite %t where %t",
           what, existing, rewrite, where);

    if (!existing || !rewrite)
    {
        Context *unitCtx = nullptr;
        if (function.unit.types)
            unitCtx = function.unit.types->TypesContext();
        if (unitCtx && unitCtx != context)
        {
            Scope_g unitWhere = nullptr;
            Rewrite_g unitRewrite = nullptr;
            Tree *unitExisting = unitCtx->Bound(what, true,
                                                &unitRewrite, &unitWhere);
            if (unitExisting && unitRewrite)
            {
                existing = unitExisting;
                rewrite = unitRewrite;
                where = unitWhere;
                record(boxed_assign,
                       "Do(Name) %t recovered from unit context: %t %t %t",
                       what, existing, rewrite, where);
            }
        }
    }

    if (!existing && !rewrite)
    {
        if (JIT::Value_p known =
            function.Known(what,
                           CompilerFunction::knowLocals |
                           CompilerFunction::knowValues))
        {
            JIT::Type_p kty = code.Type(known);
            if (kty != function.compiler.treePtrTy)
            {
                record(boxed_assign,
                       "Do(Name) %t context miss, use local/known value %v type %T",
                       what, known, kty);
                return UnwrapThickIfNeeded(function, unit, code, what, known);
            }
            record(boxed_assign,
                   "Do(Name) %t context miss, ignore local/known raw tree value %v",
                   what, known);
        }
    }
    if (!existing || !rewrite)
        record(boxed_assign, "Do(Name) invariant violation existing %t rw %t",
               existing, rewrite);
    assert(existing || !"Type checking didn't realize a name is missing");
    assert(rewrite || !"Type checking didn't keep rewrite for name binding");
    Tree *from = PatternBase(rewrite->left);
    if (JIT::Value_p result = function.Known(from))
    {
        record(boxed_assign,
               "Name %t resolved from known pattern %t => %v",
               what, from, result);
        return UnwrapThickIfNeeded(function, unit, code, what, result);
    }
    if (where == context->Symbols())
    {
        record(boxed_assign, "Name %t local symbols scope via pattern %t",
               what, from);
    }

    // Check true and false values
    if (existing == xl_true)
        return code.BooleanConstant(true);
    if (existing == xl_false)
        return code.BooleanConstant(false);

    // Check if it is a global
    if (JIT::Value_p global = unit.Global(existing))
    {
        record(boxed_assign, "Name %t resolved from global existing %t => %v",
               what, existing, global);
        return UnwrapThickIfNeeded(function, unit, code, what, global);
    }
    if (JIT::Value_p global = unit.Global(from))
    {
        record(boxed_assign, "Name %t resolved from global pattern %t => %v",
               what, from, global);
        return UnwrapThickIfNeeded(function, unit, code, what, global);
    }

    JIT::Value_p result = DoCall(what, true);
    if (!result)
        result = Value(existing);
    record(boxed_assign, "Name %t fallback result %v via existing %t",
           what, result, existing);

    return UnwrapThickIfNeeded(function, unit, code, what, result);
}


JIT::Value_p CompilerExpression::Do(Infix *infix)
// ----------------------------------------------------------------------------
//   Compile infix expressions
// ----------------------------------------------------------------------------
{
    // Sequences
    if (IsSequence(infix))
    {
        bool savedTail = inTailPosition;
        inTailPosition = false;
        JIT::Value_p left = Evaluate(infix->left, true);
        inTailPosition = savedTail;
        JIT::Value_p right = Evaluate(infix->right, true);
        if (right)
            return right;
        if (left)
            return left;
        return function.ConstantTree(xl_nil);
    }

    // Type casts - REVISIT: may need to do some actual conversion
    if (IsTypeAnnotation(infix))
        return infix->left->Do(this);

    // Declarations: it's too early to define a function just yet,
    // because we don't have the actual argument types.
    if (IsDefinition(infix))
        return function.ConstantTree(infix);

    // Assignment
    if (IsAssignment(infix))
        return DoAssignment(infix);

    // General case: expression
    return DoCall(infix);
}


JIT::Value_p CompilerExpression::Do(Prefix *what)
// ----------------------------------------------------------------------------
//   Compile prefix expressions
// ----------------------------------------------------------------------------
{
    if (Name *name = what->left->AsName())
    {
        if (name->value == "data" || name->value == "extern")
            return function.ConstantTree(what);

        if (name->value == "builtin")
        {
            // This is a builtin, find if we write to code or data
            Tree *builtin = what->right;
            Name *name = builtin->AsName();
            if (!name)
            {
                Ooops("Malformed primitive $1", name);
                return function.ConstantTree(builtin);
            }

            // Take args list for current function as input
            JIT::Values args;
            JIT::Function_p fn = function.Function();
            JITArguments inputs(fn);
            for (size_t i = 0; i < inputs.Count(); i++)
            {
                JIT::Value_p input = *inputs++;
                args.push_back(input);
            }

            // Call the primitive (effectively creating a wrapper for it)
            text op = name->value;
            uint sz = args.size();
            JIT::Value_p *a = &args[0];
            return function.Primitive(what, op, sz, a);
        }
    }

    return DoCall(what);
}


JIT::Value_p CompilerExpression::Do(Postfix *what)
// ----------------------------------------------------------------------------
//   Compile postfix expressions
// ----------------------------------------------------------------------------
{
    return DoCall(what);
}


JIT::Value_p CompilerExpression::Do(Block *block)
// ----------------------------------------------------------------------------
//   Compile blocks
// ----------------------------------------------------------------------------
{
    return block->child->Do(this);
}


JIT::Value_p CompilerExpression::DoCall(Tree *call, bool mayfail)
// ----------------------------------------------------------------------------
//   Compile expressions into calls for the right expression
// ----------------------------------------------------------------------------
{
    JIT::Value_p result = nullptr;

    record(compiler_expr, "Call %t", call);
    CompilerTypes *types = function.types;
    Tree *rewriteCall = call;
    CompilerRewriteCalls *rc = types->TreeRewriteCalls(rewriteCall);
    if (!rc)
    {
        // Top-level TypeAnalysis may not have walked nested rewrite bodies
        // (e.g. builtins). Infer the call here so rcalls exist for codegen.
        types->Type(rewriteCall);
        rc = types->TreeRewriteCalls(rewriteCall);
    }
    ApplyLazyBindingPeel(function.unit.lazyBindingSource,
                         types,
                         context,
                         call,
                         rewriteCall,
                         rc);
    if (rewriteCall != call)
    {
        SeedPeeledKnownValues(call, rewriteCall);
        record(closures, "DoCall rewrote %t -> %t", call, rewriteCall);
    }
    record(types_calls, "Looking up %t in %p: got %p", rewriteCall, types, rc);
    if (mayfail && !rc)
        return nullptr;
    if (!rc)
    {
        Ooops("No operator matches $1", call);
        return nullptr;
    }

    // Optimize the frequent case where we have a single call candidate
    uint i, max = rc->Size();
    record(compiler_expr, "Call %t has %u candidates", rewriteCall, max);
    if (max == 1)
    {
        // We now evaluate in that rewrite's type system
        CompilerRewriteCandidate* cand = rc->Candidate(0);
        if (cand->Unconditional())
        {
            result = DoRewrite(rewriteCall, (CompilerRewriteCandidate *) cand);
            if (!result && function.code.IsTerminated())
                return nullptr;
            return result;
        }
    }
    else if (max == 0)
    {
        // For Name lookup (mayfail=true), no rewrite candidate means
        // "not callable here": let caller fall back to bound value.
        if (mayfail)
            return nullptr;

        // For regular expression compilation, no candidate keeps tree shape.
        result = function.BoxedTree(rewriteCall);
        return result;
    }
    // More general case: we need to generate expression reduction
    JITBlock &code = function.code;
    JITBlock isDone(code, "done");
    JIT::Type_p storageType = function.ValueMachineType(rewriteCall, true);
    JIT::Value_p storage = nullptr;
    if (storageType)
        storage = function.NeedStorage(rewriteCall, storageType);
    Compiler &compiler = function.compiler;
    auto NormalizeResult = [&](JIT::Value_p value) -> JIT::Value_p
    {
        if (!storageType)
        {
            storageType = code.Type(value);
            function.ValueMachineType(rewriteCall, storageType);
            storage = function.NeedStorage(rewriteCall, storageType);
        }
        return function.Autobox(rewriteCall, value, storageType);
    };

    for (i = 0; i < max; i++)
    {
        // Now evaluate in that candidate's type system
        CompilerRewriteCandidate *cand = rc->Candidate(i);
        value_map savedValues = function.values;
        SeedCandidateBindings(cand);
        JIT::Value_p condition = nullptr;

        // Perform tree-kind tests to check if this candidate is valid
        for (RewriteTypeCheck &tc : cand->typechecks)
        {
            JIT::Value_p value = Value(tc.value);
            value = function.Autobox(tc.value, value, compiler.treePtrTy);
            JIT::Value_p cast = function.CallTypeCheck(tc.type, value);
            JIT::Value_p compare = code.IsNullPointer(cast, "cast");
            record(compiler_expr, "Type test for %t for value %t type %t: %v",
                   call, tc.value, tc.type, compare);
            if (condition)
                condition = code.And(condition, compare);
            else
                condition = compare;
        } // Types

        // Perform the tests to check if this candidate is valid
        for (RewriteCondition &t : cand->conditions)
        {
            JIT::Value_p compare = Compare(t.value, t.test);
            record(compiler_expr, "Condition test for %t candidate %u: %v",
                   call, i, compare);
            if (condition)
                condition = code.And(condition, compare);
            else
                condition = compare;
        }

        if (condition)
        {
            JITBlock isBad(code, "bad");
            JITBlock isGood(code, "good");
            code.IfBranch(condition, isGood, isBad);
            code.SwitchTo(isGood);
            value_map saveComputed = computed;

            // REVISIT: Insert cast of types here

            result = DoRewrite(rewriteCall, (CompilerRewriteCandidate *) cand);
            computed = saveComputed;
            result = NormalizeResult(result);
            record(compiler_expr, "Call %t candidate %u is conditional: %v",
                   rewriteCall, i, result);
            code.Store(result, storage);
            code.Branch(isDone);
            code.SwitchTo(isBad);
            function.values = savedValues;
        }
        else
        {
            // If this particular call was unconditional, we are done
            result = DoRewrite(rewriteCall, (CompilerRewriteCandidate *) cand);
            function.values = savedValues;
            if (!result && function.code.IsTerminated())
                return nullptr;
            result = NormalizeResult(result);
            code.Store(result, storage);
            code.Branch(isDone);
            code.SwitchTo(isDone);
            result = code.Load(storageType, storage);
            return result;
        }
    }

    // The final call to xl_form_error if nothing worked
    result = function.CallFormError(rewriteCall);
    result = NormalizeResult(result);
    code.Store(result, storage);
    code.Branch(isDone);
    code.SwitchTo(isDone);
    result = code.Load(storageType, storage);
    record(compiler_expr, "No match for call %t, inserted form error: %v",
           call, result);
    return result;
}


JIT::Value_p CompilerExpression::DoRewrite(Tree *call,
                                           CompilerRewriteCandidate *cand)
// ----------------------------------------------------------------------------
//   Generate code for a particular rewwrite candidate
// ----------------------------------------------------------------------------
{
    Rewrite *rw = cand->rewrite;
    JIT::Value_p result = nullptr;
    JITBlock &code = function.code;
    Compiler &compiler = function.compiler;

    record(compiler_expr, "Rewrite: %t", rw);

    bool savedTail = inTailPosition;
    inTailPosition = false;

    struct RestoreTailPosition
    {
        CompilerExpression *expr;
        bool                saved;
        RestoreTailPosition(CompilerExpression *e, bool s)
            : expr(e), saved(s)
        {}
        ~RestoreTailPosition()
        {
            expr->inTailPosition = saved;
        }
    } restoreTail(this, savedTail);

    struct ClearThickLazyArgOverride
    {
        CompilerRewriteCandidate *rc;
        explicit ClearThickLazyArgOverride(CompilerRewriteCandidate *c)
            : rc(c)
        {}
        ~ClearThickLazyArgOverride()
        {
            rc->thickLazyArgOverride.clear();
        }
    } clearThickLazyArg(cand);
    struct RestoreLazyBindingSource
    {
        lazy_binding_map &target;
        lazy_binding_map saved;
        explicit RestoreLazyBindingSource(lazy_binding_map &m)
            : target(m), saved(m)
        {}
        ~RestoreLazyBindingSource()
        {
            target.swap(saved);
        }
    } restoreLazyBindings(function.unit.lazyBindingSource);

    // Evaluate parameters
    JIT::Values args;
    RewriteBindings &bnds = cand->bindings;
    CompilerTypes   *btypes = (CompilerTypes *) cand->BindingTypes();
    CompilerTypes   *vtypes = (CompilerTypes *) cand->ValueTypes();
    CompilerTypes::Decl rwcat = CompilerTypes::RewriteCategory(cand);
    bool lazyBindings = (rwcat == CompilerTypes::Decl::NORMAL);
    record(closures, "DoRewrite call %t rw %t category %u lazy %u",
           call, rw, uint(rwcat), uint(lazyBindings));
    Tree *rhs = rw->right;
    std::vector<JIT::Type_p> thickOverrides;
    thickOverrides.reserve(bnds.size());
    auto NormalizeVoidResult = [&](JIT::Value_p value) -> JIT::Value_p
    {
        if (value && code.Type(value)->isVoidTy())
        {
            record(closures, "Rewrite %t normalizes void result to nil", rw);
            return function.ConstantTree(xl_nil);
        }
        return value;
    };
    for (RewriteBinding &b : bnds)
    {
        Tree        *arg   = b.value;
        Tree        *argtype = vtypes->ValueType(arg);
        JIT::Value_p value = nullptr;
        JIT::Type_p  sigOverride = nullptr;
        bool keepLazyShape = ArgWantsLazyThickClosure(arg);
        if (lazyBindings && !keepLazyShape)
        {
            if (Name *argName = arg->AsName())
            {
                Scope_g argWhere = nullptr;
                Rewrite_g argRewrite = nullptr;
                if (context->Bound(argName, true, &argRewrite, &argWhere) &&
                    argRewrite &&
                    argWhere == context->Symbols())
                {
                    keepLazyShape = true;
                    record(closures,
                           "Binding %t keeps lazy local name %t",
                           b.name, argName);
                }
            }
        }
        record(closures,
               "Binding %t arg %t argtype %t keepLazyShape %u",
               b.name, arg, argtype, uint(keepLazyShape));

        if (lazyBindings)
        {
            if (rhs && b.name &&
                !RewriteBodyReferencesName(rhs, b.name->value))
            {
                value = UnusedRewriteBindingValue(code,
                                                   compiler.closureValueTy);
                sigOverride = compiler.closureValueTy;
                record(compiler_expr,
                       "Rewrite %t: null thick for unused binding %t",
                       rw, b.name);
                record(closures, "Binding %t unused: null thick", b.name);
            }
            else if (keepLazyShape)
            {
                if (Name *argName = arg->AsName())
                {
                    if (JIT::Value_p snap =
                            function.Known(argName,
                                           CompilerFunction::knowValues |
                                           CompilerFunction::knowLocals))
                    {
                        JIT::Type_p snapTy = code.Type(snap);
                        if (!function.unit.IsClosureType(snapTy))
                        {
                            value = UnwrapThickIfNeeded(function,
                                                        function.unit,
                                                        code,
                                                        arg,
                                                        snap);
                            record(closures,
                                   "Binding %t lazy-shape snapshot %t => %v",
                                   b.name, argName, value);
                        }
                    }
                }
                if (!value)
                {
                    value = function.ThickClosureForExpr(arg);
                    sigOverride = compiler.closureValueTy;
                    record(compiler_expr,
                           "Rewrite %t: thick closure for lazy binding %t",
                           rw, b.name);
                    record(closures, "Binding %t thick closure value %v",
                           b.name, value);
                }
            }
            else
            {
                if (Name *argName = arg->AsName())
                {
                    auto it = function.unit.lazyBindingSource.find(argName->value);
                    if (it != function.unit.lazyBindingSource.end() &&
                        it->second && it->second != arg)
                    {
                        value = Value(it->second);
                        record(closures,
                               "Binding %t eager remap %t -> %t => %v",
                               b.name, argName, it->second, value);
                    }
                    else if (JIT::Value_p known =
                                 function.Known(argName,
                                                CompilerFunction::knowValues |
                                                CompilerFunction::knowLocals))
                    {
                        value = UnwrapThickIfNeeded(function,
                                                    function.unit,
                                                    code,
                                                    arg,
                                                    known);
                        record(closures,
                               "Binding %t eager known name %t => %v",
                               b.name, argName, value);
                    }
                }
                if (!value && b.name)
                    RemoveDeadBindingArgFromCall(arg, call, rw, b.name);
                if (!value)
                    value = BindingArgValue(arg);
                if (!value)
                    value = Value(arg);
                record(closures, "Binding %t eager value %v", b.name, value);
            }
        }
        else
        {
            if (Name *argName = arg->AsName())
            {
                if (JIT::Value_p known =
                        function.Known(argName,
                                       CompilerFunction::knowValues |
                                       CompilerFunction::knowLocals))
                {
                    value = UnwrapThickIfNeeded(function,
                                                function.unit,
                                                code,
                                                arg,
                                                known);
                    record(closures,
                           "Binding %t non-lazy known name %t => %v",
                           b.name, argName, value);
                }
                else
                {
                    auto it = function.unit.lazyBindingSource.find(argName->value);
                    if (it != function.unit.lazyBindingSource.end() &&
                        it->second && it->second != arg)
                    {
                        value = Value(it->second);
                        record(closures,
                               "Binding %t non-lazy remap %t -> %t => %v",
                               b.name, argName, it->second, value);
                    }
                }
            }
            if (!value && b.name)
                RemoveDeadBindingArgFromCall(arg, call, rw, b.name);
            if (!value)
                value = BindingArgValue(arg);
            if (!value)
                value = Value(arg);
            record(closures, "Binding %t non-lazy value %v",
                   b.name, value);
        }
        thickOverrides.push_back(sigOverride);
        args.push_back(value);
        if (lazyBindings && b.name)
        {
            function.unit.lazyBindingSource[b.name->value] = arg;
            record(closures, "Map lazy binding %t -> %t", b.name, arg);
        }

        JIT::Type_p mtype   = function.ValueMachineType(arg, true);
        if (!mtype && value)
        {
            mtype = code.Type(value);
            record(closures,
                   "Binding %t inferred machine type %T from value %v",
                   b.name, mtype, value);
        }
        if (!mtype)
            mtype = compiler.treePtrTy;
        if (!lazyBindings && !btypes->IsPatternType(argtype))
            btypes->AddBoxedType(argtype, mtype);

        record(compiler_expr, "Rewrite %t arg %t value %v", rw, arg, value);
    }
    cand->thickLazyArgOverride.swap(thickOverrides);

    // Check if this is an LLVM builtin
    Tree *builtin = nullptr;
    if (Tree *value = rw->right)
        if (Prefix *prefix = value->AsPrefix())
            if (Name *name = prefix->left->AsName())
                if (name->value == "builtin")
                    builtin = prefix->right;

    if (builtin)
    {
        record(compiler_expr, "Rewrite %t is builtin %t", rw, builtin);
        Name *name = builtin->AsName();
        if (!name)
        {
            Ooops("Malformed primitive $1", builtin);
            result = function.CallFormError(builtin);
            record(compiler_expr,
                   "Rewrite %t is malformed builtin %t: form error %v",
                   rw, builtin, result);
        }
        else
        {
            text op = name->value;
            size_t sz = args.size();
            JIT::Value_p *a = &args[0];
            result = function.Primitive(builtin, op, sz, a);
            result = NormalizeVoidResult(result);
            record(compiler_expr, "Rewrite %t is builtin %t: %v",
                   rw, builtin, result);
        }
    }
    else if (JIT::Function_p fn = function.Compile(call, cand, args))
    {
        if (savedTail && fn == function.Function() && function.loopHead)
        {
            for (size_t bi = 0; bi < cand->bindings.size(); bi++)
            {
                RewriteBinding &b = cand->bindings[bi];
                if (!b.name || bi >= args.size())
                    continue;
                auto it = function.paramStorage.find(b.name);
                if (it != function.paramStorage.end())
                    code.Store(args[bi], it->second);
            }
            code.Branch(function.loopHead);
            result = nullptr;
            record(compiler_expr, "Rewrite %t tail-jump to loop %v", rw, fn);
        }
        else
        {
            result = code.Call(fn, args);
            result = NormalizeVoidResult(result);
            record(compiler_expr, "Rewrite %t function %v call %v",
                   rw, fn, result);
        }
    }
    else
    {
        record(compiler_error,
               "Could not compile %t for rewrite %t", call, rw);

    }

    // Save the type of the return value
    if (result)
    {
        CompilerTypes *vtypes = cand->ValueTypes();
        Tree *base = vtypes->CodeGenerationType(call);
        JIT::Type_p retTy = code.Type(result);
        if (!vtypes->IsPatternType(base))
            function.AddBoxedType(base, retTy);
        record(compiler_expr, "Transporting type %t (%T) of %t into %p",
               base, retTy, call, vtypes);
    }

    return result;
}


JIT::Value_p CompilerExpression::DoAssignment(Infix *assign)
// ----------------------------------------------------------------------------
//   Store into the storage for the value being assigned to
// ----------------------------------------------------------------------------
{
    Scope_g       where;
    Rewrite_g     rw;
    JITBlock     &code        = function.code;
    Tree         *existing    = context->Bound(assign->left, true, &rw, &where);
    Tree         *binding     = existing;
    if (rw)
        binding = PatternBase(rw->left);
    if (!binding)
        binding = assign->left;
    JIT::Value_p  value       = Evaluate(assign->right);
    JIT::Type_p   storageType = function.ValueMachineType(binding, true);
    if (!storageType)
    {
        storageType = code.Type(value);
        function.ValueMachineType(binding, storageType);
        record(boxed_assign,
               "Assign inferred machine type %T for binding %t from value %v",
               storageType, binding, value);
    }
    JIT::Value_p  storage     = function.NeedStorage(binding, storageType);
    record(boxed_assign,
           "Assign left %t right %t existing %t bind %t rw %t where %t stype %T",
           assign->left, assign->right, existing, binding, rw, where, storageType);
    record(closures, "Assignment %t := %t storage %v value %v",
           assign->left, assign->right, storage, value);
    record(boxed_assign, "Assign store target %v value %v", storage, value);
    code.Store(value, storage);
    return value;
}


JIT::Value_p CompilerExpression::Value(Tree *expr)
// ----------------------------------------------------------------------------
//   Evaluate an expression once
// ----------------------------------------------------------------------------
{
    JIT::Value_p value = computed[expr];
    if (!value)
    {
        record(closures, "Value miss for %t", expr);
        value = Evaluate(expr);
        computed[expr] = value;
        record(closures, "Value computed %t => %v", expr, value);
    }
    else
    {
        record(closures, "Value hit for %t => %v", expr, value);
    }
    return value;
}


JIT::Value_p CompilerExpression::Compare(Tree *valueTree, Tree *testTree)
// ----------------------------------------------------------------------------
//   Perform a comparison between the two values and check if this matches
// ----------------------------------------------------------------------------
{
    JITBlock     &code      = function.code;
    Compiler     &compiler  = function.compiler;
    CompilerUnit &unit      = function.unit;

    if (Name *vt = valueTree->AsName())
        if (Name *tt = testTree->AsName())
            if (vt->value == tt->value)
                return code.BooleanConstant(true);

    JIT::Value_p value     = Value(valueTree);
    JIT::Value_p test      = Value(testTree);
    JIT::Type_p  valueType = code.Type(value);
    JIT::Type_p  testType  = code.Type(test);
    record(closures, "Compare %t (%v:%T) with %t (%v:%T)",
           valueTree, value, valueType, testTree, test, testType);


    // Comparison of boolean values
    if (testType == compiler.booleanTy)
    {
        if (valueType == compiler.treePtrTy ||
            valueType == compiler.nameTreePtrTy)
        {
            value = function.Autobox(valueTree, value, compiler.booleanTy);
            valueType = code.Type(value);
        }
        if (valueType != compiler.booleanTy)
        {
            record(closures, "Compare boolean mismatch %T vs %T",
                   valueType, testType);
            return code.BooleanConstant(false);
        }
        JIT::Value_p cmp = code.ICmpEQ(test, value);
        record(closures, "Compare boolean result %v", cmp);
        return cmp;
    }

    // Comparison of character values
    if (testType == compiler.characterTy)
    {
        if (valueType == compiler.textTreePtrTy)
        {
            value = function.Autobox(valueTree, value, testType);
            valueType = code.Type(value);
        }
        if (valueType != compiler.characterTy)
        {
            record(closures, "Compare char mismatch %T vs %T",
                   valueType, testType);
            return code.BooleanConstant(false);
        }
        JIT::Value_p cmp = code.ICmpEQ(test, value);
        record(closures, "Compare char result %v", cmp);
        return cmp;
    }

    // Comparison of text constants
    if (testType == compiler.textTy || testType == compiler.textPtrTy)
    {
        test = function.Autobox(testTree, test, compiler.charPtrTy);
        testType = test->getType();
    }
    if (testType == compiler.charPtrTy)
    {
        if (valueType == compiler.textTreePtrTy)
        {
            value = function.Autobox(valueTree, value, testType);
            valueType = code.Type(value);
        }
        if (valueType != compiler.charPtrTy)
            return code.BooleanConstant(false);
        value = code.Call(unit.strcmp, test, value);
        test = code.IntegerConstant(code.Type(value), 0);
        value = code.ICmpEQ(value, test);
        return value;
    }

    // Comparison of natural values
    if (testType->isIntegerTy())
    {
        if (valueType == compiler.naturalTreePtrTy)
        {
            value = function.Autobox(valueTree, value, compiler.naturalTy);
            valueType = code.Type(value);
        }
        if (!valueType->isIntegerTy())
            return code.BooleanConstant(false);
        if (valueType != testType)
            value = code.BitCast(value, testType);
        return code.ICmpEQ(test, value);
    }

    // Comparison of floating-point values
    if (testType->isFloatingPointTy())
    {
        if (valueType == compiler.realTreePtrTy)
        {
            value = function.Autobox(valueTree, value, compiler.realTy);
            valueType = code.Type(value);
        }
        if (!valueType->isFloatingPointTy())
            return code.BooleanConstant(false);
        if (valueType != testType)
        {
            if (valueType != compiler.realTy)
            {
                value = code.FPExt(value, compiler.realTy);
                valueType = code.Type(value);
            }
            if (testType != compiler.realTy)
            {
                test = code.FPExt(test, compiler.realTy);
                testType = test->getType();
            }
            if (valueType != testType)
                return code.BooleanConstant(false);
        }
        return code.FCmpOEQ(test, value);
    }

    // Test our special types
    if (testType == compiler.treePtrTy         ||
        testType == compiler.naturalTreePtrTy  ||
        testType == compiler.realTreePtrTy     ||
        testType == compiler.textTreePtrTy     ||
        testType == compiler.nameTreePtrTy     ||
        testType == compiler.blockTreePtrTy    ||
        testType == compiler.infixTreePtrTy    ||
        testType == compiler.prefixTreePtrTy   ||
        testType == compiler.postfixTreePtrTy)
    {
        if (testType != compiler.treePtrTy)
        {
            test = code.BitCast(test, compiler.treePtrTy);
            testType = test->getType();
        }

        // Convert value to a Tree * if possible
        if (valueType->isIntegerTy()                    ||
            valueType->isFloatingPointTy()              ||
            valueType == compiler.charPtrTy             ||
            valueType == compiler.textTy                ||
            valueType == compiler.textPtrTy             ||
            valueType == compiler.naturalTreePtrTy      ||
            valueType == compiler.realTreePtrTy         ||
            valueType == compiler.textTreePtrTy         ||
            valueType == compiler.nameTreePtrTy         ||
            valueType == compiler.blockTreePtrTy        ||
            valueType == compiler.infixTreePtrTy        ||
            valueType == compiler.prefixTreePtrTy       ||
            valueType == compiler.postfixTreePtrTy)
        {
            value = function.Autobox(valueTree, value, compiler.treePtrTy);
            valueType = code.Type(value);
        }

        if (testType != valueType)
            return code.BooleanConstant(false);

        // Call runtime function to perform tree comparison
        return code.Call(unit.xl_same_shape, value, test);
    }

    // Other comparisons fail for now
    return code.BooleanConstant(false);
}

XL_END
