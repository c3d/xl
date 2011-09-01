// ****************************************************************************
//  unit.cpp                                                        XLR project
// ****************************************************************************
//
//   File Description:
//
//     Information about a single compilation unit, i.e. the code generated
//     for a particular tree
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
//
// The compilation unit is where most of the "action" happens, e.g. where
// the code generation happens for a given tree. It records all information
// that is transient, i.e. only exists during a given compilation phase
//
// In the following, we will consider a rewrite such as:
//    foo X:integer, Y -> bar X + Y
//
// Such a rewrite is transformed into a function with a prototype that
// depends on the arguments, i.e. something like:
//    retType foo(int X, Tree *Y);
//
// The actual retType is determined dynamically from the return type of bar.

#include "unit.h"
#include "parms.h"
#include "args.h"
#include "expred.h"
#include "errors.h"
#include "types.h"

#include <llvm/Analysis/Verifier.h>
#include <llvm/CallingConv.h>
#include "llvm/Constants.h"
#include "llvm/LLVMContext.h"
#include "llvm/DerivedTypes.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include <llvm/PassManager.h>
#include "llvm/Support/raw_ostream.h"
#include <llvm/Support/IRBuilder.h>
#include <llvm/Support/StandardPasses.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Target/TargetData.h>
#include <llvm/Target/TargetSelect.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Support/raw_ostream.h>
#include <stdio.h>


XL_BEGIN

using namespace llvm;

CompiledUnit::CompiledUnit(Compiler *compiler, Context *context)
// ----------------------------------------------------------------------------
//   CompiledUnit constructor
// ----------------------------------------------------------------------------
    : context(context), inference(NULL),
      compiler(compiler), llvm(compiler->llvm),
      code(NULL), data(NULL), function(NULL),
      allocabb(NULL), entrybb(NULL), exitbb(NULL),
      returned(NULL),
      value(), storage(),
      closureTy(), closure()
{}


CompiledUnit::~CompiledUnit()
// ----------------------------------------------------------------------------
//   Delete what we must...
// ----------------------------------------------------------------------------
{
    if (entrybb && exitbb)
    {
        // If entrybb is clear, we may be looking at a forward declaration
        // Otherwise, if exitbb was not cleared by Finalize(), this means we
        // failed to compile. Make sure LLVM cleans the function up
        function->eraseFromParent();
    }

    delete code;
    delete data;
}


Function *CompiledUnit::TopLevelFunction()
// ----------------------------------------------------------------------------
//   Create a function for a top-level program
// ----------------------------------------------------------------------------
{
    // We must have verified the types before
    assert(inference || !"TopLevelFunction called without type check");

    llvm_types signature;
    ParameterList parameters(this);
    llvm_type retTy = compiler->treePtrTy;
    FunctionType *fnTy = FunctionType::get(retTy, signature, false);
    return InitializeFunction(fnTy, &parameters.parameters,
                              "xl_program", true, false);
}


Function *CompiledUnit::ClosureFunction(Tree *expr, TypeInference *types)
// ----------------------------------------------------------------------------
//   Create a function for a closure
// ----------------------------------------------------------------------------
{
    // We must have verified the types before
    assert((types && !inference) || !"ClosureFunction botched types");
    inference = types;

    // We have a closure type that we will build as we evaluate expression
    closureTy = OpaqueType::get(*llvm);
    static char buffer[80]; static int count = 0;
    snprintf(buffer, 80, "closure%d", count++);
    compiler->module->addTypeName(buffer, closureTy);

    // Add a single parameter to the signature
    llvm_types signature;
    llvm_type closurePtrTy = PointerType::get(closureTy, 0);
    signature.push_back(closurePtrTy);

    // Figure out the return type and function type
    Tree *rtype = inference->Type(expr);
    llvm_type retTy = compiler->MachineType(rtype);
    FunctionType *fnTy = FunctionType::get(retTy, signature, false);
    llvm_function fn = InitializeFunction(fnTy,NULL,"xl_closure",true,false);

    // Return the function
    return fn;
}


Function *CompiledUnit::RewriteFunction(RewriteCandidate &rc)
// ----------------------------------------------------------------------------
//   Create a function for a tree rewrite
// ----------------------------------------------------------------------------
{
    TypeInference *types = rc.types;
    Rewrite *rewrite = rc.rewrite;

    // We must have verified the types before
    assert((types && !inference) || !"RewriteFunction: bogus type check");
    inference = types;

    Tree *source = rewrite->from;
    Tree *def = rewrite->to;
    IFTRACE(llvm)
        std::cerr << "CompiledUnit::RewriteFunction T" << (void *) source;

    // Extract parameters from source form
    ParameterList parameters(this);
    if (!source->Do(parameters))
        return NULL;

    // Create the function signature, one entry per parameter
    llvm_types signature;
    Signature(parameters.parameters, rc, signature);

    // Compute return type:
    // - If explicitly specified, use that (TODO: Check compatibility)
    // - For definitions, infer from definition
    // - For data forms, this is the type of the data form
    llvm_type retTy;
    if (llvm_type specifiedRetTy = parameters.returned)
        retTy = specifiedRetTy;
    else if (def)
        retTy = ReturnType(def);
    else
        retTy = StructureType(signature);

    text label = "xl_eval_" + parameters.name;
    IFTRACE(labels)
        label += "[" + text(*source) + "]";

    // Check if we are actually declaring a C function
    bool isC = false;
    bool isVararg = false;
    if (Tree *defined = parameters.defined)
    {
        if (Name *name = def->AsName())
            if (name->value == "C")
                if (ValidCName(defined, label))
                    isC = true;

        if (Prefix *prefix = def->AsPrefix())
            if (Name *name = prefix->left->AsName())
                if (name->value == "C")
                    if (ValidCName(prefix->right, label))
                        isC = true;
    }

    FunctionType *fnTy = FunctionType::get(retTy, signature, isVararg);
    Function *f = InitializeFunction(fnTy, &parameters.parameters,
                                     label.c_str(), isC, isC);
    if (isC)
    {
        void *address = sys::DynamicLibrary::SearchForAddressOfSymbol(label);
        if (!address)
        {
            Ooops("No library function matching $1", rewrite->from);
            return NULL;
        }
        sys::DynamicLibrary::AddSymbol(label, address);
    }
    return f;
}


Function *CompiledUnit::InitializeFunction(FunctionType *fnTy,
                                           Parameters *parameters,
                                           kstring label,
                                           bool global, bool isC)
// ----------------------------------------------------------------------------
//   Build the LLVM function, create entry points, ...
// ----------------------------------------------------------------------------
{
    assert (!function || !"LLVM function was already built");

    // Create function and save it in the CompiledUnit
    function = Function::Create(fnTy,
                                global
                                ? Function::ExternalLinkage
                                : Function::InternalLinkage,
                                label, compiler->module);
    IFTRACE(llvm)
        std::cerr << " new F" << function << "\n";

    if (!isC)
    {
        // Create function entry point, where we will have all allocas
        allocabb = BasicBlock::Create(*llvm, "allocas", function);
        data = new IRBuilder<> (allocabb);

        // Create entry block for the function
        entrybb = BasicBlock::Create(*llvm, "entry", function);
        code = new IRBuilder<> (entrybb);

        // Build storage for the return value
        llvm_type retTy = function->getReturnType();
        returned = data->CreateAlloca(retTy, 0, "result");

        if (parameters)
        {
            // Associate the value for the additional arguments
            // (read-only, no alloca)
            Function::arg_iterator args = function->arg_begin();
            Parameters &plist = *parameters;
            for (Parameters::iterator p = plist.begin(); p != plist.end(); p++)
            {
                Parameter &parm = *p;
                llvm_value inputArg = args++;
                value[parm.name] = inputArg;
            }
        }

        // Create the exit basic block and return statement
        exitbb = BasicBlock::Create(*llvm, "exit", function);
        IRBuilder<> exitcode(exitbb);
        Value *retVal = exitcode.CreateLoad(returned, "retval");
        exitcode.CreateRet(retVal);
    }

    // Return the newly created function
    return function;
}


bool CompiledUnit::Signature(Parameters &parms,
                             RewriteCandidate &rc,
                             llvm_types &signature)
// ----------------------------------------------------------------------------
//   Extract the types from the parameter list
// ----------------------------------------------------------------------------
{
    bool hasClosures = false;

    RewriteBindings &bnds = rc.bindings;
    RewriteBindings::iterator b = bnds.begin();
    for (Parameters::iterator p = parms.begin(); p != parms.end(); p++, b++)
    {
        assert (b != bnds.end());
        RewriteBinding &binding = *b;
        if (llvm_value closure = binding.closure)
        {
            // Deferred evaluation: pass evaluation function pointer and arg
            llvm_type argTy = closure->getType();
            signature.push_back(argTy);
            hasClosures = true;
        }
        else
        {
            // Regular evaluation: just pass argument around
            signature.push_back((*p).type);
        }
    }

    return hasClosures;
}


bool CompiledUnit::TypeCheck(Tree *program)
// ----------------------------------------------------------------------------
//   Verify that the given program/expression is valid in current context
// ----------------------------------------------------------------------------
{
    TypeInference_p inferTypes = new TypeInference(context);
    bool result = inferTypes->TypeCheck(program);
    if (result)
        inference = inferTypes;
    return result;
}


llvm_value CompiledUnit::CompileTopLevel(Tree *tree)
// ----------------------------------------------------------------------------
//    Compile a given tree at top level (evaluate closures)
// ----------------------------------------------------------------------------
{
    assert (inference || !"Compile() called without type checking");
    CompileExpression cexpr(this);
    llvm_value result = cexpr.TopLevelEvaluation(tree);
    return result;
}


llvm_value CompiledUnit::Compile(Tree *tree)
// ----------------------------------------------------------------------------
//    Compile a given tree
// ----------------------------------------------------------------------------
{
    assert (inference || !"Compile() called without type checking");
    CompileExpression cexpr(this);
    llvm_value result = tree->Do(cexpr);
    return result;
}


llvm_value CompiledUnit::Compile(RewriteCandidate &rc, llvm_values &args)
// ----------------------------------------------------------------------------
//    Compile a given tree
// ----------------------------------------------------------------------------
{
    // Check if we already have built this function, e.g. recursive calls
    text fkey = compiler->FunctionKey(rc.rewrite, args);
    llvm::Function *&function = compiler->FunctionFor(fkey);

    // If we have not, then we need to build it
    if (function == NULL)
    {
        TypeInference *types = rc.types;
        Rewrite *rewrite = rc.rewrite;
        Context_p rewriteContext = types->context;
        CompiledUnit rewriteUnit(compiler, rewriteContext);

        function = rewriteUnit.RewriteFunction(rc);
        if (function && rewriteUnit.code)
        {
            rewriteUnit.ImportClosureInfo(this);
            llvm_value returned = rewriteUnit.CompileTopLevel(rewrite->to);
            if (!returned)
                return NULL;
            if (!rewriteUnit.Return(returned))
                return NULL;
            rewriteUnit.Finalize(false);
        }
    }
    return function;
}


llvm_value CompiledUnit::Closure(Name *name, Tree *expr)
// ----------------------------------------------------------------------------
//    Compile code to pass a given tree as a closure
// ----------------------------------------------------------------------------
//    Closures are represented as functions taking a pointer to a structure
//    that will contain the values being used by the closure code
{
    // Record the function that we build
    text fkey = compiler->ClosureKey(expr, context);
    llvm_function &function = compiler->FunctionFor(fkey);
    assert (function == NULL);

    // Create the evaluation function
    CompiledUnit cunit(compiler, context);
    function = cunit.ClosureFunction(expr, inference);
    if (!function || !cunit.code || !cunit.closureTy)
        return NULL;
    cunit.ImportClosureInfo(this);
    llvm_value returned = cunit.CompileTopLevel(expr);
    if (!returned)
        return NULL;
    if (!cunit.Return(returned))
        return NULL;
    cunit.Finalize(false);

    // Values imported from closure are now in cunit.closure[]
    // Allocate a local data block to pass as the closure
    llvm_value stackPtr = data->CreateAlloca(cunit.closureTy);
    compiler->MarkAsClosureType(stackPtr->getType());

    // First, store the function pointer
    uint field = 0;
    llvm_value fptr = code->CreateConstGEP2_32(stackPtr, 0, field++);
    code->CreateStore(function, fptr);

    // Then loop over all values that were detected while evaluating expr
    value_map &cls = cunit.closure;
    for (value_map::iterator v = cls.begin(); v != cls.end(); v++)
    {
        Tree *subexpr = (*v).first;
        llvm_value subval = Compile(subexpr);
        fptr = code->CreateConstGEP2_32(stackPtr, 0, field++);
        code->CreateStore(subval, fptr);
    }

    // Remember the machine type associated with this closure
    llvm_type mtype = stackPtr->getType();
    ExpressionMachineType(name, mtype);

    // Return the stack pointer that we'll use later to evaluate the closure
    return stackPtr;
}


llvm_value CompiledUnit::InvokeClosure(llvm_value result, llvm_value fnPtr)
// ----------------------------------------------------------------------------
//   Invoke a closure with a known closure function
// ----------------------------------------------------------------------------
{
    result = code->CreateCall(fnPtr, result);
    return result;
}


llvm_value CompiledUnit::InvokeClosure(llvm_value result)
// ----------------------------------------------------------------------------
//   Invoke a closure loading the function pointer dynamically
// ----------------------------------------------------------------------------
{
    // Get function pointer and argument
    llvm_value fnPtrPtr = data->CreateConstGEP2_32(result, 0, 0);
    llvm_value fnPtr = data->CreateLoad(fnPtrPtr);

    // Call the closure callback
    result = InvokeClosure(result, fnPtr);

    // Overwrite the function pointer to its original value
    // (actually improves optimizations by showing it doesn't change)
    code->CreateStore(fnPtr, fnPtrPtr);
    
    return result;
}


llvm_value CompiledUnit::Return(llvm_value value)
// ----------------------------------------------------------------------------
//   Return the given value, after appropriate boxing
// ----------------------------------------------------------------------------
{
    llvm_type retTy = function->getReturnType();
    value = Autobox(value, retTy);
    code->CreateStore(value, returned);
    return value;
}


eval_fn CompiledUnit::Finalize(bool createCode)
// ----------------------------------------------------------------------------
//   Finalize the build of the current function
// ----------------------------------------------------------------------------
{
    IFTRACE(llvm)
        std::cerr << "CompiledUnit Finalize F" << function;

    // If we had closure information, finish building the closure type
    if (closureTy)
    {
        llvm_types sig;

        // First argument is always the pointer to the evaluation function
        llvm_type fnTy = function->getType();
        sig.push_back(fnTy);

        // Loop over other elements that need a closure
        for (value_map::iterator v = closure.begin(); v != closure.end(); v++)
        {
            llvm_value value = (*v).second;
            llvm_type allocaTy = value->getType();
            const llvm::PointerType *ptrTy = cast<PointerType>(allocaTy);
            llvm_type type = ptrTy->getElementType();
            sig.push_back(type);
        }

        // Build the structure type and unify it with opaque type used in decl
        llvm_type structTy = StructType::get(*llvm, sig);
        cast<OpaqueType>(closureTy.get())->refineAbstractTypeTo(structTy);

        // Load the elements from the closure
        Function::arg_iterator args = function->arg_begin();
        llvm_value closureArg = args++;
        uint field = 1;
        for (value_map::iterator v = closure.begin(); v != closure.end(); v++)
        {
            Tree *value = (*v).first;
            llvm_value storage = NeedStorage(value);
            llvm_value ptr = data->CreateConstGEP2_32(closureArg, 0, field++);
            llvm_value input = data->CreateLoad(ptr);
            data->CreateStore(input, storage);
        }

    }

    // Branch to the exit block from the last test we did
    code->CreateBr(exitbb);

    // Connect the "allocas" to the actual entry point
    data->CreateBr(entrybb);

    // Verify the function we built
    verifyFunction(*function);
    if (compiler->optimizer)
        compiler->optimizer->run(*function);

    IFTRACE(code)
        function->print(errs());

    void *result = NULL;
    if (createCode)
    {
        compiler->moduleOptimizer->run(*compiler->module);
        result = compiler->runtime->getPointerToFunction(function);
        IFTRACE(code)
        {
            errs() << "AFTER GLOBAL OPTIMIZATIONS:\n";
            function->print(errs());
        }
        IFTRACE(llvm)
            std::cerr << " C" << (void *) result << "\n";
    }

    exitbb = NULL;              // Tell destructor we were successful
    return (eval_fn) result;
}


Value *CompiledUnit::NeedStorage(Tree *tree)
// ----------------------------------------------------------------------------
//    Allocate storage for a given tree
// ----------------------------------------------------------------------------
{
    assert(inference || !"Storage() called without type check");

    Value *result = storage[tree];
    if (!result)
    {
        // Get the associated machine type
        llvm_type mtype = ExpressionMachineType(tree);

        // Create alloca to store the new form
        text label = "loc";
        IFTRACE(labels)
            label += "[" + text(*tree) + "]";
        const char *clabel = label.c_str();
        result = data->CreateAlloca(mtype, 0, clabel);
        storage[tree] = result;

        // If this started with a value or global, initialize on function entry
        if (value.count(tree))
            data->CreateStore(value[tree], result);
        else if (Value *global = compiler->TreeGlobal(tree))
            data->CreateStore(data->CreateLoad(global), result);
    }

    return result;
}


llvm_value CompiledUnit::NeedClosure(Tree *tree)
// ----------------------------------------------------------------------------
//   Allocate a closure variable
// ----------------------------------------------------------------------------
{
    llvm_value storage = closure[tree];
    if (!storage)
    {
        storage = NeedStorage(tree);
        closure[tree] = storage;
    }
    llvm_value result = code->CreateLoad(storage);
    return result;
}


bool CompiledUnit::IsKnown(Tree *tree, uint which)
// ----------------------------------------------------------------------------
//   Check if the tree has a known local or global value
// ----------------------------------------------------------------------------
{
    if ((which & knowLocals) && storage.count(tree) > 0)
        return true;
    else if ((which & knowValues) && value.count(tree) > 0)
        return true;
    else if (which & knowGlobals)
        if (compiler->IsKnown(tree))
            return true;
    return false;
}


Value *CompiledUnit::Known(Tree *tree, uint which)
// ----------------------------------------------------------------------------
//   Return the known local or global value if any
// ----------------------------------------------------------------------------
{
    Value *result = NULL;
    if ((which & knowLocals) && storage.count(tree) > 0)
    {
        // Value is stored in a local variable
        result = code->CreateLoad(storage[tree], "loc");
    }
    else if ((which & knowValues) && value.count(tree) > 0)
    {
        // Immediate value of some sort, use that
        result = value[tree];
    }
    else if (which & knowGlobals)
    {
        // Check if this is a global
        result = compiler->TreeGlobal(tree);
        if (result)
        {
            text label = "glob";
            IFTRACE(labels)
                label += "[" + text(*tree) + "]";
            result = code->CreateLoad(result, label);
        }
    }
    return result;
}


void CompiledUnit::ImportClosureInfo(const CompiledUnit *parent)
// ----------------------------------------------------------------------------
//   Copy closure data from parent to child
// ----------------------------------------------------------------------------
{
    machineType = parent->machineType;
}


Value *CompiledUnit::ConstantInteger(Integer *what)
// ----------------------------------------------------------------------------
//    Generate an Integer tree
// ----------------------------------------------------------------------------
{
    Value *result = Known(what, knowGlobals);
    if (!result)
    {
        result = compiler->EnterConstant(what);
        result = code->CreateLoad(result, "intk");
        if (storage.count(what))
            code->CreateStore(result, storage[what]);
    }
    return result;
}


Value *CompiledUnit::ConstantReal(Real *what)
// ----------------------------------------------------------------------------
//    Generate a Real tree
// ----------------------------------------------------------------------------
{
    Value *result = Known(what, knowGlobals);
    if (!result)
    {
        result = compiler->EnterConstant(what);
        result = code->CreateLoad(result, "realk");
        if (storage.count(what))
            code->CreateStore(result, storage[what]);
    }
    return result;
}


Value *CompiledUnit::ConstantText(Text *what)
// ----------------------------------------------------------------------------
//    Generate a Text tree
// ----------------------------------------------------------------------------
{
    Value *result = Known(what, knowGlobals);
    if (!result)
    {
        result = compiler->EnterConstant(what);
        result = code->CreateLoad(result, "textk");
        if (storage.count(what))
            code->CreateStore(result, storage[what]);
    }
    return result;
}


Value *CompiledUnit::ConstantTree(Tree *what)
// ----------------------------------------------------------------------------
//    Generate a constant tree
// ----------------------------------------------------------------------------
{
    Value *result = Known(what, knowGlobals);
    if (!result)
    {
        result = compiler->EnterConstant(what);
        result = data->CreateLoad(result, "treek");
    }
    return result;
}


Value *CompiledUnit::CallFormError(Tree *what)
// ----------------------------------------------------------------------------
//   Report a type error trying to evaluate some argument
// ----------------------------------------------------------------------------
{
    Value *ptr = ConstantTree(what); assert(what);
    Value *nullContext = ConstantPointerNull::get(compiler->contextPtrTy);
    Value *callVal = code->CreateCall2(compiler->xl_form_error,
                                       nullContext, ptr);
    return callVal;
}


llvm_type CompiledUnit::ReturnType(Tree *form)
// ----------------------------------------------------------------------------
//   Compute the return type associated with the given form
// ----------------------------------------------------------------------------
{
    // Type inference gives us the return type for this form
    Tree *type = inference->Type(form);
    llvm_type mtype = compiler->MachineType(type);
    return mtype;
}


llvm_type CompiledUnit::StructureType(llvm_types &signature)
// ----------------------------------------------------------------------------
//   Compute the return type associated with the given data form
// ----------------------------------------------------------------------------
{
    StructType *stype = StructType::get(*llvm, signature);
    return stype;
}


llvm_type CompiledUnit::ExpressionMachineType(Tree *expr, llvm_type type)
// ----------------------------------------------------------------------------
//   Define the machine type associated with an expression
// ----------------------------------------------------------------------------
{
    assert (type && "ExpressionMachineType called with null type");
    assert ((machineType[expr] == NULL || machineType[expr] == type) &&
            "ExpressionMachineType overrides type");
    machineType[expr] = type;
    return type;
}


llvm_type CompiledUnit::ExpressionMachineType(Tree *expr)
// ----------------------------------------------------------------------------
//   Return the machine type associated with a given expression
// ----------------------------------------------------------------------------
{
    llvm_type type = machineType[expr];
    if (!type)
    {
        assert(inference || !"ExpressionMachineType without type check");
        Tree *typeTree = inference->Type(expr);
        type = compiler->MachineType(typeTree);
        machineType[expr] = type;
    }
    return type;
}


llvm_type CompiledUnit::MachineType(Tree *type)
// ----------------------------------------------------------------------------
//   Return the machine type associated with a given type
// ----------------------------------------------------------------------------
{
    assert(inference || !"ExpressionMachineType without type check");

    type = inference->Base(type);
    return compiler->MachineType(type);
}


llvm_value CompiledUnit::Autobox(llvm_value value, llvm_type req)
// ----------------------------------------------------------------------------
//   Automatically box/unbox primitive types
// ----------------------------------------------------------------------------
//   Primitive values like integers can exist in two forms during execution:
//   - In boxed form, e.g. as a pointer to an instance of Integer
//   - In native form, e.g. as an integer
//   This function automatically converts from one to the other as necessary
{
    llvm_type  type   = value->getType();
    llvm_value result = value;
    Function * boxFn  = NULL;

    // Short circuit if we are already there
    if (req == type)
        return result;

    if (req == compiler->booleanTy)
    {
        assert (type == compiler->treePtrTy || type == compiler->nameTreePtrTy);
        Value *falsePtr = compiler->TreeGlobal(xl_false);
        result = code->CreateLoad(falsePtr, "xl_false");
        result = code->CreateICmpNE(value, result, "notFalse");
    }
    else if (req->isIntegerTy())
    {
        if (req == compiler->characterTy && type == compiler->textTreePtrTy)
        {
            // Convert text constant to character
            result = code->CreateConstGEP2_32(result,0, TEXT_VALUE_INDEX);
            result = code->CreateConstGEP2_32(result, 0, 0);
            result = code->CreateConstGEP2_32(result, 0, 0);
            result = code->CreateLoad(result);
        }
        else
        {
            // Convert integer constants
            assert (type == compiler->integerTreePtrTy);
            result = code->CreateConstGEP2_32(value,0, INTEGER_VALUE_INDEX);
            if (req != compiler->integerTy)
                result = code->CreateTrunc(result, req);
        }
    }
    else if (req->isFloatingPointTy())
    {
        assert(type == compiler->realTreePtrTy);
        result = code->CreateConstGEP2_32(value,0, REAL_VALUE_INDEX, "rval");
        if (req != compiler->realTy)
            result = code->CreateFPTrunc(result, req);
    }
    else if (req == compiler->charPtrTy)
    {
        assert(type == compiler->textTreePtrTy);
        result = code->CreateConstGEP2_32(result,0, TEXT_VALUE_INDEX);
        result = code->CreateConstGEP2_32(result,0, 0);
        result = code->CreateLoad(result);
    }
    else if (req == compiler->textTy)
    {
        assert (type == compiler->textTreePtrTy);
        result = code->CreateConstGEP2_32(result,0, TEXT_VALUE_INDEX, "tval");
    }
    else if (type == compiler->booleanTy)
    {
        assert(req == compiler->treePtrTy || req == compiler->nameTreePtrTy);

        // Insert code corresponding to value ? xl_true : xl_false
        BasicBlock *isTrue = BasicBlock::Create(*llvm, "isTrue", function);
        BasicBlock *isFalse = BasicBlock::Create(*llvm, "isFalse", function);
        BasicBlock *exit = BasicBlock::Create(*llvm, "booleanBoxed", function);
        Value *ptr = data->CreateAlloca(compiler->treePtrTy);
        code->CreateCondBr(value, isTrue, isFalse);

        // True block
        code->SetInsertPoint(isTrue);
        Value *truePtr = compiler->TreeGlobal(xl_true);
        result = code->CreateLoad(truePtr);
        result = code->CreateStore(result, ptr);
        code->CreateBr(exit);

        // False block
        code->SetInsertPoint(isFalse);
        Value *falsePtr = compiler->TreeGlobal(xl_false);
        result = code->CreateLoad(falsePtr);
        result = code->CreateStore(result, ptr);
        code->CreateBr(exit);

        // Now on shared exit block
        code->SetInsertPoint(exit);
        result = code->CreateLoad(ptr);
        type = result->getType();
    }
    else if (type == compiler->characterTy &&
             (req == compiler->treePtrTy || req == compiler->textTreePtrTy))
    {
        boxFn = compiler->xl_new_character;
    }
    else if (type->isIntegerTy())
    {
        assert(req == compiler->treePtrTy || req == compiler->integerTreePtrTy);
        boxFn = compiler->xl_new_integer;
        if (type != compiler->integerTy)
            result = code->CreateSExt(result, type); // REVISIT: Signed?
    }
    else if (type->isFloatingPointTy())
    {
        assert(req == compiler->treePtrTy || req == compiler->realTreePtrTy);
        boxFn = compiler->xl_new_real;
        if (type != compiler->realTy)
            result = code->CreateFPExt(result, type);
    }
    else if (type == compiler->textTy)
    {
        assert(req == compiler->treePtrTy || req == compiler->textTreePtrTy);
        boxFn = compiler->xl_new_text;
    }
    else if (type == compiler->charPtrTy)
    {
        assert(req == compiler->treePtrTy || req == compiler->textTreePtrTy);
        boxFn = compiler->xl_new_ctext;
    }

    // If we need to invoke a boxing function, do it now
    if (boxFn)
    {
        result = code->CreateCall(boxFn, value);
        type = result->getType();
    }


    if (req == compiler->treePtrTy && type != req)
    {
        assert(type == compiler->integerTreePtrTy ||
               type == compiler->realTreePtrTy ||
               type == compiler->textTreePtrTy ||
               type == compiler->nameTreePtrTy ||
               type == compiler->blockTreePtrTy ||
               type == compiler->prefixTreePtrTy ||
               type == compiler->postfixTreePtrTy ||
               type == compiler->infixTreePtrTy);
        result = code->CreateBitCast(result, req);
    }

    // Return what we built if anything
    return result;
}


Value *CompiledUnit::Global(Tree *tree)
// ----------------------------------------------------------------------------
//   Return a global value if there is any
// ----------------------------------------------------------------------------
{
    // Check if this is a global
    Value *result = compiler->TreeGlobal(tree);
    if (result)
    {
        text label = "glob";
        IFTRACE(labels)
            label += "[" + text(*tree) + "]";
        result = code->CreateLoad(result, label);
    }
    return result;
}


bool CompiledUnit::ValidCName(Tree *tree, text &label)
// ----------------------------------------------------------------------------
//   Check if the name is valid for C
// ----------------------------------------------------------------------------
{
    uint len = 0;

    if (Name *name = tree->AsName())
    {
        label = name->value;
        len = label.length();
    }
    else if (Text *text = tree->AsText())
    {
        label = text->value;
        len = label.length();
    }

    if (len == 0)
    {
        Ooops("No valid C name in $1", tree);
        return false;
    }

    // We will NOT call functions beginning with _ (internal functions)
    for (uint i = 0; i < len; i++)
    {
        char c = label[i];
        if (!isalpha(c) && c != '_' && !(i && isdigit(c)))
        {
            Ooops("C name $1 contains invalid characters", tree);
            return false;
        }
    }
    return true;
}

XL_END
