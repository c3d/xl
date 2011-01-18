#ifndef COMPILER_UNIT_H
#define COMPILER_UNIT_H
// ****************************************************************************
//  unit.h                                                          XLR project
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
#include "parms.h"


XL_BEGIN

struct TypeInference;
typedef GCPtr<TypeInference> TypeInference_p;


struct CompiledUnit
// ----------------------------------------------------------------------------
//  The function we generate for a given rewrite
// ----------------------------------------------------------------------------
{
    CompiledUnit(Compiler *compiler, Context *);
    ~CompiledUnit();

public:
    llvm_function       TopLevelFunction();
    llvm_function       ClosureFunction(Tree *expr, TypeInference *types);
    llvm_function       RewriteFunction(RewriteCandidate &rc);

protected:
    llvm_function       InitializeFunction(llvm::FunctionType *,
                                           Parameters *parameters,
                                           kstring label,
                                           bool global,
                                           bool isC);
    bool                Signature(Parameters &parms, RewriteCandidate &rc,
                                  llvm_types &signature);

public:
    bool                TypeCheck(Tree *program);
    llvm_value          Compile(Tree *tree);
    llvm_value          Compile(RewriteCandidate &rc, llvm_values &args);
    llvm_value          Closure(Name *name, Tree *value);
    llvm_value          InvokeClosure(llvm_value result, llvm_value fnPtr);
    llvm_value          InvokeClosure(llvm_value result);
    llvm_value          Return(llvm_value value);
    eval_fn             Finalize(bool createCode);

    enum { knowAll = -1, knowGlobals = 1, knowLocals = 2, knowValues = 4 };
    llvm_value          NeedStorage(Tree *tree);
    llvm_value          NeedClosure(Tree *tree);
    bool                IsKnown(Tree *tree, uint which = knowAll);
    llvm_value          Known(Tree *tree, uint which = knowAll );
    void                ImportClosureInfo(const CompiledUnit *other);

    llvm_value          ConstantInteger(Integer *what);
    llvm_value          ConstantReal(Real *what);
    llvm_value          ConstantText(Text *what);
    llvm_value          ConstantTree(Tree *what);

    llvm_value          CallFormError(Tree *what);

    llvm_type           ReturnType(Tree *form);
    llvm_type           StructureType(llvm_types &signature);
    llvm_type           ExpressionMachineType(Tree *expr, llvm_type type);
    llvm_type           ExpressionMachineType(Tree *expr);
    llvm_type           MachineType(Tree *type);
    llvm_value          Autobox(llvm_value value, llvm_type requested);
    llvm_value          Global(Tree *tree);

    static bool         ValidCName(Tree *tree, text &label);

public:
    Context_p           context;        // Context in which we compile
    TypeInference_p     inference;      // Type inferences for this unit

    Compiler *          compiler;       // The compiler environment we use
    llvm::LLVMContext * llvm;           // The LLVM context we got from compiler

    llvm_builder        code;           // Instruction builder for code
    llvm_builder        data;           // Instruction builder for data
    llvm_function       function;       // Function we generate

    llvm_block          allocabb;       // Function entry point, allocas
    llvm_block          entrybb;        // Code entry point
    llvm_block          exitbb;         // Shared exit for the function
    llvm_value          returned;       // Where we store the returned value

    value_map           value;          // Map tree -> LLVM value
    value_map           storage;        // Map tree -> LLVM alloca space
    type_map            machineType;    // Map tree -> machine type

    llvm::PATypeHolder  closureTy;      // Argument type for closures
    value_map           closure;        // Arguments that need closures
};

XL_END

#endif // COMPILER_UNIT_H
