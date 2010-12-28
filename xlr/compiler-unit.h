#ifndef COMPILER_UNIT_H
#define COMPILER_UNIT_H
// ****************************************************************************
//  compiler-unit.h                                                 XLR project
// ****************************************************************************
// 
//   File Description:
// 
//     Information about a single compilation unit, i.e. the code generated
//     for a particular tree rerite.
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

#include "compiler.h"
#include "compiler-parm.h"


XL_BEGIN

struct CompiledUnit
// ----------------------------------------------------------------------------
//  The function we generate for a given rewrite
// ----------------------------------------------------------------------------
{
    CompiledUnit(Compiler *compiler);
    ~CompiledUnit();

public:
    llvm::Function *    RewriteFunction(Context *context, Rewrite *rewrite);
    llvm::Function *    TopLevelFunction(Context *context);

protected:
    llvm::Function *    InitializeFunction(llvm::FunctionType *,
                                           ParameterList &parameters,
                                           kstring label);

public:
    llvm::Value *       Compile(Tree *tree);

    eval_fn             Finalize();

    enum { knowAll = -1, knowGlobals = 1, knowLocals = 2, knowValues = 4 };
    llvm::Value *       NeedStorage(Tree *tree);
    bool                IsKnown(Tree *tree, uint which = knowAll);
    llvm::Value *       Known(Tree *tree, uint which = knowAll );
    llvm::Value *       StringPointer(text value);

    llvm::Value *       ConstantInteger(Integer *what);
    llvm::Value *       ConstantReal(Real *what);
    llvm::Value *       ConstantText(Text *what);
    llvm::Value *       ConstantTree(Tree *what);

    llvm::Value *       NeedLazy(Tree *subexpr, bool allocate = true);
    llvm::Value *       MarkComputed(Tree *subexpr, llvm::Value *value);
    llvm::BasicBlock *  BeginLazy(Tree *subexpr);
    void                EndLazy(Tree *subexpr, llvm::BasicBlock *skip);

    llvm::BasicBlock *  NeedTest();
    llvm::Value *       Left(Tree *);
    llvm::Value *       Right(Tree *);
    llvm::Value *       Copy(Tree *src, Tree *dst, bool markDone=true);
    llvm::Value *       Invoke(Tree *subexpr, Tree *callee, TreeList args);
    llvm::Value *       CallEvaluate(Tree *);
    llvm::Value *       CallNewBlock(Block *);
    llvm::Value *       CallNewPrefix(Prefix *);
    llvm::Value *       CallNewPostfix(Postfix *);
    llvm::Value *       CallNewInfix(Infix *);
    llvm::Value *       CreateClosure(Tree *callee, TreeList &args);
    llvm::Value *       CallClosure(Tree *callee, uint ntrees);
    llvm::Value *       CallFormError(Tree *what);

    llvm::BasicBlock *  TagTest(Tree *code, ulong tag);
    llvm::BasicBlock *  IntegerTest(Tree *code, longlong value);
    llvm::BasicBlock *  RealTest(Tree *code, double value);
    llvm::BasicBlock *  TextTest(Tree *code, text value);
    llvm::BasicBlock *  ShapeTest(Tree *code, Tree *other);
    llvm::BasicBlock *  InfixMatchTest(Tree *code, Infix *ref);
    llvm::BasicBlock *  TypeTest(Tree *code, Tree *type);

    llvm_type           ReturnType(Tree *form);
    llvm_type           StructureType(llvm_types &signature);

public:
    Compiler *          compiler;       // The compiler environment we use
    llvm::LLVMContext * llvm;           // The LLVM context we got from compiler

    llvm::IRBuilder<> * code;           // Instruction builder for code
    llvm::IRBuilder<> * data;           // Instruction builder for data
    llvm::Function *    function;       // Function we generate

    llvm::BasicBlock *  allocabb;       // Function entry point, allocas
    llvm::BasicBlock *  entrybb;        // Code entry point
    llvm::BasicBlock *  exitbb;         // Shared exit for the function
    llvm::BasicBlock *  failbb;         // Shared exit for failed expred tests

    value_map           value;          // Map tree -> LLVM value
    value_map           storage;        // Map tree -> LLVM alloca space
    value_map           computed;       // Map tree -> LLVM "computed" flag
    data_set            dataForm;       // Data expressions: don't evaluate
};

XL_END

#endif // COMPILER_UNIT_H
