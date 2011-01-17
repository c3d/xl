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



// ============================================================================
// 
//    Forward declarations
// 
// ============================================================================

namespace llvm
// ----------------------------------------------------------------------------
//   Forward classes from LLVM
// ----------------------------------------------------------------------------
{
    class LLVMContext;
    class Module;
    class ExecutionEngine;
    class FunctionPassManager;
    class PassManager;
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
struct CompilerLLVMTableEntry;
struct RewriteCandidate;
typedef Tree * (*program_fn) (void);
typedef Tree * (*eval_fn) (Tree *);
typedef Tree * (*adapter_fn) (native_fn callee, Context *ctx,
                              Tree *src, Tree **args);
typedef std::map<text, llvm::Function *>       functions_map;
typedef std::map<Tree *, llvm::Value *>        value_map;
typedef std::map<Tree *, Tree **>              address_map;
typedef std::map<text, llvm::GlobalVariable *> text_constants_map;
typedef std::map<uint, eval_fn>                closure_map;
typedef std::map<uint, adapter_fn>             adapter_map;
typedef std::set<Tree *>                       closure_set;
typedef std::set<Tree *>                       data_set;
typedef std::map<text,CompilerLLVMTableEntry *>llvm_entry_table;

typedef const llvm::Type *                     llvm_type;
typedef std::vector<llvm_type>                 llvm_types;
typedef llvm::Value *                          llvm_value;
typedef std::vector<llvm_value>                llvm_values;
typedef llvm::Constant *                       llvm_constant;
typedef std::vector<llvm_constant>             llvm_constants;
typedef llvm::IRBuilder<> *                    llvm_builder;
typedef llvm::Function *                       llvm_function;
typedef llvm::BasicBlock *                     llvm_block;



// ============================================================================
// 
//    Global structures to access the LLVM just-in-time compiler
// 
// ============================================================================

struct Compiler
// ----------------------------------------------------------------------------
//   Just-in-time compiler data
// ----------------------------------------------------------------------------
{
    Compiler(kstring moduleName = "xl");
    ~Compiler();

    // Top-level entry point: analyze and compile a whole program
    program_fn                CompileProgram(Context *context, Tree *program);

    void                      Setup(Options &options);
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
    llvm::GlobalVariable *    TextConstant(text value);
    eval_fn                   MarkAsClosure(Tree *closure, uint ntrees);
    bool                      IsKnown(Tree *value);

    llvm_type                 MachineType(Tree *tree);
    llvm_value                Primitive(llvm_builder builder, text name,
                                        uint arity, llvm_value *args);
    bool                      MarkAsClosureType(llvm_type type);
    bool                      IsClosureType(llvm_type type);

    text                      FunctionKey(RewriteCandidate &rc);
    text                      ClosureKey(Tree *expr, Context *context);
    llvm::Function * &        FunctionFor(text fkey) { return functions[fkey]; }

    bool                      FreeResources(Tree *tree);


public:
    llvm::LLVMContext         *llvm;
    llvm::Module              *module;
    llvm::ExecutionEngine     *runtime;
    llvm::FunctionPassManager *optimizer;
    llvm::PassManager         *moduleOptimizer;
    const llvm::IntegerType   *booleanTy;
    const llvm::IntegerType   *integerTy;
    const llvm::IntegerType   *integer8Ty;
    const llvm::IntegerType   *integer16Ty;
    const llvm::IntegerType   *integer32Ty;
    const llvm::Type          *realTy;
    const llvm::Type          *real32Ty;
    const llvm::IntegerType   *characterTy;
    llvm::PointerType         *charPtrTy;
    llvm::StructType          *textTy;
    llvm::StructType          *treeTy;
    llvm::PointerType         *treePtrTy;
    llvm::PointerType         *treePtrPtrTy;
    llvm::StructType          *integerTreeTy;
    llvm::PointerType         *integerTreePtrTy;
    llvm::StructType          *realTreeTy;
    llvm::PointerType         *realTreePtrTy;
    llvm::StructType          *textTreeTy;
    llvm::PointerType         *textTreePtrTy;
    llvm::StructType          *nameTreeTy;
    llvm::PointerType         *nameTreePtrTy;
    llvm::StructType          *blockTreeTy;
    llvm::PointerType         *blockTreePtrTy;
    llvm::StructType          *prefixTreeTy;
    llvm::PointerType         *prefixTreePtrTy;
    llvm::StructType          *postfixTreeTy;
    llvm::PointerType         *postfixTreePtrTy;
    llvm::StructType          *infixTreeTy;
    llvm::PointerType         *infixTreePtrTy;
    llvm::FunctionType        *nativeTy;
    llvm::PointerType         *nativeFnTy;
    llvm::FunctionType        *evalTy;
    llvm::PointerType         *evalFnTy;
    llvm::PointerType         *infoPtrTy;
    llvm::PointerType         *contextPtrTy;
    llvm::Function            *strcmp_fn;
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
    llvm::Function            *xl_new_ctext;
    llvm::Function            *xl_new_xtext;
    llvm::Function            *xl_new_block;
    llvm::Function            *xl_new_prefix;
    llvm::Function            *xl_new_postfix;
    llvm::Function            *xl_new_infix;
    llvm::Function            *xl_new_closure;
    functions_map              builtins;
    functions_map              functions;
    adapter_map                array_to_args_adapters;
    text_constants_map         text_constants;
    llvm_entry_table           llvm_primitives;
    llvm_types                 closure_types;
};



// ============================================================================
// 
//   Useful macros
// 
// ============================================================================

#define LLVM_INTTYPE(t)         llvm::IntegerType::get(*llvm, sizeof(t) * 8)
#define LLVM_BOOLTYPE           llvm::Type::getInt1Ty(*llvm)

// Index in data structures of fields in Tree types
#define TAG_INDEX           0
#define INFO_INDEX          1
#define INTEGER_VALUE_INDEX 2
#define REAL_VALUE_INDEX    2
#define TEXT_VALUE_INDEX    2
#define NAME_VALUE_INDEX    2
#define BLOCK_CHILD_INDEX   2
#define BLOCK_OPENING_INDEX 3
#define BLOCK_CLOSING_INDEX 4
#define LEFT_VALUE_INDEX    2
#define RIGHT_VALUE_INDEX   3
#define INFIX_NAME_INDEX    4

XL_END

extern void debugv(llvm::Value *);
extern void debugv(llvm::Type *);

#endif // COMPILER_H
