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
//    Local data types
//
// ============================================================================

typedef uint16_t                        opcode_t; // 16-bit ought to be enough
typedef opcode_t *                      opcode_p;
typedef std::map<Tree_p, Tree_p>        TypeMap;
typedef std::map<Tree_p, opcode_t>      ValueMap;


enum Opcode
// ----------------------------------------------------------------------------
//    Definition of the opcodes
// ----------------------------------------------------------------------------
{
#define OPCODE(Name, Parms, Body)      opcode_##Name,
#include "bytecode.tbl"
    OPCODE_COUNT,
    UNPATCHED           = (opcode_t) -1,
    INVALID_OPCODE      = (opcode_t) -1
};


inline bool IsConstantIndex(opcode_t index)
// ----------------------------------------------------------------------------
//   Return true if the index is for a constant
// ----------------------------------------------------------------------------
{
    return (int16_t) index < 0;
}


inline opcode_t ConstantIndex(opcode_t index)
// ----------------------------------------------------------------------------
//   Return true if the index is for a constant
// ----------------------------------------------------------------------------
{
    return (opcode_t) ~index;
}


// Array for builtins and native functions
static std::map<text, Opcode>           builtins;
static std::map<text, opcode_t>         natives_index;
static std::vector<Native::opcode_fn>   natives;


struct Parameter
// ----------------------------------------------------------------------------
//   Description of a parameter in the bytecode
// ----------------------------------------------------------------------------
{
    Parameter(Name *name, Tree *type, opcode_t arg)
        : name(name), type(type), argument(arg) {}
    Name *      name;           // Name for the parameter
    Tree *      type;           // Known type for the parameter if any
    opcode_t    argument;       // Local or constant index for the argument
};
typedef std::vector<Parameter>  Parameters;



// ============================================================================
//
//   Forward declarations
//
// ============================================================================

static Tree *evaluate(Scope_p scope, Tree_p expr);
static Tree *typecheck(Scope_p scope, Tree_p type, Tree_p value);
static Bytecode *compile(Scope *scope, Tree *expr);
static void compile(Scope *scope, Tree *expr, Bytecode *bytecode);
static Bytecode *compile(Scope *, Tree *body, Tree *form, Parameters &parms);
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

struct Bytecode
// ----------------------------------------------------------------------------
//   A sequence of instructions associated to a tree
// ----------------------------------------------------------------------------
{
    Bytecode(Scope *scope, Tree *self, Parameters &parameters):
        code(), scope(scope), self(self),
        constants(), parameters(parameters), locals(0),
        compile(new CompileInfo)
    {
        SetTypesFromParameters();
        SetLocalsFromParameters();
    }
    ~Bytecode() { Cut(0); delete compile; }

    // Evaluate the bytecode
    Tree_p      Run(RunState &state);

public:
    typedef std::vector<opaddr_t>       Patches;
    typedef std::vector<opcode_t>       Opcodes;
    typedef std::vector<opcode_t>       ArgList;

    // Record a jump in the code (to be patched later)
    struct CheckJump   {};
    struct SuccessJump {};

    // Record a local value
    struct LocalIndexWrapper
    {
        LocalIndexWrapper(opcode_t index): index(index) {}
        opcode_t index;
    };
    struct ValueIndexWrapper
    {
        ValueIndexWrapper(Tree *value): value(value) {}
        Tree *value;
    };
    struct StorageIndexWrapper
    {
        StorageIndexWrapper(Tree *value): value(value) {}
        Tree *value;
    };

    // Record a typed constant
    template <typename Rep>
    struct RepConstant
    {
        typedef typename RunValue::Representation<Rep>::literal_t literal_t;
        RepConstant(literal_t value): value(value) {}
        literal_t value;
    };

public:
    // Enter opcodes
    void        Op(Opcode op);
    template<typename ...Args>
    void        Op(Opcode op, Args... args);
    void        NativeArguments(opcode_t nat, const Parameters &, Tree *expr);
    void        BuiltinArguments(const Parameters &parms, Tree *expr);
    opcode_t    EnterRunValue(const RunValue &rv);
    opcode_t    EnterTreeConstant(Tree *cst, MachineType mtype);
    void        PatchChecks(size_t where);
    void        PatchSuccesses(size_t where);
    size_t      ChecksToPatch();
    size_t      SuccessesToPatch();
    Opcode      LastOpcode();
    void        CutToLastOpcode();
    opaddr_t    RemoveLastNoOpBranch();

    // Runtime support functions
    void        ErrorExit(Tree *msg);   // Emit an error with msg and exit
    void        Clear();
    size_t      Size();
    Scope *     EvaluationScope();
    Tree *      Self();
    bool        IsValid();
    void        Validate();
    bool        LocalMode();
    bool        LocalMode(bool mode);
    bool        VariableMode();
    bool        VariableMode(bool mode);
    void        Bind(Name *name, Scope *scope, Tree *value);
    opcode_t    ParameterIndex(Name *name);
    Parameters &ParameterList();
    opcode_t    ValueIndex(Tree *value);
    opcode_t    StorageIndex(Tree *value);
    opcode_t    Unify(Tree *source, Tree *target, bool writable);
    opcode_t    Evaluate(Scope *scope, Tree *value);
    opcode_t    EvaluateAsConstant(Tree *value);
    Tree *      Type(Tree *value);
    void        Type(Tree *value, Tree *type);
    void        SetTypesFromParameters();
    void        SetLocalsFromParameters();
    void        Dump(std::ostream &out);
    void        Dump(std::ostream &out, opaddr_t &pc);
    text        Cached(opcode_t index);

private:
    friend struct RunState;
    void        Cut(opaddr_t cutpoint);

    // Enter the various kind of arguments (in a somewhat typesafe way)
    void        EnterArg(Tree *cst);
    void        EnterArg(Name *cst);
    void        EnterArg(Infix *cst);
    void        EnterArg(Rewrite *rewrite);
    void        EnterArg(const CheckJump &);
    void        EnterArg(const SuccessJump &);
    void        EnterArg(const LocalIndexWrapper &local);
    void        EnterArg(const ValueIndexWrapper &local);
    void        EnterArg(const StorageIndexWrapper &local);
    void        EnterArg(const Parameters &parms);
    template<typename T>
    void        EnterArg(const GCPtr<T> &p)     { EnterArg(p.Pointer()); }
    template<typename Rep>
    void        EnterArg(const RepConstant<Rep> &rep);
    template<typename First, typename ...Rest>
    void        EnterArg(First first, Rest... rest);

private:
    enum OpcodeKind
    {
        JUMP,                   // New value for PC
        LOCAL,                  // Index of a local variable
        CONSTANT,               // Index of a constant
        DEFINITION,             // Index of a definition (constant rewrite)
        NATIVE,                 // Native index
        ARGUMENTS,              // Argument list
    };

private:
    // Data for the bytecode
    Opcodes     code;           // Bytecode itself
    Scope_p     scope;          // Declaration scope for the code
    Tree *      self;           // Not a Tree_p to avoid GC cycles
    RunValues   constants;      // Constants generated by EnterArg
    Parameters  parameters;     // Parameters in effect in current scope
    size_t      locals;         // Number of locals

    struct CompileInfo
    {
        CompileInfo(): last(), local(false), variable(false) {}
        Patches     checks;     // Checks to patch
        Patches     successes;  // Checks to patch
        Patches     jumps;      // Jumps to patch if we shorten
        TypeMap     types;      // Last known type for a value
        ValueMap    values;     // Already-computed value
        opaddr_t    last;       // Last instruction entered by Op
        OpcodeKind *args;       // Checking arguments
        bool        local;      // Non-recursive lookup
        bool        variable;   // Evaluate as variable
    };
    CompileInfo *compile;       // Information used at compile-time

public:
    friend struct Attempt;
    struct Attempt
    // ------------------------------------------------------------------------
    //   Helper class to manage bytecode patching
    // ------------------------------------------------------------------------
    {
        Attempt(Bytecode *bytecode)
            : bytecode(bytecode),
              compile(bytecode->compile),
              cutpoint(bytecode->Size()),
              frame(bytecode->parameters.size()),
              checks(bytecode->ChecksToPatch()),
              values(compile->values),
              types(compile->types)
        {
            compile->types.clear();
            bytecode->SetTypesFromParameters();
        }
        ~Attempt()
        {
            bytecode->PatchChecks(checks);
            Parameters &parms = bytecode->parameters;
            parms.erase(parms.begin() + frame, parms.end());
        }
        void Fail()
        {
            bytecode->Cut(cutpoint);
            Patches &chk = compile->checks;
            chk.erase(chk.begin() + checks, chk.end());
            compile->values = values;
            compile->types = types;
        }

        Bytecode    *bytecode;
        CompileInfo *compile;
        opaddr_t     cutpoint;
        size_t       frame;
        size_t       checks;
        ValueMap     values;
        TypeMap      types;
    };

    // Names for debugging purpose
    static kstring name[OPCODE_COUNT];

    template <typename ...Args>
    static std::vector<OpcodeKind> OpcodeArgList(Args... args);
    static std::vector<Bytecode::OpcodeKind> OpcodeArgs[OPCODE_COUNT];
};



// ============================================================================
//
//    Adapters for arithmetic with various data types
//
// ============================================================================

// Arithmetic helpers
#define mod_natural(r, x, y)    r = x % y
#define rem_natural(r, x, y)    r = x % y
#define pow_natural(r, x, y)                    \
{                                               \
    r = 1;                                      \
    while (y > 0)                               \
    {                                           \
        if (y & 1)                              \
            r *= x;                             \
        x *= x;                                 \
        y >>= 1;                                \
    }                                           \
}

#define mod_integer(r, x, y)                    \
{                                               \
    r = x % y;                                  \
    if (r && (x^y) < 0)                         \
        r += y;                                 \
}

#define rem_integer(r, x, y)    r = x % y
#define pow_integer(r, x, y)                    \
{                                               \
    r = y >= 0 ? 1 : 0;                         \
    while (y > 0)                               \
    {                                           \
        if (y & 1)                              \
            r *= x;                             \
        x *= x;                                 \
        y >>= 1;                                \
    }                                           \
}

#define mod_real(r, x, y)                       \
{                                               \
    r = fmod(x, y);                             \
    if (r != 0.0 && x*y < 0.0)                  \
        r += y;                                 \
}
#define rem_real(r, x, y)       r = fmod(x,y)
#define pow_real(r, x, y)       r = pow(x,y);

#define not_natural(x)          ~x
#define not_integer(x)          ~x
#define not_boolean(x)          !x

#ifdef HAVE_FLOAT16
static inline _Float16 fmod(_Float16 x, _Float16 y)
{
    return (_Float16)  ::fmod(float(x), float(y));
}
static inline _Float16 pow(_Float16 x, _Float16 y)
{
    return (_Float16)  ::pow(float(x), float(y));
}
#endif // HAVE_FLOAT16



// ============================================================================
//
//    Bytecode evaluation
//
// ============================================================================

Tree_p Bytecode::Run(RunState &state)
// ----------------------------------------------------------------------------
//   Run the bytecode in the given state
// ----------------------------------------------------------------------------
//   This function is a bit ugly in the way it is written, but the design
//   results from careful measurements of various ways to evaluate a bytecode
//   efficiently.
//   The original design for the bytecode loop was indirect function calls.
//   It was measured against Lua and Python evaluating a recursive Fibonacci
//   function with a depth of 40. Measurements done on Core i7 and ARM Cpus.
//   Measurements of various alternatives showed that:
//   - A switch is roughly equivalent to the indirect function calls
//     (everything being the same, function calls were very slightly faster)
//   - However, with a switch, you can keep the primary state in local variables
//     This makes the main run loop evaluate ~3x faster.
//   - You can also then put the top stack items in local variables.
//     As long as you don't increase the number of opcodes (i.e. jumps),
//     this results in a small saving, but makes the opcodes easier to write.
//   - Finally, you can use a GCC extension, computed gotos, to branch directly
//     between opcodes, i.e. have one jump between opcodes instread of two.
//     This gives another 30% benefit on tight loops.
//   Another minor optimization is to re-compute the base of the stack locals
//   only when the stack address may have changed (i.e. push or pop)
{
    Scope       *scope  = state.scope;
    Bytecode    *bc     = this;
    opcode_t    *pc     = &bc->code[0];
    opcode_t    *last   = pc + bc->code.size();
    opcode_t    *ret_pc = 0;
    size_t       locals = 0;
    RunValue    *frame  = nullptr;
    RunValue    *cst    = &bc->constants[0];

start:
    // Put the labels in a static const table with deltas to ease relocation
    static const ptrdiff_t entry[] =
    {
#define OPCODE(Name, Parms, Body) ((char *) &&label_##Name-(char *) &&start),
#include "bytecode.tbl"
    };


// Macros to help writing opcodes
#define DATA            (*pc++)
#define INPUT_CONSTANT      cst[ConstantIndex(*pc++)]
#define INPUT_VALUE                                                     \
    (IsConstantIndex(*pc) ? cst[ConstantIndex(*pc++)] : frame[*pc++])
#define OUTPUT_VALUE                                    frame[*pc++]
#define REFRAME         (frame = &state.stack[locals])
#define RETARGET(PC)                                                    \
    do {                                                                \
        pc   = PC;                                                      \
        last = pc + bc->code.size();                                    \
        cst  = &bc->constants[0];                                       \
    } while(0)
#define NEXT            XL_ASSERT(pc < last);                           \
                        goto *((char *) &&start + entry[*pc++])
#define BRANCH(b)       (pc += b)
#define CHAIN(op, args) do { pc -= args; goto label_##op; } while(0)

    // Jump to first opcode
    state.Allocate(bc->locals);
    REFRAME;
    NEXT;


divide_by_zero:
// ----------------------------------------------------------------------------
//    We get here if an arithmetic divide by zero occurs
// ----------------------------------------------------------------------------
    {
        Error error("Divide by zero");
        state.Error(error);
        RunValue &output = OUTPUT_VALUE;
        output = RunValue((Tree *) error, error_mtype);
    }
    CHAIN(ret, 1);


// ----------------------------------------------------------------------------
//  Magic happens here - Insert code for the various opcodes
// ----------------------------------------------------------------------------
#define OPCODE(Name, Parms, Body)               \
    label_##Name:                               \
    {                                           \
        Body;                                   \
    }                                           \
    NEXT;
#include "bytecode.tbl"


// Cleanup our macro junk
#undef DATA
#undef INPUT_VALUE
#undef OUTPUT_VALUE
#undef REFRAME
#undef RETARGET
#undef NEXT
#undef BRANCH
#undef CHAIN
}



// ============================================================================
//
//   Entering opcodes and their arguments
//
// ============================================================================

// This macro facilitates the writing of opcodes in the code
#define OP(op, ...)                                     \
    do                                                  \
    {                                                   \
        bytecode->Op(opcode_##op, ## __VA_ARGS__);      \
    } while(0)


// This makes it possible to directly insert a check, a success or a local
static Bytecode::CheckJump CHECK;
static Bytecode::SuccessJump SUCCESS;
typedef Bytecode::LocalIndexWrapper LocalIndex;
typedef Bytecode::ValueIndexWrapper ValueIndex;
typedef Bytecode::StorageIndexWrapper StorageIndex;


template <typename Rep>
static Bytecode::RepConstant<Rep> Constant(Rep rep)
// ----------------------------------------------------------------------------
//   Create a constant for a representation type
// ----------------------------------------------------------------------------
{
    return Bytecode::RepConstant<Rep>(rep);
}


static Bytecode::RepConstant<text> Constant(text rep)
// ----------------------------------------------------------------------------
//   Create a constant for a representation type
// ----------------------------------------------------------------------------
{
    return Bytecode::RepConstant<text>(rep.c_str());
}


template <typename ...Args>
std::vector<Bytecode::OpcodeKind> Bytecode::OpcodeArgList(Args... args)
// ----------------------------------------------------------------------------
//   Build a list from the parameters given to OPCODE macro
// ----------------------------------------------------------------------------
{
    return std::vector<Bytecode::OpcodeKind>{ args... };
}


std::vector<Bytecode::OpcodeKind> Bytecode::OpcodeArgs[OPCODE_COUNT] =
// ----------------------------------------------------------------------------
//   Pre-compute the argument list for each opcode
// ----------------------------------------------------------------------------
{
#define OPCODE(Name, Parms, Body)       OpcodeArgList Parms,
#include "bytecode.tbl"
};


opcode_t Bytecode::EnterRunValue(const RunValue &rv)
// ----------------------------------------------------------------------------
//   Enter a runvalue directly
// ----------------------------------------------------------------------------
{
    size_t size = constants.size();
    opcode_t index;
    for (index = 0; index < size; index++)
        if (constants[index].type == rv.type &&
            constants[index].as_natural == rv.as_natural)
            break;
    if (index >= size)
    {
        constants.push_back(rv);
        XL_ASSERT(size < std::numeric_limits<opcode_t>::max());
    }
    record(opcode, "Constant %t = %u", rv.AsTree(), index);
    return ConstantIndex(index);
}


opcode_t Bytecode::EnterTreeConstant(Tree *cst, MachineType mtype)
// ----------------------------------------------------------------------------
//   Record an opcode with a constant in the bytecode
// ----------------------------------------------------------------------------
{
    RunValue rv(cst, mtype);
    return EnterRunValue(rv);
}


void Bytecode::Op(Opcode opcode)
// ----------------------------------------------------------------------------
//   Record an opcode in the bytecode
// ----------------------------------------------------------------------------
{
    record(opcode, "Opcode %+s", name[opcode]);
    compile->last = code.size();
    code.push_back(opcode);
#ifdef XL_ASSERT
    compile->args = &OpcodeArgs[opcode][0];
#endif
}


template<typename ...Args>
void Bytecode::Op(Opcode op, Args... args)
// ----------------------------------------------------------------------------
//   Enter an opcode with arguments
// ----------------------------------------------------------------------------
{
    Op(op);
    EnterArg(args...);
}


template<typename First, typename ...Rest>
void Bytecode::EnterArg(First first, Rest...rest)
// ----------------------------------------------------------------------------
//   Enter a sequence of arguments
// ----------------------------------------------------------------------------
{
    EnterArg(first);
    EnterArg(rest...);
}


void Bytecode::EnterArg(const Bytecode::CheckJump &)
// ----------------------------------------------------------------------------
//   Record that we have a check to patch
// ----------------------------------------------------------------------------
{
    XL_ASSERT(*compile->args++ == JUMP);
    compile->checks.push_back(code.size());
    code.push_back(UNPATCHED);
}


void Bytecode::EnterArg(const Bytecode::SuccessJump &)
// ----------------------------------------------------------------------------
//   Record that we have a successful value
// ----------------------------------------------------------------------------
{
    XL_ASSERT(*compile->args++ == JUMP);
    compile->successes.push_back(code.size());
    code.push_back(UNPATCHED);
}


void Bytecode::EnterArg(const Bytecode::LocalIndexWrapper &local)
// ----------------------------------------------------------------------------
//   Record the index
// ----------------------------------------------------------------------------
{
    XL_ASSERT(*compile->args++ == LOCAL);
    record(opcode, "Local %u", local.index);
    code.push_back(local.index);
}


void Bytecode::EnterArg(const Bytecode::ValueIndexWrapper &local)
// ----------------------------------------------------------------------------
//   Record a constant indicating a bytecode local
// ----------------------------------------------------------------------------
{
    XL_ASSERT(*compile->args++ == LOCAL);
    record(opcode, "Local value %t", local.value);
    code.push_back(ValueIndex(local.value));
}


void Bytecode::EnterArg(const Bytecode::StorageIndexWrapper &local)
// ----------------------------------------------------------------------------
//   Record a constant indicating a bytecode local
// ----------------------------------------------------------------------------
{
    XL_ASSERT(*compile->args++ == LOCAL);
    record(opcode, "Local storage %t", local.value);
    code.push_back(StorageIndex(local.value));
}


void Bytecode::EnterArg(const Parameters &parms)
// ----------------------------------------------------------------------------
//   Enter an argument list in the order expected for a normal call
// ----------------------------------------------------------------------------
{
    XL_ASSERT(*compile->args++ == ARGUMENTS);
    record(opcode, "Parameters %u", parms.size());
    code.push_back(parms.size());
    for (auto &p : parms)
        code.push_back(p.argument);
}


void Bytecode::EnterArg(Tree *cst)
// ----------------------------------------------------------------------------
//   Enter a tree constant
// ----------------------------------------------------------------------------
{
    XL_ASSERT((*compile->args == CONSTANT || *compile->args == LOCAL) &&
              compile->args++);
    opcode_t index = EnterTreeConstant(cst, tree_mtype);
    code.push_back(index);
}


void Bytecode::EnterArg(Name *cst)
// ----------------------------------------------------------------------------
//   Enter a name constant
// ----------------------------------------------------------------------------
{
    XL_ASSERT((*compile->args == CONSTANT || *compile->args == LOCAL) &&
              compile->args++);
    opcode_t index = EnterTreeConstant(cst, name_mtype);
    code.push_back(index);
}


void Bytecode::EnterArg(Infix *cst)
// ----------------------------------------------------------------------------
//   Enter an infix constant
// ----------------------------------------------------------------------------
{
    XL_ASSERT((*compile->args == CONSTANT || *compile->args == LOCAL) &&
              compile->args++);
    opcode_t index = EnterTreeConstant(cst, infix_mtype);
    code.push_back(index);
}


void Bytecode::EnterArg(Rewrite *rw)
// ----------------------------------------------------------------------------
//   Record an opcode with a definition (rewrite) in the bytecode
// ----------------------------------------------------------------------------
{
    XL_ASSERT(*compile->args++ == DEFINITION);
    opcode_t index = EnterTreeConstant(rw, definition_mtype);
    code.push_back(index);
}


template <typename Rep>
void Bytecode::EnterArg(const RepConstant<Rep> &rep)
// ----------------------------------------------------------------------------
//   Enter a constant for a given machine typeo
// ----------------------------------------------------------------------------
{
    XL_ASSERT(*compile->args++ == CONSTANT);
    RunValue rv(rep.value);

    if (sizeof(uint64_t) == sizeof(Natural::value_t) &&
        rv.type == natural64_mtype)
        rv.type = natural_mtype;
    if (sizeof(uint32_t) == sizeof(Natural::value_t) &&
        rv.type == natural32_mtype)
        rv.type = natural_mtype;
    if (sizeof(int64_t) == sizeof(Natural::value_t) &&
        rv.type == integer64_mtype)
        rv.type = natural_mtype;
    if (sizeof(int32_t) == sizeof(Natural::value_t) &&
        rv.type == integer32_mtype)
        rv.type = natural_mtype;

    opcode_t index = EnterRunValue(rv);
    code.push_back(index);
}


void Bytecode::NativeArguments(opcode_t native,
                               const Parameters &args,
                               Tree *expr)
// ----------------------------------------------------------------------------
//   Push arguments for a native function
// ----------------------------------------------------------------------------
//   Since the native interface pops arguments in the C order, we need to
//   push the first argument last.
{
    XL_ASSERT(*compile->args++ == NATIVE);
    XL_ASSERT(*compile->args++ == ARGUMENTS);
    size_t n = args.size();
    record(opcode, "NativeArguments index %u, args=%u", native, n);
    code.push_back(native);
    code.push_back(n);
    while (n --> 0)
        code.push_back(args[n].argument);
    opcode_t index = StorageIndex(expr);
    code.push_back(index);
}


void Bytecode::BuiltinArguments(const Parameters &args, Tree *expr)
// ----------------------------------------------------------------------------
//   Push arguments for a native function
// ----------------------------------------------------------------------------
{
    size_t count = args.size();
    record(opcode, "BuiltinArguments %u", count);
    for (size_t idx = 0; idx < count; idx++)
        code.push_back(args[idx].argument);
    opcode_t index = StorageIndex(expr);
    code.push_back(index);
}


void Bytecode::PatchChecks(size_t where)
// ----------------------------------------------------------------------------
//   Patch the pending checks to the current position
// ----------------------------------------------------------------------------
{
    opaddr_t target = code.size();
    Patches &checks = compile->checks;
    Patches &jumps = compile->jumps;
    while (checks.size() > where)
    {
        opaddr_t check = checks.back();
        XL_ASSERT(code[check] == UNPATCHED);
        code[check] = target - (check + 1);
        checks.pop_back();
        jumps.push_back(check);
    }
}


size_t Bytecode::ChecksToPatch()
// ----------------------------------------------------------------------------
//   Return where we are in the checks list
// ----------------------------------------------------------------------------
{
    return compile->checks.size();
}


opaddr_t Bytecode::RemoveLastNoOpBranch()
// ----------------------------------------------------------------------------
//   Remove the last no-op branch
// ----------------------------------------------------------------------------
{
    opaddr_t target = code.size();
    Patches &jumps = compile->jumps;
    if (jumps.size() >= 1 && jumps.back() == target - 1)
    {
        code.erase(code.begin() + jumps.back() - 1, code.end());
        jumps.pop_back();
        compile->last = 0;
        target = code.size();
    }
    return target;
}


void Bytecode::PatchSuccesses(size_t where)
// ----------------------------------------------------------------------------
//   Patch the pending checks to the current position
// ----------------------------------------------------------------------------
{
    opaddr_t target = RemoveLastNoOpBranch();
    Patches &successes = compile->successes;
    Patches &jumps = compile->jumps;

    while (successes.size() > where)
    {
        opaddr_t success = successes.back();
        XL_ASSERT(code[success] == UNPATCHED);
        code[success] = target - (success + 1);
        successes.pop_back();
        jumps.push_back(success);
        target = RemoveLastNoOpBranch();
    }

}


size_t Bytecode::SuccessesToPatch()
// ----------------------------------------------------------------------------
//   Return where we are in the successes list
// ----------------------------------------------------------------------------
{
    return compile->successes.size();
}


Opcode Bytecode::LastOpcode()
// ----------------------------------------------------------------------------
//   Return the last opcode, or INVALID_OPCODE if none
// ----------------------------------------------------------------------------
{
    if (compile->last < code.size())
        return (Opcode) code[compile->last];
    return INVALID_OPCODE;
}


void Bytecode::CutToLastOpcode()
// ----------------------------------------------------------------------------
//   Cut to the last opcode
// ----------------------------------------------------------------------------
{
    Cut(compile->last);
}



// ============================================================================
//
//    Runtime support functions
//
// ============================================================================

void Bytecode::ErrorExit(Tree *expr)
// ----------------------------------------------------------------------------
//   Emit an error and exit evaluation
// ----------------------------------------------------------------------------
{
    Bytecode *bytecode = this;
    OP(error_exit, expr);
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
    parameters.clear();
    compile->types.clear();
    compile->values.clear();
    compile->local = false;
    compile->variable = false;
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
    PatchChecks(0);
    if (code.back() != opcode_ret)
    {
        Bytecode *bytecode = this;
        OP(ret, XL::ValueIndex(bytecode->self));
    }
    locals -= parameters.size();
    if (RECORDER_TRACE(bytecode))
    {
        std::cerr << "Compiled " << self << " as\n";
        Dump(std::cerr);
        std::cerr << "\n";
    }
    delete compile;
    compile = nullptr;
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


void Bytecode::Bind(Name *name, Scope *scope, Tree *value)
// ----------------------------------------------------------------------------
//   Bind a parameter in the current bytecode
// ----------------------------------------------------------------------------
{
    opcode_t index = Evaluate(scope, value);
    Tree *type = Type(value);
    parameters.push_back(Parameter(name, type, index));
}


opcode_t Bytecode::ParameterIndex(Name *name)
// ----------------------------------------------------------------------------
//    Find the index of a parameter in the current bytecode
// ----------------------------------------------------------------------------
{
    opaddr_t max = parameters.size();
    opcode_t index = max;
    while (index --> 0)
        if (parameters[index].name == name)
            return index;
    return UNPATCHED;                   // Not a valid parameter index
}


inline Parameters &Bytecode::ParameterList()
// ----------------------------------------------------------------------------
//   Return current parameter list for the bytecode
// ----------------------------------------------------------------------------
{
    return parameters;
}


static Tree *MachineTypeToXLType(MachineType mtype)
// ----------------------------------------------------------------------------
//    Generate the type for a given machine type
// ----------------------------------------------------------------------------
{
    switch(mtype)
    {
#define RUNTIME_TYPE(Name, Rep, BC)                             \
    case Name##_mtype:  return Name##_type;
#include "machine-types.tbl"
    default:
        break;
    }
    return nullptr;
}


opcode_t Bytecode::ValueIndex(Tree *value)
// ----------------------------------------------------------------------------
//   Find the local or constant index for a given value
// ----------------------------------------------------------------------------
{
    opcode_t index = EvaluateAsConstant(value);
    if (index)
        return index;
    return StorageIndex(value);
}


opcode_t Bytecode::StorageIndex(Tree *value)
// ----------------------------------------------------------------------------
//   Return the local storage index for a given value
// ----------------------------------------------------------------------------
{
    opcode_t index;
    ValueMap &values = compile->values;
    auto found = values.find(value);
    if (found != values.end())
    {
        // Already compiled and evaluated - Reload it
        index = (*found).second;
    }
    else
    {
        index = locals++;
        values[value] = index;
    }
    return index;
}


opcode_t Bytecode::Unify(Tree *source, Tree *target, bool writable)
// ----------------------------------------------------------------------------
//   Share the storage between source and target
// ----------------------------------------------------------------------------
{
    opcode_t index = ValueIndex(source);
    ValueMap &values = compile->values;
    Bytecode *bytecode = this;
    auto tf = values.find(target);
    if (tf == values.end())
    {
        if (writable && IsConstantIndex(index))
        {
            opcode_t tgt = locals++;
            OP(copy, LocalIndex(index), LocalIndex(tgt));
            values[target] = tgt;
            return tgt;
        }
        else
        {
            values[target] = index;
            return index;
        }
    }
    if (index != (*tf).second)
        OP(copy, LocalIndex(index), LocalIndex((*tf).second));
    return index;
}


opcode_t Bytecode::EvaluateAsConstant(Tree *value)
// ----------------------------------------------------------------------------
//   Return a constant index if we can evaluate as a constant
// ----------------------------------------------------------------------------
//   Note that if we don't find a constant, e.g. [0], we need to return false
//   in order to do a proper lookup, which usually fails, so we enter a
//   constant, which is checked after compilation
{
    Tree *type = tree_type;
    opcode_t index;

    // Check if we have a constant
    size_t max = constants.size();
    switch(value->Kind())
    {
    case NATURAL:
    {
        Natural *nat = (Natural *) value;
        Natural::value_t ref = nat->value;
        if (nat->IsSigned())
        {
            type = integer_type;
            for (index = 0; index < max; index++)
            {
                RunValue &cst = constants[index];
                if (IsInteger(cst.type) && ref == cst.AsInteger())
                    goto found;
            }
        }
        else
        {
            type = natural_type;
            for (index = 0; index < max; index++)
            {
                RunValue &cst = constants[index];
                if (IsNatural(cst.type) && ref == cst.AsNatural())
                    goto found;
            }
        }
        break;
    }
    case REAL:
    {
        Real::value_t ref = ((Real *) value)->value;
        type = real_type;
        for (index = 0; index < max; index++)
        {
            RunValue &cst = constants[index];
            if (IsReal(cst.type) && ref == cst.AsReal())
                goto found;
        }
        break;
    }
    case TEXT:
    {
        Text *txt = (Text *) value;
        if (txt->IsCharacter())
        {
            char ref = txt->value[0];
            type = character_type;
            for (index = 0; index < max; index++)
            {
                RunValue &cst = constants[index];
                if (cst.type == character_mtype && ref == cst.AsText()[0])
                    goto found;
            }
        }
        else
        {
            Text::value_t ref = txt->value;
            type = text_type;
            for (index = 0; index < max; index++)
            {
                RunValue &cst = constants[index];
                if (cst.type == text_mtype && ref == cst.AsText())
                    goto found;
            }
        }
        break;
    }

    case NAME:
    {
        Name *name = (Name *) value;

        // Check as a name constant
        for (index = 0; index < max; index++)
        {
            RunValue &cst = constants[index];
            if (cst.type == name_mtype && name->value == cst.as_name->value)
            {
                type = name_type;
                goto found;
            }
        }
        break;
    }
    default:
        // Check as a tree constant
        for (index = 0; index < max; index++)
        {
            RunValue &cst = constants[index];
            if (IsTree(cst.type) && value == cst.as_tree)
            {
                type = MachineTypeToXLType(cst.type);
                goto found;
            }
        }
        break;
    }
    return 0;

found:
    Type(value, type);
    return ConstantIndex(index);
}


opcode_t Bytecode::Evaluate(Scope *scope, Tree *value)
// ----------------------------------------------------------------------------
//   Evaluate a value but only once
// ----------------------------------------------------------------------------
{
    opcode_t index;
    ValueMap &values = compile->values;

    for (int pass = 0; pass < 2; pass++)
    {
        if (opcode_t cst = EvaluateAsConstant(value))
            return cst;

        auto found = values.find(value);
        if (found != values.end())
        {
            // Already compiled and evaluated - Reload it
            index = (*found).second;
        }
        else if (pass)
        {
            index = locals++;
            values[value] = index;
            if (index < parameters.size() && value != parameters[index].name)
                Type(value, parameters[index].type);
        }
        else
        {
            XL::compile(scope, value, this);
        }
    }
    return index;
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


void Bytecode::SetTypesFromParameters()
// ----------------------------------------------------------------------------
//   Resset the types to what is in the parameter list
// ----------------------------------------------------------------------------
{
    for (auto &p : parameters)
        Type(p.name, p.type);
}


void Bytecode::SetLocalsFromParameters()
// ----------------------------------------------------------------------------
//   Initialize the values map from the index of parameters
// ----------------------------------------------------------------------------
{
    opcode_t  index  = 0;
    ValueMap &values = compile->values;
    for (auto &p : parameters)
        values[p.name] = index++;
    locals = index;
}



// ============================================================================
//
//    Debug utilities
//
// ============================================================================

text Bytecode::Cached(opcode_t local)
// ----------------------------------------------------------------------------
//   Return the cached expression corresponding to an index
// ----------------------------------------------------------------------------
{
    text result = "";
    if (compile)
    {
        for (const auto &it : compile->values)
            if (local == it.second)
                result = result + "[" + text(*it.first) + "]";
    }
    return result;
}


void Bytecode::Dump(std::ostream &out)
// ----------------------------------------------------------------------------
//   Dump the bytecocde
// ----------------------------------------------------------------------------
{
    size_t csts = constants.size();
    if (csts)
    {
        out << "Constants:\n";
        for (size_t cst = 0; cst < csts; cst++)
            out << "  C" << cst << "\t= " << constants[cst].AsTree() << "\n";
    }

    size_t parms = parameters.size();
    if (parms)
    {
        out << "Parameters:\n";
        for (size_t parm = 0; parm < parms; parm++)
            out << "  A" << parm << "\t= " << parameters[parm].name << "\n";
    }

    if (compile)
    {
        if (locals)
        {
            out << "Locals:\n";
            for (opcode_t index = 0; index < locals; index++)
            {
                out << (index < parameters.size() ? "  A" : "  L") << index
                    << "\t= " << Cached(index)
                    << "\n";
            }
        }
        if (compile->types.size())
        {
            out << "Types:\n";
            for (auto &it : compile->types)
                out << "  " << it.first << "\tas\t" << it.second << "\n";
        }
    }

    out << "Opcodes ("
        << locals << " locals, "
        << parms << " parameters";
    if (locals > parms)
        out << ", " << locals - parms << " temporaries";
    out << ")\n";
    opaddr_t max = code.size();
    opaddr_t pc = 0;
    while (pc < max)
        Dump(out, pc);
}


void Bytecode::Dump(std::ostream &out, opaddr_t &pcr)
// ----------------------------------------------------------------------------
//   Dump a single bytecode
// ----------------------------------------------------------------------------
{
    std::vector<OpcodeKind> args;
    opaddr_t pc = pcr;
    bool debug = RECORDER_TRACE(bytecode) > 1;

    switch(code[pc++])
    {
#define OPCODE(Name, Parms, Body)                                       \
    case opcode_##Name:                                                 \
    {                                                                   \
        std::cout << std::right << std::setw(6) << pc-1 << ": "         \
                  << std::left  << std::setw(20) << #Name;              \
        args = OpcodeArgs[opcode_##Name];                               \
        break;                                                          \
    }
#include "bytecode.tbl"
    default:
        std::cout << std::right << std::setw(6) << pc-1 << ": "
                  << std::left  << std::setw(20) << "INVALID " << code[pc-1];
        break;
    }

    // Print arguments
    text sep;
    for (auto a : args)
    {
        opcode_t value = code[pc++];
        out << sep;
        switch(a)
        {
        case JUMP:
            if (value == UNPATCHED)
                out << "?";
            else
                out << "+" << value << "=" << pc+value;
            break;
        case LOCAL:
            if (IsConstantIndex(value))
            {
                out << "C" << ConstantIndex(value);
                if (debug)
                    out << "["
                        << constants[ConstantIndex(value)].AsTree()
                        << "]";
            }
            else if (value < parameters.size())
            {
                out << "A" << value;
                if (debug)
                    out << "[" << parameters[value].name << "]";
            }
            else
            {
                out << "L" << value;
                if (debug)
                    out << Cached(value);
            }
            break;
        case CONSTANT:
            value = ConstantIndex(value);
            out << "C" << value;
            if (debug && value < constants.size())
                out << "[" << constants[value].AsTree() << "]";
            break;
        case ARGUMENTS:
            out << "(" << value << ")";
            for (size_t a = 0; a < value; a++)
            {
                opcode_t index = code[pc++];
                out << "\n  "
                    << std::right << std::setw(6) << a;
                if (IsConstantIndex(index))
                {
                    index = ConstantIndex(index);
                    out << ": C" << index;
                    if (debug)
                        out << "[" << constants[index].AsTree() << "]";
                }
                else
                {
                    out << ": L" << index;
                    if (debug)
                    {
                        if (index < parameters.size())
                            out << "[" << parameters[index].name << "]";
                        else
                            out << Cached(index);
                    }
                }
            }
            sep = "\n       => ";
            continue;
        case NATIVE:
            out << "#" << value;
            for (auto &n : natives_index)
                if (n.second == value)
                    out << "=" << n.first;
            break;
        default:
            out << "Unhandled parameter kind " << a;
            break;
        }
        sep = ", ";
    }

    out << "\n";
    pcr = pc;
}



// ============================================================================
//
//   RunState functions that access the bytecode
//
// ============================================================================

void RunState::Dump(std::ostream &out)
// ----------------------------------------------------------------------------
//   Dump the run state for debugging purpose
// ----------------------------------------------------------------------------
{
    size_t max = stack.size();
    for (size_t i = 0; i < max; i++)
        out << i << ":\t" << stack[i] << "\n";
    if (error)
        out << "ERROR: " << error << "\n";
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
    bool        PerfectMatch()          { return match == PERFECT; }

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
    void        Bind(Name *name, Tree *value);

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
    OP(check_natural, ValueIndex(test), Constant(what->value), CHECK);
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

    // Otherwise, we need to evaluate and check at runtime
    MustEvaluate();
    OP(check_real, ValueIndex(test), Constant(what->value), CHECK);
    return Possible();
}


strength BytecodeBindings::Do(Text *what)
// ----------------------------------------------------------------------------
//   The pattern contains a text: check we have the same
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
    OP(check_text, ValueIndex(test), Constant(what->value), CHECK);
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
        opcode_t index = bytecode->ParameterIndex(what);
        XL_ASSERT(index != UNPATCHED);
        OP(check_same, ValueIndex(test), LocalIndex(index), CHECK);
        return Possible();
    }

    // Otherwise, bind test value to name
    StripBlocks();
    Bind(what, test);
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
        OP(check_same, ValueIndex(test), meta, CHECK);
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
            Bind(name, test);
            OP(check_borrow, ValueIndex(name), CHECK);
            return Possible();
        }
        else if (Infix *typecast = binding->AsInfix())
        {
            name = typecast->left->AsName();
            Tree_p type = EvaluateType(typecast->right);
            bytecode->Type(test, type);
            Bind(name, test);
            if (type != bytecode->Type(test))
                OP(check_typed_borrow, ValueIndex(name),  type, CHECK);
            return Possible();
        }
        return Failed();
    }

    // If the test itself is already a prefix, match directly
    if (Prefix *pfx = test->AsPrefix())
    {
        // Check prefix left first, which may set 'defined' to name
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


static MachineType IsMachineType(Tree *type)
// ----------------------------------------------------------------------------
//   Check if a type is a built-in type
// ----------------------------------------------------------------------------
{
#define RUNTIME_TYPE(Name, Rep, BC)                     \
    if (type == Name##_type) return Name##_mtype;
#include "machine-types.tbl"
    return runvalue_unset;
}


static bool IncompatibleTypes(MachineType want, MachineType have)
// ----------------------------------------------------------------------------
//   Check if types are incompatible
// ----------------------------------------------------------------------------
{
    if (want == have)
        return false;
    if (want == tree_mtype || have == tree_mtype)
        return false;
    if (want == symbol_mtype && (have == name_mtype || have == operator_mtype))
        return false;
    if ((want == name_mtype || want == operator_mtype) && have == symbol_mtype)
        return false;
    if ((want == text_mtype || want == character_mtype) &&
        (have == text_mtype || have == character_mtype))
        return false;
    if ((IsInteger(want) || IsNatural(want) || IsReal(want)) &&
        (IsNatural(have) || IsInteger(have)))
        return false;
    return true;
}


static Opcode MachineTypeCast(MachineType mtype)
// ----------------------------------------------------------------------------
//    Generate the machine cast for a given machine type
// ----------------------------------------------------------------------------
{
    switch(mtype)
    {
#define RUNTIME_TYPE(Name, Rep, BC)                     \
    case Name##_mtype:  return opcode_##Name##_cast;
#include "machine-types.tbl"
    default:
        break;
    }
    return opcode_nil;
}


static Opcode MachineTypeCheck(MachineType mtype)
// ----------------------------------------------------------------------------
//    Generate the machine cast for a given machine type
// ----------------------------------------------------------------------------
{
    switch(mtype)
    {
#define RUNTIME_TYPE(Name, Rep, BC)                             \
    case Name##_mtype:  return opcode_##Name##_typecheck;
#include "machine-types.tbl"
    default:
        break;
    }
    return opcode_nil;
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
        what->right = want;

        // Need to match the left part with the converted value
        if (!outermost && !defined)
            defined = what;
        Tree_p value = test;
        what->left->Do(this);

        // Type check value against type
        Tree_p have = bytecode->Type(test);
        if (outermost)
        {
            type = want;
        }
        else if (want != have)
        {
            // Check hard incompatible types, e.g. integer vs text
            if (MachineType wm = IsMachineType(want))
            {
                MachineType hm = IsMachineType(have);
                if (hm)
                {
                    if (IncompatibleTypes(wm, hm))
                        return Failed();
                }
                bytecode->Type(value, want);

                if (wm != hm)
                {
                    Opcode cast = MachineTypeCheck(wm);
                    bytecode->Op(cast, StorageIndex(value), CHECK);
                    return Possible();
                }
                return Perfect();
            }

            OP(check_input_type, want, ValueIndex(test), CHECK);
        }
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
        OP(check_guard, ValueIndex(what->right), CHECK);

        return Possible();
    }

    // The test itself should be an infix with a matching name
    if (Infix *ifx = test->AsInfix())
    {
        if (ifx->name == what->name)
        {
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
        OP(check_infix, ValueIndex(test), what,
           StorageIndex(what->left), StorageIndex(what->right),
           CHECK);
        bytecode->Type(name, infix_type);
        return Possible();
    }

    // Otherwise, it's a mismatch
    return Failed();
}


void BytecodeBindings::Bind(Name *name, Tree *value)
// ----------------------------------------------------------------------------
//   Bind a local name to the value at the top of stack
// ----------------------------------------------------------------------------
{
    Context context(arguments);
    Context eval(evaluation);
    context.Define(name, eval.Enclose(value));
    bytecode->Bind(name, evaluation, value);
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
                opcode_t index = bytecode->ParameterIndex(name);
                if (index != UNPATCHED)
                {
                    bytecode->Type(expr, bytecode->Type(name));
                    bytecode->Unify(name, expr, false);
                    done = true;
                }
                else if (IsSelf(decl->Definition()))
                {
                    bytecode->Type(expr, bytecode->Type(name));
                    bytecode->EnterTreeConstant(name, name_mtype);
                    done = true;
                }
            }
        }

        // Insert code to evaluate the body
        if (!done)
        {
            Tree  *body = decl->Definition();
            Scope *locals = bindings.ArgumentsScope();
            Parameters parms = bytecode->ParameterList();
            parms.erase(parms.begin(), parms.begin() + attempt.frame);

            if (Text *name = IsNative(body))
            {
                auto found = natives_index.find(name->value);
                if (found == natives_index.end())
                {
                    record(opcode_error,
                           "Nonexistent native function %t", name);
                    attempt.Fail();
                    return nullptr;
                }
                opcode_t index = (*found).second;
                OP(native);
                bytecode->NativeArguments(index, parms, expr);
            }
            else if (Text *name = IsBuiltin(body))
            {
                Opcode opcode = builtins[name->value];
                if (!opcode)
                {
                    record(opcode_error, "Nonexistent builtin %t", name);
                    attempt.Fail();
                    return nullptr;
                }

                // Insert the opcode, followed by the parameters
                bytecode->Op(opcode);
                bytecode->BuiltinArguments(parms, expr);
            }
            else if (body->Kind() <= TEXT)
            {
                // Inline the compilation of constants, e.g. [0]
                compile(locals, body, bytecode);
                bytecode->Unify(body, expr, true);
            }
            else
            {
                // Real call
                compile(locals, body, pattern, parms);

                // Transfer evaluation to the body
                OP(call, pattern, parms, StorageIndex(expr));
            }

            // Check the result type if we had one
            if (Tree *type = bindings.ResultType())
            {
                if (bytecode->Type(expr) != type)
                    OP(check_result_type, type, StorageIndex(expr));
                bytecode->Type(expr, type);
            }
        }

        OP(branch, SUCCESS);
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
            OP(check_init_type, type, ValueIndex(init), CHECK);
        }
        OP(init_value, rw);
    }
}


static bool doConstant(Scope *scope, Tree *tree, Bytecode *bytecode)
// ----------------------------------------------------------------------------
//   For a constant, simply emit it
// ----------------------------------------------------------------------------
{
    switch(tree->Kind())
    {
    case NATURAL:
    {
        Natural *n = (Natural *) tree;
        RunValue rv = n->IsSigned()
            ? RunValue((longlong) n->value, integer_mtype)
            : RunValue(n->value, natural_mtype);
        bytecode->EnterRunValue(rv);
        bytecode->Type(tree, natural_type);
        break;
    }
    case REAL:
    {
        Real *r = (Real *) tree;
        RunValue rv(r->value, real_mtype);
        bytecode->EnterRunValue(rv);
        bytecode->Type(tree, real_type);
        break;
    }
    case TEXT:
    {
        Text *t = (Text *) tree;
        RunValue rv = t->IsCharacter()
            ? RunValue(t->value[0])
            : RunValue(t->value);
        bytecode->EnterRunValue(rv);
        bytecode->Type(tree, text_type);
        break;
    }
    default:
        XL_ASSERT(!"doConstant called with non-constant tree");
        break;
    }

    return true;
}


static bool doName(Scope *scope, Name *name, Bytecode *bytecode)
// ----------------------------------------------------------------------------
//   Deal with special names
// ----------------------------------------------------------------------------
{
    if (name->value == "nil")
    {
        OP(nil, StorageIndex(name));
        return true;
    }
    if (name->value == "true")
    {
        OP(true, StorageIndex(name));
        return true;
    }
    if (name->value == "false")
    {
        OP(false, StorageIndex(name));
        return true;
    }
    if (name->value == "scope" || name->value == "context")
    {
        OP(get_scope, StorageIndex(name));
        return true;
    }
    if (name->value == "super")
    {
        OP(get_super, StorageIndex(name));
        return true;
    }
    if (name->value == "self")
    {
        OP(get_self, StorageIndex(name));
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
        OP(set_scope, ValueIndex(prefix->left));
        doInitializers(inits, bytecode);
        compile(closure, prefix->right, bytecode);
        return true;
    }

    // Check if we have [variable T], if so build a variable type
    if (IsVariableType(prefix))
    {
        compile(scope, prefix->right, bytecode);
        return true;
    }

    // Check if we are evaluating a pattern matching type
    if (Tree *matching = IsPatternMatchingType(prefix))
    {
        Tree_p pattern = xl_parse_tree(scope, matching);
        Tree_p entered = new Prefix(xl_matching, pattern);
        bytecode->EnterTreeConstant(entered, prefix_mtype);
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
        bytecode->EnterTreeConstant(quote, tree_mtype);
        return true;
    }

    // Filter out import statements (processed also during parsing)
    if (Name *import = prefix->left->AsName())
    {
        if (eval_fn callback = MAIN->Importer(import->value))
        {
            // Process the import statement
            callback(scope, prefix);
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
        if (Name *name = pattern->AsName())
        {
            // Compile the initialization and initialize the constant
            compile(scope, infix->right, bytecode);
            OP(init_value, ValueIndex(infix->right), StorageIndex(infix->left));
        }
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
            OP(check_typecast, want, ValueIndex(infix->left), CHECK);
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
        OP(copy, ValueIndex(infix->right), StorageIndex(infix->left));
        return true;
    }
    return false;
}


static Bytecode *compile(Scope *scope, Tree *expr)
// ----------------------------------------------------------------------------
//   Compile a definition or module and create its bytecode
// ----------------------------------------------------------------------------
{
    Parameters parameters;
    if (Name *name = expr->AsName())
    {
        Context context(scope);
        Tree *form = context.Bound(name);
        if (!form)
            form = expr;        // Case for names such as "false"
        return compile(scope, expr, form, parameters);
    }
    return compile(scope, expr, expr, parameters);
}


static Bytecode *compile(Scope *scope,
                         Tree *expr,
                         Tree *pattern,
                         Parameters &parameters)
// ----------------------------------------------------------------------------
//   Compile a definition with parameters and create its bytecode
// ----------------------------------------------------------------------------
{
    if (Closure *closure = expr->As<Closure>())
    {
        expr = closure->Value();
        scope = closure->CapturedScope();
    }
    Bytecode *bytecode = (Bytecode *) pattern->code;
    if (!bytecode)
    {
        // We need to create the bytecode for this expression
        bytecode = new Bytecode(scope, expr, parameters);
        pattern->code = bytecode;

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
                OP(get_scope, StorageIndex(expr));
        }

        if (hasInstructions)
            compile(scope, expr, bytecode);
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
    Tree *input = expr;
    Tree *last = nullptr;
    bool definition = false;
    while ((expr = next()))
    {
        // If not the first statement, check result of previous statement
        if (last)
        {
            if (!definition)
                OP(check_statement, ValueIndex(last));
        }

        // Lookup expression
        size_t patches = bytecode->SuccessesToPatch();
        Context::LookupMode mode = lookupMode(bytecode);
        BytecodeBindings bindings(expr, bytecode);
        context.Lookup(expr, lookupCandidate, &bindings, mode);

        // If we did not find a matching form, check standard evaluation
        int done = bindings.Successes() != 0 ? -1 : 0;
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
        if (done)
        {
            if (done < 0 && !bindings.PerfectMatch())
                OP(form_error, StorageIndex(expr));
            bytecode->PatchSuccesses(patches);
        }
        else
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
        last = expr;
    }
    if (last)
        bytecode->Unify(last, input, false);
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
    Tree_p result = bytecode->Run(state);

    // Result is on top of stack
    GarbageCollector::SafePoint();
    return result;
}



// ============================================================================
//
//   Type checking
//
// ============================================================================

static Tree *typecheck(Scope_p scope, Tree_p type, Tree_p tree)
// ----------------------------------------------------------------------------
//   Implementation of type checking in bytecode
// ----------------------------------------------------------------------------
{
    Tree *value = tree;

    // Process builtin types
#define NATURAL_TYPE(Name, Rep)                                 \
    if (type == Name##_type)                                    \
    {                                                           \
        if (Natural *number = value->AsNatural())               \
        {                                                       \
            if (Natural *natural = number->MakeUnsigned())      \
            {                                                   \
                Rep test = natural->value;                      \
                if (test == natural->value)                     \
                    return natural;                             \
            }                                                   \
        }                                                       \
        return nullptr;                                         \
    }
#define INTEGER_TYPE(Name, Rep)                                 \
    if (type == Name##_type)                                    \
    {                                                           \
        if (Natural *number = value->AsNatural())               \
        {                                                       \
            if (Natural *natural = number->MakeSigned())        \
            {                                                   \
                Rep test = natural->value;                      \
                if (test == (longlong) natural->value)          \
                    return natural;                             \
            }                                                   \
        }                                                       \
        return nullptr;                                         \
    }
#define REAL_TYPE(Name, Rep)                                    \
    if (type == Name##_type)                                    \
    {                                                           \
        if (Real *number = value->AsReal())                     \
        {                                                       \
            Rep test = number->value;                           \
            if (test == number->value)                          \
                return number;                                  \
        }                                                       \
        return nullptr;                                         \
    }
#define TREE_TYPE(Name, Rep, Cast)              \
    if (type == Name##_type)                    \
        return (Cast);
#include "machine-types.tbl"


    // Otherwise go through normal evaluation
    Infix_p test = new Infix("as", value, type, type->Position());
    return evaluate(scope, test);
}



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
    if (!xl_self)
    {
#define NAME(N)                 xl_##N = new Name(#N);
#define TYPE(N, Body)           N##_type = new Name(#N);
#define UNARY(Name, ReturnType, LeftType, Body)
#define BINARY(Name, ReturnType, LeftType, RightType, Body)

#include "builtins.tbl"
    }
}



// ============================================================================
//
//    Initialize top-level context
//
// ============================================================================

kstring Bytecode::name[OPCODE_COUNT] =
// ----------------------------------------------------------------------------
//   Generate the bytecode name array
// ----------------------------------------------------------------------------
{
#define OPCODE(Name, Parms, Body)       #Name,
#include "bytecode.tbl"
};


#define NORM2(x) Normalize(x), x

void BytecodeEvaluator::InitializeContext(Context &context)
// ----------------------------------------------------------------------------
//    Initialize all the names, e.g. xl_self, and all builtins
// ----------------------------------------------------------------------------
//    Load the outer context with the type check operators referring to
//    the builtins.
{
    // Insert opcodes
#define BUILTIN(Name, Parms, Body)      builtins[#Name] = opcode_##Name;
#define OPCODE(Name, Parms, Body)
#include "bytecode.tbl"

    // Insert machine types in the context
#define RUNTIME_TYPE(Type, Rep, BC)                                     \
    Infix *Type##_pattern =                                             \
        new Infix("as",                                                 \
                  new Prefix(xl_lambda,                                 \
                             new Name(NORM2("Value"))),                 \
                  Type##_type);                                         \
    Prefix *Type##_value =                                              \
        new Prefix(xl_builtin,                                          \
                   new Text(#Type"_cast"));                             \
    context.Define(Type##_type, xl_self);                               \
    context.Define(Type##_pattern, Type##_value);
#include "machine-types.tbl"

    // Record native functions
    Name_p native_name = new Name(NORM2("native"));
    for (Native *native = Native::First(); native; native = native->Next())
    {
        record(native, "Found %t", native->Shape());
        kstring symbol = native->Symbol();
        size_t index = natives.size();
        natives.push_back(native->opcode_function);
        natives_index[symbol] = index;
        Tree *shape = native->Shape();
        Prefix *body = new Prefix(native_name, new Text(symbol));
        context.Define(shape, body);
    }

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
