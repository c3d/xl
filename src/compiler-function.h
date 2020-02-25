#ifndef COMPILER_FUNCTION_H
#define COMPILER_FUNCTION_H
// *****************************************************************************
// compiler-function.h                                                XL project
// *****************************************************************************
//
// File description:
//
//     A function generated in the compiler unit
//
//     There are, broadly, two kinds of functions being generated:
//     1. Eval functions, with evalTy as their signature (aka eval_fn)
//     2. Optimized functions, with arbitrary signatures.
//
//     Optimized functions have a 'closure type' as their first argument
//     if symbols from surrounding contexts were captured during analysis
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2018-2019, Christophe de Dinechin <christophe@dinechin.org>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
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

#include "compiler.h"
#include "compiler-unit.h"
#include "compiler-args.h"
#include "compiler-prototype.h"
#include "compiler-types.h"


XL_BEGIN

class CompilerFunction : public CompilerPrototype
// ----------------------------------------------------------------------------
//    A function generated in a compile unit
// ----------------------------------------------------------------------------
{
protected:
    Compiler &          compiler;   // The compiler environment we use
    JIT &               jit;        // The JIT compiler (LLVM API stabilizer)
    Tree_p              body;       // Body for this function
    JITBlock            data;       // A basic block for local variables
    JITBlock            code;       // A basic block for current code
    JITBlock            exit;       // A basic block for shared exit
    JIT::BasicBlock_p   entry;      // The entry point for the function code
    JIT::Value_p        returned;   // Returned value
    JIT::Type_p         closure;    // Closure type if any
    value_map           values;     // Tree -> LLVM value
    value_map           storage;    // Tree -> LLVM storage (alloca)

    friend class CompilerExpression;

public:
    // Constructors for the top-level functions
    CompilerFunction(CompilerUnit &unit,
                     Tree *form,
                     Tree *body,
                     Types *types,
                     JIT::FunctionType_p ftype,
                     text name);
    CompilerFunction(CompilerFunction &caller, RewriteCandidate *rc);
    ~CompilerFunction();

    bool                IsInterfaceOnly() override;


    JIT::Function_p     Compile(Tree *tree, bool forceEvaluation = false);
    JIT::Value_p        Return(Tree *tree, JIT::Value_p value);
    eval_fn             Finalize(bool createCode);

    JIT::Type_p         ValueMachineType(Tree *expr, bool mayfail = false);
    void                ValueMachineType(Tree *expr, JIT::Type_p type);
    JIT::Type_p         BoxedType(Tree *type);

private:
    // Function interface creation
    void                InitializeArgs();
    void                InitializeArgs(RewriteCandidate *rc);

    // Machine types management
    void                AddBoxedType(Tree *treeType, JIT::Type_p machineType);
    JIT::Type_p         HasBoxedType(Tree *type);

    JIT::Type_p         ReturnType(Tree *form);
    JIT::Type_p         StructureType(Tree *rwform, Tree *type);
    JIT::Type_p         StructureType(const JIT::Signature &signature,
                                      Tree *rwform, Tree *type);
    JIT::Value_p        BoxedTree(Tree *what);
    void                BoxedTreeType(JIT::Signature &sig, Tree *what);

private:
    // Compilation of rewrites and data
    JIT::Value_p        Compile(Tree *call,
                                RewriteCandidate *rc,
                                const JIT::Values &args);
    JIT::Value_p        Data(Tree *form,
                             JIT::Value_p box,
                             unsigned &index);
    JIT::Value_p        Autobox(Tree *source,
                                JIT::Value_p value,
                                JIT::Type_p requested);
    JIT::Function_p     UnboxFunction(JIT::Type_p type,
                                      Tree *form);
    JIT::Value_p        Unbox(JIT::Value_p arg,
                              Tree *form,
                              uint &index);
    JIT::Value_p        Primitive(Tree *,
                                  text name,
                                  uint arity,
                                  JIT::Value_p *args);

    // Storage management
    enum { knowAll = -1, knowGlobals = 1, knowLocals = 2, knowValues = 4 };
    JIT::Value_p        NeedStorage(Tree *tree, JIT::Type_p ty = nullptr);
    bool                IsKnown(Tree *tree, uint which = knowAll);
    JIT::Value_p        Known(Tree *tree, uint which = knowAll );

    // Creating constants
    JIT::Value_p        ConstantInteger(Integer *what);
    JIT::Value_p        ConstantReal(Real *what);
    JIT::Value_p        ConstantText(Text *what);
    JIT::Value_p        ConstantTree(Tree *what);

    // Error management
    JIT::Value_p        CallFormError(Tree *what);

protected:
    // Primitives, i.e. functions generating native LLVM code
    typedef JIT::Value_p (CompilerFunction::*primitive_fn)(Tree *,
                                                           JIT::Value_p *args);
    struct PrimitiveInfo
    {
        primitive_fn    function;
        unsigned        arity;
    };
    typedef std::map<text,PrimitiveInfo> Primitives;
    static Primitives   primitives;

    static void         InitializePrimitives();

    // Define LLVM accessors for primitives
#define UNARY(Name)                                                     \
    JIT::Value_p        llvm_##Name(Tree *source, JIT::Value_p *args);
#define BINARY(Name)                                                    \
    JIT::Value_p        llvm_##Name(Tree *source, JIT::Value_p *args);
#define CAST(Name)                                                      \
    JIT::Value_p        llvm_##Name(Tree *source, JIT::Value_p *args);
#define SPECIAL(Name, Arity, Code)                                      \
    JIT::Value_p        llvm_##Name(Tree *source, JIT::Value_p *args);

#define ALIAS(from, arity, to)
#define EXTERNAL(Name, ...)

#include "compiler-primitives.tbl"
};


class CompilerEval : public CompilerFunction
// ----------------------------------------------------------------------------
//   A compiler eval function
// ----------------------------------------------------------------------------
{
public:
    CompilerEval(CompilerUnit &unit,
                 Tree *body,
                 Types *types);

};

XL_END

RECORDER_DECLARE(compiler_function);
RECORDER_DECLARE(parameter_bindings);

#endif // COMPILER_FUNCTION_H
