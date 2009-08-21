#ifndef COMPILER_H
#define COMPILER_H
// ****************************************************************************
//  compiler.h                       (C) 1992-2009 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//    Just-in-time compiler for the trees
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
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

#include "tree.h"
#include <map>
#include <llvm/Support/IRBuilder.h>


namespace llvm
{
    class Module;
    class ModuleProvider;
    class ExecutionEngine;
    class FunctionPassManager;
    class StructType;
    class PointerType;
    class FunctionType;
    class BasicBlock;
    class Value;
};

XL_BEGIN

struct CompiledUnit;
typedef std::map<Tree *, llvm::Value *>         value_map;
typedef std::map<Tree *, llvm::Function *>      function_map;


struct Compiler
// ----------------------------------------------------------------------------
//   Just-in-time compiler data
// ----------------------------------------------------------------------------
{
    Compiler(kstring moduleName = "xl");

    CompiledUnit *      NewUnit(kstring name);

public:
    llvm::Module              *module;
    llvm::ModuleProvider      *provider;
    llvm::ExecutionEngine     *runtime;
    llvm::FunctionPassManager *optimizer;
    llvm::StructType          *treeTy;
    llvm::PointerType         *treePtrTy;
    llvm::PointerType         *treePtrPtrTy;
    llvm::StructType          *integerTreeTy;
    llvm::PointerType         *integerTreePtrTy;
    llvm::StructType          *realTreeTy;
    llvm::PointerType         *realTreePtrTy;
    llvm::StructType          *prefixTreeTy;
    llvm::PointerType         *prefixTreePtrTy;
    llvm::FunctionType        *evalTy;
    llvm::PointerType         *evalFnTy;
    llvm::Function            *xl_evaluate;
    llvm::Function            *xl_same_text;
    llvm::Function            *xl_same_shape;
    llvm::Function            *xl_type_check;
    function_map               functions;
};


struct CompiledUnit
// ----------------------------------------------------------------------------
//  A compilation unit, which typically corresponds to an expression
// ----------------------------------------------------------------------------
{
    CompiledUnit(Compiler *comp, Tree *source, tree_list parms);
    ~CompiledUnit();

    bool                IsForwardCall()         { return entrybb == NULL; }

    llvm::BasicBlock *  BeginInvokation();
    void                EndInvokation(llvm::BasicBlock *bb, bool success);

    llvm::Value *       Invoke(Tree *callee, tree_list args);
    llvm::Value *       Return(Tree *value);
    eval_fn             Finalize();

    llvm::BasicBlock *  NeedTest();
    llvm::Value *       NeedLazy(Tree *tree);
    llvm::Value *       Left(Tree *);
    llvm::Value *       Right(Tree *);

    llvm::Value *       LazyEvaluation(Tree *code, ulong id);
    llvm::Value *       EagerEvaluation(Tree *code);
    llvm::BasicBlock *  TagTest(Tree *code, ulong tag);
    llvm::BasicBlock *  IntegerTest(Tree *code, longlong value);
    llvm::BasicBlock *  RealTest(Tree *code, double value);
    llvm::BasicBlock *  TextTest(Tree *code, text value);
    llvm::BasicBlock *  ShapeTest(Tree *code, Tree *other);
    llvm::BasicBlock *  TypeTest(Tree *code, Tree *type);

public:
    Compiler *          compiler;       // The compiler environment we use
    ulong               parameters;     // Size of the tree input list
    llvm::Value *       result;         // Returned value from function
    llvm::IRBuilder<> * builder;        // Instruction builder
    llvm::Function *    function;       // Function we generate
    llvm::BasicBlock *  entrybb;        // Entry point for that code
    llvm::BasicBlock *  exitbb;         // Exit point for that code
    llvm::BasicBlock *  invokebb;       // Entry basic block for current invoke
    llvm::BasicBlock *  failbb;         // Where we go if tests fail
    value_map           map;            // Map tree -> LLVM value
    value_map           lazy;           // Map trees that need lazy eval
};

#define LLVM_INTTYPE(t)         llvm::IntegerType::get(sizeof(t) * 8)

XL_END

#endif // COMPILER_H

