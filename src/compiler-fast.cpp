// ****************************************************************************
//  compiler-fast.cpp                                                XL project
// ****************************************************************************
//
//   File Description:
//
//
//
//
//
//
//
//
//
//
// ****************************************************************************
// (C) 2011-2014,2017,2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2011-2014, Jérôme Forissier <jerome@taodyne.com>
// This software is licensed under the GNU General Public License v3
// ****************************************************************************
//   This file is part of XL.
//
//   XL is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   Foobar is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with XL.  If not, see <https://www.gnu.org/licenses/>.
// ****************************************************************************

#include "compiler-fast.h"
#include "compiler-types.h"
#include "context.h"
#include "save.h"
#include "tree.h"
#include "errors.h"
#include "options.h"
#include "renderer.h"
#include "basics.h"
#include "compiler.h"
#include "runtime.h"
#include "main.h"
#include "tree-clone.h"
#include "llvm-crap.h"
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <algorithm>


// ============================================================================
//
//    Recorders
//
// ============================================================================

RECORDER(statictypes,           16, "Static types in the fast compiler");
RECORDER(adapters,              16, "Array to args adapters");
RECORDER(closure,               16, "Compilation of closures");
RECORDER(closure_warning,       16, "Warnings during compilation of closures");
RECORDER(rewrites,              16, "Compilation of rewrites (fast compiler)");
RECORDER(labels,                16, "Show expressions for generated labels");

XL_BEGIN
// ============================================================================
//
//    Implementation of the fast compiler
//
// ============================================================================
//
//  The implementation is essentially the same as for the optimizing compiler,
//  but with a different translation unit implementation

FastCompiler::FastCompiler(kstring name, unsigned opts, int argc, char **argv)
// ----------------------------------------------------------------------------
//   Constructor for the fast compiler
// ----------------------------------------------------------------------------
    : Compiler(name, opts, argc, argv),
      calls(),
      adapters(),
      closures()
{}


FastCompiler::~FastCompiler()
// ----------------------------------------------------------------------------
//   Destructor for fast compiler
// ----------------------------------------------------------------------------
{
}


Tree * FastCompiler::Evaluate(Scope *scope, Tree *source)
// ----------------------------------------------------------------------------
//   Compile the tree, then run the evaluation function
// ----------------------------------------------------------------------------
//   This is the entry point used to compile a top-level XL program.
//   It will process all the declarations in the program and then compile
//   the rest of the code as a function taking no arguments.
{
    record(compiler, "Fast compiling program %t in scope %t", source, scope);
    if (!source || !scope)
        return nullptr;

    eval_fn code = CompileAll(scope, source);
    record(compiler, "Fast compiled %t in scope %t as %p",
           source, scope, (void *) code);

    if (!code)
    {
        Ooops("Error compiling $1", source);
        return source;
    }

    Tree *result = code(scope, source);
    return result;
}


Tree * FastCompiler::TypeCheck(Scope *, Tree *type, Tree *val)
// ----------------------------------------------------------------------------
//   Compile a type check
// ----------------------------------------------------------------------------
{
    return val;
}



// ============================================================================
//
//    Interface that was previously in struct Symbol
//
// ============================================================================

Tree *FastCompiler::Compile(Scope         *scope,
                            Tree          *source,
                            O1CompileUnit &unit,
                            bool           nullIfBad,
                            bool           keepAlternatives,
                            bool           noData)
// ----------------------------------------------------------------------------
//    Return an optimized version of the source tree, ready to run
// ----------------------------------------------------------------------------
//    keepAlternatives means that we preserve branches that could be statically
//    eliminated. This is used when live-patching values is allowed, e.g. Tao3D
{
    record(compiler, "Compile %t in %t %+s alternatives",
           source, scope, keepAlternatives ? "with" : "without");

    // Record rewrites and data declarations in the current context
    Tree *result = source;
    Context context(scope);

    // Check if there is any code to execute
    bool hasInstructions = context.ProcessDeclarations(result);
    if (hasInstructions)
    {
        // Compile code for that tree
        CompileAction compile(scope, unit, nullIfBad, keepAlternatives, noData);
        result = source->Do(compile);

        // If we didn't compile successfully, report
        if (!result)
        {
            if (nullIfBad)
                return result;
            return Ooops("Couldn't compile $1", source);
        }
    }

    // If we compiled successfully, return the input tree
    return result;
}


eval_fn FastCompiler::CompileAll(Scope *scope,
                                 Tree  *source,
                                 bool   nullIfBad,
                                 bool   keepAlternatives,
                                 bool   noData)
// ----------------------------------------------------------------------------
//   Compile a top-level tree
// ----------------------------------------------------------------------------
//    This associates an eval_fn to the tree, i.e. code that takes a tree
//    as input and returns a tree as output.
//    keepAlternatives is set by CompileCall to avoid eliding alternatives
//    based on the value of constants, so that if we compile
//    (key "X"), we also generate the code for (key "Y"), knowing that
//    CompileCall may change the constant at run-time. The objective is
//    to avoid re-generating LLVM code for each and every call
//    (it's more difficult to avoid leaking memory from LLVM)
{
    record(compiler, "Fast compile all %t in %t", source, scope);

    Errors errors;
    JITModule module(jit, "xl.fast");
    TreeList noParms;
    const bool notClosure = false;
    O1CompileUnit unit (*this, scope, source, noParms, notClosure);
    XL_ASSERT(!unit.IsForwardCall() && "Top-level unit is forward declaration");

    Tree *result = Compile(scope, source, unit,
                           nullIfBad, keepAlternatives, noData);
    if (!result)
        return nullptr;

    eval_fn fn = unit.Finalize(true);
    record(compiler, "Fast compiled %t in %t as %p", source, scope, fn);
    return fn;
}


Tree *FastCompiler::CompileCall(Scope    *scope,
                                text      callee,
                                TreeList &argList,
                                bool      callIt)
// ----------------------------------------------------------------------------
//   Compile a top-level call, reusing calls if possible
// ----------------------------------------------------------------------------
// bool nullIfBad=false, bool cached = true);
{
    uint arity = argList.size();

    // Build key for this call
    text key = "";
    const char keychars[] = "IRTN.[]|";
    std::ostringstream keyBuilder;
    keyBuilder << callee << "@" << (void *) scope << ":";
    for (uint i = 0; i < arity; i++)
        keyBuilder << keychars[argList[i]->Kind()];
    key = keyBuilder.str();

    // Build the call tree
    TreePosition pos = arity ? argList[0]->Position() : Tree::NOWHERE;
    Tree *source = new Name(callee, pos);
    if (arity)
    {
        Tree *args = argList[arity + ~0];
        for (uint a = 1; a < arity; a++)
        {
            Tree *arg = argList[arity + ~a];
            args = new Infix(",", arg, args, pos);
        }
        source = new Prefix(source, args, pos);
    }

    // Check if we already had code for that
    call_map::iterator found = calls.find(key);
    eval_fn code;
    if (found == calls.end())
    {
        // Not compiled yet, create machine code
        JITModule module(jit, "xl.call");
        O1CompileUnit unit (*this, scope, source, argList, false);
        XL_ASSERT(!unit.IsForwardCall() && "A call is a forward call?");

        bool nullIfBad = false;
        bool keepAlternatives = true;
        bool noData = false;
        Tree *compiled = Compile(scope, source, unit,
                                 nullIfBad, keepAlternatives, noData);
        if (!compiled)
            return source;

        // Remember what we had for this call
        code = unit.Finalize(true);
        calls[key] = code;
    }
    else
    {
        code = (*found).second;
    }

    Tree *result = source;
    if (callIt)
    {
        adapter_fn adapt = ArrayToArgsAdapter(arity);
        result = adapt(code, scope, source, (Tree **) &argList[0]);
    }
    return result;
}


adapter_fn FastCompiler::ArrayToArgsAdapter(uint numargs)
// ----------------------------------------------------------------------------
//   Generate code to call a function with N arguments
// ----------------------------------------------------------------------------
//   The generated code serves as an adapter between code that has
//   tree arguments in a C array and code that expects them as an arg-list.
//   For example, it allows you to call foo(Scope *,Tree *src,Tree *a1,Tree *a2)
//   by calling generated_adapter(foo, Scope *Tree *src, Tree *args[2])
{
    record(adapters, "ArrayToArgsAdapter %u args", numargs);

    // Check if we already computed it
    adapter_fn result = adapters[numargs];
    record(adapters, "Existing adapter for %u is %p", numargs, result);
    if (result)
        return result;

    // We need a new independent module for this adapter with the MCJIT
    JITModule module(jit, "xl.array2arg");

    // Generate the function type:
    // Tree *generated(Scope *, native_fn, Tree *, Tree **)
    JIT::Signature parms;
    parms.push_back(evalFnTy);
    parms.push_back(scopePtrTy);
    parms.push_back(treePtrTy);
    parms.push_back(treePtrPtrTy);
    JIT::FunctionType_p fnType = jit.FunctionType(treePtrTy, parms);
    JIT::Function_p adapter = jit.Function(fnType, "xl.adapter");

    // Generate the function type for the called function
    JIT::Signature called;
    called.push_back(scopePtrTy);
    called.push_back(treePtrTy);
    for (uint a = 0; a < numargs; a++)
        called.push_back(treePtrTy);
    JIT::FunctionType_p calledType = jit.FunctionType(treePtrTy, called);
    JIT::PointerType_p calledPtrType = jit.PointerType(calledType);

    // Create the entry for the function we generate
    JITBlock code(jit, adapter, "adapt");

    // Read the arguments from the function we are generating
    JITArguments inputs(adapter);
    JIT::Value_p fnToCall = *inputs++;
    JIT::Value_p contextPtr = *inputs++;
    JIT::Value_p sourceTree = *inputs++;
    JIT::Value_p treeArray = *inputs++;

    // Cast the input function pointer to right type
    JIT::Value_p fnTyped = code.BitCast(fnToCall, calledPtrType, "xl.fnCast");

    // Add source as first argument to output arguments
    JIT::Values outArgs;
    outArgs.push_back (contextPtr);
    outArgs.push_back (sourceTree);

    // Read other arguments from the input array
    for (uint a = 0; a < numargs; a++)
    {
        JIT::Value_p elementPtr = code.ArrayGEP(treeArray, a, "argp");
        JIT::Value_p fromArray = code.Load(elementPtr, "arg");
        outArgs.push_back(fromArray);
    }

    // Call the function
    JIT::Value_p retVal = code.Call(fnTyped, outArgs);

    // Return the result
    code.Return(retVal);

    // Enter the result in the map
    jit.Finalize(adapter);
    record(llvm_code, "Code for ArrayToArgs(%u) is %v", numargs, adapter);
    result = (adapter_fn) jit.ExecutableCode(adapter);
    adapters[numargs] = result;
    record(adapters, "New adapter for %u is %p", numargs, result);

    // And return it to the caller
    return result;
}


eval_fn FastCompiler::ClosureAdapter(uint numtrees)
// ----------------------------------------------------------------------------
//   Check if the compiler has a closure adapter for that size, or build one
// ----------------------------------------------------------------------------
//   We build it with an indirect call so that we generate one closure call
//   subroutine per number of arguments only.
//   The input is a block containing a sequence of infix \n that looks like:
//   {
//      P1 is V1
//      P2 is V2
//      P3 is V3
//      [...]
//      E
//   }
//   where P1..Pn are the parameter names, V1..Vn their values, and E is the
//   original expression to evaluate.
//   The generated function takes the 'code' field of the last infix before E,
//   and calls it using C conventions with arguments (E, V1, V2, V3, ...)
{
    eval_fn result = closures[numtrees];
    if (result)
        return result;

    // We need a new independent module for this adapter with the MCJIT
    JITModule module(jit, "xl.closure");
    JIT::Signature fnsig { scopePtrTy, treePtrTy };
    JIT::FunctionType_p fnty = jit.FunctionType(evalFnTy, fnsig);
    JIT::Function_p function = jit.Function(fnty, "xl.closure");
    JITBlock code(jit, function, "entry");

    // Read input arguments for generated function
    JITArguments args(function);
    JIT::Value_p scopePtr = *args++;
    JIT::Value_p ptr = *args++;

    // Load the target code saved in the tree by xl_new_closure
    JIT::Signature xlccSig { treePtrTy };
    JIT::FunctionType_p xlccTy = jit.FunctionType(evalFnTy, xlccSig);
    JIT::Function_p xl_closure_code = jit.ExternFunction(xlccTy,
                                                         "xl_closure_code");
    JIT::Value_p callCode = code.Call(xl_closure_code, ptr);

    // Build argument list
    JIT::Values argV;
    JIT::Signature signature;
    argV.push_back(scopePtr);   // Pass context pointer
    signature.push_back(scopePtrTy);
    argV.push_back(ptr);        // Self is the closure expression
    signature.push_back(treeTy);

    // Extract child of surrounding block
    JIT::Value_p block = code.BitCast(ptr, blockTreePtrTy);
    ptr = code.StructGEP(block, BLOCK_CHILD_INDEX, "closure_child");
    ptr = code.Load(ptr);

    // Build additional arguments
    for (uint i = 0; i < numtrees; i++)
    {
        // Load the left of the \n which is a decl of the form P->V
        JIT::Value_p infix = code.BitCast(ptr, infixTreePtrTy);
        JIT::Value_p lf = code.StructGEP(infix, LEFT_VALUE_INDEX, "closure_lt");
        JIT::Value_p decl = code.Load(lf);
        decl = code.BitCast(decl, infixTreePtrTy);

        // Load the value V out of [P is V] and pass it as an argument
        JIT::Value_p arg = code.StructGEP(decl,
                                          RIGHT_VALUE_INDEX, "closure_rt");
        arg = code.Load(arg);
        argV.push_back(arg);
        signature.push_back(treeTy);

        // Load the next element in the list
        JIT::Value_p rt = code.StructGEP(infix,
                                         RIGHT_VALUE_INDEX, "closure_next");
        ptr = code.Load(rt);
    }

    // Replace the 'self' argument with the expression sans closure
    argV[1] = ptr;

    // Call the resulting function
    JIT::FunctionType_p fnTy = jit.FunctionType(treeTy, signature, false);
    JIT::PointerType_p fnPtrTy = jit.PointerType(fnTy);
    JIT::Value_p toCall = code.BitCast(callCode, fnPtrTy);
    JIT::Value_p callVal = code.Call(toCall, argV);
    code.Return(callVal);

    // Generate machine code for the function
    jit.Finalize(function);
    result = (eval_fn) jit.ExecutableCode(function);
    closures[numtrees] = result;

    return result;
}


struct FastCompilerInfo : Info
// ----------------------------------------------------------------------------
//   Information about compiler-related data structures
// ----------------------------------------------------------------------------
{
    FastCompilerInfo(Tree *tree): function(0), closure(0), code(nullptr) {}
    ~FastCompilerInfo() {}
    llvm::Function *            function;
    llvm::Function *            closure;
    eval_fn                     code;

    // We must mark builtins in a special way, see bug #991
    bool        IsBuiltin()     { return function && function == closure; }
};


FastCompilerInfo *FastCompiler::Info(Tree *tree, bool create)
// ----------------------------------------------------------------------------
//   Find or create the compiler-related info for a given tree
// ----------------------------------------------------------------------------
{
    FastCompilerInfo *result = tree->GetInfo<FastCompilerInfo>();
    if (!result && create)
    {
        result = new FastCompilerInfo(tree);
        tree->SetInfo<FastCompilerInfo>(result);
    }
    return result;
}


JIT::Function_p FastCompiler::TreeFunction(Tree *tree)
// ----------------------------------------------------------------------------
//   Return the function associated to the tree
// ----------------------------------------------------------------------------
{
    FastCompilerInfo *info = Info(tree);
    return info ? info->function : nullptr;
}


void FastCompiler::SetTreeFunction(Tree *tree, JIT::Function_p function)
// ----------------------------------------------------------------------------
//   Associate a function to the given tree
// ----------------------------------------------------------------------------
{
    FastCompilerInfo *info = Info(tree, true);
    info->function = function;
}


JIT::Function_p FastCompiler::TreeClosure(Tree *tree)
// ----------------------------------------------------------------------------
//   Return the closure associated to the tree
// ----------------------------------------------------------------------------
{
    FastCompilerInfo *info = Info(tree);
    return info ? info->closure : nullptr;
}


void FastCompiler::SetTreeClosure(Tree *tree, JIT::Function_p closure)
// ----------------------------------------------------------------------------
//   Associate a closure to the given tree
// ----------------------------------------------------------------------------
{
    FastCompilerInfo *info = Info(tree, true);
    info->closure = closure;
}


eval_fn FastCompiler::TreeCode(Tree *tree)
// ----------------------------------------------------------------------------
//   Return the code generated compiling the tree as a closure
// ----------------------------------------------------------------------------
{
    FastCompilerInfo *info = Info(tree);
    return info ? info->code : nullptr;
}


void FastCompiler::SetTreeCode(Tree *tree, eval_fn code)
// ----------------------------------------------------------------------------
//   Associate a closure to the given tree
// ----------------------------------------------------------------------------
{
    FastCompilerInfo *info = Info(tree, true);
    info->code = code;
}



// ============================================================================
//
//    Argument matching - Test input arguments against parameters
//
// ============================================================================

Tree *ArgumentMatch::Compile(Tree *source, bool noData)
// ----------------------------------------------------------------------------
//    Compile the source tree, and record we use the value in expr cache
// ----------------------------------------------------------------------------
{
    // Compile the code
    O1CompileUnit &unit = compile.unit;
    if (!unit.IsKnown(source))
    {
        bool nullIfBad = true;
        bool keepAlt = false;
        FastCompiler &compiler = unit.compiler;
        source = compiler.Compile(symbols,
                                  source, unit, nullIfBad, keepAlt, noData);
    }
    else
    {
        // Generate code to evaluate the argument
        Save<bool> nib(compile.nullIfBad, true);
        Save<bool> nod(compile.noDataForms, noData);
        source = source->Do(compile);
    }

    return source;
}


Tree *ArgumentMatch::CompileValue(Tree *source, bool noData)
// ----------------------------------------------------------------------------
//   Compile the source and make sure we evaluate it
// ----------------------------------------------------------------------------
{
    Tree *result = Compile(source, noData);
    if (result)
    {
        if (Name *name = result->AsName())
        {
            O1CompileUnit &unit = compile.unit;
            JIT::BasicBlock_p bb = unit.BeginLazy(name);
            unit.NeedStorage(name);
            unit.CallEvaluate(name);
            unit.EndLazy(name, bb);
        }
    }
    return result;
}


Tree *ArgumentMatch::CompileClosure(Tree *source)
// ----------------------------------------------------------------------------
//    Compile the source tree for lazy evaluation, i.e. wrap in code
// ----------------------------------------------------------------------------
{
    // Compile leaves normally
    if (source->IsLeaf())
        return Compile(source, true);

    // For more complex expression, return a constant tree
    O1CompileUnit &unit = compile.unit;
    unit.ConstantTree(source);

    // Record which elements of the expression are captured from context
    FastCompiler &compiler = unit.compiler;
    EnvironmentScan env(symbols);
    Tree *envOK = source->Do(env);
    if (!envOK)
    {
        Ooops("Internal: what environment in $1?", source);
        return nullptr;
    }

    // Create the parameter list with all imported locals
    TreeList parms, args;
    for (auto &c : env.captured)
    {
        Tree *name = c.first;
        Tree *value = c.second;
        if (!unit.IsKnown(value))
            value = Compile(value, true);
        if (unit.IsKnown(value))
        {
            // This is a local: simply pass it around
            parms.push_back(name);
            args.push_back(value);
        }
        else
        {
            // This is a local 'name' like a pattern definition
            // We don't need to pass these around.
            record(closure_warning, "Name %t not allocated in LLVM", name);
        }
    }

    // Create the compilation unit for the code to enclose
    bool isCallableDirectly = parms.size() == 0;
    O1CompileUnit subUnit(compiler, symbols, source, args, !isCallableDirectly);
    if (!subUnit.IsForwardCall())
    {
        // If there is an error compiling, make sure we report it
        // but only if we attempt to actually evaluate the tree
        if (!compiler.Compile(symbols, source, subUnit, true))
            subUnit.CallTypeError(source);
    }

    // Create a call to xl_new_closure to save the required trees
    if (!isCallableDirectly)
        unit.CreateClosure(source, parms, args, subUnit.function);

    return source;
}


Tree *ArgumentMatch::Do(Tree *)
// ----------------------------------------------------------------------------
//   Default is to return failure
// ----------------------------------------------------------------------------
{
    return nullptr;
}


Tree *ArgumentMatch::Do(Integer *what)
// ----------------------------------------------------------------------------
//   An integer argument matches the exact value
// ----------------------------------------------------------------------------
{
    // If the tested tree is a constant, it must be an integer with same value
    if (test->IsConstant())
    {
        Integer *it = test->AsInteger();
        if (!it)
            return nullptr;
        if (!compile.keepAlternatives)
        {
            if (it->value == what->value)
                return what;
            return nullptr;
        }
    }

    // Compile the test tree
    Tree *compiled = CompileValue(test, true);
    if (!compiled)
        return nullptr;

    // Compare at run-time the actual tree value with the test value
    compile.unit.IntegerTest(compiled, what->value);
    return compiled;
}


Tree *ArgumentMatch::Do(Real *what)
// ----------------------------------------------------------------------------
//   A real matches the exact value
// ----------------------------------------------------------------------------
{
    // If the tested tree is a constant, it must be an integer with same value
    if (test->IsConstant())
    {
        Real *rt = test->AsReal();
        if (!rt)
            return nullptr;
        if (!compile.keepAlternatives)
        {
            if (rt->value == what->value)
                return what;
            return nullptr;
        }
    }

    // Compile the test tree
    Tree *compiled = CompileValue(test, true);
    if (!compiled)
        return nullptr;

    // Compare at run-time the actual tree value with the test value
    compile.unit.RealTest(compiled, what->value);
    return compiled;
}


Tree *ArgumentMatch::Do(Text *what)
// ----------------------------------------------------------------------------
//   A text matches the exact value
// ----------------------------------------------------------------------------
{
    // If the tested tree is a constant, it must be an integer with same value
    if (test->IsConstant())
    {
        Text *tt = test->AsText();
        if (!tt)
            return nullptr;
        if (!compile.keepAlternatives)
        {
            if (tt->value == what->value)
                return what;
            return nullptr;
        }
    }

    // Compile the test tree
    Tree *compiled = CompileValue(test, true);
    if (!compiled)
        return nullptr;

    // Compare at run-time the actual tree value with the test value
    compile.unit.TextTest(compiled, what->value);
    return compiled;
}


Tree *ArgumentMatch::Do(Name *what)
// ----------------------------------------------------------------------------
//    Bind arguments to parameters being defined in the shape
// ----------------------------------------------------------------------------
{
    O1CompileUnit &unit = compile.unit;
    if (!defined)
    {
        // The first name we see must match exactly, e.g. 'sin' in 'sin X'
        defined = what;
        if (Name *nt = test->AsName())
            if (nt->value == what->value)
                return what;
        return nullptr;
    }
    else
    {
        // Check if the name already exists, e.g. 'false' or 'A+A'
        // If it does, we generate a run-time check to verify equality
        if (Tree *existing = argContext.Named(what->value))
        {
            // Check if the test is an identity
            if (Name *nt = test->AsName())
            {
                if (data)
                {
                    if (nt->value == what->value)
                        return what;
                    return nullptr;
                }
            }

            if (existing->Kind() == NAME ||
                existing == argContext.Named(what->value, false))
            {
                // Insert a dynamic tree comparison test
                Tree *testCode = Compile(test, false);
                if (!testCode || !unit.IsKnown(testCode))
                    return nullptr;
                Tree *thisCode = Compile(existing, false);
                if (!thisCode || !unit.IsKnown(thisCode))
                    return nullptr;
                unit.ShapeTest(testCode, thisCode);

                // Return compilation success
                return what;
            }
        }

        // Bind expression to name, not value of expression (create a closure)
        Tree *compiled = CompileClosure(test);
        if (!compiled)
            return nullptr;

        // If first occurence of the name, enter it in symbol table
        Rewrite *rewrite = argContext.Define(what, compiled);
        parms.push_back(PatternBase(rewrite->left));
        args.push_back(compiled);
        return what;
    }
}


Tree *ArgumentMatch::Do(Block *what)
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
        (what->opening == "{" && what->closing == "}") ||
        (what->opening == Block::indent && what->closing == Block::unindent))
    {
        return what->child->Do(this);
    }

    return nullptr;
}


Tree *ArgumentMatch::Do(Infix *what)
// ----------------------------------------------------------------------------
//   Check if we match an infix operator
// ----------------------------------------------------------------------------
{
    O1CompileUnit &unit = compile.unit;

    // Check if we match an infix tree like 'x,y' with a name like 'A'
    if (what->name != ":")
    {
        if (Name *name = test->AsName())
        {
            if (!unit.IsKnown(test))
            {
                if (Tree *value = symbols.Named(name->value))
                {
                    // For non-names, evaluate the expression
                    if (!unit.IsKnown(value))
                    {
                        value = CompileValue(value, false);
                        if (!value)
                            return nullptr;
                    }
                    if (unit.IsKnown(value))
                        test = value;
                }
            }

            if (unit.IsKnown(test))
            {
                // Build an infix tree corresponding to what we extract
                Name *left = new Name("left");
                Name *right = new Name("right");
                Infix *extracted = new Infix(what->name, left, right);

                // Extract the infix parameters from actual value
                unit.InfixMatchTest(test, extracted);

                // Proceed with the infix we extracted to
                // map the remaining args
                test = extracted;
            }
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
                return nullptr;
            test = it->right;
            Tree *rr = what->right->Do(this);
            test = it;
            if (!rr)
                return nullptr;
            return what;
        }
    }

    // Check if we match a type, e.g. 2 vs. 'K : integer'
    if (what->name == ":")
    {
        record(statictypes, "Matching %t against %t", test, what);

        // Check the variable name, e.g. K in example above
        Name *varName = what->left->AsName();
        if (!varName)
        {
            Ooops("Expected a name, got $1 ", what->left);
            return nullptr;
        }

        // Check for types that don't require a type check
        Tree *typeExpr = what->right;
        bool needEvaluation = true;
        bool needRTTypeTest = true;
        bool needTypeExprCompilation = true;
        if (Name *declTypeName = what->right->AsName())
        {
            if (Tree *namedType = symbols.Named(declTypeName->value))
            {
                record(statictypes, "Found type name %t", namedType);

                typeExpr = namedType;
                needTypeExprCompilation = false;
                if (namedType == tree_type)
                    return Do(varName);

                bool isConstantType = (namedType == text_type ||
                                       namedType == integer_type ||
                                       namedType == real_type);
                if (isConstantType)
                {
                    Block *block;
                    while ((block = test->AsBlock()) && block->IsParentheses())
                        test = block->child;
                }
                kind tk = test->Kind();

                // Check built-in types against built-in constants
                if (tk == INTEGER || tk == REAL || tk == TEXT)
                {
                    if (isConstantType)
                    {
                        record(statictypes, "Built-in types and constant");
                        if (namedType == text_type    && tk != TEXT)
                            return nullptr;
                        if (namedType == integer_type && tk != INTEGER)
                            return nullptr;
                        if (namedType == real_type && tk == TEXT)
                            return nullptr;
                        needEvaluation = false;
                        needRTTypeTest = false;
                        record(statictypes, "Constant matches type");
                    }
                    else if (namedType == name_type ||
                             namedType == operator_type ||
                             namedType == boolean_type ||
                             namedType == block_type ||
                             namedType == infix_type ||
                             namedType == prefix_type ||
                             namedType == postfix_type)
                    {
                        record(statictypes, "Structure type mismatch");
                        return nullptr;
                    }

                }

                // Check special cases of symbol and operator
                if (tk == NAME)
                {
                    Name *nameTest = test->AsName();
                    text n = nameTest->value;
                    if (namedType == symbol_type)
                    {
                        bool validSymbol = n.length() && isalpha(n[0]);
                        record(statictypes, "Symbol check: %+s",
                               validSymbol ? "pass" : "fail");
                        if (validSymbol)
                            namedType = name_type;
                        else
                            return nullptr;
                    }
                    if (namedType == operator_type)
                    {
                        bool validOp = n.length() && !isalpha(n[0]);
                        record(statictypes, "Operator check %+s",
                               validOp ? "pass" : "fail");
                        if (validOp)
                            namedType = name_type;
                        else
                            return nullptr;
                    }
                }

                if ((tk != NAME    && namedType == source_type) ||
                    (tk == BLOCK   && namedType == block_type)  ||
                    (tk == INFIX   && namedType == infix_type)  ||
                    (tk == PREFIX  && namedType == prefix_type) ||
                    (tk == POSTFIX && namedType == postfix_type))
                {
                    needEvaluation = false;
                    needRTTypeTest = false;
                    record(statictypes,
                           "No evaluation for static type %t", namedType);
                }
                if (namedType == reference_type)
                {
                    record(statictypes, "Reference evaluation");

                    // Only evaluate local parameters
                    if (tk == NAME)
                    {
                        record(statictypes, "Passing a name against %t", test);
                        Name *name = (Name *) (Tree *) test;
                        Rewrite *rw = symbols.Reference(name, false);
                        if (rw && rw->left == rw->right &&
                            rw->left->Kind() == NAME)
                        {
                            record(statictypes, "Evaluating name %t", varName);
                            return Do(varName);
                        }
                    }

                    // In other case, lazy evaluation, no runtime type test
                    needEvaluation = false;
                    needRTTypeTest = false;

                    record(statictypes,
                           "Lazy name evaluation for %t", varName);
                }
            }
        }

        // Evaluate type expression, e.g. 'integer' in example above
        if (needRTTypeTest)
        {
            if (needTypeExprCompilation)
            {
                typeExpr = Compile(what->right, true);
                if (!typeExpr)
                {
                    record(statictypes, "Invalid type %t", what->right);
                    return nullptr;
                }
            }
            if (typeExpr == tree_type)
            {
                record(statictypes,
                       "Disabling type check for tree type %t", typeExpr);
                needRTTypeTest = false;
            }
        }

        // Compile what we are testing against
        Tree *compiled = test;
        Tree *exprType = nullptr;
        if (needEvaluation)
        {
            record(statictypes, "Need evaluation for %t", test);
            compiled = Compile(compiled, true);
            record(statictypes, "Test %t compiles as %t", test, compiled);
            if (!compiled)
                return nullptr;
            exprType = symbols.Type(compiled);
            record(statictypes, "Type of compiled is %t", exprType);
        }
        else
        {
            record(statictypes, "Return constant tree %t", compiled);
            unit.ConstantTree(compiled);
            kind tk = compiled->Kind();
            switch(tk)
            {
            case INTEGER:       exprType = integer_type;
                if (typeExpr == real_type)
                {
                    record(statictypes, "Promote %t to real", compiled);
                    compiled = new Prefix(real_type, compiled,
                                          compiled->Position());
                    unit.CallInteger2Real(compiled, test);
                }
                break;
            case REAL:          exprType = real_type;    break;
            case TEXT:          exprType = text_type;    break;
            case NAME:          exprType = name_type;    break;
            case BLOCK:         exprType = block_type;   break;
            case PREFIX:        exprType = prefix_type;  break;
            case POSTFIX:       exprType = postfix_type; break;
            case INFIX:         exprType = infix_type;   break;
            }

            record(statictypes,
                   "Type for constant %t is %t", compiled, exprType);
        }

        // Insert a run-time type test
        if (needRTTypeTest)
        {
            record(statictypes,
                   "Runtime type check matching %t to %t", exprType, typeExpr);
            if (typeExpr == real_type && exprType == integer_type)
            {
                record(statictypes, "Promote integer %t to real", compiled);
                compiled = new Prefix(real_type, compiled,
                                      compiled->Position());
                unit.CallInteger2Real(compiled, test);
            }
            else
            {
                if (exprType && exprType != tree_type &&
                    typeExpr && typeExpr != exprType)
                {
                    record(statictypes, "Static type mismatch %t vs %t",
                           exprType, typeExpr);
                    return nullptr;
                }

                if (exprType != typeExpr)
                {
                    record(statictypes,
                           "Dynamic type check %t vs %t", compiled, typeExpr);
                    unit.TypeTest(compiled, typeExpr);
                }
                else
                {
                    record(statictypes, "Static type match");
                }
            }
        }

        // Enter the compiled expression in the symbol table
        Rewrite *rw = argContext.Define(varName->value, compiled);
        argContext.SetType(PatternBase(rw->left), typeExpr);
        argContext.SetType(PatternBase(rw->right), typeExpr);
        record(statictypes, "Entering %t as %t:%t",compiled,varName,typeExpr);
        return what;
    }

    // Otherwise, this is a mismatch
    return nullptr;
}


Tree *ArgumentMatch::Do(Prefix *what)
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
            defined = nullptr;

        test = pt->left;
        Tree *lr = what->left->Do(this);
        test = pt;
        if (!lr)
            return nullptr;
        test = pt->right;
        Tree *rr = what->right->Do(this);
        if (!rr)
        {
            if (Block *br = test->AsBlock())
            {
                test = br->child;
                rr = what->right->Do(this);
            }
        }
        test = pt;
        if (!rr)
            return nullptr;
        if (!defined && defined_infix)
            defined = defined_infix;
        return what;
    }
    return nullptr;
}


Tree *ArgumentMatch::Do(Postfix *what)
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
            return nullptr;
        test = pt->left;
        Tree *lr = what->left->Do(this);
        if (!lr)
        {
            if (Block *br = test->AsBlock())
            {
                test = br->child;
                lr = what->left->Do(this);
            }
        }
        test = pt;
        if (!lr)
            return nullptr;
        return what;
    }
    return nullptr;
}



// ============================================================================
//
//    Environment scan - Identify which names are imported from context
//
// ============================================================================

Tree *EnvironmentScan::Do(Tree *what)
// ----------------------------------------------------------------------------
//   Nothing to do for leaves
// ----------------------------------------------------------------------------
{
    return what;
}


Tree *EnvironmentScan::Do(Name *what)
// ----------------------------------------------------------------------------
//    Check if name is found in context, if so record where we took it from
// ----------------------------------------------------------------------------
{
    Context context(symbols);
    Rewrite_p rewrite;
    Scope_p scope;
    if (Tree *found = context.Bound(what, true, &rewrite, &scope))
        if (Tree *tree = PatternBase(rewrite->left))
            if (Name *name = tree->AsName())
                if (!captured.count(name))
                    captured[name] = found;
    return what;
}


Tree *EnvironmentScan::Do(Block *what)
// ----------------------------------------------------------------------------
//   Parameters in a block are in its child
// ----------------------------------------------------------------------------
{
    what->child->Do(this);
    return what;
}


Tree *EnvironmentScan::Do(Infix *what)
// ----------------------------------------------------------------------------
//   Check if we match an infix operator
// ----------------------------------------------------------------------------
{
    // Test left and right
    what->left->Do(this);
    what->right->Do(this);
    return what;
}


Tree *EnvironmentScan::Do(Prefix *what)
// ----------------------------------------------------------------------------
//   For prefix expressions, simply test left then right
// ----------------------------------------------------------------------------
{
    if (what->left->Kind() != NAME)
        what->left->Do(this);
    what->right->Do(this);
    return what;
}


Tree *EnvironmentScan::Do(Postfix *what)
// ----------------------------------------------------------------------------
//    For postfix expressions, simply test right, then left
// ----------------------------------------------------------------------------
{
    // Order shouldn't really matter here (unlike ParameterMach)
    if (what->right->Kind() != NAME)
        what->right->Do(this);
    what->left->Do(this);
    return what;
}



// ============================================================================
//
//   EvaluateChildren action: Build a non-leaf after evaluating children
//
// ============================================================================

Tree *EvaluateChildren::Do(Integer *what)
// ----------------------------------------------------------------------------
//   Compile integer constants
// ----------------------------------------------------------------------------
{
    return compile.Do(what);
}


Tree *EvaluateChildren::Do(Real *what)
// ----------------------------------------------------------------------------
//   Compile real constants
// ----------------------------------------------------------------------------
{
    return compile.Do(what);
}


Tree *EvaluateChildren::Do(Text *what)
// ----------------------------------------------------------------------------
//   Compile text constants
// ----------------------------------------------------------------------------
{
    return compile.Do(what);
}


Tree *EvaluateChildren::Do(Name *what)
// ----------------------------------------------------------------------------
//   Compile names
// ----------------------------------------------------------------------------
{
    return compile.Do(what, true);
}


Tree *EvaluateChildren::Do(Prefix *what)
// ----------------------------------------------------------------------------
//   Evaluate children of a prefix
// ----------------------------------------------------------------------------
{
    O1CompileUnit &unit = compile.unit;
    unit.ConstantTree(what->left);
    Tree *right = what->right->Do(compile);
    if (!right)
        return nullptr;
    unit.CallFillPrefix(what);
    return what;
}


Tree *EvaluateChildren::Do(Postfix *what)
// ----------------------------------------------------------------------------
//   Evaluate children of a postfix
// ----------------------------------------------------------------------------
{
    O1CompileUnit &unit = compile.unit;
    Tree *left = what->left->Do(compile);
    if (!left)
        return nullptr;
    unit.ConstantTree(what->right);
    unit.CallFillPostfix(what);
    return what;
}


Tree *EvaluateChildren::Do(Infix *what)
// ----------------------------------------------------------------------------
//   Evaluate children, then build an infix
// ----------------------------------------------------------------------------
{
    O1CompileUnit &unit = compile.unit;
    Tree *left = what->left->Do(compile);
    if (!left)
        return nullptr;
    Tree *right = what->right->Do(compile);
    if (!right)
        return nullptr;
    unit.CallFillInfix(what);
    return what;
}


Tree *EvaluateChildren::Do(Block *what)
// ----------------------------------------------------------------------------
//   Evaluate children, then build a new block
// ----------------------------------------------------------------------------
{
    O1CompileUnit &unit = compile.unit;
    Tree *child = what->child->Do(compile);
    if (!child)
        return nullptr;
    unit.CallFillBlock(what);
    return what;
}



// ============================================================================
//
//   Declaration action - Enter all tree rewrites in the current symbols
//
// ============================================================================

Tree *DeclarationAction::Do(Tree *what)
// ----------------------------------------------------------------------------
//   Default is to leave trees alone (for native trees)
// ----------------------------------------------------------------------------
{
    return what;
}


Tree *DeclarationAction::Do(Block *what)
// ----------------------------------------------------------------------------
//   Declarations in a block belong to the child, not to us
// ----------------------------------------------------------------------------
{
    return what->child->Do(this);
}


Tree *DeclarationAction::Do(Infix *what)
// ----------------------------------------------------------------------------
//   Compile built-in operators: \n ; -> and :
// ----------------------------------------------------------------------------
{
    // Check if this is an instruction list
    if (IsSequence(what))
    {
        // For instruction list, string declaration results together
        what->left->Do(this);
        what->right->Do(this);
        return what;
    }

    // Check if this is a rewrite declaration
    if (what->name == "->")
    {
        // Enter the rewrite
        EnterRewrite(what->left, what->right);
        return what;
    }

    return what;
}


Tree *DeclarationAction::Do(Prefix *what)
// ----------------------------------------------------------------------------
//    All prefix operations translate into a rewrite
// ----------------------------------------------------------------------------
{
    // Deal with 'data' declarations and 'load' statements
    if (Name *name = what->left->AsName())
    {
        // Check if there is some stuff that needs to be done at decl time
        // This is used for 'load' and 'use'
        if (eval_fn declarator = MAIN->Declarator(name->value))
            if (Tree *result = declarator(symbols, what))
                return result;

        if (name->value == "data")
        {
            Context context(symbols);
            Name *self = new Name("self", what->right->Position());
            context.Define(what->right, self);
            return what;
        }
    }

    return what;
}


void DeclarationAction::EnterRewrite(Tree *defined,
                                     Tree *definition)
// ----------------------------------------------------------------------------
//   Add a definition in the current context
// ----------------------------------------------------------------------------
{
    Context context(symbols);
    if (definition)
    {
#if CREATE_NAME_FOR_PREFIX
        // When entering 'foo X,Y -> bar', also update the definition of 'foo'
        if (Prefix *prefix = defined->AsPrefix())
        {
            if (Name *left = prefix->left->AsName())
            {
                Infix *redef = new Infix("->",
                                         prefix->right, definition,
                                         prefix->Position());
                symbols->ExtendName(left->value, redef);
            }
        }
#endif // CREATE_NAME_FOR_PREFIX

#if CREATE_NAMES_FOR_POSTFIX_AND_INFIX
        // Same thing for a postfix
        if (Postfix *postfix = defined->AsPostfix())
        {
            if (Name *right = postfix->right->AsName())
            {
                Infix *redef = new Infix("->",
                                         postfix->left, definition,
                                         postfix->Position());
                symbols->ExtendName(right->value, redef);
            }
        }

        // Same thing for an infix, but use X,Y on the left of the rewrite
        if (Infix *infix = defined->AsInfix())
        {
            if (infix->name != "," && infix->name != ";" && infix->name != "\n")
            {
                if (Name *left = infix->left->AsName())
                {
                    if (Name *right = infix->right->AsName())
                    {
                        Infix *comma = new Infix(",", left, right,
                                                 infix->Position());
                        Infix *redef = new Infix("->", comma, definition,
                                                 infix->Position());
                        symbols->ExtendName(infix->name, redef);
                    }
                }
            }
        }
#endif // CREATE_NAMES_FOR_POSTFIX_AND_INFIX
    }

    if (!definition)
        definition = defined;
    context.Define(defined, definition);
}



// ============================================================================
//
//   Compilation action - Generation of "optimized" native trees
//
// ============================================================================

CompileAction::CompileAction(Scope *scope, O1CompileUnit &u,
                             bool nib, bool ka, bool ndf)
// ----------------------------------------------------------------------------
//   Constructor
// ----------------------------------------------------------------------------
    : symbols(scope), unit(u),
      nullIfBad(nib), keepAlternatives(ka), noDataForms(ndf),
      debugRewrites(0)
{}


Tree *CompileAction::Do(Tree *what)
// ----------------------------------------------------------------------------
//   Default is to leave trees alone (for native trees)
// ----------------------------------------------------------------------------
{
    return what;
}


Tree *CompileAction::Do(Integer *what)
// ----------------------------------------------------------------------------
//   Integers evaluate directly
// ----------------------------------------------------------------------------
{
    unit.ConstantInteger(what);
    return what;
}


Tree *CompileAction::Do(Real *what)
// ----------------------------------------------------------------------------
//   Reals evaluate directly
// ----------------------------------------------------------------------------
{
    unit.ConstantReal(what);
    return what;
}


Tree *CompileAction::Do(Text *what)
// ----------------------------------------------------------------------------
//   Text evaluates directly
// ----------------------------------------------------------------------------
{
    unit.ConstantText(what);
    return what;
}


Tree *CompileAction::Do(Name *what)
// ----------------------------------------------------------------------------
//   Normal name evaluation: don't force evaluation
// ----------------------------------------------------------------------------
{
    return Do(what, false);
}


Tree *CompileAction::Do(Name *what, bool forceEval)
// ----------------------------------------------------------------------------
//   Build a unique reference in the context for the entity
// ----------------------------------------------------------------------------
{
    // Lookup rewrite for that name
    Rewrite *rw = symbols.Reference(what, false);
    if (!rw)
    {
        if (nullIfBad)
        {
            unit.ConstantTree(what);
            return what;
        }
        Ooops("Name $1 does not exist", what);
        return nullptr;
    }
    if (Tree *type = symbols.Type(PatternBase(rw->left)))
        symbols.SetType(what, type);

    // Normally, the name should have been declared in ParameterMatch
    Tree *result = rw->right;

    // Try to compile the definition of the name
    TreeList xparms, xargs;
    if (!result->AsName())
    {
        result = CompileRewrite(symbols, result, xparms, xargs);
        if (!result)
            return result;
    }

    // Check if there is code we need to call
    FastCompiler &compiler = unit.compiler;
    JIT::Function_p function = compiler.TreeFunction(result);
    if (function && function != unit.function)
    {
        // Case of "Name -> Foo": Invoke Name
        unit.NeedStorage(what);
        unit.Invoke(what, result, xargs);
        return what;
    }
    else if (forceEval && unit.IsKnown(result))
    {
        unit.CallEvaluate(result);
    }
    else if (unit.IsKnown(result))
    {
        // Case of "Foo(A,B) -> B" with B: evaluate B lazily
        unit.Copy(result, what, false);
        return what;
    }
    else
    {
        // Return the name itself by default
        unit.ConstantTree(result);
        unit.Copy(result, what);
    }

    return result;
}


Tree *CompileAction::Do(Block *what)
// ----------------------------------------------------------------------------
//   Optimize away indent or parenthese blocks, evaluate others
// ----------------------------------------------------------------------------
{
    // If the block only contains an empty name, return that (it's for () )
    if (Name *name = what->child->AsName())
    {
        if (name->value == "")
        {
            unit.ConstantTree(what);
            return what;
        }
    }

    // Evaluate the child
    Tree *result = what->child->Do(this);
    if (result)
    {
        if (unit.IsKnown(result))
            unit.Copy(result, what);
        if (Tree *type = symbols.Type(result))
            symbols.SetType(what, type);
        return result;
    }

    // If evaluating the child failed, see if we have a rewrite that works
    result = Rewrites(what);
    return result;
}


Tree *CompileAction::Do(Infix *what)
// ----------------------------------------------------------------------------
//   Compile built-in operators: \n ; -> and :
// ----------------------------------------------------------------------------
{
    // Check if this is an instruction list
    if (IsSequence(what))
    {
        // For instruction list, string compile results together
        // Force evaluation of names on the left of a sequence
        if (Name *leftAsName = what->left->AsName())
        {
            if (!Do(leftAsName, true))
                return nullptr;
        }
        else if (!what->left->Do(this))
        {
            return nullptr;
        }
        if (Name *rightAsName = what->right->AsName())
        {
            if (!Do(rightAsName, true))
                return nullptr;
        }
        else if (!what->right->Do(this))
        {
            return nullptr;
        }
        if (unit.IsKnown(what->right))
        {
            unit.Copy(what->right, what);
        }
        else if (unit.IsKnown(what->left))
        {
            unit.Copy(what->left, what);
        }
        return what;
    }

    // Check if this is a rewrite declaration
    if (IsDefinition(what))
    {
        // If so, skip, this has been done in DeclarationAction
        return what;
    }

    // In all other cases, look up the rewrites
    return Rewrites(what);
}


static TextOption debugPrefixOption("debug-prefix",
                                    "Select a prefix to debug",
                                    "");

Tree *CompileAction::Do(Prefix *what)
// ----------------------------------------------------------------------------
//    Deal with data declarations, otherwise translate as a rewrite
// ----------------------------------------------------------------------------
{
    if (Name *name = what->left->AsName())
    {
        if (name->value == "data")
            return what;

        // A breakpoint location for convenience
        if (name->value == debugPrefixOption.value)
        {
            Save<char> saveDebugFlag(debugRewrites, debugRewrites+1);
            return Rewrites(what);
        }
    }

    return Rewrites(what);
}


Tree *CompileAction::Do(Postfix *what)
// ----------------------------------------------------------------------------
//    All postfix operations translate into a rewrite
// ----------------------------------------------------------------------------
{
    return Rewrites(what);
}


static Tree * lookupRewrite(Scope *evalScope,
                            Scope *declScope,
                            Tree *what,
                            Infix *decl,
                            void *info)
// ----------------------------------------------------------------------------
//   Implemnetation of rewrites
// ----------------------------------------------------------------------------
{
    ExpressionReduction &reduction = *((ExpressionReduction *) info);
    CompileAction &compile = reduction.compile;
    O1CompileUnit &unit = compile.unit;
    Tree *pattern = PatternBase(decl->left);
    Tree *body = decl->right;
    bool foundUnconditional = false;
    Context context(evalScope);
    record(rewrites,
           "Candidate %t declaration %t in declaration scope %t",
           pattern, decl->left, declScope);

    // Create the invokation point
    reduction.NewForm();
    bool isDataForm = IsSelf(decl->right);
    ArgumentMatch match(compile, what, evalScope, declScope, isDataForm);
    Tree *matched = what->Do(match);
    Tree *returnType = reduction.returnType;
    record(rewrites, "Candidate %t %+s", pattern, matched ? "ok" : "failed");

    if (matched)
    {
        // Record that we found something
        ++reduction.matches;

        // If this is a data form, we are done
        if (isDataForm)
        {
            // Set the symbols for the result
            compile.RewriteChildren(what);
            foundUnconditional = !unit.failbb;
            unit.dataForm.insert(what);
            reduction.Succeeded();
        }
        else
        {
            // We should have same number of args and parms
            XL_ASSERT(match.parms.size() == match.args.size());

            // Compile the candidate
            Tree *code = compile.CompileRewrite(declScope, body,
                                                match.parms, match.args);
            if (code)
            {
                // Invoke the candidate
                unit.Invoke(what, code, match.args);

                // If there was no test code, don't keep testing
                foundUnconditional = !unit.failbb;

                // This is the end of a successful invokation
                reduction.Succeeded();

                // Compute return type of expression
                Tree *exprType = context.Type(code);
                if (!returnType)
                    returnType = exprType;
                else if (exprType != returnType)
                    returnType = tree_type;
            }
            else
            {
                reduction.Failed();
            }
        } // if (data form)

    } // Match args
    else
    {
        // Indicate unsuccessful invokation
        reduction.Failed();
    }

    // If we didn't match anything, then emit an error at runtime
    if (!foundUnconditional)
    {
        // Special case the A[B] notation
        if (Prefix *pfx = what->AsPrefix())
        {
            if (Block *br = pfx->right->AsBlock())
            {
                if (br->IsSquare())
                {
                    pfx->left->Do(compile);
                    br->child->Do(compile);
                    if (unit.IsKnown(pfx->left) && unit.IsKnown(br->child))
                    {
                        unit.CallArrayIndex(pfx, pfx->left, br->child);
                        foundUnconditional = true;
                    }
                }
            }
        }

        if (!foundUnconditional)
        {
            unit.CallTypeError(what);
            returnType = nullptr;
        }
    }

    // Store the return type if we found one
    reduction.returnType = returnType;
    if (returnType)
        context.SetType(what, returnType);

    return foundUnconditional ? what : nullptr;
}



Tree *CompileAction::Rewrites(Tree *what)
// ----------------------------------------------------------------------------
//   Build code selecting among rewrites in current context
// ----------------------------------------------------------------------------
{
    record(rewrites, "Looking up rewrites for %t in scope %t",
           what, symbols.Symbols());
    ExpressionReduction reduction(*this, what);
    Context context(symbols);
    Tree *result = context.Lookup(what, lookupRewrite, &reduction, true);

    // If we didn't find anything, report it
    if (!result)
    {
        if (nullIfBad)
        {
            // Set the symbols for the result
            if (!noDataForms)
                RewriteChildren(what);
            return nullptr;
        }
        Ooops("No rewrite candidate for $1", what);
        return nullptr;
    }


    return result;
}


Tree *CompileAction::RewriteChildren(Tree *what)
// ----------------------------------------------------------------------------
//   Generate code for children of a structured tree
// ----------------------------------------------------------------------------
{
    EvaluateChildren eval(*this);
    return what->Do(eval);
}


Tree *CompileAction::CompileRewrite(Scope *scope,
                                    Tree *body,
                                    TreeList &xparms,
                                    TreeList &xargs)
// ----------------------------------------------------------------------------
//   Compile code for the 'to' form
// ----------------------------------------------------------------------------
//   This is similar to Context::Compile, except that it may generate a
//   function with more parameters, i.e. Tree *f(Tree * , Tree * , ...),
//   where there is one input arg per variable in the 'from' tree or per
//   captured variable from the surrounding context
{
    // Check if there are variables in the environment that we need to capture
    EnvironmentScan scan(Enclosing(symbols));
    Tree *envOK = body->Do(scan);
    if (!envOK)
        Ooops("Internal: environment capture error in $1", body);
    captures &ct = scan.captured;
    for (auto &ci : ct)
    {
        // We only capture local arguments
        if (Name *n1 = ci.first->AsName())
        {
            if (Name *n2 = ci.second->AsName())
            {
                if (n1->value == n2->value)
                {
                    xparms.push_back(n2);
                    xargs.push_back(n2);
                }
            }
        }
    }

    // Create the compilation unit and check if we are already compiling this
    FastCompiler &compiler = unit.compiler;
    O1CompileUnit unit(compiler, scope, body, xparms, false);
    if (unit.IsForwardCall())
    {
        // Recursive compilation of that form
        return body;              // We know how to invoke it anyway
    }

    // Record rewrites and data declarations in the current context
    DeclarationAction declaration(scope);
    Tree *toDecl = body->Do(declaration);
    if (!toDecl)
        Ooops("Internal: Declaration error for $1", body);

    // Compile the body of the rewrite
    CompileAction compile(declaration.symbols, unit, false, false, false);
    Tree *result = body->Do(compile);
    if (!result)
    {
        Ooops("Error compiling rewrite $1", body);
        return nullptr;
    }

    // Even if technically, this is not an 'eval_fn' (it has more args),
    // we still record it to avoid recompiling multiple times
    unit.Finalize(false);
    return body;
}



// ============================================================================
//
//   O1CompileUnit - A particular piece of code we generate for a tree
//
// ============================================================================
//  This is the "old" version that generates relatively inefficient machine code
//  However, it is at the moment more complete than the 'new' version and is
//  therefore preferred for the beta.

O1CompileUnit::O1CompileUnit(FastCompiler &compiler,
                             Scope *scope,
                             Tree *source,
                             TreeList parms,
                             bool closure)
// ----------------------------------------------------------------------------
//   O1CompileUnit constructor
// ----------------------------------------------------------------------------
    : compiler(compiler), symbols(scope), source(source),
      function(),
      code(compiler.jit), data(compiler.jit),
      entrybb(), exitbb(), failbb(),
      scopePtr()
{
    JIT &jit = compiler.jit;
    record(compiler,
           "Create O1 compile unit %p for %t in %t", this, source, scope);

    // If a compilation for that tree is alread in progress, fwd decl
    function = closure
        ? compiler.TreeClosure(source)
        : compiler.TreeFunction(source);
    if (function)
    {
        // We exit here without setting entrybb (see IsForward())
        record(compiler, "Function %v for %t already exists", function, source);
        return;
    }

    // Create the function signature, one entry per parameter + one for source
    JIT::Signature signature;
    signature.push_back(compiler.scopePtrTy);
    JIT::Type_p treePtrTy = compiler.treePtrTy;
    for (ulong p = 0; p <= parms.size(); p++)
        signature.push_back(treePtrTy);
    JIT::FunctionType_p fnTy = jit.FunctionType(treePtrTy, signature, false);
    function = jit.Function(fnTy, "xl_eval");
    record(labels, "%v is function for %t", function, source);
    code = JITBlock(jit, function, "code");
    data = JITBlock(jit, function, "data");

    // Save it in the compiler
    if (closure)
        compiler.SetTreeClosure(source, function);
    else
        compiler.SetTreeFunction(source, function);
    record(compiler, "New function %v for %t", function, source);

    // Get the function entry point
    entrybb = code.Block();

    // Associate the value for the input tree
    JITArguments args(function);
    scopePtr = *args++;
    JIT::Value_p inputArg = *args++;
    JIT::Value_p resultStorage = data.Alloca(treePtrTy, "result");
    data.Store(inputArg, resultStorage);
    storage[source] = resultStorage;
    value[source] = inputArg;

    // Associate the value for the additional arguments (read-only, no alloca)
    ulong parmsCount = 0;
    for (Tree *parm : parms)
    {
        inputArg = *args++;
        value[parm] = inputArg;
        parmsCount++;
    }

    // Create the exit basic block, stack pop and return statement
    JITBlock exitcode(jit, function, "exit");
    exitbb = exitcode.Block();
    JIT::Value_p retVal = exitcode.Load(resultStorage, "retval");
    exitcode.Return(retVal);

    // Record current entry/exit points for the current expression
    failbb = nullptr;

    // Local copy of the types for the macro below
    JIT::IntegerType_p  booleanTy        = compiler.booleanTy;
    JIT::IntegerType_p  integerTy        = compiler.integerTy;
    JIT::IntegerType_p  unsignedTy       = compiler.unsignedTy;
    JIT::IntegerType_p  ulongTy          = compiler.ulongTy;
    JIT::IntegerType_p  ulonglongTy      = compiler.ulonglongTy;
    JIT::Type_p         realTy           = compiler.realTy;
    JIT::IntegerType_p  characterTy      = compiler.characterTy;
    JIT::PointerType_p  charPtrTy        = compiler.charPtrTy;
    JIT::StructType_p   textTy           = compiler.textTy;
    JIT::PointerType_p  textPtrTy        = compiler.textPtrTy;
    JIT::PointerType_p  integerTreePtrTy = compiler.integerTreePtrTy;
    JIT::PointerType_p  realTreePtrTy    = compiler.realTreePtrTy;
    JIT::PointerType_p  textTreePtrTy    = compiler.textTreePtrTy;
    JIT::PointerType_p  blockTreePtrTy   = compiler.blockTreePtrTy;
    JIT::PointerType_p  prefixTreePtrTy  = compiler.prefixTreePtrTy;
    JIT::PointerType_p  postfixTreePtrTy = compiler.postfixTreePtrTy;
    JIT::PointerType_p  infixTreePtrTy   = compiler.infixTreePtrTy;
    JIT::PointerType_p  scopePtrTy       = compiler.scopePtrTy;
    JIT::PointerType_p  evalFnTy         = compiler.evalFnTy;

    // Initialize all the static functions
#define MTYPE(Name, Arity, Code)
#define UNARY(Name)
#define BINARY(Name)
#define CAST(Name)
#define ALIAS(Name, Arity, Original)
#define SPECIAL(Name, Arity, Code)
#define EXTERNAL(Name, RetTy, ...)                              \
    {                                                           \
        JIT::Signature sig { __VA_ARGS__ };                     \
        JIT::FunctionType_p fty = jit.FunctionType(RetTy, sig); \
        Name = jit.Function(fty, #Name);                        \
    }
#define VA_EXTERNAL(Name, RetTy, ...)                                   \
    {                                                                   \
        JIT::Signature sig { __VA_ARGS__ };                             \
        JIT::FunctionType_p fty = jit.FunctionType(RetTy, sig, true);   \
        Name = jit.Function(fty, #Name);                                \
    }
#include "compiler-primitives.tbl"
}


O1CompileUnit::~O1CompileUnit()
// ----------------------------------------------------------------------------
//   Delete what we must...
// ----------------------------------------------------------------------------
{
    if (entrybb && exitbb)
    {
        // If entrybb is clear, we may be looking at a forward declaration
        // Otherwise, if exitbb was not cleared by Finalize(), this means we
        // failed to compile. Make sure the compiler forgets the function
        compiler.SetTreeFunction(source, nullptr);
        function->eraseFromParent();
    }
}


eval_fn O1CompileUnit::Finalize(bool topLevel)
// ----------------------------------------------------------------------------
//   Finalize the build of the current function
// ----------------------------------------------------------------------------
{
    JIT &jit = compiler.jit;

    record(compiler, "Finalize function %v for %t", function, source);

    // Branch to the exit block from the last test we did
    code.Branch(exitbb);
    data.Branch(entrybb);

    // Generate the code
    if (RECORDER_TRACE(llvm_code) & 1)
        jit.Print("Unoptimized (fast compiler):\n", function);
    jit.Finalize(function);
    if (RECORDER_TRACE(llvm_code) & 2)
        jit.Print("Optimized (fast compiler):\n", function);

    void *result = nullptr;
    if (topLevel)
    {
        result = jit.ExecutableCode(function);
        if (RECORDER_TRACE(llvm_code) & 4)
            jit.Print("After code generation (fast compiler):\n", function);
        record(llvm_functions, "Fast code %p for %v", result, function);
    }

    exitbb = nullptr;              // Tell destructor we were successful
    return (eval_fn) result;
}


JIT::Value_p O1CompileUnit::NeedStorage(Tree *tree, Tree *source)
// ----------------------------------------------------------------------------
//    Allocate storage for a given tree
// ----------------------------------------------------------------------------
{
    JIT::Value_p result = storage[tree];
    if (!result)
    {
        // Create alloca to store the new form
        result = data.Alloca(compiler.treePtrTy, "loc");
        record(labels, "%v = storage for %t source %t", result, tree, source);
        storage[tree] = result;
    }

    // Deal with uninitialized values
    if (!value.count(tree) && source && value.count(source))
        value[tree] = value[source];

    // Store the initial value in the storage at beginning of function
    if (value.count(tree))
    {
        data.Store(value[tree], result);
    }
    else
    {
        if (!source)
            source = tree;
        JIT::Constant_p init = ConstantTree(source);
        data.Store(init, result);
    }

    return result;
}


bool O1CompileUnit::IsKnown(Tree *tree, uint which)
// ----------------------------------------------------------------------------
//   Check if the tree has a known local or global value
// ----------------------------------------------------------------------------
{
    if ((which & knowLocals) && storage.count(tree) > 0)
        return true;
    else if ((which & knowValues) && value.count(tree) > 0)
        return true;
    return false;
}


JIT::Value_p O1CompileUnit::Known(Tree *tree, uint which)
// ----------------------------------------------------------------------------
//   Return the known local or global value if any
// ----------------------------------------------------------------------------
{
    JIT::Value_p result = nullptr;
    if ((which & knowLocals) && storage.count(tree) > 0)
    {
        // Value is stored in a local variable
        result = code.Load(storage[tree], "loc");
    }
    else if ((which & knowValues) && value.count(tree) > 0)
    {
        // Immediate value of some sort, use that
        result = value[tree];
    }
    return result;
}


JIT::Constant_p O1CompileUnit::ConstantInteger(Integer *what)
// ----------------------------------------------------------------------------
//    Generate an Integer tree
// ----------------------------------------------------------------------------
{
    return ConstantTree(what);
}


JIT::Constant_p O1CompileUnit::ConstantReal(Real *what)
// ----------------------------------------------------------------------------
//    Generate a Real tree
// ----------------------------------------------------------------------------
{
    return ConstantTree(what);
}


JIT::Constant_p O1CompileUnit::ConstantText(Text *what)
// ----------------------------------------------------------------------------
//    Generate a Text tree
// ----------------------------------------------------------------------------
{
    return ConstantTree(what);
}


JIT::Constant_p O1CompileUnit::ConstantTree(Tree *what)
// ----------------------------------------------------------------------------
//    Generate a constant tree
// ----------------------------------------------------------------------------
{
    JIT::Constant_p result = data.PointerConstant(compiler.treePtrTy, what);
    if (storage.count(what))
        data.Store(result, storage[what]);
    return result;
}


JIT::Value_p O1CompileUnit::NeedLazy(Tree *subexpr, bool allocate)
// ----------------------------------------------------------------------------
//   Record that we need a 'computed' flag for lazy evaluation of the subexpr
// ----------------------------------------------------------------------------
{
    JIT::Value_p result = computed[subexpr];
    if (!result && allocate)
    {
        result = data.Alloca(compiler.booleanTy, "computed");
        record(labels, "%v is computed flag for %t", result, subexpr);
        JIT::Constant_p falseFlag = data.BooleanConstant(false);
        data.Store(falseFlag, result);
        computed[subexpr] = result;
    }
    return result;
}


JIT::Value_p O1CompileUnit::MarkComputed(Tree *subexpr, JIT::Value_p val)
// ----------------------------------------------------------------------------
//   Record that we computed that particular subexpression
// ----------------------------------------------------------------------------
{
    // Store the value we were given as the result
    if (val)
    {
        JIT::Value_p ptr = NeedStorage(subexpr);
        code.Store(val, ptr);
    }

    // Set the 'lazy' flag or lazy evaluation
    JIT::Value_p result = NeedLazy(subexpr);
    JIT::Value_p trueFlag = code.BooleanConstant(true);
    code.Store(trueFlag, result);

    // Return the test flag
    return result;
}


JIT::BasicBlock_p O1CompileUnit::BeginLazy(Tree *subexpr)
// ----------------------------------------------------------------------------
//    Begin lazy evaluation of a block of code
// ----------------------------------------------------------------------------
{
    JIT::BasicBlock_p skip = code.NewBlock("skip");
    JIT::BasicBlock_p work = code.NewBlock("work");
    record(labels, "For %t: %v is skip, %v is work", subexpr, skip, work);

    JIT::Value_p lazyFlagPtr = NeedLazy(subexpr);
    JIT::Value_p lazyFlag = code.Load(lazyFlagPtr, "lazy");
    record(labels, "%v is lazy flag for %t", lazyFlag, subexpr);
    code.IfBranch(lazyFlag, skip, work);
    code.SwitchTo(work);
    return skip;
}


void O1CompileUnit::EndLazy(Tree *subexpr, JIT::BasicBlock_p skip)
// ----------------------------------------------------------------------------
//   Finish lazy evaluation of a block of code
// ----------------------------------------------------------------------------
{
    record(labels, "%v is skip for %t", skip, subexpr);
    code.Branch(skip);
    code.SwitchTo(skip);
}


JIT::Value_p O1CompileUnit::Invoke(Tree *subexpr, Tree *callee, TreeList args)
// ----------------------------------------------------------------------------
//    Generate a call with the given arguments
// ----------------------------------------------------------------------------
{
    // Check if the resulting form is a name or literal
    if (callee->IsConstant())
    {
        if (JIT::Value_p known = Known(callee))
        {
            MarkComputed(subexpr, known);
            return known;
        }
        else
        {
            record(compiler_warning,
                   "No value for constant %t subexpr %t", callee, subexpr);
        }
    }

    JIT::Function_p toCall = compiler.TreeFunction(callee);
    XL_ASSERT(toCall);

    // Add the context argument
    JIT::Values argV;
    argV.push_back(scopePtr);

    // Add the 'self' argument
    JIT::Value_p selfVal = ConstantTree(subexpr);
    argV.push_back(selfVal);

    for (Tree *arg : args)
    {
        JIT::Value_p value = Known(arg);
        if (!value)
            value = ConstantTree(arg);
        argV.push_back(value);
    }

    JIT::Value_p callVal = code.Call(toCall, argV);

    // Store the flags indicating that we computed the value
    MarkComputed(subexpr, callVal);

    return callVal;
}


JIT::BasicBlock_p O1CompileUnit::NeedTest()
// ----------------------------------------------------------------------------
//    Indicates that we need an exit basic block to jump to
// ----------------------------------------------------------------------------
{
    if (!failbb)
    {
        JIT &jit = compiler.jit;
        JITBlock fail(jit, function, "fail");
        failbb = fail.Block();
    }
    return failbb;
}


JIT::Value_p O1CompileUnit::Left(Tree *tree)
// ----------------------------------------------------------------------------
//    Return the value for the left of the current tree
// ----------------------------------------------------------------------------
{
    // Check that the tree has the expected kind
    assert (tree->Kind() >= BLOCK);

    // Check if we already know the result, if so just return it
    // HACK: The following code assumes Prefix, Infix and Postfix have the
    // same layout for their pointers.
    Prefix *prefix = (Prefix *) tree;
    JIT::Value_p result = Known(prefix->left);
    if (result)
        return result;

    // Check that we already have a value for the given tree
    JIT::Value_p parent = Known(tree);
    if (parent)
    {
        JIT::Value_p ptr = NeedStorage(prefix->left);

        // WARNING: This relies on the layout of all nodes beginning the same
        JIT::Value_p pptr = code.BitCast(parent,
                                         compiler.prefixTreePtrTy,
                                         "pfxl");
        result = code.StructGEP(pptr, LEFT_VALUE_INDEX, "lptr");
        result = code.Load(result, "left");
        code.Store(result, ptr);
    }
    else
    {
        Ooops("Internal: Using left of uncompiled $1", tree);
    }

    return result;
}


JIT::Value_p O1CompileUnit::Right(Tree *tree)
// ----------------------------------------------------------------------------
//    Return the value for the right of the current tree
// ----------------------------------------------------------------------------
{
    // Check that the tree has the expected kind
    assert(tree->Kind() > BLOCK);

    // Check if we already known the result, if so just return it
    // HACK: The following code assumes Prefix, Infix and Postfix have the
    // same layout for their pointers.
    Prefix *prefix = (Prefix *) tree;
    JIT::Value_p result = Known(prefix->right);
    if (result)
        return result;

    // Check that we already have a value for the given tree
    JIT::Value_p parent = Known(tree);
    if (parent)
    {
        JIT::Value_p ptr = NeedStorage(prefix->right);

        // WARNING: This relies on the layout of all nodes beginning the same
        JIT::Value_p pptr = code.BitCast(parent, compiler.prefixTreePtrTy, "pfxr");
        result = code.StructGEP(pptr,RIGHT_VALUE_INDEX, "rptr");
        result = code.Load(result, "right");
        code.Store(result, ptr);
    }
    else
    {
        Ooops("Internal: Using right of uncompiled $14", tree);
    }
    return result;
}


JIT::Value_p O1CompileUnit::Copy(Tree *source, Tree *dest, bool markDone)
// ----------------------------------------------------------------------------
//    Copy data from source to destination
// ----------------------------------------------------------------------------
{
    JIT::Value_p result = Known(source); XL_ASSERT(result);
    JIT::Value_p ptr = NeedStorage(dest, source); XL_ASSERT(ptr);
    code.Store(result, ptr);

    if (markDone)
    {
        // Set the target flag to 'done'
        JIT::Value_p doneFlag = NeedLazy(dest);
        JIT::Value_p trueFlag = code.BooleanConstant(true);
        code.Store(trueFlag, doneFlag);
    }
    else if (JIT::Value_p oldDoneFlag = NeedLazy(source, false))
    {
        // Copy the flag from the source
        JIT::Value_p newDoneFlag = NeedLazy(dest);
        JIT::Value_p computed = code.Load(oldDoneFlag);
        code.Store(computed, newDoneFlag);
    }

    return result;
}


JIT::Value_p O1CompileUnit::CallEvaluate(Tree *tree)
// ----------------------------------------------------------------------------
//   Call the evaluate function for the given tree
// ----------------------------------------------------------------------------
{
    JIT::Value_p treeValue = Known(tree); XL_ASSERT(treeValue);
    if (dataForm.count(tree))
        return treeValue;

    JIT::Value_p evaluated = code.Call(xl_evaluate, scopePtr, treeValue);
    MarkComputed(tree, evaluated);
    return evaluated;
}


JIT::Value_p O1CompileUnit::CallFillBlock(Block *block)
// ----------------------------------------------------------------------------
//    Compile code generating the children of the block
// ----------------------------------------------------------------------------
{
    JIT::Value_p blockValue = ConstantTree(block);
    JIT::Value_p childValue = Known(block->child);
    blockValue = code.BitCast(blockValue, compiler.blockTreePtrTy);
    JIT::Value_p result = code.Call(xl_new_block, blockValue, childValue);
    result = code.BitCast(result, compiler.treePtrTy);
    MarkComputed(block, result);
    return result;
}


JIT::Value_p O1CompileUnit::CallFillPrefix(Prefix *prefix)
// ----------------------------------------------------------------------------
//    Compile code generating the children of a prefix
// ----------------------------------------------------------------------------
{
    JIT::Value_p prefixValue = ConstantTree(prefix);
    JIT::Value_p leftValue = Known(prefix->left);
    JIT::Value_p rightValue = Known(prefix->right);
    prefixValue = code.BitCast(prefixValue, compiler.prefixTreePtrTy);
    JIT::Value_p result = code.Call(xl_new_prefix,
                               prefixValue, leftValue, rightValue);
    result = code.BitCast(result, compiler.treePtrTy);
    MarkComputed(prefix, result);
    return result;
}


JIT::Value_p O1CompileUnit::CallFillPostfix(Postfix *postfix)
// ----------------------------------------------------------------------------
//    Compile code generating the children of a postfix
// ----------------------------------------------------------------------------
{
    JIT::Value_p postfixValue = ConstantTree(postfix);
    JIT::Value_p leftVal = Known(postfix->left);
    JIT::Value_p rightVal = Known(postfix->right);
    postfixValue = code.BitCast(postfixValue,compiler.postfixTreePtrTy);
    JIT::Value_p result = code.Call(xl_new_postfix,
                                    postfixValue, leftVal, rightVal);
    result = code.BitCast(result, compiler.treePtrTy);
    MarkComputed(postfix, result);
    return result;
}


JIT::Value_p O1CompileUnit::CallFillInfix(Infix *infix)
// ----------------------------------------------------------------------------
//    Compile code generating the children of an infix
// ----------------------------------------------------------------------------
{
    JIT::Value_p infixValue = ConstantTree(infix);
    JIT::Value_p leftVal = Known(infix->left);
    JIT::Value_p rightVal = Known(infix->right);
    infixValue = code.BitCast(infixValue, compiler.infixTreePtrTy);
    JIT::Value_p result = code.Call(xl_fill_infix,
                                    infixValue, leftVal, rightVal);
    result = code.BitCast(result, compiler.treePtrTy);
    MarkComputed(infix, result);
    return result;
}


JIT::Value_p O1CompileUnit::CallInteger2Real(Tree *compiled, Tree *value)
// ----------------------------------------------------------------------------
//    Compile code generating the children of an infix
// ----------------------------------------------------------------------------
{
    JIT::Value_p result = Known(value);
    result = code.Call(xl_integer2real, result);
    NeedStorage(compiled);
    MarkComputed(compiled, result);
    return result;
}


JIT::Value_p O1CompileUnit::CallArrayIndex(Tree *self, Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//    Compile code calling xl_array_index for a form like A[B]
// ----------------------------------------------------------------------------
{
    JIT::Value_p leftVal = Known(left);
    JIT::Value_p rightVal = Known(right);
    JIT::Value_p result = code.Call(xl_array_index,
                                    scopePtr, leftVal, rightVal);
    NeedStorage(self);
    MarkComputed(self, result);
    return result;
}


JIT::Value_p O1CompileUnit::CreateClosure(Tree *callee,
                                          TreeList &parms,
                                          TreeList &args,
                                          JIT::Function_p fn)
// ----------------------------------------------------------------------------
//   Create a closure for an expression we want to evaluate later
// ----------------------------------------------------------------------------
{
    JIT::Values argV;
    JIT::Value_p calleeVal = Known(callee);
    if (!calleeVal)
        return nullptr;
    JIT::Value_p countVal = code.IntegerConstant(compiler.unsignedTy,
                                            (unsigned) args.size());
    TreeList::iterator a, p;

    // Cast given function pointer to eval_fn and create argument list
    JIT::Value_p evalFn = code.BitCast(fn, compiler.evalFnTy);
    argV.push_back(evalFn);
    argV.push_back(calleeVal);
    argV.push_back(countVal);
    for (a = args.begin(), p = parms.begin();
         a != args.end() && p != parms.end();
         a++, p++)
    {
        Tree *name = *p;
        JIT::Value_p llvmName = ConstantTree(name);
        argV.push_back(llvmName);

        Tree *value = *a;
        JIT::Value_p llvmValue = Known(value); assert(llvmValue);
        argV.push_back(llvmValue);
    }

    JIT::Value_p callVal = code.Call(xl_new_closure, argV);

    // Need to store result, but not mark it as evaluated
    NeedStorage(callee);
    code.Store(callVal, storage[callee]);
    // MarkComputed(callee, callVal);

    return callVal;
}


JIT::Value_p O1CompileUnit::CallTypeError(Tree *what)
// ----------------------------------------------------------------------------
//   Report a type error trying to evaluate some argument
// ----------------------------------------------------------------------------
{
    JIT::Value_p ptr = ConstantTree(what); XL_ASSERT(what);
    JIT::Value_p callVal = code.Call(xl_form_error, scopePtr, ptr);
    MarkComputed(what, callVal);
    return callVal;
}


JIT::BasicBlock_p O1CompileUnit::TagTest(Tree *tree, unsigned tagValue)
// ----------------------------------------------------------------------------
//   Test if the input tree is an integer tree with the given value
// ----------------------------------------------------------------------------
{
    // Where we go if the tests fail
    JIT::BasicBlock_p notGood = NeedTest();

    // Check if the tag is INTEGER
    JIT::Value_p treeValue = Known(tree);
    if (!treeValue)
    {
        Ooops("No value for $1", tree);
        return nullptr;
    }
    JIT::Value_p tagPtr = code.StructGEP(treeValue, 0, "tagPtr");
    JIT::Value_p tag = code.Load(tagPtr, "tag");
    JIT::Type_p tagTy = code.Type(tag);
    JIT::Value_p mask = code.IntegerConstant(tagTy, Tree::KINDMASK);
    JIT::Value_p kind = code.And(tag, mask, "tagAndMask");
    JIT::Constant_p refTag = code.IntegerConstant(tagTy, tagValue);
    JIT::Value_p isRightTag = code.ICmpEQ(kind, refTag, "isRightTag");
    JIT::BasicBlock_p isRightKindBB = code.NewBlock("isRightKind");
    code.IfBranch(isRightTag, isRightKindBB, notGood);

    code.SwitchTo(isRightKindBB);
    return isRightKindBB;
}


JIT::BasicBlock_p O1CompileUnit::IntegerTest(Tree *tree, longlong value)
// ----------------------------------------------------------------------------
//   Test if the input tree is an integer tree with the given value
// ----------------------------------------------------------------------------
{
    // Where we go if the tests fail
    JIT::BasicBlock_p notGood = NeedTest();

    // Check if the tag is INTEGER
    JIT::BasicBlock_p isIntegerBB = TagTest(tree, INTEGER);
    if (!isIntegerBB)
        return isIntegerBB;

    // Check if the value is the same
    JIT::Value_p treeValue = Known(tree);
    assert(treeValue);
    treeValue = code.BitCast(treeValue, compiler.integerTreePtrTy);
    JIT::Value_p valueFieldPtr = code.StructGEP(treeValue,
                                           INTEGER_VALUE_INDEX,
                                           "valuePtr");
    JIT::Value_p tval = code.Load(valueFieldPtr, "treeValue");
    JIT::Constant_p rval = code.IntegerConstant(tval->getType(),
                                                (int64_t) value);
    JIT::Value_p isGood = code.ICmpEQ(tval, rval, "isGood");
    JIT::BasicBlock_p isGoodBB = code.NewBlock("isGood");
    code.IfBranch(isGood, isGoodBB, notGood);

    // If the value is the same, then go on, switch to the isGood basic block
    code.SwitchTo(isGoodBB);
    return isGoodBB;
}


JIT::BasicBlock_p O1CompileUnit::RealTest(Tree *tree, double value)
// ----------------------------------------------------------------------------
//   Test if the input tree is a real tree with the given value
// ----------------------------------------------------------------------------
{
    // Where we go if the tests fail
    JIT::BasicBlock_p notGood = NeedTest();

    // Check if the tag is REAL
    JIT::BasicBlock_p isRealBB = TagTest(tree, REAL);
    if (!isRealBB)
        return isRealBB;

    // Check if the value is the same
    JIT::Value_p treeValue = Known(tree);
    assert(treeValue);
    treeValue = code.BitCast(treeValue, compiler.realTreePtrTy);
    JIT::Value_p valueFieldPtr = code.StructGEP(treeValue,
                                           REAL_VALUE_INDEX,
                                           "valuePtr");
    JIT::Value_p tval = code.Load(valueFieldPtr, "treeValue");
    JIT::Constant_p rval = code.FloatConstant(tval->getType(), value);
    JIT::Value_p isGood = code.FCmpOEQ(tval, rval, "isGood");
    JIT::BasicBlock_p isGoodBB = code.NewBlock("isGood");
    code.IfBranch(isGood, isGoodBB, notGood);

    // If the value is the same, then go on, switch to the isGood basic block
    code.SwitchTo(isGoodBB);
    return isGoodBB;
}


JIT::BasicBlock_p O1CompileUnit::TextTest(Tree *tree, text value)
// ----------------------------------------------------------------------------
//   Test if the input tree is a text tree with the given value
// ----------------------------------------------------------------------------
{
    // Where we go if the tests fail
    JIT::BasicBlock_p notGood = NeedTest();

    // Check if the tag is TEXT
    JIT::BasicBlock_p isTextBB = TagTest(tree, TEXT);
    if (!isTextBB)
        return isTextBB;

    // Check if the value is the same, call xl_same_text
    JIT::Value_p treeValue = Known(tree);
    assert(treeValue);
    JIT::Value_p refVal = data.TextConstant(value);
    JIT::Value_p isGood = code.Call(xl_same_text, treeValue, refVal);
    JIT::BasicBlock_p isGoodBB = code.NewBlock("isGood");
    code.IfBranch(isGood, isGoodBB, notGood);

    // If the value is the same, then go on, switch to the isGood basic block
    code.SwitchTo(isGoodBB);
    return isGoodBB;
}


JIT::BasicBlock_p O1CompileUnit::ShapeTest(Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//   Test if the two given trees have the same shape
// ----------------------------------------------------------------------------
{
    JIT::Value_p leftVal = Known(left);
    JIT::Value_p rightVal = Known(right);
    assert(leftVal);
    assert(rightVal);
    if (leftVal == rightVal) // How unlikely?
        return nullptr;

    // Where we go if the tests fail
    JIT::BasicBlock_p notGood = NeedTest();
    JIT::Value_p isGood = code.Call(xl_same_shape, leftVal, rightVal);
    JIT::BasicBlock_p isGoodBB = code.NewBlock("isGood");
    code.IfBranch(isGood, isGoodBB, notGood);

    // If the value is the same, then go on, switch to the isGood basic block
    code.SwitchTo(isGoodBB);
    return isGoodBB;
}


JIT::BasicBlock_p O1CompileUnit::InfixMatchTest(Tree *actual, Infix *reference)
// ----------------------------------------------------------------------------
//   Test if the actual tree has the same shape as the given infix
// ----------------------------------------------------------------------------
{
    // Check that we know how to evaluate both
    JIT::Value_p actualVal = Known(actual);           assert(actualVal);
    JIT::Value_p refVal = NeedStorage(reference);     assert (refVal);

    // Extract the name of the reference
    JIT::Value_p refName = data.TextConstant(reference->name);

    // Where we go if the tests fail
    JIT::BasicBlock_p notGood = NeedTest();
    JIT::Value_p afterExtract = code.Call(xl_infix_match_check,
                                     scopePtr, actualVal, refName);
    JIT::Constant_p null = code.PointerConstant(compiler.treePtrTy, nullptr);
    JIT::Value_p isGood = code.ICmpNE(afterExtract, null, "isGoodInfix");
    JIT::BasicBlock_p isGoodBB = code.NewBlock("isGood");
    code.IfBranch(isGood, isGoodBB, notGood);

    // If the value is the same, then go on, switch to the isGood basic block
    code.SwitchTo(isGoodBB);

    // We are on the right path: extract left and right
    code.Store(afterExtract, refVal);
    MarkComputed(reference, nullptr);
    MarkComputed(reference->left, nullptr);
    MarkComputed(reference->right, nullptr);
    Left(reference);
    Right(reference);

    return isGoodBB;
}


JIT::BasicBlock_p O1CompileUnit::TypeTest(Tree *value, Tree *type)
// ----------------------------------------------------------------------------
//   Test if the given value has the given type
// ----------------------------------------------------------------------------
{
    // Don't do a type cast for any type where type test is a no-op
    if (type == tree_type       ||
        type == source_type     ||
        type == code_type       ||
        type == reference_type  ||
        type == value_type)
        return nullptr;

    JIT::Value_p treeValue = Known(value);     assert(treeValue);

    // Quick inline check with the tag to see if need runtime test
    JIT::Value_p tagPtr = code.StructGEP(treeValue, TAG_INDEX, "tagPtr");
    JIT::Value_p tag = code.Load(tagPtr, "tag");
    JIT::Type_p tagTy = code.Type(tag);
    JIT::Value_p mask = code.IntegerConstant(tagTy, Tree::KINDMASK);
    JIT::Value_p kindValue = code.And(tag, mask, "tagAndMask");

    uint kind = ~0U;
    if (type == integer_type)           kind = INTEGER;
    else if (type == real_type)         kind = REAL;
    else if (type == text_type)         kind = TEXT;
    else if (type == name_type)         kind = NAME;
    else if (type == operator_type)     kind = NAME;
    else if (type == boolean_type)      kind = NAME;
    else if (type == infix_type)        kind = INFIX;
    else if (type == prefix_type)       kind = PREFIX;
    else if (type == postfix_type)      kind = POSTFIX;
    else if (type == block_type)        kind = BLOCK;

    JIT::Constant_p refTag = code.IntegerConstant(tagTy, kind);
    JIT::Value_p isRightTag = code.ICmpEQ(kindValue, refTag, "isTagOK");
    JIT::BasicBlock_p isKindOK = code.NewBlock("isKindOK");
    JIT::BasicBlock_p isKindBad = code.NewBlock("isKindBad");
    code.IfBranch(isRightTag, isKindOK, isKindBad);

    // Degraded for integer: may simply need to promote to real
    code.SwitchTo(isKindBad);
    if (type == real_type)
    {
        JIT::Constant_p intTag = code.IntegerConstant(tagTy, INTEGER);
        JIT::Value_p isInt = code.ICmpEQ(kindValue, intTag, "isInt");
        JIT::BasicBlock_p isIntOK = code.NewBlock("isIntOK");
        JIT::BasicBlock_p isIntBad = code.NewBlock("isIntBad");
        code.IfBranch(isInt, isIntOK, isIntBad);

        code.SwitchTo(isIntOK);
        JIT::Value_p asReal = code.Call(xl_integer2real, treeValue);
        JIT::Value_p realPtr = NeedStorage(value);
        code.Store(asReal, realPtr);
        code.Branch(isKindOK);

        code.SwitchTo(isIntBad);
        treeValue = Known(value);
    }

    JIT::Value_p typeVal = Known(type);       assert(typeVal);

    // Where we go if the tests fail
    JIT::BasicBlock_p notGood = NeedTest();
    JIT::Value_p afterCast = code.Call(xl_typecheck, scopePtr, treeValue, typeVal);
    JIT::Constant_p null = code.PointerConstant(compiler.treePtrTy, nullptr);
    JIT::Value_p isGood = code.ICmpNE(afterCast, null, "isGoodType");
    JIT::BasicBlock_p isGoodBB = code.NewBlock("isGood");
    code.IfBranch(isGood, isGoodBB, notGood);

    // If the value matched, we may have a type cast, remember it
    code.SwitchTo(isGoodBB);
    JIT::Value_p ptr = NeedStorage(value);
    code.Store(afterCast, ptr);
    code.Branch(isKindOK);

    code.SwitchTo(isKindOK);
    return isKindOK;
}



// ============================================================================
//
//    Expression reduction
//
// ============================================================================
//   An expression reduction typically compiles as:
//     if (cond1) if (cond2) if (cond3) invoke(T)
//   However, we may determine during compilation of if(cond2) that the
//   call is statically not valid. So we save the initial basic block,
//   and decide at the end to connect it or not. Let LLVM optimize branches
//   and dead code away...

ExpressionReduction::ExpressionReduction(CompileAction &compile, Tree *source)
// ----------------------------------------------------------------------------
//    Snapshot current basic blocks in the compiled unit
// ----------------------------------------------------------------------------
    : compile(compile), source(source),
      storage(nullptr), computed(nullptr),
      savedfailbb(nullptr),
      entrybb(nullptr), savedbb(nullptr), successbb(nullptr),
      savedvalue(compile.unit.value),
      matches(0)
{
    // We need storage and a compute flag to skip this computation if needed
    O1CompileUnit &unit = compile.unit;
    storage = unit.NeedStorage(source);
    computed = unit.NeedLazy(source);

    // Save compile unit's data
    savedfailbb = unit.failbb;
    unit.failbb = nullptr;

    // Create the end-of-expression point if we find a match or had it already
    successbb = unit.BeginLazy(source);
}


ExpressionReduction::~ExpressionReduction()
// ----------------------------------------------------------------------------
//   Destructor restores the prevous sucess and fail points
// ----------------------------------------------------------------------------
{
    O1CompileUnit &unit = compile.unit;

    // Mark the end of a lazy expression evaluation
    unit.EndLazy(source, successbb);

    // Restore saved 'failbb' and value map
    unit.failbb = savedfailbb;
    unit.value = savedvalue;
}


void ExpressionReduction::NewForm ()
// ----------------------------------------------------------------------------
//    Indicate that we are testing a new form for evaluating the expression
// ----------------------------------------------------------------------------
{
    O1CompileUnit &unit = compile.unit;

    // Save previous basic blocks in the compiled unit
    savedbb = unit.code.Block();
    assert(savedbb || !"NewForm called after unconditional success");

    // Create entry / exit basic blocks for this expression
    entrybb = unit.code.NewBlock("subexpr");
    unit.failbb = nullptr;

    // Set the insertion point to the new invokation code
    unit.code.SwitchTo(entrybb);

}


void ExpressionReduction::Succeeded(void)
// ----------------------------------------------------------------------------
//   We successfully compiled a reduction for that expression
// ----------------------------------------------------------------------------
//   In that case, we connect the basic blocks to evaluate the expression
{
    O1CompileUnit &unit = compile.unit;

    // Branch from current point (end of expression) to exit of evaluation
    unit.code.Branch(successbb);

    // Branch from initial basic block position to this subcase
    unit.code.SwitchTo(savedbb);
    unit.code.Branch(entrybb);

    // If there were tests, we keep testing from that 'else' spot
    if (unit.failbb)
    {
        unit.code.SwitchTo(unit.failbb);
    }
    else
    {
        // Create a fake basic block in case someone decides to add code
        JIT::BasicBlock_p empty = unit.code.NewBlock("empty");
        unit.code.SwitchTo(empty);
    }
    unit.failbb = nullptr;
}


void ExpressionReduction::Failed()
// ----------------------------------------------------------------------------
//    We figured out statically that the current form doesn't apply
// ----------------------------------------------------------------------------
{
    O1CompileUnit &unit = compile.unit;

    unit.CallTypeError(source);
    unit.code.Branch(successbb);
    if (unit.failbb)
    {
        unit.code.SwitchTo(unit.failbb);
        unit.CallTypeError(source);
        unit.code.Branch(successbb);
        unit.failbb = nullptr;
    }

    unit.code.SwitchTo(savedbb);
}

XL_END



// ============================================================================
//
//    Runtime support that is specific to the fast compiler
//
// ============================================================================

using namespace XL;

Tree *xl_array_index(Scope *scope, Tree *data, Tree *index)
// ----------------------------------------------------------------------------
//    Index an array (to be reimplemented)
// ----------------------------------------------------------------------------
{
    Ooops("Array index no longer implemented for $1 $2", data, index);
    return data;
}


Tree *xl_new_closure(eval_fn toCall, Tree *expr, uint ntrees, ...)
// ----------------------------------------------------------------------------
//   Create a new closure at runtime, capturing the various trees
// ----------------------------------------------------------------------------
{
    // Immediately return anything we could evaluate at no cost
    if (!ntrees || !expr || expr->IsConstant())
        return expr;

    record(closure, "Closure for code %p arity %u on expression %t",
           toCall, ntrees, expr);

    // Build the list of parameter names and associated arguments
    Tree_p result = nullptr;
    Tree_p *treePtr = &result;
    Infix *decl = nullptr;
    va_list va;
    va_start(va, ntrees);
    for (uint i = 0; i < ntrees; i++)
    {
        Name *name = va_arg(va, Name *);
        Tree *arg = va_arg(va, Tree *);
        record(closure, "  Parm %t = Arg %t", name, arg);
        decl = new Infix("is", name, arg, arg->Position());
        if (*treePtr)
        {
            Infix *infix = new Infix("\n", *treePtr, decl);
            *treePtr = infix;
            treePtr = &infix->right;
        }
        else
        {
            *treePtr = decl;
        }
    }
    va_end(va);

    // Build the final infix with the original expression
    *treePtr = new Infix("\n", *treePtr, expr);

    // Wrap everything in a block so that all closures look like blocks
    result = new Block(result, "{", "}", expr->Position());

    // Generate the code to pass the arguments from the closure
    FastCompiler *compiler = (FastCompiler *) MAIN->evaluator;
    compiler->ClosureAdapter(ntrees);
    FastCompiler::SetTreeCode(result, toCall);

    return result;
}


eval_fn xl_closure_code(Tree *tree)
// ----------------------------------------------------------------------------
//   Return the code generated for closure code if any
// ----------------------------------------------------------------------------
{
    eval_fn code = FastCompiler::TreeCode(tree);
    XL_ASSERT(code);
    return code;
}
