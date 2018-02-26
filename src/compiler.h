#ifndef COMPILER_H
#define COMPILER_H
// ****************************************************************************
//  compiler.h                       (C) 1992-2009 Christophe de Dinechin (ddd)
//                                                               XL project
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

#include "tree.h"
#include "context.h"
#include "llvm-crap.h"
#include <map>
#include <set>



// ============================================================================
//
//    Forward declarations
//
// ============================================================================

XL_BEGIN

struct CompiledUnit;
struct CompilerInfo;
struct Context;
struct Options;
struct CompilerLLVMTableEntry;
struct RewriteCandidate;

// llvm_type defined in llvm-crap.h, 3.0 API breakage
typedef std::vector<llvm_type>                 llvm_types;
typedef std::vector<llvm_value>                llvm_values;
typedef llvm::Constant *                       llvm_constant;
typedef std::vector<llvm_constant>             llvm_constants;
typedef llvm::Function *                       llvm_function;
typedef llvm::BasicBlock *                     llvm_block;

typedef Tree *(*adapter_fn) (eval_fn callee,Context *ctx,Tree *src,Tree **args);
typedef std::map<text, llvm_function>          functions_map;
typedef std::map<Tree *, llvm_value>           value_map;
typedef std::map<Tree *, llvm_type>            type_map;
typedef std::map<llvm_type, Tree *>            unboxing_map;
typedef std::map<Tree *, Tree **>              address_map;
typedef std::map<text, llvm::GlobalVariable *> text_constants_map;
typedef std::map<uint, eval_fn>                closure_map;
typedef std::map<uint, adapter_fn>             adapter_map;
typedef std::set<Tree *>                       closure_set;
typedef std::set<Tree *>                       data_set;
typedef std::map<text,CompilerLLVMTableEntry *>llvm_entry_table;


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
    Compiler(kstring moduleName, int argc, char **argv);
    ~Compiler();

    // Top-level entry point: analyze and compile a tree or a whole program
    eval_fn                   Compile(Context *context, Tree *program);

    void                      Setup(Options &options);
    void                      Reset();
    CompilerInfo *            Info(Tree *tree, bool create = false);
    llvm::Function *          TreeFunction(Tree *tree);
    void                      SetTreeFunction(Tree *tree, llvm::Function *);
    llvm::Function *          TreeClosure(Tree *tree);
    void                      SetTreeClosure(Tree *tree, llvm::Function *);
    llvm::GlobalValue *       TreeGlobal(Tree *tree);
    void                      SetTreeGlobal(Tree*, llvm::GlobalValue*, void*);
    llvm::Function *          EnterBuiltin(text name,
                                           Tree *from, Tree *to,
                                           eval_fn code);
    llvm::Function *          ExternFunction(kstring name, void *address,
                                             llvm_type retType,
                                             int parmCount, ...);
    adapter_fn                ArrayToArgsAdapter(uint numtrees);
    llvm::Value *             EnterGlobal(Name *name, Name_p *address);
    llvm::Value *             EnterConstant(Tree *constant);
    llvm::GlobalVariable *    TextConstant(text value);
    eval_fn                   MarkAsClosure(Tree *closure, uint ntrees);
    bool                      IsKnown(Tree *value);

    void                      MachineType(Tree *source, llvm_type mtype);
    llvm_type                 MachineType(Tree *tree);
    llvm_type                 TreeMachineType(Tree *tree);
    bool                      IsTreePtrType(llvm_type t);
    bool                      IsIntegerType(llvm_type t);
    bool                      IsRealType(llvm_type t);
    bool                      CanCastMachineType(llvm_type from, llvm_type to);
    llvm_function             UnboxFunction(Context_p ctx, llvm_type type,
                                            Tree *form);
    llvm_value                Primitive(CompiledUnit &unit,
                                        llvm_builder builder, text name,
                                        uint arity, llvm_value *args);
    bool                      MarkAsClosureType(llvm_type type);
    bool                      IsClosureType(llvm_type type);

    text                      FunctionKey(Infix *rw, llvm_values &values);
    text                      ClosureKey(Tree *expr, Context *context);
    llvm::Function * &        FunctionFor(text fkey) { return functions[fkey]; }

    bool                      FreeResources(Tree *tree);
    void                      Dump();


public:
    llvm::LLVMContext            &llvm;
    llvm::Module                 *module;
    llvm::ExecutionEngine        *runtime;
    LLVMCrap_FunctionPassManager *optimizer;
    LLVMCrap_PassManager         *moduleOptimizer;
    llvm_integer_type             booleanTy;
    llvm_integer_type             integerTy;
    llvm_integer_type             integer8Ty;
    llvm_integer_type             integer16Ty;
    llvm_integer_type             integer32Ty;
    llvm_type                     realTy;
    llvm_type                     real32Ty;
    llvm_integer_type             characterTy;
    llvm::PointerType            *charPtrTy;
    llvm::PointerType            *charPtrPtrTy;
    llvm::StructType             *textTy;
    llvm::StructType             *treeTy;
    llvm::PointerType            *treePtrTy;
    llvm::PointerType            *treePtrPtrTy;
    llvm::StructType             *integerTreeTy;
    llvm::PointerType            *integerTreePtrTy;
    llvm::StructType             *realTreeTy;
    llvm::PointerType            *realTreePtrTy;
    llvm::StructType             *textTreeTy;
    llvm::PointerType            *textTreePtrTy;
    llvm::StructType             *nameTreeTy;
    llvm::PointerType            *nameTreePtrTy;
    llvm::StructType             *blockTreeTy;
    llvm::PointerType            *blockTreePtrTy;
    llvm::StructType             *prefixTreeTy;
    llvm::PointerType            *prefixTreePtrTy;
    llvm::StructType             *postfixTreeTy;
    llvm::PointerType            *postfixTreePtrTy;
    llvm::StructType             *infixTreeTy;
    llvm::PointerType            *infixTreePtrTy;
    llvm::FunctionType           *nativeTy;
    llvm::PointerType            *nativeFnTy;
    llvm::FunctionType           *evalTy;
    llvm::PointerType            *evalFnTy;
    llvm::PointerType            *infoPtrTy;
    llvm::PointerType            *contextPtrTy;
    llvm::Function               *strcmp_fn;
    llvm::Function               *xl_same_shape;
    llvm::Function               *xl_form_error;
    llvm::Function               *xl_stack_overflow;
    llvm::Function               *xl_new_integer;
    llvm::Function               *xl_new_real;
    llvm::Function               *xl_new_character;
    llvm::Function               *xl_new_text;
    llvm::Function               *xl_new_ctext;
    llvm::Function               *xl_new_xtext;
    llvm::Function               *xl_new_block;
    llvm::Function               *xl_new_prefix;
    llvm::Function               *xl_new_postfix;
    llvm::Function               *xl_new_infix;
    llvm::Constant               *xl_recursion_count_ptr;
    functions_map                 builtins;
    functions_map                 functions;
    adapter_map                   array_to_args_adapters;
    closure_map                   closures;
    text_constants_map            text_constants;
    llvm_entry_table              llvm_primitives;
    llvm_types                    closure_types;
    type_map                      machineTypes;
};


inline bool Compiler::IsTreePtrType(llvm_type t)
// ----------------------------------------------------------------------------
//   Check if it's any of the tree ptr types
// ----------------------------------------------------------------------------
{
    return (t == blockTreePtrTy         ||
            t == prefixTreePtrTy        ||
            t == postfixTreePtrTy       ||
            t == infixTreePtrTy         ||
            t == treePtrTy);
}


inline bool Compiler::IsIntegerType(llvm_type t)
// ----------------------------------------------------------------------------
//   Check if it's any of the integer types
// ----------------------------------------------------------------------------
{
    return (t == booleanTy              ||
            t == integerTy              ||
            t == integer8Ty             ||
            t == integer16Ty            ||
            t == integer32Ty            ||
            t == characterTy);
}


inline bool Compiler::IsRealType(llvm_type t)
// ----------------------------------------------------------------------------
//   Check if it's any of the integer types
// ----------------------------------------------------------------------------
{
    return (t == realTy || t == real32Ty);
}



// ============================================================================
//
//   Useful macros
//
// ============================================================================

#define LLVM_INTTYPE(t)         llvm::IntegerType::get(llvm, sizeof(t) * 8)
#define LLVM_BOOLTYPE           llvm::Type::getInt1Ty(llvm)

// Index in data structures of fields in Tree types
#define TAG_INDEX           0
#define INFO_INDEX          1
#define INTEGER_VALUE_INDEX 2
#define REAL_VALUE_INDEX    2
#define TEXT_VALUE_INDEX    2
#define TEXT_OPENING_INDEX  3
#define TEXT_CLOSING_INDEX  4
#define NAME_VALUE_INDEX    2
#define BLOCK_CHILD_INDEX   2
#define BLOCK_OPENING_INDEX 3
#define BLOCK_CLOSING_INDEX 4
#define LEFT_VALUE_INDEX    2
#define RIGHT_VALUE_INDEX   3
#define INFIX_NAME_INDEX    4

XL_END

RECORDER_DECLARE(compiler);
RECORDER_DECLARE(llvm);
RECORDER_TWEAK_DECLARE(labels);
RECORDER_TWEAK_DECLARE(llvm_ir);

extern void debugv(llvm::Value *);
extern void debugv(llvm::Type *);

#endif // COMPILER_H
