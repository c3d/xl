#ifndef COMPILER_UNIT_H
#define COMPILER_UNIT_H
// ****************************************************************************
//  compiler-unit.h                                                XL project
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

#include "compiler.h"
#include "compiler-parms.h"
#include "types.h"


XL_BEGIN

struct Types;
struct CompilerPrimitive;
struct RewriteCandidate;
typedef GCPtr<Types> Types_p;

typedef Tree *(*adapter_fn) (eval_fn callee,Context *ctx,Tree *src,Tree **args);
typedef std::map<Tree *, Value_p>          value_map;
typedef std::map<Tree *, Type_p>           mtype_map;
typedef std::map<Type_p, Tree *>           unboxing_map;
typedef std::map<uint, eval_fn>            closure_map;
typedef std::map<uint, adapter_fn>         adapter_map;
typedef std::set<Tree *>                   closure_set;
typedef std::set<Tree *>                   data_set;
typedef std::map<text,CompilerPrimitive *> primitive_map;


struct CompilerUnit
// ----------------------------------------------------------------------------
//  The function we generate for a given rewrite
// ----------------------------------------------------------------------------
{
    CompilerUnit(Compiler &compiler, Scope *, Tree *);
    CompilerUnit(Compiler &compiler, Scope *, Tree *,
                 Function_p function,
                 const Types &types,
                 const mtype_map &mtypes);
    ~CompilerUnit();

public:
    Function_p  EvaluationFunctionPrototype();
    bool        TypeAnalysis();


    Function_p  ClosureFunction(Tree *expr, Types *types);
    Function_p  RewriteFunction(RewriteCandidate &rc);

    Function_p  InitializeFunction(FunctionType_p,
                                   Parameters *parameters,
                                   kstring label,
                                   bool global,
                                   bool isC);
    bool        ExtractSignature(Parameters &parms,
                                 RewriteCandidate &rc,
                                 Signature &signature);

public:
    Value_p     Compile(Tree *tree, bool forceEvaluation = false);
    Value_p     Compile(RewriteCandidate &rc, Values &args);
    Value_p     Data(Tree *form, uint &index);
    Value_p     Unbox(Value_p arg, Tree *form, uint &index);
    Function_p  UnboxFunction(Context_p ctx, Type_p type, Tree *form);
    Value_p     Primitive(CompilerUnit &unit,
                          text name, uint arity, Value_p *args);

    Function_p  ExternFunction(kstring name, void *address,
                                       Type_p retType, int parmCount, ...);
    adapter_fn  ArrayToArgsAdapter(uint numtrees);

    text        FunctionKey(Rewrite *rw, Values &values);
    text        ClosureKey(Tree *expr, Context *context);


    Value_p     Closure(Name *name, Tree *value);
    Value_p     InvokeClosure(Value_p result, Value_p fnPtr);
    Value_p     InvokeClosure(Value_p result);
    Value_p     Return(Value_p value);
    eval_fn     Finalize(bool createCode);

    enum { knowAll = -1, knowGlobals = 1, knowLocals = 2, knowValues = 4 };
    Value_p     NeedStorage(Tree *tree);
    Value_p     NeedClosure(Tree *tree);
    bool        IsKnown(Tree *tree, uint which = knowAll);
    Value_p     Known(Tree *tree, uint which = knowAll );
    void        ImportClosureInfo(const CompilerUnit &other);

    Value_p     ConstantInteger(Integer *what);
    Value_p     ConstantReal(Real *what);
    Value_p     ConstantText(Text *what);
    Value_p     ConstantTree(Tree *what);

    Value_p     CallFormError(Tree *what);

    Type_p      ReturnType(Tree *form);
    Type_p      StructureType(Signature &signature, Tree *source);
    Type_p      ExpressionMachineType(Tree *expr, Type_p type);
    Type_p      ExpressionMachineType(Tree *expr);
    Type_p      MachineType(Tree *type);
    void        InheritMachineTypes(CompilerUnit &unit);
    Value_p     Autobox(Value_p value, Type_p requested);
    Value_p     Global(Tree *tree);

    static bool ValidCName(Tree *tree, text &label);

public:
    Compiler &  compiler;       // The compiler environment we use
    JIT &       jit;            // The JIT compiler (LLVM API stabilizer)
    Context     context;        // Context in which we compile
    Tree_p      source;         // Source for what is being compiled
    Function_p  function;       // The function we currently generate
    Types       types;          // Type inferences for this unit
    JITBlock    data;           // A basic block for local variables
    JITBlock    code;           // A basic block for current code
    JITBlock    exit;           // A basic block for shared exit
    Value_p     returned;       // Returned value
    value_map   values;         // Map tree -> LLVM value
    value_map   storage;        // Map tree -> LLVM alloca space
    mtype_map   mtypes;          // Map tree -> machine type
    closure_set captured;       // List of trees captured

#define UNARY(Name)
#define BINARY(Name)
#define CAST(Name)
#define ALIAS(Name, Arity, Original)
#define SPECIAL(Name, Arity, Code)
#define EXTERNAL(Name, RetTy, ...)      Function_p  Name;
#include "compiler-llvm.tbl"
};

XL_END

RECORDER_DECLARE(unit);

#endif // COMPILER_UNIT_H
