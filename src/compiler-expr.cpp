// ****************************************************************************
//  compiler-expr.cpp                                              XL project
// ****************************************************************************
//
//   File Description:
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

#include "compiler-expr.h"
#include "compiler-unit.h"
#include "compiler-args.h"
#include "types.h"
#include "save.h"
#include "errors.h"
#include "renderer.h"
#include "llvm-crap.h"


RECORDER(compiler_expr, 128, "Expression reduction (compilation of calls)");

XL_BEGIN


// ============================================================================
//
//    Compile an expression
//
// ============================================================================

CompilerExpression::CompilerExpression(CompilerFunction &function)
// ----------------------------------------------------------------------------
//   Constructor for a compiler expression
// ----------------------------------------------------------------------------
    : function(function),
      unit(function.unit),
      compiler(function.compiler),
      code(function.code)
{}



Value_p CompilerExpression::Evaluate(Tree *expr, bool force)
// ----------------------------------------------------------------------------
//   For top-level expressions, make sure we evaluate closures
// ----------------------------------------------------------------------------
{
    Value_p result = expr->Do(this);
    if (result && force)
    {
        Type_p resultTy = JIT::Type(result);
        if (unit.IsClosureType(resultTy))
            result = function.InvokeClosure(result);
    }
    return result;
}




Value_p CompilerExpression::DoInteger(Integer *what)
// ----------------------------------------------------------------------------
//   Compile an integer constant
// ----------------------------------------------------------------------------
{
    return code.IntegerConstant(compiler.integerTy, what->value);
}


Value_p CompilerExpression::DoReal(Real *what)
// ----------------------------------------------------------------------------
//   Compile a real constant
// ----------------------------------------------------------------------------
{
    return code.FloatConstant(compiler.realTy, what->value);
}


Value_p CompilerExpression::DoText(Text *what)
// ----------------------------------------------------------------------------
//   Compile a text constant
// ----------------------------------------------------------------------------
{
    if (what->IsCharacter())
    {
        char c = what->value.length() ? what->value[0] : 0;
        return code.IntegerConstant(compiler.characterTy, c);
    }
    return code.TextConstant(what->value);
}


Value_p CompilerExpression::DoName(Name *what)
// ----------------------------------------------------------------------------
//   Compile a name
// ----------------------------------------------------------------------------
{
    Scope_p    where;
    Rewrite_p  rewrite;
    Context   *context  = function.context;
    Tree      *existing = context->Bound(what, true, &rewrite, &where);
    assert(existing || !"Type checking didn't realize a name is missing");
    Tree *from = RewriteDefined(rewrite->left);
    if (where == context->CurrentScope())
        if (Value_p result = function.Known(from))
            return result;

    // Check true and false values
    if (existing == xl_true)
        return code.BooleanConstant(true);
    if (existing == xl_false)
        return code.BooleanConstant(false);

    // Check if it is a global
    if (Value_p global = unit.Global(existing))
        return global;
    if (Value_p global = unit.Global(from))
        return global;

    // If we are in a context building a closure, record dependency
    if (function.closureTy)
        return function.NeedClosure(from);

    return DoCall(what);
}


Value_p CompilerExpression::DoInfix(Infix *infix)
// ----------------------------------------------------------------------------
//   Compile infix expressions
// ----------------------------------------------------------------------------
{
    // Sequences
    if (infix->name == "\n" || infix->name == ";")
    {
        Value_p left = Evaluate(infix->left, true);
        Value_p right = Evaluate(infix->right, true);
        if (right)
            return right;
        if (left)
            return left;
        return function.ConstantTree(xl_nil);
    }

    // Type casts - REVISIT: may need to do some actual conversion
    if (infix->name == ":" || infix->name == "as")
    {
        return infix->left->Do(this);
    }

    // Declarations: it's too early to define a function just yet,
    // because we don't have the actual argument types.
    if (infix->name == "is")
        return function.ConstantTree(infix);

    // General case: expression
    return DoCall(infix);
}


Value_p CompilerExpression::DoPrefix(Prefix *what)
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
            Values args;
            Function_p fn = function.Function();
            uint i, max = fn->arg_size();
            Function::arg_iterator arg = fn->arg_begin();
            for (i = 0; i < max; i++)
            {
                Value_p inputArg = &*arg++;
                args.push_back(inputArg);
            }

            // Call the primitive (effectively creating a wrapper for it)
            text op = name->value;
            uint sz = args.size();
            Value_p *a = &args[0];
            return function.Primitive(what, op, sz, a);
        }
    }
    return DoCall(what);
}


Value_p CompilerExpression::DoPostfix(Postfix *what)
// ----------------------------------------------------------------------------
//   Compile postfix expressions
// ----------------------------------------------------------------------------
{
    return DoCall(what);
}


Value_p CompilerExpression::DoBlock(Block *block)
// ----------------------------------------------------------------------------
//   Compile blocks
// ----------------------------------------------------------------------------
{
    return block->child->Do(this);
}


Value_p CompilerExpression::DoCall(Tree *call)
// ----------------------------------------------------------------------------
//   Compile expressions into calls for the right expression
// ----------------------------------------------------------------------------
{
    Value_p result = nullptr;

    record(compiler_expr, "Call %t", call);
    Types *types = unit.types;
    rcall_map &rcalls = types->TypesRewriteCalls();
    rcall_map::iterator found = rcalls.find(call);
    record(types_calls, "Looking up %t in %p (%u entries)",
           call, types, rcalls.size());
    assert(found != rcalls.end() || !"Type analysis botched on expression");

    RewriteCalls *rc = (*found).second;
    RewriteCandidates &calls = rc->candidates;

    // Optimize the frequent case where we have a single call candidate
    uint i, max = calls.size();
    record(compiler_expr, "Call %t has %u candidates", call, max);
    if (max == 1)
    {
        // We now evaluate in that rewrite's type system
        RewriteCandidate* cand = calls[0];
        Save<Types_p> saveTypes(unit.types, cand->btypes);
        if (cand->Unconditional())
        {
            result = DoRewrite(call, cand);
            return result;
        }
    }
    else if (max == 0)
    {
        // If it passed type check and there is no candidate, return tree as is
        result = function.BoxedTree(call);
        return result;
    }

    // More general case: we need to generate expression reduction
    JITBlock &code = function.code;
    JITBlock isDone(code, "done");
    Value_p storage = function.NeedStorage(call);
    Type_p storageType = function.ValueMachineType(call);

    for (i = 0; i < max; i++)
    {
        // Now evaluate in that candidate's type system
        RewriteCandidate *cand = calls[i];
        Save<typed_value_map> saveComputed(computed, computed);
        Save<Types_p> saveTypes(unit.types, cand->btypes);
        Value_p condition = nullptr;

        // Perform tree-kind tests to check if this candidate is valid
        for (RewriteKind &k : cand->kinds)
        {
            Value_p value = Evaluate(k.value);
            Type_p type = JIT::Type(value);

            if (type == compiler.treePtrTy        ||
                type == compiler.integerTreePtrTy ||
                type == compiler.realTreePtrTy    ||
                type == compiler.textTreePtrTy    ||
                type == compiler.nameTreePtrTy    ||
                type == compiler.blockTreePtrTy   ||
                type == compiler.prefixTreePtrTy  ||
                type == compiler.postfixTreePtrTy ||
                type == compiler.infixTreePtrTy)
            {
                // Unboxed value, check the dynamic kind
                Value_p tagPtr = code.StructGEP(value, TAG_INDEX, "tagp");
                Value_p tag = code.Load(tagPtr, "tag");
                Type_p  tagType = JIT::Type(tag);
                Value_p mask = code.IntegerConstant(tagType, Tree::KINDMASK);
                Value_p kindValue = code.And(tag, mask, "kind");
                Value_p kindCheck = code.IntegerConstant(tagType, k.test);
                Value_p compare = code.ICmpEQ(kindValue, kindCheck);
                record(compiler_expr, "Kind test for %t candidate %u: %v",
                       call, i, compare);

                if (condition)
                    condition = code.And(condition, compare);
                else
                    condition = compare;
            }
            else
            {
                // If we have boxed values, it was a static check
                if (!((type->isIntegerTy()        && k.test == INTEGER)   ||
                      (type->isFloatingPointTy()  && k.test == REAL)      ||
                      (type == compiler.charPtrTy && k.test == TEXT)))
                {
                    Ooops("Invalid type combination in kind for $1", call);
                }
            }
        } // Kinds

        // Perform the tests to check if this candidate is valid
        for (RewriteCondition &t : cand->conditions)
        {
            Value_p compare = Compare(t.value, t.test);
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
            typed_value_map saveComputed = computed;
            result = DoRewrite(call, cand);
            computed = saveComputed;
            result = function.Autobox(call, result, storageType);
            record(compiler_expr, "Call %t candidate %u is conditional: %v",
                   call, i, result);
            code.Store(result, storage);
            code.Branch(isDone);
            code.SwitchTo(isBad);
        }
        else
        {
            // If this particular call was unconditional, we are done
            result = DoRewrite(call, cand);
            result = function.Autobox(call, result, storageType);
            code.Store(result, storage);
            code.Branch(isDone);
            code.SwitchTo(isDone);
            result = code.Load(storage);
            return result;
        }
    }

    // The final call to xl_form_error if nothing worked
    function.CallFormError(call);
    code.Branch(isDone);
    code.SwitchTo(isDone);
    result = code.Load(storage);
    record(compiler_expr, "No match for call %t, inserted form error: %v",
           call, result);
    return result;
}


Value_p CompilerExpression::DoRewrite(Tree *call, RewriteCandidate *cand)
// ----------------------------------------------------------------------------
//   Generate code for a particular rewwrite candidate
// ----------------------------------------------------------------------------
{
    Rewrite *rw = cand->rewrite;
    Value_p result = nullptr;
    Save<Types_p> saveTypes(unit.types, cand->btypes);

    record(compiler_expr, "Rewrite: %t", rw);

    // Evaluate parameters
    Values args;
    RewriteBindings &bnds = cand->bindings;
    for (RewriteBinding &b : bnds)
    {
        Tree *tree = b.value;
        if (Value_p closure = b.Closure(function))
        {
            record(compiler_expr, "Rewrite %t arg %t closure %v",
                   rw, tree, closure);
            args.push_back(closure);
        }
        else if (Value_p value = Value(tree))
        {
            args.push_back(value);
            Type_p mtype = JIT::Type(value);
            function.ValueMachineType(b.name, mtype);
            function.ValueMachineType(b.value, mtype);
            if (unit.IsClosureType(mtype))
                b.closure = value;
            record(compiler_expr, "Rewrite %t arg %t value %v machine type %T",
                   rw, tree, value, mtype);
        }
        else
        {
            record(compiler_expr, "Rewrite %t arg %t not found", rw, tree);
        }
    }

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
            uint sz = args.size();
            Value_p *a = &args[0];
            result = function.Primitive(builtin, op, sz, a);
            record(compiler_expr, "Rewrite %t is builtin %t: %v",
                   rw, builtin, result);
        }
    }
    else
    {
        Value_p fn = function.Compile(call, cand, args);
        if (fn)
            result = code.Call(fn, args);
        record(compiler_expr, "Rewrite %t function %v call %v",
               rw, fn, result);
    }

    // Save the type of the return value
    if (result)
    {
        Types *vtypes = cand->vtypes;
        Tree *base = vtypes->CodegenType(call);
        Type_p retTy = JIT::Type(result);
        unit.types = vtypes;
        function.AddBoxedType(base, retTy);
        record(compiler_expr, "Transporting type %t (%T) of %t into %p",
               base, retTy, call, vtypes);
    }

    return result;
}


Value_p CompilerExpression::Value(Tree *expr)
// ----------------------------------------------------------------------------
//   Evaluate an expression once
// ----------------------------------------------------------------------------
{
    Types *types = unit.types;
    Value_p value = computed[types][expr];
    if (!value)
    {
        value = Evaluate(expr);
        computed[types][expr] = value;
    }
    return value;
}


Value_p CompilerExpression::Compare(Tree *valueTree, Tree *testTree)
// ----------------------------------------------------------------------------
//   Perform a comparison between the two values and check if this matches
// ----------------------------------------------------------------------------
{
    JITBlock &code = function.code;

    if (Name *vt = valueTree->AsName())
        if (Name *tt = testTree->AsName())
            if (vt->value == tt->value)
                return code.BooleanConstant(true);

    Value_p value = Value(valueTree);
    Value_p test = Value(testTree);
    Type_p valueType = JIT::Type(value);
    Type_p testType = JIT::Type(test);

    // Comparison of boolean values
    if (testType == compiler.booleanTy)
    {
        if (valueType == compiler.treePtrTy ||
            valueType == compiler.nameTreePtrTy)
        {
            value = function.Autobox(valueTree, value, compiler.booleanTy);
            valueType = JIT::Type(value);
        }
        if (valueType != compiler.booleanTy)
            return code.BooleanConstant(false);
        return code.ICmpEQ(test, value);
    }

    // Comparison of character values
    if (testType == compiler.characterTy)
    {
        if (valueType == compiler.textTreePtrTy)
        {
            value = function.Autobox(valueTree, value, testType);
            valueType = JIT::Type(value);
        }
        if (valueType != compiler.characterTy)
            return code.BooleanConstant(false);
        return code.ICmpEQ(test, value);
    }

    // Comparison of text constants
    if (testType == compiler.textTy)
    {
        test = function.Autobox(testTree, test, compiler.charPtrTy);
        testType = test->getType();
    }
    if (testType == compiler.charPtrTy)
    {
        if (valueType == compiler.textTreePtrTy)
        {
            value = function.Autobox(valueTree, value, testType);
            valueType = JIT::Type(value);
        }
        if (valueType != compiler.charPtrTy)
            return code.BooleanConstant(false);
        value = code.Call(unit.strcmp, test, value);
        test = code.IntegerConstant(JIT::Type(value), 0);
        value = code.ICmpEQ(value, test);
        return value;
    }

    // Comparison of integer values
    if (testType->isIntegerTy())
    {
        if (valueType == compiler.integerTreePtrTy)
        {
            value = function.Autobox(valueTree, value, compiler.integerTy);
            valueType = JIT::Type(value);
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
            valueType = JIT::Type(value);
        }
        if (!valueType->isFloatingPointTy())
            return code.BooleanConstant(false);
        if (valueType != testType)
        {
            if (valueType != compiler.realTy)
            {
                value = code.FPExt(value, compiler.realTy);
                valueType = JIT::Type(value);
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
        testType == compiler.integerTreePtrTy  ||
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
        if (valueType->isIntegerTy() ||
            valueType->isFloatingPointTy() ||
            valueType == compiler.charPtrTy ||
            valueType == compiler.textTy ||
            valueType == compiler.integerTreePtrTy  ||
            valueType == compiler.realTreePtrTy     ||
            valueType == compiler.textTreePtrTy     ||
            valueType == compiler.nameTreePtrTy     ||
            valueType == compiler.blockTreePtrTy    ||
            valueType == compiler.infixTreePtrTy    ||
            valueType == compiler.prefixTreePtrTy   ||
            valueType == compiler.postfixTreePtrTy)
        {
            value = function.Autobox(valueTree, value, compiler.treePtrTy);
            valueType = JIT::Type(value);
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
