// *****************************************************************************
// bytecode.cpp                                                    XL project
// *****************************************************************************
//
// File description:
//
//    An bytecode for XL that does not rely on LLVM at all
//
//
//
//
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2015-2021, Christophe de Dinechin <christophe@dinechin.org>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License,
// or (at your option) any later version.
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

#include "bytecode.h"
#include "gc.h"
#include "info.h"
#include "errors.h"
#include "renderer.h"
#include "builtins.h"
#include "runtime.h"
#include "save.h"
#include "native.h"

#include <cmath>
#include <algorithm>

RECORDER(bytecode,      32, "Bytecode class evaluation of XL code");
RECORDER(opcode,        32, "Bytecode opcodes as they are being emitted");
RECORDER(opcode_error , 16, "Errors in the generated opcodes");


XL_BEGIN
// ============================================================================
//
//   Options
//
// ============================================================================

namespace Opt
{
NaturalOption   stackDepth("stack_depth",
                           "Maximum stack depth for recursive evaluation",
                           1000, 25, 25000);
}



// ============================================================================
//
//   Forward declarations
//
// ============================================================================

static Tree *evaluate(Scope_p scope, Tree_p expr);
static Tree *typecheck(Scope_p scope, Tree_p type, Tree_p value);
static void compile(Scope *scope, Tree *expr, Bytecode *bytecode);
static Tree *unwrap(Tree *expr);




// ============================================================================
//
//   Interpeter main entry points
//
// ============================================================================

BytecodeEvaluator::BytecodeEvaluator()
// ----------------------------------------------------------------------------
//   Constructor for bytecode
// ----------------------------------------------------------------------------
{
    record(bytecode, "Created bytecode evaluator %p", this);
}


BytecodeEvaluator::~BytecodeEvaluator()
// ----------------------------------------------------------------------------
//   Destructor for bytecode
// ----------------------------------------------------------------------------
{
    record(bytecode, "Destroyed bytecode evaluator %p", this);
}


Tree *BytecodeEvaluator::Evaluate(Scope *scope, Tree *expr)
// ----------------------------------------------------------------------------
//    Evaluate 'what', finding the final, non-closure result
// ----------------------------------------------------------------------------
{
    Tree_p result = evaluate(scope, expr);
    result = unwrap(result);
    return result;
}


Tree *BytecodeEvaluator::TypeCheck(Scope *scope, Tree *type, Tree *value)
// ----------------------------------------------------------------------------
//   Check if 'value' matches 'type' in the given context
// ----------------------------------------------------------------------------
{
    return typecheck(scope, type, value);
}



// ============================================================================
//
//   General utilities
//
// ============================================================================

template<typename ...Args>
static Tree_p ErrorMessage(Args...args)
// ----------------------------------------------------------------------------
//   Generate an error message in tree form
// ----------------------------------------------------------------------------
{
    Tree_p msg = XL::Error(args...);
    return msg;
}



// ============================================================================
//
//   Bytecode: Evaluation of opcodes
//
// ============================================================================

#define OP(name)                                \
    do                                          \
    {                                           \
        bytecode->Op(name, #name);              \
    } while(0)

#define OPCST(name,cst)                         \
   do                                           \
   {                                            \
       bytecode->Op(name, #name, cst);          \
   } while(0)

#define OPCHK(name, ...)                        \
    do                                          \
    {                                           \
        bytecode->Op(name, #name);              \
        Tree_p msg = ErrorMessage(__VA_ARGS__); \
        bytecode->CheckNil(msg);                \
    } while(0)

#define OPCHKCST(name,cst, ...)                 \
    do                                          \
    {                                           \
        bytecode->Op(name, #name, cst);         \
        Tree_p msg = ErrorMessage(__VA_ARGS__); \
        bytecode->CheckNil(msg);                \
    } while(0)


struct Bytecode : Info
// ----------------------------------------------------------------------------
//   A sequence of instructions associated to a tree
// ----------------------------------------------------------------------------
{
    Bytecode(Scope *scope, Tree *self):
        code(), checks(), scope(scope), self(self),
        constants(), rewrites(), parameters(),
        validated(0), local(false), variable(false) {}
    ~Bytecode() { Cut(0); }


    void        Run(RunState &state);
    void        Op(opcode_fn opcode, kstring name);
    void        Op(opcode_fn opcode, kstring name, Tree *cst);
    void        Op(opcode_fn opcode, kstring name, Rewrite *rewrite);
    void        Op(opcode_fn opcode, kstring name, opaddr_t data);
    void        Success();
    void        CheckNil(Tree *msg);    // Fail if top of stack is nil
    void        ErrorExit(Tree *msg);   // Emit an error with msg and exit
    void        Patch();
    void        Clear();

    size_t      Size();
    Scope *     EvaluationScope();
    Tree *      Self();
    bool        IsValid();
    void        Validate();
    Tree_p      Constant(opaddr_t index);
    Rewrite_p   Rewrite(opaddr_t index);
    void        EnterData(opaddr_t addr);
    opaddr_t    EnterConstant(Tree_p tree);
    opaddr_t    EnterRewrite(Rewrite_p rw);
    bool        LocalMode();
    bool        LocalMode(bool mode);
    bool        VariableMode();
    bool        VariableMode(bool mode);
    NameList *  Parameters(NameList *parameters);
    NameList *  Parameters();
    opaddr_t    Parameter(Name *name);

    typedef std::vector<opaddr_t>       Patches;
    typedef std::vector<opcode_fn>      Opcodes;

private:
    friend struct RunState;
    void        EnterCheck();
    void        EnterSuccess();
    void        PatchChecks();
    void        PatchSuccesses();
    void        Cut(opaddr_t cutpoint);

    Opcodes     code;           // Bytecode itself
    Patches     checks;         // Checks to patch
    Patches     successes;      // Checks to patch
    Scope_p     scope;          // Declaration scope for the code
    Tree *      self;           // Not a Tree_p to avoid GC cycles
    TreeList    constants;      // Constants generated by EnterConstant
    RewriteList rewrites;       // Definitions in order of binding
    NameList   *parameters;     // Parameters in effect in current scope
    opaddr_t    validated;      // Number of bytecode entries validated
    bool        local;          // Non-recursive lookup
    bool        variable;       // Evaluate as variable

public:
    friend struct Attempt;
    struct Attempt
    // ------------------------------------------------------------------------
    //   Helper class to manage bytecode patching
    // ------------------------------------------------------------------------
    {
        Attempt(Bytecode *bytecode)
            : bytecode(bytecode),
              cutpoint(bytecode->Size()),
              checks(bytecode->checks),
              successes(bytecode->successes)
        {
            bytecode->checks.clear();
            bytecode->successes.clear();
        }
        ~Attempt()
        {
            bytecode->PatchChecks();
            bytecode->checks = checks;
            bytecode->successes = successes;
        }
        void Fail()
        {
            bytecode->Cut(cutpoint);
            bytecode->checks.clear();
        }

        Bytecode *bytecode;
        opaddr_t  cutpoint;
        Patches   checks;
        Patches   successes;
    };
};


// Load the bytecode ops that we may need below
#include "bytecode-ops.h"


void Bytecode::Run(RunState &state)
// ----------------------------------------------------------------------------
//   Run the bytecode in the given state
// ----------------------------------------------------------------------------
{
    Bytecode *bc = this;
    while (bc)
    {
        opaddr_t max = bc->code.size();
        state.pc = 0;
        state.bytecode = bc;
        state.transfer = nullptr;
        while (state.pc < max)
            bc->code[state.pc++](state);
        bc = state.transfer;
    }
}


void Bytecode::Op(opcode_fn opcode, kstring name)
// ----------------------------------------------------------------------------
//   Record an opcode in the bytecode
// ----------------------------------------------------------------------------
{
    record(opcode, "In %p opcode %+s %p", this, name, opcode);
    code.push_back(opcode);
}


void Bytecode::Op(opcode_fn opcode, kstring name, Tree *cst)
// ----------------------------------------------------------------------------
//   Record an opcode with a constant in the bytecode
// ----------------------------------------------------------------------------
{
    record(opcode, "In %p opcode %+s %p constant %t", this, name, opcode, cst);
    code.push_back(opcode);
    EnterConstant(cst);
}


void Bytecode::Op(opcode_fn opcode, kstring name, struct Rewrite *rw)
// ----------------------------------------------------------------------------
//   Record an opcode with a rewrite in the bytecode
// ----------------------------------------------------------------------------
{
    record(opcode, "In %p opcode %+s %p rewrite %t", this, name, opcode, rw);
    code.push_back(opcode);
    EnterRewrite(rw);
}


void Bytecode::Op(opcode_fn opcode, kstring name, opaddr_t data)
// ----------------------------------------------------------------------------
//   Record an opcode with a constant in the bytecode
// ----------------------------------------------------------------------------
{
    record(opcode, "In %p opcode %+s %p data %llu", this, name, opcode, data);
    code.push_back(opcode);
    EnterData(data);
}


inline void Bytecode::EnterData(opaddr_t data)
// ----------------------------------------------------------------------------
//   Record a constant address (e.g. index of a local, etc)
// ----------------------------------------------------------------------------
{
    // Add the opcode itself
    code.push_back((opcode_fn) data);
}


inline void Bytecode::EnterCheck()
// ----------------------------------------------------------------------------
//   Enter an offset as an opcode to be patched by PatchChecks
// ----------------------------------------------------------------------------
{
    checks.push_back(code.size());
    code.push_back(0);          // Offset, to be patched by PatchChecks
}


inline void Bytecode::EnterSuccess()
// ----------------------------------------------------------------------------
//   Enter an offset as an opcode to be patched by PatchSuccesses
// ----------------------------------------------------------------------------
{
    successes.push_back(code.size());
    code.push_back(0);          // Offset, to be patched by PatchSuccesses
}


opaddr_t Bytecode::EnterConstant(Tree_p tree)
// ----------------------------------------------------------------------------
//   Add a constant to the tree
// ----------------------------------------------------------------------------
{
    size_t size = constants.size();
    opaddr_t index;
    for (index = 0; index < size; index++)
        if (tree == constants[index])
            break;
    if (index >= size)
        constants.push_back(tree);
    code.push_back((opcode_fn) index);
    return index;
}


opaddr_t Bytecode::EnterRewrite(Rewrite_p rewrite)
// ----------------------------------------------------------------------------
//   Add a rewrite to the rewrite
// ----------------------------------------------------------------------------
{
    size_t size = rewrites.size();
    opaddr_t index;
    for (index = 0; index < size; index++)
        if (rewrite == rewrites[index])
            break;
    if (index >= size)
        rewrites.push_back(rewrite);
    code.push_back((opcode_fn) index);
    return index;
}


void Bytecode::ErrorExit(Tree *expr)
// ----------------------------------------------------------------------------
//   Emit an error and exit evaluation
// ----------------------------------------------------------------------------
{
    code.push_back(error_exit);
    EnterConstant(expr);
}


void Bytecode::Patch()
// ----------------------------------------------------------------------------
//   Patch the branches to point here
// ----------------------------------------------------------------------------
{
    PatchChecks();
    PatchSuccesses();
}


void Bytecode::Clear()
// ----------------------------------------------------------------------------
//   In case of failure, clear the bytecode
// ----------------------------------------------------------------------------
{
    code.clear();
    checks.clear();
    successes.clear();
    constants.clear();
    rewrites.clear();
    parameters = nullptr;
    validated = 0;
    local = false;
    variable = false;
}


void Bytecode::CheckNil(Tree *msg)
// ----------------------------------------------------------------------------
//   Check if we have a nil, if so fail with given error
// ----------------------------------------------------------------------------
{
    // Add the check opcode and record that we need to patch it
    code.push_back(check);
    EnterConstant(msg);
    EnterCheck();
}


void Bytecode::Success()
// ----------------------------------------------------------------------------
//   Evaluation success: branch to the success exit
// ----------------------------------------------------------------------------
{
    // Add the check opcode and record that we need to patch it
    code.push_back(branch);
    EnterSuccess();
}


void Bytecode::PatchChecks()
// ----------------------------------------------------------------------------
//   Patch the pending checks to the current position
// ----------------------------------------------------------------------------
{
    opaddr_t current = code.size();
    for (auto check : checks)
        code[check] = (opcode_fn) current;
    checks.clear();
}


void Bytecode::PatchSuccesses()
// ----------------------------------------------------------------------------
//   Patch the pending checks to the current position
// ----------------------------------------------------------------------------
{
    opaddr_t current = code.size();
    for (auto success : successes)
        code[success] = (opcode_fn) current;
    successes.clear();
}


inline size_t Bytecode::Size()
// ----------------------------------------------------------------------------
//   Return the size of the generated code
// ----------------------------------------------------------------------------
{
    return code.size();
}


inline Scope *Bytecode::EvaluationScope()
// ----------------------------------------------------------------------------
//   Get the curren evaluation scope
// ----------------------------------------------------------------------------
{
    return scope;
}


inline Tree *Bytecode::Self()
// ----------------------------------------------------------------------------
//   Get the expression being evaluated
// ----------------------------------------------------------------------------
{
    return self;
}


void Bytecode::Cut(opaddr_t where)
// ----------------------------------------------------------------------------
//   Cut the bytecode at a given cut point
// ----------------------------------------------------------------------------
{
    if (where < code.size())
        code.resize(where);
}


bool Bytecode::IsValid()
// ----------------------------------------------------------------------------
//   Check if we are done writing this bytecode
// ----------------------------------------------------------------------------
{
    return validated == code.size() + 1;
}


void Bytecode::Validate()
// ----------------------------------------------------------------------------
//   Ensure that we have patched the remaining patches, and validate
// ----------------------------------------------------------------------------
{
    PatchChecks();

    // Avoid recursing if we evaluate something as last step
    if (code.back() == (opcode_fn) evaluate)
        code.back() = transfer;

    validated = code.size() + 1;
}


Tree_p Bytecode::Constant(opaddr_t index)
// ----------------------------------------------------------------------------
//   Return a given constant
// ----------------------------------------------------------------------------
{
    return constants[index];
}


Rewrite_p Bytecode::Rewrite(opaddr_t index)
// ----------------------------------------------------------------------------
//   Return a given rewrite
// ----------------------------------------------------------------------------
{
    return rewrites[index];
}


inline bool Bytecode::LocalMode()
// ----------------------------------------------------------------------------
//   Return local mdoe (non-recursive lookup)
// ----------------------------------------------------------------------------
{
    return local;
}


bool Bytecode::LocalMode(bool mode)
// ----------------------------------------------------------------------------
//   Change local mode, return old
// ----------------------------------------------------------------------------
{
    bool old = local;
    local = mode;
    return old;
}


inline bool Bytecode::VariableMode()
// ----------------------------------------------------------------------------
//   Return variable mode (reference lookup)
// ----------------------------------------------------------------------------
{
    return variable;
}


bool Bytecode::VariableMode(bool mode)
// ----------------------------------------------------------------------------
//   Change variable mode, return old
// ----------------------------------------------------------------------------
{
    bool old = variable;
    variable = mode;
    return old;;
}


NameList *Bytecode::Parameters(NameList *parms)
// ----------------------------------------------------------------------------
//    Set the namelist for the input parameters
// ----------------------------------------------------------------------------
{
    NameList *old = parameters;
    parameters = parms;
    return old;
}


NameList *Bytecode::Parameters()
// ----------------------------------------------------------------------------
//   Return the current namelist
// ----------------------------------------------------------------------------
{
    return parameters;
}


opaddr_t Bytecode::Parameter(Name *name)
// ----------------------------------------------------------------------------
//    Find the index of a parameter in the current bytecode
// ----------------------------------------------------------------------------
{
    if (parameters)
    {
        text symbol = name->value;
        opaddr_t max = parameters->size();
        for (opaddr_t index = 0; index < max; index++)
            if ((*parameters)[index]->value == symbol)
                return index;
    }
    return (opaddr_t) -1;
}


// ============================================================================
//
//   RunState functions that access the bytecode
//
// ============================================================================

Scope_p RunState::DeclarationScope()
// ----------------------------------------------------------------------------
//   The declaration scope is associated to the bytecode
// ----------------------------------------------------------------------------
{
    return bytecode->EvaluationScope();
}


Tree_p RunState::Self()
// ----------------------------------------------------------------------------
//   The value of 'self' is associated to the bytecode being evaluated
// ----------------------------------------------------------------------------
{
    return bytecode->Self();
}


opaddr_t RunState::Data()
// ----------------------------------------------------------------------------
//   Read the next opcode as an intptr_t, e.g. branch offsets
// ----------------------------------------------------------------------------
{
    return (opaddr_t) bytecode->code[pc++];
}


Tree_p RunState::Constant()
// ----------------------------------------------------------------------------
//   Get a constant from the bytecode
// ----------------------------------------------------------------------------
{
    opaddr_t index = (opaddr_t) bytecode->code[pc++];
    return bytecode->Constant(index);
}



// ============================================================================
//
//   BytecodeBindings: Helper class to bind parameters in the code
//
// ============================================================================

struct BytecodeBindings
// ----------------------------------------------------------------------------
//   Structure used to record bindings
// ----------------------------------------------------------------------------
{
    enum strength { FAILED, POSSIBLE, PERFECT };
    typedef strength            value_type;

    BytecodeBindings(Tree *expr, Bytecode *bytecode);
    ~BytecodeBindings();

    // Start considering a new candidate
    void        Candidate(Rewrite *decl, Scope *evalScope, Scope *declScope);

    // Tree::Do interface to check the pattern
    strength    Do(Natural *what);
    strength    Do(Real *what);
    strength    Do(Text *what);
    strength    Do(Name *what);
    strength    Do(Prefix *what);
    strength    Do(Postfix *what);
    strength    Do(Infix *what);
    strength    Do(Block *what);

    // Context of the expression we are looking at
    Scope *     EvaluationScope()       { return evaluation; }
    Scope *     DeclarationScope()      { return declaration; }
    Scope *     ArgumentsScope()        { return arguments; }
    Tree  *     Self()                  { return self; }
    Tree  *     ResultType()            { return type; }
    NameList    Parameters()            { return parameters; }
    Bytecode *  Code()                  { return bytecode; }
    strength    Strength()              { return match; }

private:
    // Bind a parameter, and get its local ID for the bytecode
    void        Bind(Name *name);
    opaddr_t    Parameter(Name *name);

    // Report if we match or not
    strength    Perfect()               { return match; }
    strength    Failed()                { match = FAILED; return match; }
    strength    Possible()
    {
        if (match==PERFECT)
            match = POSSIBLE;
        return match;
    }

    Tree_p      Evaluate(Tree *expr)
    {
        return evaluate(evaluation, expr);
    }
    Tree_p      EvaluateType(Tree *type)
    {
        return evaluate(declaration, type);
    }
    Tree_p      EvaluateInDeclaration(Tree *metabox)
    {
        return evaluate(declaration, metabox);
    }
    void        StripBlocks()
    {
        while (Block *block = test->AsBlock())
            test = block->child;
    }


private:
    Scope_p             evaluation;     // Where we evaluate expressions
    Scope_p             declaration;    // Where the pattern was defined
    Scope_p             arguments;      // Where we put bound arguments
    Tree_p              self;           // The top-level expression tested
    Tree_p              test;           // Sub-exrpression being tested
    Bytecode *          bytecode;       // Bytecode we generate code in
    NameList            parameters;     // Parameters we are defining
    strength            match;
    Tree_p              defined;
    Tree_p              type;
};



// ============================================================================
//
//    Implementation of BytecodeBindings
//
// ============================================================================

typedef BytecodeBindings::strength strength;


BytecodeBindings::BytecodeBindings(Tree *expr, Bytecode *bytecode)
// ----------------------------------------------------------------------------
//   BytecodeBindings constructor
// ----------------------------------------------------------------------------
    : evaluation(),
      declaration(),
      arguments(),
      self(expr),
      test(expr),
      bytecode(bytecode),
      match(FAILED),
      defined(nullptr)
{
    record(bindings, ">Created bindings %p for %t", this, expr);
}


BytecodeBindings::~BytecodeBindings()
// ----------------------------------------------------------------------------
//   When removing bindings, unmark all the borrowed items
// ----------------------------------------------------------------------------
{
    record(bindings, "<Destroyed bindings %p", this);
}


void BytecodeBindings::Candidate(Rewrite *rw, Scope *eval, Scope *decl)
// ----------------------------------------------------------------------------
//   Check a particular declaration
// ----------------------------------------------------------------------------
{
    record(bindings, "Set %t for scope %t decl %t", self, eval, decl);
    evaluation = eval;
    declaration = decl;
    Context argsCtx(decl);
    arguments = argsCtx.CreateScope(eval->Position());
    test = self;
    type = nullptr;
    match = PERFECT;
    defined = nullptr;
}



// ============================================================================
//
//   Generating bytecode to match the pattern
//
// ============================================================================

strength BytecodeBindings::Do(Natural *what)
// ----------------------------------------------------------------------------
//   The pattern contains an natural: check we have the same
// ----------------------------------------------------------------------------
{
    StripBlocks();

    // If an exact match, no need to even check the value at runtime
    if (Natural *ival = test->AsNatural())
    {
        if (ival->value == what->value)
            return Perfect();
        else
            return Failed();
    }

    // Otherwise, we need to evaluate and check at runtime
    OPCHKCST(match_natural, what, "Argument $1 does not match $2", test, what);
    return Possible();
}


strength BytecodeBindings::Do(Real *what)
// ----------------------------------------------------------------------------
//   The pattern contains a real: check we have the same
// ----------------------------------------------------------------------------
{
    StripBlocks();

    // If an exact match, no need to check the value at runtime
    if (Real *rval = test->AsReal())
    {
        if (rval->value == what->value)
            return Perfect();
        else
            return Failed();
    }

    // Promote natural values in pattern (is this a good idea?)
    if (Natural *nval = test->AsNatural())
    {
        if (nval->value == what->value)
            return Perfect();
        else
            return Failed();
    }

    // Otherwise, we need to evaluate and check at runtime
    OPCHKCST(match_real, what, "Argument $1 does not match $2", test, what);
    return Possible();
}


strength BytecodeBindings::Do(Text *what)
// ----------------------------------------------------------------------------
//   The pattern contains a real: check we have the same
// ----------------------------------------------------------------------------
{
    StripBlocks();

    if (Text *ival = test->AsText())
    {
        if (ival->value == what->value)
            return Perfect();
        else
            return Failed();
    }

    // Otherwise, we need to evaluate and check at runtime
    OPCHKCST(match_text, what, "Argument $1 does not match $2", test, what);
    return Possible();
}


strength BytecodeBindings::Do(Name *what)
// ----------------------------------------------------------------------------
//   The pattern contains a name
// ----------------------------------------------------------------------------
//   If we are in 'defining' mode (top-level, left of a prefix), then
//   we want an exact match.
//   Otherwise, we want to bind to an argument
{
    // Check if we are testing e.g. [getpid as integer32] vs. [getpid]
    if (!defined)
    {
        StripBlocks();
        if (Name *name = test->AsName())
            if (name->value == what->value)
                return Perfect();

        // If matching to anything else, hard fail
        return Failed();
    }

    // If there is already a binding for that name, value must match
    // This covers both a pattern with 'pi' in it and things like 'X+X'
    Context args(arguments);
    if (Tree *bound = args.Bound(what, false))
    {
        OPCST(local, Parameter(what));
        OPCHK(match_same, "The values to bind to $1 do not match", what);
        return Possible();
    }

    // Otherwise, bind test value to name
    Bind(what);
    return Perfect();
}


strength BytecodeBindings::Do(Block *what)
// ----------------------------------------------------------------------------
//   The pattern contains a block: look inside
// ----------------------------------------------------------------------------
{
    // Deal with the case of a metablock: evaluate expression inside
    if (Tree *expr = what->IsMetaBox())
    {
        // Evaluate metabox at "compile time"
        Tree_p meta = EvaluateInDeclaration(expr);
        OPCST(constant, meta);
        OPCHK(match_same, "The argument $1 does not match $2", meta, what);
        return Possible();
    }

    // Match the child
    return what->child->Do(this);
}


strength BytecodeBindings::Do(Prefix *what)
// ----------------------------------------------------------------------------
//   The pattern contains prefix: check that the left part matches
// ----------------------------------------------------------------------------
{
    StripBlocks();

    // If we have a lambda prefix, match the name as a parameter
    if (Name *name = IsLambda(what))
    {
        defined = what;
        return name->Do(this);
    }

    // Check if we have a variable binding, i.e. [var X]
    if (Tree *binding = IsVariableBinding(what))
    {
        if (Name *name = binding->AsName())
        {
            OPCHK(borrow, "The argument for $1 is not a variable", name);
            Bind(name);
            return Possible();
        }
        else if (Infix *typecast = binding->AsInfix())
        {
            Tree_p type = EvaluateType(typecast->right);
            OPCHKCST(typed_borrow, type,
                     "The argument for $1 is not a variable of type $2",
                     typecast, type);
            Bind(name);
            return Possible();
        }
        return Failed();
    }

    // The test itself should be a prefix
    if (Prefix *pfx = test->AsPrefix())
    {
        // Check prefix left first, which may set 'defined' to name
        OPCHK(match_prefix,
              "The argument $1 does not match prefix $2", test, what);
        defined = nullptr;
        test = pfx->left;
        what->left->Do(this);
        OP(swap);
        defined = what;
        test = pfx->right;
        what->right->Do(this);
        OP(make_prefix);
        return Perfect();
    }

    // Otherwise, it's a mismatch
    return Failed();
}


strength BytecodeBindings::Do(Postfix *what)
// ----------------------------------------------------------------------------
//   The pattern contains posfix: check that the right part matches
// ----------------------------------------------------------------------------
{
    // The test itself should be a postfix
    if (Postfix *pfx = test->AsPostfix())
    {
        // Check postfix left first, which may set 'defined' to name
        OPCHK(match_postfix,
              "The argument $1 does not match postfix $2", test, what);
        defined = nullptr;
        test = pfx->right;
        what->right->Do(this);
        OP(swap);
        defined = what;
        test = pfx->left;
        what->left->Do(this);
        OP(make_postfix);
        return Perfect();
    }

    // Otherwise, it's a mismatch
    return Failed();
}


strength BytecodeBindings::Do(Infix *what)
// ----------------------------------------------------------------------------
//   The complicated case: various definitions
// ----------------------------------------------------------------------------
{
    // Check if we have a type annotation, like [X:natural]
    if (IsTypeAnnotation(what))
    {
        bool outermost = test == self;

        // Check if we test [A+B as integer] against [lambda N as integer]
        if (Name *declared = IsTypeCastDeclaration(what))
        {
            if (Infix *cast = IsTypeCast(test))
            {
                // The two types must evaluate identically at compile time
                Tree_p wtype = EvaluateType(what->right);
                Tree_p ttype = EvaluateType(cast->right);
                if (wtype != ttype)
                    return Failed();

                // Get the value to test, e.g. [A+B] in the example above
                test = cast->left;

                // Test that against [lambda N]
                defined = what;
                declared->Do(this);
                return Perfect();
            }

            // Not a type cast - Can't match
            return Failed();
        }

        // Need to evaluate the type on the right
        Tree_p want = EvaluateType(what->right);

        // Type check value against type
        if (outermost)
            type = want;
        else
            OPCHKCST(cast, want,
                     "Argument $1 does not match type $2", test, want);

        // Need to match the left part with the converted value
        if (!outermost && !defined)
            defined = what;
        return what->left->Do(this);
    }

    // Make sure names bind to values
    defined = what;

    // Check if we have a guard clause
    if (IsPatternCondition(what))
    {
        Tree_p tested = what->left;

        // It must pass the rest (need to bind values first)
        what->left->Do(this);

        // Now we want to evaluate the right hand side in arguments context
        OPCHKCST(guard, what->right,
                 "The expression $1 does not verify guard condition $2",
                 tested, what->right);
        return Possible();
    }

    // The test itself should be an infix with a matching name
    if (Infix *ifx = test->AsInfix())
    {
        OPCHKCST(match_infix, what,
                 "Argument $1 does not match $2", ifx, what);
        test = ifx->left;
        what->left->Do(this);
        OP(swap);
        test = ifx->right;
        what->right->Do(this);
        OPCST(make_infix, what);
        return Perfect();
    }

    // Otherwise, it's a mismatch
    return Failed();
}


void BytecodeBindings::Bind(Name *name)
// ----------------------------------------------------------------------------
//   Bind a local name to the value at the top of stack
// ----------------------------------------------------------------------------
{
    OP(bind);
    parameters.push_back(name);
}


opaddr_t BytecodeBindings::Parameter(Name *name)
// ----------------------------------------------------------------------------
//   Find the index of the named parameter
// ----------------------------------------------------------------------------
{
    text symbol = name->value;
    for (size_t index = 0; index < parameters.size(); index++)
        if (parameters[index]->value == symbol)
            return index;
    record(opcode_error, "Invalid parameter %t", name);
    return 0;
}



// ============================================================================
//
//   Helper functions
//
// ============================================================================

static Tree *unwrap(Tree *expr)
// ----------------------------------------------------------------------------
//   Evaluate all closures inside a tree
// ----------------------------------------------------------------------------
{
    while (expr)
    {
        switch(expr->Kind())
        {
        case NATURAL:
        case REAL:
        case TEXT:
        case NAME:
            return expr;
        case INFIX:
        {
            Infix *infix = (Infix *) expr;
            Tree *left = unwrap(infix->left);
            Tree *right = unwrap(infix->right);
            if (left != infix->left || right != infix->right)
                infix = new Infix(infix, left, right);
            return infix;
        }
        case PREFIX:
        {
            Prefix *prefix = (Prefix *) expr;
            if (Closure *closure = expr->As<Closure>())
            {
                Scope *scope = closure->CapturedScope();
                expr = evaluate(scope, closure->Value());
                continue;
            }
            Tree *left = unwrap(prefix->left);
            Tree *right = unwrap(prefix->right);
            if (left != prefix->left || right != prefix->right)
                prefix = new Prefix(left, right);
            return prefix;
        }
        case POSTFIX:
        {
            Postfix *postfix = (Postfix *) expr;
            Tree *left = unwrap(postfix->left);
            Tree *right = unwrap(postfix->right);
            if (left != postfix->left || right != postfix->right)
                postfix = new Postfix(left, right);
            return postfix;
        }
        case BLOCK:
        {
            Block *block = (Block *) expr;
            if (Scope *scope = expr->As<Scope>())
                return scope;
            expr = block->child;
        }
        }
    }
    return expr;
}



// ============================================================================
//
//    Evaluation engine
//
// ============================================================================

inline Context::LookupMode lookupMode(Bytecode *bytecode)
// ----------------------------------------------------------------------------
//   Compute the lookup mode for a given scenario
// ----------------------------------------------------------------------------
{
    // C++ was designed with this kind of code in mind
    return bytecode->LocalMode()
        ? bytecode->VariableMode()
        ? Context::REFERENCE_SCOPE
        : Context::SINGLE_SCOPE
        : bytecode->VariableMode()
        ? Context::REFERENCE
        : Context::RECURSIVE;
}


static Tree *lookupCandidate(Scope   *evalScope,
                             Scope   *declScope,
                             Tree    *expr,
                             Rewrite *decl,
                             void    *bnds)
// ----------------------------------------------------------------------------
//   Check if we can bind 'expr' to 'decl'
// ----------------------------------------------------------------------------
{
    // Fetch the relevant data
    BytecodeBindings &bindings = *((BytecodeBindings *) bnds);
    Bytecode *bytecode = bindings.Code();
    Tree *pattern = decl->Pattern();
    XL_ASSERT(bindings.Self() == expr);

    // Save the top of stack before this candidate attempts to analyze it
    OP(dup);

    // Add bytecode to check argument against parameters and create locals
    Bytecode::Attempt attempt(bytecode);
    bindings.Candidate(decl, evalScope, declScope);
    strength matched = pattern->Do(bindings);

    // Check if the candidate succeeded or failed
    if (matched == BytecodeBindings::FAILED)
    {
        // Failure: Cut the bytecode to where it was before
        attempt.Fail();
    }
    else
    {
        bool done = false;

        // Check if we matched a parameter
        if (matched == BytecodeBindings::PERFECT)
        {
            if (Name *name = pattern->AsName())
            {
                opaddr_t index = bytecode->Parameter(name);
                if (index != (opaddr_t) -1)
                {
                    OPCST(local, index);
                    done = true;
                }
                else if (IsSelf(decl->Definition()))
                {
                    OPCST(constant, pattern);
                    done = true;
                }
            }
        }

        // Insert code to evaluate the body
        if (!done)
        {
            compile(declScope, decl->Definition(), bytecode);
            OPCST(constant, decl->Definition());
            OP(evaluate);

            // Check the result type if we had one
            if (Tree *type = bindings.ResultType())
                OPCHKCST(cast, type, "Returned value does not match $1", type);
        }

        // On success, remove the value that we duplicated
        OP(swap_drop);

        bytecode->Success();
    }

    // Keep looking for more unless it's a perfect match
    return (matched == BytecodeBindings::PERFECT) ? decl->right : nullptr;
}


static void doTypeCheck(Scope *scope,Tree *type,Tree *expr, Bytecode *bytecode)
// ----------------------------------------------------------------------------
//   Compile a type check (which is just an [X as Y] expression)
// ----------------------------------------------------------------------------
{
    Infix_p infix = new Infix("as", expr, type);
    compile(scope, infix, bytecode);
}


static void doInitializers(Initializers &inits, Bytecode *bytecode)
// ----------------------------------------------------------------------------
//   Compile the bytecode for the given initializers
// ----------------------------------------------------------------------------
{
    for (auto &i : inits)
    {
        Scope *scope = i.scope;
        Rewrite_p &rw = i.rewrite;
        Tree_p init = rw->right;
        Infix_p typedecl = rw->left->AsInfix();
        Tree_p type = typedecl->right;
        doTypeCheck(scope, type, init, bytecode);

        // We want a non-null result here
        Error error("Initial value $1 does not match type of $2", init, rw);
        bytecode->CheckNil(error);
        OPCST(init_constant, rw);
    }
}


static bool doConstant(Scope *scope, Tree *tree, Bytecode *bytecode)
// ----------------------------------------------------------------------------
//   For a constant, simply emit it
// ----------------------------------------------------------------------------
{
    OPCST(constant, tree);
    return true;
}


static bool doName(Scope *scope, Name *name, Bytecode *bytecode)
// ----------------------------------------------------------------------------
//   Deal with special names
// ----------------------------------------------------------------------------
{
    if (name->value == "nil")
    {
        OPCST(constant, (Tree *) nullptr);
        return true;
    }
    if (name->value == "scope" || name->value == "context")
    {
        OP(get_scope);
        return true;
    }
    if (name->value == "super")
    {
        OP(get_super);
        return true;
    }
    if (name->value == "self")
    {
        OP(get_self);
        return true;
    }
    return false;
}


static bool doPrefix(Scope *scope, Prefix *prefix, Bytecode *bytecode)
// ----------------------------------------------------------------------------
//   Deal with special prefixen
// ----------------------------------------------------------------------------
{
    // Check if the left is a block containing only declarations
    // This deals with [(X is 3) (X + 1)], i.e. closures
    Initializers inits;
    Context context(scope);
    if (Scope_p closure = context.ProcessScope(prefix->left, inits))
    {
        compile(scope, prefix->left, bytecode);
        OP(set_scope);
        doInitializers(inits, bytecode);
        compile(closure, prefix->right, bytecode);
        return true;
    }

    // Check if we have [variable T], if so build a variable type
    if (IsVariableType(prefix))
    {
        compile(scope, prefix->right, bytecode);
        OP(make_variable);
        return true;
    }

    // Check if we are evaluating a pattern matching type
    if (Tree *matching = IsPatternMatchingType(prefix))
    {
        Tree_p pattern = xl_parse_tree(scope, matching);
        OPCST(make_matching, pattern);
        return true;
    }

    // Check 'with' type declarations
    if (IsWithDeclaration(prefix))
        // This is just a syntactic convenience, no effect at runtime
        return true;

    // Filter out declaration such as [extern foo(bar)]
    if (IsDefinition(prefix))
        return true;

    // Process [quote X] - REVISIT: Should just be a C function now?
    if (Tree *quoted = IsQuote(prefix))
    {
        Tree_p quote = xl_parse_tree(scope, quoted);
        OPCST(constant, quote);
        return true;
    }

    // Filter out import statements (processed also during parsing)
    if (Name *import = prefix->left->AsName())
    {
        if (eval_fn callback = MAIN->Importer(import->value))
        {
            // Process the import statement
            callback(scope, prefix);
            return true;
        }
    }

    // Turn statements like [IO.print "Hello"] into [IO.{print "Hello"}]
    if (Infix *qualified = IsDot(prefix->left))
    {
        Tree *qualifier = qualified->left;
        Tree *name = qualified->right;
        Tree *args = prefix->right;
        Prefix_p inner = new Prefix(prefix, name, args);
        Infix_p outer = new Infix(qualified, qualifier, inner);
        compile(scope, outer, bytecode);
        return true;
    }

    // Check if this is a builtin
    if (Text *name = IsBuiltin(prefix))
    {
        opcode_fn opcode = BytecodeEvaluator::builtins[name->value];
        if (!opcode)
        {
            record(opcode_error, "Nonexistent builtin %t", name);
            return false;
        }
        bytecode->Op(opcode, name->value.c_str());
        return true;
    }

    // Check if this is a native function
    if (Text *name = IsNative(prefix))
    {
        opcode_fn opcode = BytecodeEvaluator::natives[name->value];
        if (!opcode)
        {
            record(opcode_error, "Nonexistent native function %t", name);
            return false;
        }
        bytecode->Op(opcode, name->value.c_str());
        return true;
    }

    return false;
}


static bool doPostfix(Scope *scope, Postfix *postfix, Bytecode *bytecode)
// ----------------------------------------------------------------------------
//   Deal with special postfix forms
// ----------------------------------------------------------------------------
{
    // There shall be only none.
    return false;
}


static bool doInfix(Scope *scope, Infix *infix, Bytecode *bytecode)
// ----------------------------------------------------------------------------
//   Deal with all forms of special infix
// ----------------------------------------------------------------------------
{
    // For [Name is Expr], compute [Expr] at the time we hit the definition
    if (IsDefinition(infix))
    {
        if (Name *name = infix->left->AsName())
        {
            // Find the index for the corresponding declaration
            Context context(scope);
            Rewrite *rewrite = context.Declared(name, Context::REFERENCE_SCOPE);

            // Compile the initialization and initialize the constant
            compile(scope, infix->right, bytecode);
            OPCST(init_constant, rewrite);
        }
        return true;
    }

    // Evaluate [X.Y]
    if (IsDot(infix))
    {
        compile(scope, infix->left, bytecode);
        bool local = bytecode->LocalMode(true);
        compile(scope, infix->right, bytecode);
        bytecode->LocalMode(local);
        return true;
    }

    // Check [X as matching(P)]
    if (IsTypeCast(infix))
    {
        Tree_p want = evaluate(scope, infix->right);
        if (want)
        {
            if (Tree *matching = IsPatternMatchingType(want))
            {
                Tree *expr = infix->left;
                Bytecode::Attempt attempt(bytecode);
                BytecodeBindings bindings(expr, bytecode);
                Rewrite_p rewrite = new Rewrite(matching, xl_self);
                bindings.Candidate(rewrite, scope, scope);
                strength match = expr->Do(bindings);
                if (match == BytecodeBindings::FAILED)
                    attempt.Fail();
                return true;
            }
        }
        compile(scope, infix->left, bytecode);
        OPCHKCST(cast, want,
                 "Value $1 does not match type $2", infix->left, infix->right);
        return true;
    }

    // Evaluate assignments such as [X := Y]
    if (IsAssignment(infix))
    {
        bool variable = bytecode->VariableMode(true);
        compile(scope, infix->left, bytecode);
        bytecode->VariableMode(variable);
        compile(scope, infix->right, bytecode);
        OP(assign);
        return true;
    }
    return false;
}


static void compile(Scope *scope, Tree *expr, Bytecode *bytecode)
// ----------------------------------------------------------------------------
//   Compile an expression in the bytecode
// ----------------------------------------------------------------------------
{
    Bytecode *compiled = expr->GetInfo<Bytecode>();
    bool toplevel = bytecode->Size() == 0;

    // Check if we are recursively compiling this expression
    if (compiled == bytecode && !toplevel)
    {
        OP(transfer);
        return;
    }

    // If we are calling a new expression...
    if (!compiled)
    {
        // Create its own bytecode
        compiled = new Bytecode(scope, expr);
        expr->SetInfo<Bytecode>(compiled);
        compile(scope, expr, compiled);

        // Transfer to that bytecode from the old invokation point
        OP(transfer);
        return;
    }

    // Check if we have instructions to process. If not, return declarations
    Context context(scope);
    if (expr->AsBlock() || IsSequence(expr))
    {
        scope = context.CreateScope();
        Initializers inits;
        bool hasInstructions = context.ProcessDeclarations(expr, inits);
        bool hasLocals = !scope->IsEmpty();
        if (hasLocals && hasInstructions)
            OP(enter);
        else
            scope = context.PopScope();
        if (hasLocals)
            doInitializers(inits, bytecode);
        if (!hasInstructions)
        {
            OP(drop);
            OP(get_scope);
            return;
        }
    }

    Tree::Iterator next(expr);
    bool first = true;
    while ((expr = next()))
    {
        // If not the first statement, check result of previous statement
        if (!first)
            OP(check_statement);

        // Lookup expression
        Context::LookupMode mode = lookupMode(bytecode);
        BytecodeBindings bindings(expr, bytecode);
        context.Lookup(expr, lookupCandidate, &bindings, mode);
        bytecode->Patch();

        // If we did not find a matching form, check standard evaluation
        bool done = bindings.Strength() != BytecodeBindings::FAILED;
        if (!done) switch(expr->Kind())
        {
        case NATURAL:
        case REAL:
        case TEXT:
            done = doConstant(scope, expr, bytecode);
            break;
        case NAME:
            done = doName(scope, (Name *) expr, bytecode);
            break;
        case PREFIX:
            done = doPrefix(scope, (Prefix *) expr, bytecode);
            break;
        case POSTFIX:
            done = doPostfix(scope, (Postfix *) expr,   bytecode);
            break;
        case INFIX:
            done = doInfix(scope, (Infix *) expr,   bytecode);
            break;
        default:        break;
        }

        // If we failed, clean up and error out
        if (!done)
        {
            bytecode->Clear();
            Tree_p msg = ErrorMessage("No form matches $1", expr);
            bytecode->ErrorExit(msg);
            return;
        }
        first = false;
    }
}


Tree *evaluate(Scope_p scope, Tree_p expr)
// ----------------------------------------------------------------------------
//   Internal evaluator - Short circuits a few common expressions
// ----------------------------------------------------------------------------
{
    // Check stack depth during evaluation
    static uint depth = 0;
    Save<uint>  save(depth, depth+1);
    if (depth > Opt::stackDepth)
    {
        Errors::Abort(Ooops("Stack depth exceeded evaluating $1", expr));
        record(opcode_error, "Stack depth exceeded evaluating %t", expr);
        return nullptr;
    }

    // Check if already have bytecode for this
    Bytecode *bytecode = expr->GetInfo<Bytecode>();
    if (!bytecode)
    {
        // We need to create the bytecode for this expression
        bytecode = new Bytecode(scope, expr);
        expr->SetInfo<Bytecode>(bytecode);
        compile(scope, expr, bytecode);
        bytecode->Validate();
    }
    else if (!bytecode->IsValid())
    {
        // Recursive evaluation during compilation (unexpected)
        record(opcode_error, "Unexpectd recursive compilation of %t", expr);
        compile(scope, expr, bytecode);

        // Force validation of what we have
        bytecode->Validate();
    }

    // Run the bytecode in the scope
    RunState state(scope, expr);
    bytecode->Run(state);

    // Result is on top of stack
    Tree_p result = state.Pop();
    GarbageCollector::SafePoint();
    return result;
}



// ============================================================================
//
//   Type checking
//
// ============================================================================

static Tree *typecheck(Scope_p scope, Tree_p type, Tree_p value)
// ----------------------------------------------------------------------------
//   Implementation of type checking in bytecode
// ----------------------------------------------------------------------------
{
    // For built-in names, just call the opcode directly
    if (Name *name = type->AsName())
    {
        auto found = BytecodeEvaluator::types.find(name->value);
        if (found != BytecodeEvaluator::types.end())
        {
            opcode_fn callback = (*found).second;
            RunState state(scope, type);
            Tree_p evaluated = evaluate(scope, value);
            state.Push(evaluated);
            callback(state);
            return state.Pop();
        }
    }

    // Otherwise go through normal evaluation
    Infix_p test = new Infix("as", value, type, type->Position());
    return evaluate(scope, test);
}



// ============================================================================
//
//    Generate the Op subclasses for builtins
//
// ============================================================================

#define R_NAT(x)        result = new Natural((x), self->Position());
#define R_INT(x)        result = Natural::Signed((x), self->Position());
#define R_REAL(x)       result = new Real((x), self->Position())
#define R_TEXT(x)       result = new Text((x), self->Position())
#define R_BOOL(x)       result = (x) ? xl_true : xl_false
#define R_TYPE(x)       do { state.Push(x); return; } while(0)
#define x               (x_tree->value)
#define y               (y_tree->value)
#define x_boolean       (x == "true")
#define y_boolean       (y == "true")
#define x_integer       ( (longlong) x)
#define y_integer       ( (longlong) y)
#define x_natural       ((ulonglong) x)
#define y_natural       ((ulonglong) y)
#define x_real          (x)
#define y_real          (y)
#define x_text          (x)
#define y_text          (y)
#define DIV0            if (y == 0) Ooops("Divide $1 by zero", self)

#define UNARY(Name, ReturnType, ArgType, Body)                  \
static void unary_##Name(RunState &state)                       \
{                                                               \
    Tree *self = state.Self();                                  \
    Tree_p result = self;                                       \
    Tree_p tree = state.Pop();                                  \
    ArgType *x_tree  = tree->As<ArgType>();                     \
    if (!x_tree)                                                \
        record(opcode_error,                                    \
               "Argument %t is not a " #ArgType                 \
               " in builtin " #Name, self);                     \
    Body;                                                       \
    state.Push(result);                                         \
}                                                               \


#define BINARY(Name, ReturnType, XType, YType, Body)            \
static void binary_##Name(RunState &state)                      \
{                                                               \
    Tree *self = state.Self();                                  \
    Tree_p result = self;                                       \
    Tree_p yt = state.Pop();                                    \
    Tree_p xt = state.Pop();                                    \
    XType *x_tree  = xt->As<XType>();                           \
    if (!x_tree)                                                \
        record(opcode_error,                                    \
               "First argument $1 is not a " #XType             \
               " in builtin " #Name, xt);                       \
    YType *y_tree = yt->As<YType>();                            \
    if (!y_tree)                                                \
        record(opcode_error,                                    \
               "Second argument $1 is not a " #YType            \
               " in builtin " #Name, yt);                       \
    Body;                                                       \
    state.Push(result);                                         \
}                                                               \


#define NAME(N) /* Nothing */


#define TYPE(N, Body)                                           \
static void N##_typecheck(RunState &state)                      \
{                                                               \
    Tree *type = N##_type;                                      \
    Tree_p value = state.Pop();                                 \
    Tree_p result;                                              \
    record(typecheck, "Opcode typecheck %t as %t = " #N,        \
           value, type);                                        \
    Body;                                                       \
    state.Push(nullptr);                                        \
}                                                               \


// In opcode evaluation mode, whether we evaluated or not was decided earlier
#define TYPE_VALUE      value
#define TYPE_TREE       value
#define TYPE_CLOSURE    value
#define TYPE_NAMED      value
#define TYPE_VARIABLE   value

#include "builtins.tbl"



// ============================================================================
//
//   Initialize names and types like xl_self or natural_type
//
// ============================================================================

void BytecodeEvaluator::InitializeBuiltins()
// ----------------------------------------------------------------------------
//    Initialize all the names, e.g. xl_self, and all builtins
// ----------------------------------------------------------------------------
//    This has to be done before the first context is created, because
//    scopes are initialized with xl_self below
{
#define NAME(N)         xl_##N = new Name(#N);
#define TYPE(N, Body)   N##_type = new Name(#N);
#define UNARY(Name, ReturnType, XType, Body)
#define BINARY(Name, ReturnType, XType, YType, Body)

#include "builtins.tbl"
}



// ============================================================================
//
//    Initialize top-level context
//
// ============================================================================

std::map<text, opcode_fn> BytecodeEvaluator::builtins;
std::map<text, opcode_fn> BytecodeEvaluator::natives;
std::map<text, opcode_fn> BytecodeEvaluator::types;


#define NORM2(x) Normalize(x), x

void BytecodeEvaluator::InitializeContext(Context &context)
// ----------------------------------------------------------------------------
//    Initialize all the names, e.g. xl_self, and all builtins
// ----------------------------------------------------------------------------
//    Load the outer context with the type check operators referring to
//    the builtins.
{
#define UNARY(Name, ReturnType, XType, Body)                            \
    builtins[#Name] = unary_##Name;

#define BINARY(Name, RetTy, XType, YType, Body)                         \
    builtins[#Name] = binary_##Name;

#define NAME(N)                                                         \
    context.Define(xl_##N, xl_self);

#define TYPE(N, Body)                                                   \
    types[#N] = N##_typecheck;                                          \
    builtins[#N"_typecheck"] = N##_typecheck;                           \
                                                                        \
    Infix *pattern_##N =                                                \
        new Infix("as",                                                 \
                  new Prefix(xl_lambda,                                 \
                             new Name(NORM2("Value"))),                 \
                  N##_type);                                            \
    Prefix *value_##N =                                                 \
        new Prefix(xl_builtin,                                          \
                   new Text(#N"_typecheck"));                           \
    context.Define(N##_type, xl_self);                                  \
    context.Define(pattern_##N, value_##N);

#include "builtins.tbl"

    Name_p C = new Name(NORM2("native"));
    for (Native *native = Native::First(); native; native = native->Next())
    {
        record(native, "Found %t", native->Shape());
        kstring symbol = native->Symbol();
        natives[symbol] = native->opcode;
        Tree *shape = native->Shape();
        Prefix *body = new Prefix(C, new Text(symbol));
        context.Define(shape, body);
    }

    if (RECORDER_TRACE(symbols_sort))
        context.Dump(std::cerr, context.Symbols(), false);
}

XL_END
