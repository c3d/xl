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
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#include "tree.h"
#include "context.h"
#include <map>
#include <set>
#include <llvm/Support/IRBuilder.h>


namespace llvm
// ----------------------------------------------------------------------------
//   Forward classes from LLVM
// ----------------------------------------------------------------------------
{
    class LLVMContext;
    class Module;
    class ExecutionEngine;
    class FunctionPassManager;
    class StructType;
    class PointerType;
    class FunctionType;
    class BasicBlock;
    class Value;
    class Function;
    class GlobalValue;
    class Type;
};

XL_BEGIN

struct CompiledUnit;
struct CompilerInfo;
struct Context;
struct Options;
typedef Tree * (*adapter_fn) (native_fn callee, Context *ctx,
                              Tree *src, Tree **args);
typedef std::map<text, llvm::Function *>    builtins_map;
typedef std::map<Tree *, llvm::Value *>     value_map;
typedef std::map<Tree *, Tree **>           address_map;
typedef std::map<uint, eval_fn>             closure_map;
typedef std::map<uint, adapter_fn>          adapter_map;
typedef std::set<Tree *>                    closure_set;
typedef std::set<Tree *>                    data_set;


struct Compiler
// ----------------------------------------------------------------------------
//   Just-in-time compiler data
// ----------------------------------------------------------------------------
{
    Compiler(kstring moduleName = "xl", uint optimize_level = 999);
    ~Compiler();

    void                      Reset();
    CompilerInfo *            Info(Tree *tree, bool create = false);
    llvm::Function *          TreeFunction(Tree *tree);
    void                      SetTreeFunction(Tree *tree, llvm::Function *);
    llvm::GlobalValue *       TreeGlobal(Tree *tree);
    void                      SetTreeGlobal(Tree*, llvm::GlobalValue*, void*);
    llvm::Function *          EnterBuiltin(text name,
                                           Tree *to,
                                           TreeList parms,
                                           eval_fn code);
    llvm::Function *          ExternFunction(kstring name, void *address,
                                             const llvm::Type *retType,
                                             int parmCount, ...);
    adapter_fn                ArrayToArgsAdapter(uint numtrees);
    llvm::Value *             EnterGlobal(Name *name, Name_p *address);
    llvm::Value *             EnterConstant(Tree *constant);
    bool                      IsKnown(Tree *value);

    bool                      FreeResources(Tree *tree);


public:
    llvm::LLVMContext         *context;
    llvm::Module              *module;
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
    llvm::FunctionType        *nativeTy;
    llvm::PointerType         *nativeFnTy;
    llvm::FunctionType        *evalTy;
    llvm::PointerType         *evalFnTy;
    llvm::PointerType         *infoPtrTy;
    llvm::PointerType         *contextPtrTy;
    llvm::PointerType         *charPtrTy;
    llvm::Function            *xl_evaluate;
    llvm::Function            *xl_same_text;
    llvm::Function            *xl_same_shape;
    llvm::Function            *xl_infix_match_check;
    llvm::Function            *xl_type_check;
    llvm::Function            *xl_form_error;
    llvm::Function            *xl_new_integer;
    llvm::Function            *xl_new_real;
    llvm::Function            *xl_new_character;
    llvm::Function            *xl_new_text;
    llvm::Function            *xl_new_xtext;
    llvm::Function            *xl_new_block;
    llvm::Function            *xl_new_prefix;
    llvm::Function            *xl_new_postfix;
    llvm::Function            *xl_new_infix;
    llvm::Function            *xl_new_closure;
    builtins_map               builtins;
    closure_map                closures;
    adapter_map                array_to_args_adapters;
};


struct CompiledUnit
// ----------------------------------------------------------------------------
//  A compilation unit, which typically corresponds to an expression
// ----------------------------------------------------------------------------
{
    CompiledUnit(Compiler *comp, Tree *source, TreeList parms);
    ~CompiledUnit();

    bool                IsForwardCall()         { return entrybb == NULL; }
    eval_fn             Finalize();

    enum { knowAll = -1, knowGlobals = 1, knowLocals = 2, knowValues = 4 };

    llvm::Value *       NeedStorage(Tree *tree);
    bool                IsKnown(Tree *tree, uint which = knowAll);
    llvm::Value *       Known(Tree *tree, uint which = knowAll );

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

public:
    Compiler *          compiler;       // The compiler environment we use
    llvm::LLVMContext * context;        // The context we got from compiler
    Tree_p              source;         // The original source we compile

    llvm::IRBuilder<> * code;           // Instruction builder for code
    llvm::IRBuilder<> * data;           // Instruction builder for data
    llvm::Function *    function;       // Function we generate

    llvm::BasicBlock *  allocabb;       // Function entry point, allocas
    llvm::BasicBlock *  entrybb;        // Entry point for that code
    llvm::BasicBlock *  exitbb;         // Exit point for that code
    llvm::BasicBlock *  failbb;         // Where we go if tests fail

    value_map           value;          // Map tree -> LLVM value
    value_map           storage;        // Map tree -> LLVM alloca space
    value_map           computed;       // Map tree -> LLVM "computed" flag
    data_set            dataForm;       // Data expressions we don't evaluate
};


struct ExpressionReduction
// ----------------------------------------------------------------------------
//   Record compilation state around a specific expression reduction
// ----------------------------------------------------------------------------
{
    ExpressionReduction(CompiledUnit &unit, Tree *source);
    ~ExpressionReduction();

    void                NewForm();
    void                Succeeded();
    void                Failed();

public:
    CompiledUnit &      unit;           // Compilation unit we use
    Tree *              source;         // Tree we build (mostly for debugging)
    llvm::LLVMContext * context;        // Inherited context

    llvm::Value *       storage;        // Storage for expression value
    llvm::Value *       computed;       // Flag telling if value was computed

    llvm::BasicBlock *  savedfailbb;    // Saved location of failbb

    llvm::BasicBlock *  entrybb;        // Entry point to subcase
    llvm::BasicBlock *  savedbb;        // Saved position before subcase
    llvm::BasicBlock *  successbb;      // Successful completion of expression

    value_map           savedvalue;     // Saved compile unit value map
};


struct CompilerGarbageCollectionListener : TypeAllocator::Listener
// ----------------------------------------------------------------------------
//   Listen to the garbage collection to put away LLVM data structures
// ----------------------------------------------------------------------------
{
    CompilerGarbageCollectionListener(Compiler *compiler)
        : compiler(compiler) {}

    virtual void BeginCollection();
    virtual bool CanDelete (void *obj);
    virtual void EndCollection();

    Compiler *compiler;
};


struct CompilerInfo : Info
// ----------------------------------------------------------------------------
//   Information about compiler-related data structures
// ----------------------------------------------------------------------------
{
    CompilerInfo(Tree *tree): tree(tree), global(0), function(0) {}
    ~CompilerInfo();
    Tree *                      tree;
    llvm::GlobalValue *         global;
    llvm::Function *            function;
};


#define LLVM_INTTYPE(t)         llvm::IntegerType::get(*context, sizeof(t) * 8)
#define LLVM_BOOLTYPE           llvm::Type::getInt1Ty(*context)

XL_END

extern void debugv(void *);

#endif // COMPILER_H
