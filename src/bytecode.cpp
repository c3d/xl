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
static Bytecode *compile(Scope *scope, Tree *expr);
static Bytecode *compile(Scope *, Tree *body, Tree *form, NameList &parameters);
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

#define CHECK(name)                             \
    do                                          \
    {                                           \
        bytecode->Op(name, #name);              \
        bytecode->Check();                      \
    } while(0)

#define CHECKCST(name,cst)                      \
    do                                          \
    {                                           \
        bytecode->Op(name, #name, cst);         \
        bytecode->Check();                      \
    } while(0)


typedef std::map<Tree_p, Tree_p>        TypeMap;
typedef std::map<Tree_p, size_t>        ValueMap;



struct Bytecode : Info
// ----------------------------------------------------------------------------
//   A sequence of instructions associated to a tree
// ----------------------------------------------------------------------------
{
    Bytecode(Scope *scope, Tree *self, NameList &parameters):
        code(), scope(scope), self(self),
        constants(), rewrites(), parameters(parameters),
        compile(new CompileInfo)
    {}
    ~Bytecode() { Cut(0); delete compile; }

    void        Run(RunState &state);
    void        Op(opcode_fn opcode, kstring name);
    void        Op(opcode_fn opcode, kstring name, Tree *cst);
    void        Op(opcode_fn opcode, kstring name, Rewrite *rewrite);
    void        Op(opcode_fn opcode, kstring name, opaddr_t data);
    void        Success();
    void        Check();                // Check a condition
    void        ErrorExit(Tree *msg);   // Emit an error with msg and exit
    void        Clear();
    void        RemoveLastDefinition();
    void        PatchChecks();
    size_t      SuccessesToPatch();
    void        PatchSuccesses(size_t where);

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
    void        Bind(Name *name);
    opaddr_t    Parameter(Name *name);
    NameList &  Parameters();
    opaddr_t    FrameSize(size_t base);
    bool        Cached(Tree *value);
    void        Evaluate(Scope *scope, Tree *value);
    Tree *      Type(Tree *value);
    void        Type(Tree *value, Tree *type);
    void        Dump(std::ostream &out);
    void        Dump(std::ostream &out, opaddr_t pc);
    opaddr_t    Data(opaddr_t pc);

    typedef std::vector<opaddr_t>       Patches;
    typedef std::vector<opcode_fn>      Opcodes;

private:
    friend struct RunState;
    void        EnterCheck();
    void        EnterSuccess();
    void        Cut(opaddr_t cutpoint);

    Opcodes     code;           // Bytecode itself
    Scope_p     scope;          // Declaration scope for the code
    Tree *      self;           // Not a Tree_p to avoid GC cycles
    TreeList    constants;      // Constants generated by EnterConstant
    RewriteList rewrites;       // Definitions in order of binding
    NameList    parameters;     // Parameters in effect in current scope

    struct CompileInfo
    {
        CompileInfo(): local(false), variable(false) {}
        Patches     checks;     // Checks to patch
        Patches     successes;  // Checks to patch
        TypeMap     types;      // Last known type for a value
        ValueMap    values;     // Already-computed value
        bool        local;      // Non-recursive lookup
        bool        variable;   // Evaluate as variable
    };
    CompileInfo *compile;       // Information used at compile-time

    enum OpcodeKind
    {
        OPCODE,                 // Regular opcode (function pointer)
        UNPATCHED,              // Unpatched jump
        JUMP,                   // New value for PC
        LOCAL,                  // Index of a local variable
        CONSTANT,               // Index of a constant
        REWRITE,                // Index of a rewrite
        PARAMETER,              // Index of a parameter
        INVALID,                // Invalid

        OPSHIFT = 3,            // Shift value for constants
        OPMASK = 7              // Mask to get the kind
    };

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
              frame(bytecode->parameters.size()),
              checks(bytecode->compile->checks),
              values(bytecode->compile->values),
              types(bytecode->compile->types)
        {
            bytecode->compile->checks.clear();
            bytecode->compile->types.clear();
        }
        ~Attempt()
        {
            bytecode->PatchChecks();
            bytecode->compile->checks = checks;
            NameList &parms = bytecode->parameters;
            parms.erase(parms.begin() + frame, parms.end());
        }
        void Fail()
        {
            bytecode->Cut(cutpoint);
            bytecode->compile->checks.clear();
            bytecode->compile->values = values;
            bytecode->compile->types = types;
        }

        Bytecode *bytecode;
        opaddr_t  cutpoint;
        size_t    frame;
        Patches   checks;
        ValueMap  values;
        TypeMap   types;
    };

    // Names for debugging purpose
    static std::map<opcode_fn, kstring> names;
};


// Load the bytecode ops that we may need below
#include "bytecode.tbl"


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


void Bytecode::Dump(std::ostream &out)
// ----------------------------------------------------------------------------
//   Dump the bytecocde
// ----------------------------------------------------------------------------
{
    size_t parms = parameters.size();
    if (parms)
    {
        out << "Parameters:\n";
        for (size_t parm = 0; parm < parms; parm++)
            out << parm << "\t= " << parameters[parm] << "\n";
    }

    if (compile)
    {
        if (compile->values.size())
        {
            out << "Cached values:\n";
            for (auto &it : compile->values)
                out << (it.second >> OPSHIFT) << ":\t" << it.first << "\n";
        }
        if (compile->values.size())
        {
            out << "Types:\n";
            for (auto &it : compile->types)
                out << it.first << "\tas\t" << it.second << "\n";
        }
    }

    out << "Opcodes:\n";
    opaddr_t max = code.size();
    for (opaddr_t pc = 0; pc < max; pc++)
        Dump(out, pc);
}


void Bytecode::Dump(std::ostream &out, opaddr_t pc)
// ----------------------------------------------------------------------------
//   Dump a single bytecode
// ----------------------------------------------------------------------------
{
    opcode_fn opcode = code[pc];
    opaddr_t data = (opaddr_t) opcode;
    opaddr_t kind = data & OPMASK;
    opaddr_t index = data >> OPSHIFT;

    switch ((opaddr_t) opcode & OPMASK)
    {
    case OPCODE:
        if (names.count(opcode))
            out << pc << ":\t" << names[opcode] << "\n";
        else
            out << pc << ":\t" << "Invalid " << (void *) opcode << "\n";
        break;
    case UNPATCHED:
        out << pc << ":\t" << "  JUMP?\t#" << index << "\n";
        break;
    case JUMP:
        out << pc << ":\t" << "  JUMP\t#" << index << "\n";
        break;
    case LOCAL:
        out << pc << ":\t" << "  LOCAL\t#" << index << "\n";
        break;
    case CONSTANT:
        out << pc << ":\t" << "  CST\t#" << index << "\t";
        if (index < constants.size())
            out << constants[index];
        else
            out << "(Invalid)";
        out << "\n";
        break;
    case REWRITE:
        out << pc << ":\t" << "  RWR\t#" << index << "\t";
        if (index < rewrites.size())
            out << rewrites[index];
        else
            out << "(Invalid)";
        out << "\n";
        break;
    case PARAMETER:
        out << pc << ":\t" << "  PARM\t#" << index << "\t";
        if (index < parameters.size())
            out << parameters[index];
        else
            out << "(Invalid)";
        out << "\n";
        break;
    default:
        out << pc << ":\t" << "  INVALID\t" << kind << "\n";
        break;
    }
}


opaddr_t Bytecode::Data(opaddr_t pc)
// ----------------------------------------------------------------------------
//   Read data from a given PC
// ----------------------------------------------------------------------------
{
    return (opaddr_t) code[pc] >> OPSHIFT;
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
    compile->checks.push_back(code.size());
    code.push_back((opcode_fn) UNPATCHED);
}


inline void Bytecode::EnterSuccess()
// ----------------------------------------------------------------------------
//   Enter an offset as an opcode to be patched by PatchSuccesses
// ----------------------------------------------------------------------------
{
    compile->successes.push_back(code.size());
    code.push_back((opcode_fn) UNPATCHED);
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
    code.push_back((opcode_fn) ((index << OPSHIFT) | CONSTANT));
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
    code.push_back((opcode_fn) ((index << OPSHIFT) | REWRITE));
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


void Bytecode::Clear()
// ----------------------------------------------------------------------------
//   In case of failure, clear the bytecode
// ----------------------------------------------------------------------------
{
    code.clear();
    compile->checks.clear();
    compile->successes.clear();
    constants.clear();
    rewrites.clear();
    parameters.clear();
    compile->types.clear();
    compile->values.clear();
    compile->local = false;
    compile->variable = false;
}


void Bytecode::RemoveLastDefinition()
// ----------------------------------------------------------------------------
//   Remove definitions inserted in sequences
// ----------------------------------------------------------------------------
//   A definition adds an OPCST(constant, rewrite) for the rewrite
//   This occupies two opcodes, so remove both
{
    code.erase(code.end() - 2, code.end());
}


void Bytecode::Check()
// ----------------------------------------------------------------------------
//   For conditional opcodes, insert a check to be patched
// ----------------------------------------------------------------------------
{
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
    Patches &checks = compile->checks;
    for (auto check : checks)
    {
        XL_ASSERT(code[check] == (opcode_fn) UNPATCHED);
        code[check] = (opcode_fn) ((current << OPSHIFT) | JUMP);
    }
    checks.clear();
}


size_t Bytecode::SuccessesToPatch()
// ----------------------------------------------------------------------------
//   Return where we are in the successes list
// ----------------------------------------------------------------------------
{
    return compile->successes.size();
}


void Bytecode::PatchSuccesses(size_t where)
// ----------------------------------------------------------------------------
//   Patch the pending checks to the current position
// ----------------------------------------------------------------------------
{
    opaddr_t current = code.size();
    Patches &successes = compile->successes;

    // If we have a branch to patch as the last instruction, we can remove it
    while (current >= 2 &&
           code[current - 2] == branch &&
           successes.size() > where &&
           successes.back() == current-1)
    {
        code.erase(code.end() - 2, code.end());
        successes.pop_back();
        current = code.size();
    }

    while (successes.size() > where)
    {
        opaddr_t success = successes.back();
        XL_ASSERT(code[success] == (opcode_fn) UNPATCHED);
        code[success] = (opcode_fn) ((current << OPSHIFT) | JUMP);
        successes.pop_back();
    }
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
    return !compile;
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

    delete compile;
    compile = nullptr;
}


Tree_p Bytecode::Constant(opaddr_t index)
// ----------------------------------------------------------------------------
//   Return a given constant
// ----------------------------------------------------------------------------
{
    XL_ASSERT(index < constants.size());
    return constants[index];
}


Rewrite_p Bytecode::Rewrite(opaddr_t index)
// ----------------------------------------------------------------------------
//   Return a given rewrite
// ----------------------------------------------------------------------------
{
    XL_ASSERT(index < rewrites.size());
    return rewrites[index];
}


inline bool Bytecode::LocalMode()
// ----------------------------------------------------------------------------
//   Return local mdoe (non-recursive lookup)
// ----------------------------------------------------------------------------
{
    return compile->local;
}


bool Bytecode::LocalMode(bool mode)
// ----------------------------------------------------------------------------
//   Change local mode, return old
// ----------------------------------------------------------------------------
{
    bool old = compile->local;
    compile->local = mode;
    return old;
}


inline bool Bytecode::VariableMode()
// ----------------------------------------------------------------------------
//   Return variable mode (reference lookup)
// ----------------------------------------------------------------------------
{
    return compile->variable;
}


bool Bytecode::VariableMode(bool mode)
// ----------------------------------------------------------------------------
//   Change variable mode, return old
// ----------------------------------------------------------------------------
{
    bool old = compile->variable;
    compile->variable = mode;
    return old;;
}


void Bytecode::Bind(Name *name)
// ----------------------------------------------------------------------------
//   Bind a parameter in the current bytecode
// ----------------------------------------------------------------------------
{
    parameters.push_back(name);
}


opaddr_t Bytecode::Parameter(Name *name)
// ----------------------------------------------------------------------------
//    Find the index of a parameter in the current bytecode
// ----------------------------------------------------------------------------
{
    opaddr_t max = parameters.size();
    opaddr_t index = max;
    while (index --> 0)
        if (parameters[index] == name)
            return (index << OPSHIFT) | PARAMETER;
    return 0;                   // Not a valid parameter index
}


inline NameList &Bytecode::Parameters()
// ----------------------------------------------------------------------------
//   Return current parameter list for the bytecode
// ----------------------------------------------------------------------------
{
    return parameters;
}


inline opaddr_t Bytecode::FrameSize(size_t base)
// ----------------------------------------------------------------------------
//   Return current parameter list for the bytecode
// ----------------------------------------------------------------------------
{
    size_t needed = parameters.size() - base;
    return (needed << OPSHIFT) | LOCAL;
}


bool Bytecode::Cached(Tree *value)
// ----------------------------------------------------------------------------
//   Evaluate a value but only once
// ----------------------------------------------------------------------------
{
    ValueMap &values = compile->values;
    auto found = values.find(value);
    if (found != values.end())
    {
        Bytecode *bytecode = this;
        opaddr_t data = (*found).second;
        OPCST(load, data);
        return true;
    }
    return false;
}


void Bytecode::Evaluate(Scope *scope, Tree *value)
// ----------------------------------------------------------------------------
//   Evaluate a value but only once
// ----------------------------------------------------------------------------
{
    Bytecode *bytecode = this;
    if (!Cached(value))
    {
        ValueMap &values = compile->values;
        XL::compile(scope, value, bytecode);
        size_t index = values.size();
        opaddr_t data = (index << OPSHIFT) | LOCAL;
        values[value] = data;
        OPCST(store, data);
    }
}


Tree *Bytecode::Type(Tree *value)
// ----------------------------------------------------------------------------
//   Return the last known type of the top of stack
// ----------------------------------------------------------------------------
{
    return compile->types[value];
}


void Bytecode::Type(Tree *value, Tree *type)
// ----------------------------------------------------------------------------
//   Return the last known type of the top of stack
// ----------------------------------------------------------------------------
{
    compile->types[value] = type;
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


opaddr_t RunState::Jump()
// ----------------------------------------------------------------------------
//   Read the next opcode as an intptr_t, e.g. branch offsets
// ----------------------------------------------------------------------------
{
    opaddr_t data = (opaddr_t) bytecode->code[pc++];
    XL_ASSERT((data & Bytecode::OPMASK) == Bytecode::JUMP);
    return (opaddr_t) (data >> Bytecode::OPSHIFT);
}


Tree *RunState::Local()
// ----------------------------------------------------------------------------
//   Get a local binding from the bytecode
// ----------------------------------------------------------------------------
{
    opaddr_t data = (opaddr_t) bytecode->code[pc++];
    XL_ASSERT((data & Bytecode::OPMASK) == Bytecode::PARAMETER ||
              (data & Bytecode::OPMASK) == Bytecode::LOCAL);
    opaddr_t index = data >> Bytecode::OPSHIFT;
    XL_ASSERT(locals + index < frame);
    return stack[locals + index];
}


void RunState::Save(Tree *value)
// ----------------------------------------------------------------------------
//   Save value to a local binding
// ----------------------------------------------------------------------------
{
    opaddr_t data = (opaddr_t) bytecode->code[pc++];
    XL_ASSERT((data & Bytecode::OPMASK) == Bytecode::LOCAL);
    opaddr_t index = data >> Bytecode::OPSHIFT;
    XL_ASSERT(stack.size() >= frame + index);
    stack.insert(stack.begin() + frame + index, value);
}


Tree *RunState::Saved()
// ----------------------------------------------------------------------------
//   Save value to a local binding
// ----------------------------------------------------------------------------
{
    opaddr_t data = (opaddr_t) bytecode->code[pc++];
    XL_ASSERT((data & Bytecode::OPMASK) == Bytecode::LOCAL);
    opaddr_t index = data >> Bytecode::OPSHIFT;
    XL_ASSERT(stack.size() >= frame + index);
    return stack[frame + index];
}


size_t RunState::FrameSize()
// ----------------------------------------------------------------------------
//   Get a frame size
// ----------------------------------------------------------------------------
{
    opaddr_t data = (opaddr_t) bytecode->code[pc++];
    XL_ASSERT((data & Bytecode::OPMASK) == Bytecode::LOCAL);
    opaddr_t size = data >> Bytecode::OPSHIFT;
    return size;
}


Tree *RunState::Constant()
// ----------------------------------------------------------------------------
//   Get a constant from the bytecode
// ----------------------------------------------------------------------------
{
    opaddr_t data = (opaddr_t) bytecode->code[pc++];
    XL_ASSERT((data & Bytecode::OPMASK) == Bytecode::CONSTANT);
    opaddr_t index = data >> Bytecode::OPSHIFT;
    return bytecode->Constant(index);
}


Rewrite *RunState::Rewrite()
// ----------------------------------------------------------------------------
//   Get a rewrite from the bytecode
// ----------------------------------------------------------------------------
{
    opaddr_t data = (opaddr_t) bytecode->code[pc++];
    XL_ASSERT((data & Bytecode::OPMASK) == Bytecode::REWRITE);
    opaddr_t index = data >> Bytecode::OPSHIFT;
    return bytecode->Rewrite(index);
}


void RunState::Dump(std::ostream &out)
// ----------------------------------------------------------------------------
//   Dump the run state for debugging purpose
// ----------------------------------------------------------------------------
{
    size_t max = stack.size();
    for (size_t i = 0; i < max; i++)
    {
        if (frame == i)
            out << "FRAME:\n";
        if (locals == i)
            out << "LOCAL:\n";
        out << i << ":\t" << stack[i] << "\n";
    }
    if (error)
    {
        out << "ERROR: " << error << "\n";
    }
    if (bytecode)
    {
        out << "Bytecode " << (void *) bytecode << " PC " << pc << "\n";
        bytecode->Dump(out, pc);
    }
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
    void        EvaluateCandidate()     { if (match != FAILED) successes++; }
    size_t      Successes()             { return successes; }

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
    Bytecode *  Code()                  { return bytecode; }
    strength    Strength()              { return match; }

private:
    // Bind a parameter, and get its local ID for the bytecode
    void        Bind(Name *name);

    // Report if we match or not
    strength    Perfect()               { return match; }
    strength    Failed()                { match = FAILED; return match; }
    strength    Possible()
    {
        if (match==PERFECT)
            match = POSSIBLE;
        return match;
    }

    void        MustEvaluate()
    {
        bytecode->Evaluate(evaluation, test);
    }

    Tree_p Evaluate(Tree *expr)
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
    strength            match;          // Check how well we matched
    Tree_p              defined;        // Have we bound a name yet?
    Tree_p              type;           // Result type
    size_t              successes;      // Number of cases where it worked
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
      defined(nullptr),
      type(),
      successes(0)
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
    bytecode->Type(test, natural_type);

    // If an exact match, no need to even check the value at runtime
    if (Natural *ival = test->AsNatural())
    {
        if (ival->value == what->value)
            return Perfect();
        else
            return Failed();
    }

    // Otherwise, we need to evaluate and check at runtime
    MustEvaluate();
    CHECKCST(check_natural, what);
    return Possible();
}


strength BytecodeBindings::Do(Real *what)
// ----------------------------------------------------------------------------
//   The pattern contains a real: check we have the same
// ----------------------------------------------------------------------------
{
    StripBlocks();
    bytecode->Type(test, real_type);

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
    MustEvaluate();
    CHECKCST(check_real, what);
    return Possible();
}


strength BytecodeBindings::Do(Text *what)
// ----------------------------------------------------------------------------
//   The pattern contains a real: check we have the same
// ----------------------------------------------------------------------------
{
    StripBlocks();
    bytecode->Type(test, text_type);

    if (Text *ival = test->AsText())
    {
        if (ival->value == what->value)
            return Perfect();
        else
            return Failed();
    }

    // Otherwise, we need to evaluate and check at runtime
    MustEvaluate();
    CHECKCST(check_text, what);
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
        OPCST(local, bytecode->Parameter(what));
        CHECK(check_same);
        return Possible();
    }

    // Otherwise, bind test value to name
    compile(evaluation, test, bytecode);
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
        MustEvaluate();
        OPCST(constant, meta);
        CHECK(check_same);
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
            compile(evaluation, test, bytecode);
            CHECK(check_borrow);
            Bind(name);
            return Possible();
        }
        else if (Infix *typecast = binding->AsInfix())
        {
            Tree_p type = EvaluateType(typecast->right);
            compile(evaluation, test, bytecode);
            if (type != bytecode->Type(test))
                CHECKCST(check_typed_borrow, type);
            bytecode->Type(test, type);
            Bind(name);
            return Possible();
        }
        return Failed();
    }

    // If the test itself is already a prefix, match directly
    if (Prefix *pfx = test->AsPrefix())
    {
        // Check prefix left first, which may set 'defined' to name
        bytecode->Type(pfx, prefix_type);
        defined = nullptr;
        test = pfx->left;
        what->left->Do(this);

        // Check prefix argument, which typically binds values
        defined = what;
        test = pfx->right;
        what->right->Do(this);
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
        bytecode->Type(pfx, postfix_type);
        defined = nullptr;
        test = pfx->right;
        what->right->Do(this);
        defined = what;
        test = pfx->left;
        what->left->Do(this);
        return Perfect();
    }

    // Otherwise, it's a mismatch
    return Failed();
}


static Name *IsBuiltinType(Tree *type)
// ----------------------------------------------------------------------------
//   Check if a type is a built-in type
// ----------------------------------------------------------------------------
{
#define NAME(N)
#define UNARY_OP(Name, ReturnType, LeftTYpe, Body)
#define BINARY_OP(Name, ReturnType, LeftTYpe, RightType, Body)
#define TYPE(N, Body)   if (type == N##_type) return N##_type;
#include "builtins.tbl"
    return nullptr;
}


static bool IncompatibleTypes(Name *want, Name *have)
// ----------------------------------------------------------------------------
//   Check if types are incompatible
// ----------------------------------------------------------------------------
{
    if (want == have)
        return false;
    if (want == tree_type || have == tree_type)
        return false;
    if (want == value_type || have == value_type)
        return false;
    if (want == symbol_type && (have == name_type || have == operator_type))
        return false;
    if ((want == name_type || want == operator_type) && have == symbol_type)
        return false;
    if ((want == text_type || want == character_type) &&
        (have == text_type || have == character_type))
        return false;
    text wn = want->value;
    text hn = have->value;
#define like(t)       compare(0, sizeof(t)-1, t) == 0
    if ((wn.like("natural") ||
         wn.like("integer") ||
         wn.like("real")) &&
        (hn.like("natural") ||
         hn.like("integer")))
        return false;
#undef like

    return true;
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
                bytecode->Type(test, wtype);

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

        // Need to match the left part with the converted value
        if (!outermost && !defined)
            defined = what;
        Tree_p value = test;
        what->left->Do(this);

        // Check hard incompatible types, e.g. integer vs text
        Tree_p have = bytecode->Type(test);
        if (Name *wn = IsBuiltinType(want))
            if (Name *hn = IsBuiltinType(have))
                if (IncompatibleTypes(wn, hn))
                    return Failed();

        // Type check value against type
        if (outermost)
            type = want;
        else if (want != have)
            CHECKCST(check_input_type, want);
        bytecode->Type(value, want);
        return Possible();
    }

    // Make sure names bind to values
    defined = what;

    // Check if we have a guard clause
    if (IsPatternCondition(what))
    {
        Tree_p tested = what->left;

        // It must pass the rest (need to bind values first)
        what->left->Do(this);

        // Compile the condition and check it
        compile(evaluation, what->right, bytecode);
        CHECK(check_guard);

        return Possible();
    }

    // The test itself should be an infix with a matching name
    if (Infix *ifx = test->AsInfix())
    {
        if (ifx->name == what->name)
        {
            bytecode->Type(ifx, infix_type);
            test = ifx->left;
            what->left->Do(this);
            test = ifx->right;
            what->right->Do(this);
            return Perfect();
        }
    }
    else if (Name *name = test->AsName())
    {
        MustEvaluate();
        CHECKCST(check_infix, what);
        bytecode->Type(name, infix_type);
        return Possible();
    }

    // Otherwise, it's a mismatch
    return Failed();
}


void BytecodeBindings::Bind(Name *name)
// ----------------------------------------------------------------------------
//   Bind a local name to the value at the top of stack
// ----------------------------------------------------------------------------
{
    bytecode->Bind(name);
    Context context(arguments);
    Context eval(evaluation);
    context.Define(name, eval.Enclose(test));
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

    // Add bytecode to check argument against parameters and create locals
    Bytecode::Attempt attempt(bytecode);
    bindings.Candidate(decl, evalScope, declScope);
    strength matched = pattern->Do(bindings);
    bindings.EvaluateCandidate();

    // Check if the candidate succeeded or failed
    if (matched == BytecodeBindings::FAILED)
    {
        // Failure: Cut the bytecode to where it was before
        attempt.Fail();
    }
    else
    {
        bool done = false;

        // Check if we matched a formal parameter
        if (matched == BytecodeBindings::PERFECT)
        {
            if (Name *name = pattern->AsName())
            {
                opaddr_t index = bytecode->Parameter(name);
                if (index)
                {
                    attempt.Fail();     // Cut any generated code
                    OPCST(local, index);
                    done = true;
                }
                else if (IsSelf(decl->Definition()))
                {
                    attempt.Fail();     // Cut any generated code
                    OPCST(constant, pattern);
                    done = true;
                }
            }
        }

        // Insert code to evaluate the body
        if (!done)
        {
            Tree  *body = decl->Definition();
            Scope *locals = bindings.ArgumentsScope();
            if (IsNative(body) || IsBuiltin(body) || body->Kind() <= TEXT)
            {
                // Fake call, inline
                compile(locals, body, bytecode);
            }
            else
            {
                // Real call
                NameList parms = bytecode->Parameters();
                parms.erase(parms.begin(), parms.begin() + attempt.frame);
                compile(locals, body, pattern, parms);

                // Transfer evaluation to the body
                opaddr_t framesize = bytecode->FrameSize(attempt.frame);
                OPCST(constant, pattern);
                OPCST(call, framesize);
            }

            // Check the result type if we had one
            if (Tree *type = bindings.ResultType())
            {
                if (bytecode->Type(expr) != type)
                    CHECKCST(check_result_type, type);
                bytecode->Type(expr, type);
            }
        }

        bytecode->Success();
    }

    // Keep looking for more unless it's a perfect match
    return (matched == BytecodeBindings::PERFECT) ? decl->right : nullptr;
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
        compile(scope, init, bytecode);
        if (Infix *typedecl = rw->left->AsInfix())
        {
            // If there is a type, insert a typecheck
            Tree_p type = typedecl->right;
            type = evaluate(scope, type);
            CHECKCST(check_init_type, type);
        }
        OPCST(init_value, rw);
    }
}


static bool doConstant(Scope *scope, Tree *tree, Bytecode *bytecode)
// ----------------------------------------------------------------------------
//   For a constant, simply emit it
// ----------------------------------------------------------------------------
{
    OPCST(constant, tree);
    switch(tree->Kind())
    {
    case NATURAL:       bytecode->Type(tree, natural_type); break;
    case REAL:          bytecode->Type(tree, real_type); break;
    case TEXT:          bytecode->Type(tree, text_type); break;
    default:            break;
    }

    return true;
}


static void binary_Sub(RunState &state);
static void binary_Add(RunState &state);
static void magic(RunState &state)
{
    Tree_p input = state.Pop();
    Natural_p value = input->AsNatural();
    static Natural_p one = new Natural(1);
    if (value->value == 0)
    {
        state.Push(one);
        return;
    }
    if (value->value == 1)
    {
        state.Push(one);
        return;
    }
    value->value -= 1;
    state.Push(value);
    magic(state);
    value->value -= 1;
    state.Push(value);
    magic(state);
    value->value += 2;
    binary_Add(state);
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
    if (name->value == "magic")
    {
#if 0
        Context context(scope);
        Name *fib = new Name("fib");
        Name *N = new Name("n");
        Prefix *pattern = new Prefix(fib, N);
        Tree *target = context.DeclaredPattern(pattern);

        opaddr_t zero = (0 << 3) | 3;
        opaddr_t one = (1 << 3) | 3;
        OPCST(local, zero);
        CHECKCST(check_natural, new Natural((int) 0));
        OPCST(constant, new Natural(1));
        bytecode->Success();
        bytecode->PatchChecks();
        OPCST(local, zero);
        CHECKCST(check_natural, new Natural(1));
        OPCST(constant, new Natural(1));
        bytecode->Success();
        bytecode->PatchChecks();
        OPCST(local, zero);
        OPCST(constant, new Natural(1));
        OP(binary_Sub);
        OPCST(constant, target);
        OPCST(call, one);
        OPCST(local, zero);
        OPCST(constant, new Natural(2));
        OP(binary_Sub);
        OPCST(constant, target);
        OPCST(call, one);
        OP(binary_Add);
        bytecode->PatchSuccesses(0);
#else
        OP(magic);
#endif
        return true;
    }
    return false;
}


const int IS_DEFINITION = 2;


static int doPrefix(Scope *scope, Prefix *prefix, Bytecode *bytecode)
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
            OPCST(constant, import);
            return IS_DEFINITION;
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

    // Check if we have a compile-time error
    if (Tree *message = IsError(prefix))
    {
        TreeList args;
        while (Infix *infix = message->AsInfix())
        {
            if (infix->name != ",")
                break;
            args.push_back(infix->left);
            message = infix->right;
        }
        args.push_back(message);
        Text *text = args[0]->AsText();
        Error error(text->value.c_str());
        for (uint i = 1; i < args.size(); i++)
            error.Arg(args[i]);
        error.Display();
        std::exit(1);
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


static int doInfix(Scope *scope, Infix *infix, Bytecode *bytecode)
// ----------------------------------------------------------------------------
//   Deal with all forms of special infix
// ----------------------------------------------------------------------------
{
    // For [Name is Expr], compute [Expr] at the time we hit the definition
    if (IsDefinition(infix))
    {
        // Find the corresponding declaration
        Context context(scope);
        Tree *pattern = infix->left;
        Rewrite *rewrite = context.Declared(pattern, Context::REFERENCE_SCOPE);
        if (Name *name = pattern->AsName())
        {
            // Compile the initialization and initialize the constant
            compile(scope, infix->right, bytecode);
            OPCST(init_value, rewrite);
        }
        // Return the definition
        OPCST(constant, rewrite);
        return IS_DEFINITION;
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
        if (bytecode->Type(infix->left) != want)
            CHECKCST(check_typecast, want);
        bytecode->Type(infix->left, want);
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


static Bytecode *compile(Scope *scope, Tree *expr)
// ----------------------------------------------------------------------------
//   Compile a definition or module and create its bytecode
// ----------------------------------------------------------------------------
{
    NameList parameters;
    if (Name *name = expr->AsName())
    {
        Context context(scope);
        Tree *form = context.Bound(name);
        return compile(scope, expr, form, parameters);
    }
    return compile(scope, expr, expr, parameters);
}


static Bytecode *compile(Scope *scope,
                         Tree *expr,
                         Tree *pattern,
                         NameList &parameters)
// ----------------------------------------------------------------------------
//   Compile a definition with parameters and create its bytecode
// ----------------------------------------------------------------------------
{
    if (Closure *closure = expr->As<Closure>())
    {
        expr = closure->Value();
        scope = closure->CapturedScope();
    }
    Bytecode *bytecode = pattern->GetInfo<Bytecode>();
    if (!bytecode)
    {
        // We need to create the bytecode for this expression
        bytecode = new Bytecode(scope, expr, parameters);
        pattern->SetInfo<Bytecode>(bytecode);

        // Check if we have instructions to process. If not, return declarations
        Context context(scope);
        bool hasInstructions = true;
        if (expr->AsBlock() || IsSequence(expr))
        {
            Initializers inits;
            hasInstructions = context.ProcessDeclarations(expr, inits);
            if (!scope->IsEmpty())
                doInitializers(inits, bytecode);
            if (!hasInstructions)
                OP(get_scope);
        }

        if (hasInstructions)
            compile(scope, expr, bytecode);
        if (RECORDER_TRACE(bytecode))
        {
            std::cerr << "Compiled " << expr << " as\n";
            bytecode->Dump(std::cerr);
            std::cerr << "\n";
        }
        bytecode->Validate();
}

    return bytecode;
}


static void compile(Scope *scope, Tree *expr, Bytecode *bytecode)
// ----------------------------------------------------------------------------
//   Compile an expression in the given bytecode
// ----------------------------------------------------------------------------
{
    Context context(scope);
    Tree::Iterator next(expr);
    bool first = true;
    bool definition = false;
    while ((expr = next()))
    {
        // If not the first statement, check result of previous statement
        if (!first)
        {
            if (definition)
                bytecode->RemoveLastDefinition();
            else
                OP(check_statement);
        }

        // Lookup expression
        size_t patches = bytecode->SuccessesToPatch();
        Context::LookupMode mode = lookupMode(bytecode);
        BytecodeBindings bindings(expr, bytecode);
        context.Lookup(expr, lookupCandidate, &bindings, mode);
        bytecode->PatchSuccesses(patches);

        // If we did not find a matching form, check standard evaluation
        int done = bindings.Successes() != 0;
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
            record(bytecode, "Giving up on bytecode for %t", expr);
            if (RECORDER_TRACE(bytecode))
                bytecode->Dump(std::cerr);
            bytecode->Clear();
            Tree_p msg = ErrorMessage("No form matches $1", expr);
            bytecode->ErrorExit(msg);
            return;
        }
        definition = done == IS_DEFINITION;
        first = false;
    }
}


Tree *evaluate(Scope_p scope, Tree_p expr)
// ----------------------------------------------------------------------------
//   Internal evaluator - Short circuits a few common expressions
// ----------------------------------------------------------------------------
{
    // Quick exit without compilation if the value was computed
    if (!expr->IsFromSource())
        return expr;

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
    Bytecode *bytecode = compile(scope, expr);
    XL_ASSERT(bytecode->IsValid() && "Cannot evaluate before code is compiled");

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
std::map<opcode_fn, kstring> Bytecode::names;


#define NORM2(x) Normalize(x), x

void BytecodeEvaluator::InitializeContext(Context &context)
// ----------------------------------------------------------------------------
//    Initialize all the names, e.g. xl_self, and all builtins
// ----------------------------------------------------------------------------
//    Load the outer context with the type check operators referring to
//    the builtins.
{
#define UNARY(Name, ReturnType, XType, Body)                            \
    builtins[#Name] = unary_##Name;                                     \
    Bytecode::names[unary_##Name] = "unary_" #Name;

#define BINARY(Name, RetTy, XType, YType, Body)                         \
    builtins[#Name] = binary_##Name;                                    \
    Bytecode::names[binary_##Name] = "binary_" #Name;

#define NAME(N)                                                         \
    context.Define(xl_##N, xl_self);

#define TYPE(N, Body)                                                   \
    types[#N] = N##_typecheck;                                          \
    builtins[#N"_typecheck"] = N##_typecheck;                           \
    Bytecode::names[N##_typecheck] = #N "_typecheck";                   \
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
        Bytecode::names[native->opcode] = symbol;
        Tree *shape = native->Shape();
        Prefix *body = new Prefix(C, new Text(symbol));
        context.Define(shape, body);
    }

#define BC_NAME(N)      Bytecode::names[N] = #N;
#include "bytecode-names.h"

    if (RECORDER_TRACE(symbols_sort))
        context.Dump(std::cerr, context.Symbols(), false);
}

XL_END

XL::Bytecode *xldebug(XL::Bytecode *bytecode)
// ----------------------------------------------------------------------------
//   Dump the bytecode for debug
// ----------------------------------------------------------------------------
{
    std::cout << "Bytecode " << (void *) bytecode << "\n";
    if (bytecode)
        bytecode->Dump(std::cout);
    return bytecode;
}


XL::RunState *xldebug(XL::RunState *state)
// ----------------------------------------------------------------------------
//   Dump the state for debugging
// ----------------------------------------------------------------------------
{
    std::cout << "RunState " << (void *) state << "\n";
    state->Dump(std::cout);
    return state;
}


XL::RunState *xldebug(XL::RunState &state)
// ----------------------------------------------------------------------------
//   Dump the state for debugging
// ----------------------------------------------------------------------------
{
    return xldebug(&state);
}
