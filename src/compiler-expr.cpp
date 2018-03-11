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
    : function(function), unit(function.unit), jit(function.jit)
{}


Value_p CompilerExpression::DoInteger(Integer *what)
// ----------------------------------------------------------------------------
//   Compile an integer constant
// ----------------------------------------------------------------------------
{
    Compiler *compiler = unit.compiler;
    return ConstantInt::get(compiler.integerTy, what->value);
}


Value_p CompilerExpression::DoReal(Real *what)
// ----------------------------------------------------------------------------
//   Compile a real constant
// ----------------------------------------------------------------------------
{
    Compiler *compiler = unit.compiler;
    return ConstantFP::get(compiler.realTy, what->value);
}


Value_p CompilerExpression::DoText(Text *what)
// ----------------------------------------------------------------------------
//   Compile a text constant
// ----------------------------------------------------------------------------
{
    Compiler *compiler = unit.compiler;
    if (what->IsCharacter())
        return ConstantInt::get(compiler.characterTy,
                                what->value.length() ? what->value[0] : 0);
    Value_p textConstant = compiler.TextConstant(unit.data, what->value);
    return textConstant;
}


Value_p CompilerExpression::DoName(Name *what)
// ----------------------------------------------------------------------------
//   Compile a name
// ----------------------------------------------------------------------------
{
    Scope_p    where;
    Rewrite_p  rewrite;
    Context   *context  = unit.context;
    Tree      *existing = context->Bound(what, true, &rewrite, &where);
    assert(existing || !"Type checking didn't realize a name is missing");
    Tree *from = RewriteDefined(rewrite->left);
    if (where == context->CurrentScope())
        if (Value_p result = unit.Known(from))
            return result;

    // Check true and false values
    if (existing == xl_true)
        return ConstantInt::get(unit.compiler.booleanTy, 1);
    if (existing == xl_false)
        return ConstantInt::get(unit.compiler.booleanTy, 0);

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
        Value_p left = ForceEvaluation(infix->left);
        Value_p right = ForceEvaluation(infix->right);
        if (right)
            return right;
        if (left)
            return left;
        return NULL;
    }

    // Type casts - REVISIT: may need to do some actual conversion
    if (infix->name == ":" || infix->name == "as")
    {
        return infix->left->Do(this);
    }

    // Declarations: it's too early to define a function just yet,
    // because we don't have the actual argument types.
    if (infix->name == "is")
        return NULL;

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
            return NULL;

        if (name->value == "opcode")
        {
            // This is a builtin, find if we write to code or data
            llvm_builder bld = unit.code;
            Tree *builtin = what->right;
            if (Prefix *prefix = builtin->AsPrefix())
            {
                if (Name *name = prefix->left->AsName())
                {
                    if (name->value == "data")
                    {
                        bld = unit.data;
                        builtin = prefix->right;
                    }
                }
            }

            // Take args list for current function as input
            Value_ps args;
            Function *function = unit.function;
            uint i, max = function->arg_size();
            Function::arg_iterator arg = function->arg_begin();
            for (i = 0; i < max; i++)
            {
                Value_p inputArg = &*arg++;
                args.push_back(inputArg);
            }

            // Call the primitive (effectively creating a wrapper for it)
            Name *name = builtin->AsName();
            assert(name || !"Malformed primitive");
            Compiler *compiler = unit.compiler;
            text op = name->value;
            uint sz = args.size();
            Value_p *a = &args[0];
            return compiler.Primitive(*unit, bld, op, sz, a);
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
    Value_p result = NULL;

    record(calls, "Call %t", call);
    rcall_map &rcalls = unit.types->rcalls;
    rcall_map::iterator found = rcalls.find(call);
    assert(found != rcalls.end() || !"Type analysis botched on expression");

    Function *function = unit.function;
    LLVMContext &llvm = unit.llvm;
    RewriteCalls *rc = (*found).second;
    RewriteCandidates &calls = rc->candidates;

    // Optimize the frequent case where we have a single call candidate
    uint i, max = calls.size();
    record(calls, "Call %t has %u candidates", call, max);
    if (max == 1)
    {
        // We now evaluate in that rewrite's type system
        RewriteCandidate &cand = calls[0];
        Save<Types_p> saveTypes(unit.types, cand.types);

        if (cand.Unconditional())
        {
            result = DoRewrite(cand);
            return result;
        }
    }
    else if (max == 0)
    {
        // If it passed type check and there is no candidate, return tree as is
        result = unit.ConstantTree(call);
        return result;
    }

    // More general case: we need to generate expression reduction
    llvm_block isDone = BasicBlock::Create(llvm, "done", function);
    llvm_builder code = unit.code;
    Value_p storage = unit.NeedStorage(call);
    llvm_type storageType = unit.ExpressionMachineType(call);

    for (i = 0; i < max; i++)
    {
        Save<value_map> saveComputed(computed, computed);

        // Now evaluate in that candidate's type system
        RewriteCandidate &cand = calls[i];
        Save<Types_p> saveTypes(unit.types, cand.types);
        Value_p condition = NULL;

        // Perform the tests to check if this candidate is valid
        RewriteConditions &conds = cand.conditions;
        RewriteConditions::iterator t;
        for (t = conds.begin(); t != conds.end(); t++)
        {
            Value_p compare = Compare((*t).value, (*t).test);
            record(calls, "Condition test for %t candidate %u: %v",
                   call, i, compare);
            if (condition)
                condition = code->CreateAnd(condition, compare);
            else
                condition = compare;
        }

        if (condition)
        {
            llvm_block isBad = BasicBlock::Create(llvm, "bad", function);
            llvm_block isGood = BasicBlock::Create(llvm, "good", function);
            code->CreateCondBr(condition, isGood, isBad);
            code->SetInsertPoint(isGood);
            value_map saveComputed = computed;
            result = DoRewrite(cand);
            computed = saveComputed;
            result = unit.Autobox(result, storageType);
            record(calls, "Call %t candidate %u is conditional: %v",
                   call, i, result);
            code->CreateStore(result, storage);
            code->CreateBr(isDone);
            code->SetInsertPoint(isBad);
        }
        else
        {
            // If this particular call was unconditional, we are done
            result = DoRewrite(cand);
            result = unit.Autobox(result, storageType);
            code->CreateStore(result, storage);
            code->CreateBr(isDone);
            code->SetInsertPoint(isDone);
            result = code->CreateLoad(storage);
            return result;
        }
    }

    // The final call to xl_form_error if nothing worked
    unit.CallFormError(call);
    code->CreateBr(isDone);
    code->SetInsertPoint(isDone);
    result = code->CreateLoad(storage);
    record(calls, "No match for call %t, inserted form error: %v",
           call, result);
    return result;
}


Value_p CompilerExpression::DoRewrite(RewriteCandidate &cand)
// ----------------------------------------------------------------------------
//   Generate code for a particular rewwrite candidate
// ----------------------------------------------------------------------------
{
    Rewrite *rw = cand.rewrite;
    Value_p result = NULL;

    record(calls, "Rewrite: %t", rw);

    // Evaluate parameters
    Value_ps args;
    RewriteBindings &bnds = cand.bindings;
    RewriteBindings::iterator b;
    for (b = bnds.begin(); b != bnds.end(); b++)
    {
        Tree *tree = (*b).value;
        if (Value_p closure = (*b).Closure(unit))
        {
            record(calls, "Rewrite %t arg %t closure %v", rw, tree, closure);
            args.push_back(closure);
        }
        else if (Value_p value = Value(tree))
        {
            args.push_back(value);
            llvm_type mtype = value->getType();
            if (unit.compiler.IsClosureType(mtype))
                (*b).closure = value;
            record(calls, "Rewrite %t arg %t value %v", rw, tree, value);
        }
        else
        {
            record(calls, "Rewrite %t arg %t not found", rw, tree);
        }
    }

    // Check if this is an LLVM builtin
    Tree *builtin = NULL;
    if (Tree *value = rw->right)
        if (Prefix *prefix = value->AsPrefix())
            if (Name *name = prefix->left->AsName())
                if (name->value == "opcode")
                    builtin = prefix->right;

    if (builtin)
    {
        record(calls, "Rewrite %t is builtin %t", rw, builtin);
        llvm_builder bld = unit.code;
        if (Prefix *prefix = builtin->AsPrefix())
        {
            if (Name *name = prefix->left->AsName())
            {
                if (name->value == "data")
                {
                    bld = unit.data;
                    builtin = prefix->right;
                }
            }
        }

        Name *name = builtin->AsName();
        if (!name)
        {
            Ooops("Malformed primitive $1", builtin);
            result = unit.CallFormError(builtin);
            record(calls, "Rewrite %t is malformed builtin %t: form error %v",
                   rw, builtin, result);
        }
        else
        {
            Compiler *compiler = unit.compiler;
            text op = name->value;
            uint sz = args.size();
            Value_p *a = &args[0];
            result = compiler.Primitive(*unit, bld, op, sz, a);
            if (!result)
                Ooops("Invalid primitive $1", builtin);
            record(calls, "Rewrite %t is builtin %t: %v", rw, builtin, result);
        }
    }
    else
    {
        Value_p function = unit.Compile(cand, args);
        if (function)
            result = llvm.CreateCall(unit.code, function, args);
        record(calls, "Rewrite %t function %v call %v", rw, function, result);
    }

    return result;
}


Value_p CompilerExpression::Value(Tree *expr)
// ----------------------------------------------------------------------------
//   Evaluate an expression once
// ----------------------------------------------------------------------------
{
    Value_p value = computed[expr];
    if (!value)
    {
        value = expr->Do(this);
        computed[expr] = value;
    }
    return value;
}


Value_p CompilerExpression::Compare(Tree *valueTree, Tree *testTree)
// ----------------------------------------------------------------------------
//   Perform a comparison between the two values and check if this matches
// ----------------------------------------------------------------------------
{
    Unit &u = *unit;
    Compiler &c = *u.compiler;

    if (Name *vt = valueTree->AsName())
        if (Name *tt = testTree->AsName())
            if (vt->value == tt->value)
                return ConstantInt::get(c.booleanTy, 1);

    Value_p value = Value(valueTree);
    Value_p test = Value(testTree);
    llvm_type valueType = value->getType();
    llvm_type testType = test->getType();

    llvm_builder code = u.code;

    // Comparison of boolean values
    if (testType == c.booleanTy)
    {
        if (valueType == c.treePtrTy || valueType == c.nameTreePtrTy)
        {
            value = u.Autobox(value, c.booleanTy);
            valueType = value->getType();
        }
        if (valueType != c.booleanTy)
            return ConstantInt::get(c.booleanTy, 0);
        return code->CreateICmpEQ(test, value);
    }

    // Comparison of character values
    if (testType == c.characterTy)
    {
        if (valueType == c.textTreePtrTy)
        {
            value = u.Autobox(value, testType);
            valueType = value->getType();
        }
        if (valueType != c.characterTy)
            return ConstantInt::get(c.booleanTy, 0);
        return code->CreateICmpEQ(test, value);
    }

    // Comparison of text constants
    if (testType == c.textTy)
    {
        test = u.Autobox(test, c.charPtrTy);
        testType = test->getType();
    }
    if (testType == c.charPtrTy)
    {
        if (valueType == c.textTreePtrTy)
        {
            value = u.Autobox(value, testType);
            valueType = value->getType();
        }
        if (valueType != c.charPtrTy)
            return ConstantInt::get(c.booleanTy, 0);
        value = llvm.CreateCall(code, c.strcmp_fn, test, value);
        test = ConstantInt::get(value->getType(), 0);
        value = code->CreateICmpEQ(value, test);
        return value;
    }

    // Comparison of integer values
    if (testType->isIntegerTy())
    {
        if (valueType == c.integerTreePtrTy)
        {
            value = u.Autobox(value, c.integerTy);
            valueType = value->getType();
        }
        if (!valueType->isIntegerTy())
            return ConstantInt::get(c.booleanTy, 0);
        if (valueType != c.integerTy)
            value = code->CreateSExt(value, c.integerTy);
        if (testType != c.integerTy)
            test = code->CreateSExt(test, c.integerTy);
        return code->CreateICmpEQ(test, value);
    }

    // Comparison of floating-point values
    if (testType->isFloatingPointTy())
    {
        if (valueType == c.realTreePtrTy)
        {
            value = u.Autobox(value, c.realTy);
            valueType = value->getType();
        }
        if (!valueType->isFloatingPointTy())
            return ConstantInt::get(c.booleanTy, 0);
        if (valueType != testType)
        {
            if (valueType != c.realTy)
            {
                value = code->CreateFPExt(value, c.realTy);
                valueType = value->getType();
            }
            if (testType != c.realTy)
            {
                test = code->CreateFPExt(test, c.realTy);
                testType = test->getType();
            }
            if (valueType != testType)
                return ConstantInt::get(c.booleanTy, 0);
        }
        return code->CreateFCmpOEQ(test, value);
    }

    // Test our special types
    if (testType == c.treePtrTy         ||
        testType == c.integerTreePtrTy  ||
        testType == c.realTreePtrTy     ||
        testType == c.textTreePtrTy     ||
        testType == c.nameTreePtrTy     ||
        testType == c.blockTreePtrTy    ||
        testType == c.infixTreePtrTy    ||
        testType == c.prefixTreePtrTy   ||
        testType == c.postfixTreePtrTy)
    {
        if (testType != c.treePtrTy)
        {
            test = code->CreateBitCast(test, c.treePtrTy);
            testType = test->getType();
        }

        // Convert value to a Tree * if possible
        if (valueType->isIntegerTy() ||
            valueType->isFloatingPointTy() ||
            valueType == c.charPtrTy ||
            valueType == c.textTy ||
            valueType == c.integerTreePtrTy  ||
            valueType == c.realTreePtrTy     ||
            valueType == c.textTreePtrTy     ||
            valueType == c.nameTreePtrTy     ||
            valueType == c.blockTreePtrTy    ||
            valueType == c.infixTreePtrTy    ||
            valueType == c.prefixTreePtrTy   ||
            valueType == c.postfixTreePtrTy)
        {
            value = u.Autobox(value, c.treePtrTy);
            valueType = value->getType();
        }

        if (testType != valueType)
            return ConstantInt::get(c.booleanTy, 0);

        // Call runtime function to perform tree comparison
        return llvm.CreateCall(code, c.xl_same_shape, value, test);
    }

    // Other comparisons fail for now
    return ConstantInt::get(c.booleanTy, 0);
}

XL_END
