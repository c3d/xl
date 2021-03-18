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

enum Opcode
// ----------------------------------------------------------------------------
//    Definition of the opcodes
// ----------------------------------------------------------------------------
{
#define OPCODE(Name, Parms, Body)      opcode_##Name,
#include "bytecode.tbl"
    OPCODE_COUNT,
    UNPATCHED = (uint16_t) -1
};


typedef uint16_t                        opcode_t; // 16-bit ought to be enough
typedef std::map<Tree_p, Tree_p>        TypeMap;
typedef std::map<Tree_p, opcode_t>      ValueMap;


struct Bytecode : Info
// ----------------------------------------------------------------------------
//   A sequence of instructions associated to a tree
// ----------------------------------------------------------------------------
{
    Bytecode(Scope *scope, Tree *self, NameList &parameters):
        code(), scope(scope), self(self),
        constants(), rewrites(), parameters(parameters), locals(0),
        compile(new CompileInfo)
    {}
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
    struct Local
    {
        Local(opcode_t index): index(index) {}
        opcode_t index;
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
    void        PatchChecks(size_t where);
    void        PatchSuccesses(size_t where);
    size_t      ChecksToPatch();
    size_t      SuccessesToPatch();

    // Runtime support functions
    void        ErrorExit(Tree *msg);   // Emit an error with msg and exit
    void        Clear();
    void        RemoveLastDefinition();
    size_t      Size();
    Scope *     EvaluationScope();
    Tree *      Self();
    bool        IsValid();
    void        Validate();
    Tree_p      Constant(opcode_t index);
    template<typename Rep>
    Rep         Constant(opaddr_t &pc);
    Rewrite_p   Definition(opcode_t index);
    bool        LocalMode();
    bool        LocalMode(bool mode);
    bool        VariableMode();
    bool        VariableMode(bool mode);
    void        Bind(Name *name, Scope *scope, Tree *value);
    opcode_t    Parameter(Name *name);
    NameList &  Parameters();
    ArgList &   Arguments();
    opcode_t    Evaluate(Scope *scope, Tree *value);
    Tree *      Type(Tree *value);
    void        Type(Tree *value, Tree *type);
    void        Dump(std::ostream &out);
    void        Dump(std::ostream &out, opaddr_t &pc);
    Tree *      Cached(opcode_t local);
    static bool IsTree(MachineType mtype);
    static bool IsNatural(MachineType mtype);
    static bool IsInteger(MachineType mtype);
    static bool IsReal(MachineType mtype);
    static bool IsScalar(MachineType mtype);


private:
    friend struct RunState;
    void        Cut(opaddr_t cutpoint);

    // Enter the various kind of arguments (in a somewhat typesafe way)
    void        EnterArg(Tree *cst);
    void        EnterArg(Name *cst)             { EnterArg((Tree *) cst); }
    void        EnterArg(Infix *cst)            { EnterArg((Tree *) cst); }
    void        EnterArg(Rewrite *rewrite);
    void        EnterArg(const CheckJump &);
    void        EnterArg(const SuccessJump &);
    void        EnterArg(const Local &local);
    void        EnterArg(const ArgList &args);
    template<typename T>
    void        EnterArg(const GCPtr<T> &p)     { EnterArg(p.Pointer()); }
    template<typename Rep>
    void        EnterArg(const RepConstant<Rep> &rep);
    template<typename First, typename ...Rest>
    void        EnterArg(First first, Rest... rest);

private:
    // Data for the bytecode
    Opcodes     code;           // Bytecode itself
    Scope_p     scope;          // Declaration scope for the code
    Tree *      self;           // Not a Tree_p to avoid GC cycles
    TreeList    constants;      // Constants generated by EnterConstant
    RewriteList rewrites;       // Definitions in order of binding
    NameList    parameters;     // Parameters in effect in current scope
    ArgList     arguments;      // Index of corresponding arguments
    size_t      locals;         // Number of locals

    struct CompileInfo
    {
        CompileInfo(): local(false), variable(false) {}
        Patches     checks;     // Checks to patch
        Patches     successes;  // Checks to patch
        Patches     jumps;      // Jumps to patch if we shorten
        TypeMap     types;      // Last known type for a value
        ValueMap    values;     // Already-computed value
        bool        local;      // Non-recursive lookup
        bool        variable;   // Evaluate as variable
    };
    CompileInfo *compile;       // Information used at compile-time

    enum OpcodeKind
    {
        JUMP,                   // New value for PC
        LOCAL,                  // Index of a local variable
        CONSTANT,               // Index of a constant
        DEFINITION,             // Index of a definition
        ARGUMENTS,              // Argument list
#define TREE_TYPE(Name, Rep, Cast)
#define RUNTIME_TYPE(Name, Rep, BC)             \
        CONSTANT_##Name,
#include "machine-types.tbl"
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
              compile(bytecode->compile),
              cutpoint(bytecode->Size()),
              frame(bytecode->parameters.size()),
              checks(bytecode->ChecksToPatch()),
              values(compile->values),
              types(compile->types)
        {
            compile->types.clear();
        }
        ~Attempt()
        {
            bytecode->PatchChecks(checks);
            NameList &parms = bytecode->parameters;
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
};



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
    opcode_t    *code   = &bc->code[0];
    opaddr_t     max    = bc->code.size();
    opaddr_t     pc     = 0;
    size_t       locals = 0;
    MachineType  xtype  = tree_mtype;
    MachineType  ytype  = tree_mtype;
    RunValue     x, y;
    RunValue    *frame  = nullptr;
    enum XYSelector { NONE, X, Y, XY } xy = NONE;

#define RUNLOOP_TYPE(Name, Rep)        Rep x_##Name = Rep(), y_##Name = Rep();
#include "machine-types.tbl"

start:
    // Put the labels in a static const table with deltas to ease relocation
    static const ptrdiff_t entry[] =
    {
#define OPCODE(Name, Parms, Body) ((char *) &&label_##Name-(char *) &&start),
#include "bytecode.tbl"
    };


// Macros to help writing opcodes
#define DATA            (code[pc++])
#define REFRAME         (frame = &state.stack[locals])
#define RETARGET        (max = bc->code.size(), pc = 0, code = &bc->code[0])
#define NEXT            XL_ASSERT(pc < max);                            \
                        goto *((char *) &&start + entry[code[pc++]])
#define BRANCH(b)       (pc += b)
#define CHAIN(op)       goto label_##op
#define TYPE(t)         xtype = t##_mtype
#define TYPES(x,y)      (xtype = x##_mtype, ytype = y##_mtype)
#define WANT(t)         XL_ASSERT(xtype == t##_mtype)
#define WANTXY(x,y)     XL_ASSERT(xtype == x##_mtype && ytype == y##_mtype)
#define WANT_XTREE      if (!IsTree(xtype)) { xy =  X; goto want_xtree; }
#define WANT_YTREE      if (!IsTree(ytype)) { xy =  Y; goto want_trees; }
#define WANT_TREES      if (!IsTree(xtype) ||                           \
                            !IsTree(ytype)) { xy = XY; goto want_trees; }
#define WANT_XRUNVALUE  if (!x.type) { xy =  X; goto want_xrunvalue; }
#define WANT_YRUNVALUE  if (!y.type) { xy =  Y; goto want_runvalues; }
#define WANT_RUNVALUES  if (!x.type ||                                  \
                            !y.type) { xy = XY; goto want_runvalues; }
#define READ_XRUNVALUE  xy = X;  goto read_xrunvalue;
#define READ_YRUNVALUE  xy = Y;  goto read_runvalues;
#define READ_RUNVALUES  xy = XY; goto read_runvalues;
#define CLASSIFY        xy =  X; goto classify;
#define XSELECT         ((int) xy & X)
#define YSELECT         ((int) xy & Y)

    // Jump to first opcode
    REFRAME;
    NEXT;


want_runvalues:
// ----------------------------------------------------------------------------
//  Set X and Y RunValue from scalar values
// ----------------------------------------------------------------------------
//  When this is done, the RunValues x or y are valid.
//  While the values in registers like x_natural are not lost, they are stale

    if (YSELECT)
    {
        switch(ytype)
        {
#define RUNTIME_TYPE(Name, Rep, BC)             \
        case Name##_mtype:                      \
            y = RunValue(y_##BC);               \
            break;
#include "machine-types.tbl"
        default:
            y = RunValue(y_tree, ytype);
            break;
        }
    }

want_xrunvalue:
    // Decrement the program counter to retry at end
    pc--;

    if (XSELECT)
    {
        switch(xtype)
        {
#define RUNTIME_TYPE(Name, Rep, BC)             \
        case Name##_mtype:                      \
            x = RunValue(x_##BC);               \
            break;
#include "machine-types.tbl"
        default:
            x = RunValue(x_tree, xtype);
            break;
        }
    }

    // Retry the opcode that made the request
    NEXT;


read_runvalues:
// ----------------------------------------------------------------------------
//  Set X and Y scalar values, e.g. x_natural from X and Y RunValues
// ----------------------------------------------------------------------------
//  When this is done, the corresponding runvalue has `runvalue_unset`
//  and only one of the scalar variables is set, e.g. x_natural.

    if (YSELECT)
    {
        // Unbox the current xvalue
        switch(y.type)
        {
#define RUNTIME_TYPE(Name, Rep, BC)                     \
        case Name##_mtype:                              \
            y_##BC = RunValue::Representation<Rep>      \
                ::ToValue(y.as_##Name);                 \
            break;
#include "machine-types.tbl"
        default:
            y_tree = y.as_tree;
            break;
        }
        ytype = y.type;
        y.type = runvalue_unset;
    }

read_xrunvalue:
    if (XSELECT)
    {
        switch(x.type)
        {
#define RUNTIME_TYPE(Name, Rep, BC)                     \
        case Name##_mtype:                              \
            x_##BC = RunValue::Representation<Rep>      \
                ::ToValue(x.as_##Name);                 \
            break;
#include "machine-types.tbl"
        default:
            x_tree = x.as_tree;
            break;
        }
        xtype = x.type;
        x.type = runvalue_unset;
    }
    NEXT;


want_trees:
// ----------------------------------------------------------------------------
//  Convert X and/or Y into tree form, i.e. x_tree or y_tree
// ----------------------------------------------------------------------------
//  On input, the values must already be in scalar form
//  After evaluating this, numerical values have been converted to a tree
//  This is necessary to call functions that expect a parse tree.

    if (YSELECT)
    {
        switch(ytype)
        {
#define RUNTIME_TYPE(Name, Rep, BC)                     \
        case Name##_mtype:                              \
            y_tree = RunValue::AsTree(y_##BC);          \
            break;
#include "machine-types.tbl"
        default:
            break;
        }
        ytype = tree_mtype;
    }

want_xtree:
    // Decrement the program counter to retry after generating tree
    pc--;

    if (XSELECT)
    {

        switch(xtype)
        {
#define RUNTIME_TYPE(Name, Rep, BC)                     \
        case Name##_mtype:                              \
            x_tree = RunValue::AsTree(x_##BC);          \
            break;
#include "machine-types.tbl"
        default:
            break;
        }
        xtype = tree_mtype;
    }

    NEXT;


classify:
// ----------------------------------------------------------------------------
//  Classify a tree into possible scalars
// ----------------------------------------------------------------------------
//  On input, the value must be in x_tree. This computes x_type and
//  may generate a scalar.

    if (!x_tree)
    {
        xtype = nil_mtype;
    }
    else
    {
        switch(x_tree->Kind())
        {
        case NATURAL:
        {
            Natural *xn = (Natural *) (Tree *) x_tree;
            xtype = xn->IsSigned() ? integer_mtype : natural_mtype;
            if (xtype == integer_mtype)
                x_integer = xn->value;
            else
                x_natural = xn->value;
            break;
        }
        case REAL:
        {
            Real *xr = (Real *) (Tree *) x_tree;
            xtype = real_mtype;
            x_real = xr->value;
            break;
        }
        case TEXT:
        {
            Text *xt = (Text *) (Tree *) x_tree;
            xtype = xt->IsCharacter() ? character_mtype : text_mtype;
            if (xtype == character_mtype)
                x_character = xt->value[0];
            else
                x_text = xt->value;
            break;
        }
        case NAME:
        {
            Name *xn = (Name *) (Tree *) x_tree;
            xtype = (xn->IsName()       ? name_mtype
                     : xn->IsOperator() ? operator_mtype
                     : symbol_mtype);
            break;
        }
        case INFIX:
            xtype = infix_mtype;
            break;
        case PREFIX:
            xtype = IsError(x_tree) ? error_mtype : prefix_mtype;
            break;
        case POSTFIX:
            xtype = postfix_mtype;
            break;
        case BLOCK:
            xtype = block_mtype;
            break;
        }
    }
    NEXT;


divide_by_zero:
// ----------------------------------------------------------------------------
//    We get here if an arithmetic divide by zero occurs
// ----------------------------------------------------------------------------
    {
        Error error("Divide by zero");
        state.Error(error);
        TYPE(error);
    }
    CHAIN(ret);


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
#undef REFRAME
#undef RETARGET
#undef NEXT
#undef BRANCH
#undef CHAIN
#undef TYPE
#undef TYPES
#undef WANT
#undef WANTXY
#undef WANT_XTREE
#undef WANT_YTREE
#undef WANT_TREES

#undef WANT_XRUNVALUE
#undef WANT_XRUNVALUE
#undef WANT_RUNVALUES

#undef READ_XRUNVALUE
#undef READ_YRUNVALUE
#undef READ_RUNVALUES
#undef XCLASSIFY
#undef YCLASSIFY
#undef CLASSIFYS
#undef XSELECT
#undef YSELECT
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
typedef Bytecode::Local Local;


template <typename Rep>
static Bytecode::RepConstant<Rep> Constant(Rep rep)
// ----------------------------------------------------------------------------
//   Create a constant for a representation type
// ----------------------------------------------------------------------------
{
    return Bytecode::RepConstant<Rep>(rep);
}


template <>
Bytecode::RepConstant<text> Constant(text rep)
// ----------------------------------------------------------------------------
//   Create a constant for a representation type
// ----------------------------------------------------------------------------
{
    return Bytecode::RepConstant<text>(rep.c_str());
}


void Bytecode::Op(Opcode opcode)
// ----------------------------------------------------------------------------
//   Record an opcode in the bytecode
// ----------------------------------------------------------------------------
{
    record(opcode, "Opcode %+s", name[opcode]);
    code.push_back(opcode);
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
    compile->checks.push_back(code.size());
    code.push_back(UNPATCHED);
}


void Bytecode::EnterArg(const Bytecode::SuccessJump &)
// ----------------------------------------------------------------------------
//   Record that we have a successful value
// ----------------------------------------------------------------------------
{
    compile->successes.push_back(code.size());
    code.push_back(UNPATCHED);
}


void Bytecode::EnterArg(const Bytecode::Local &local)
// ----------------------------------------------------------------------------
//   Record a constant indicating a bytecode local
// ----------------------------------------------------------------------------
{
    record(opcode, "Local %u", local.index);
    code.push_back(local.index);
}


void Bytecode::EnterArg(const ArgList &args)
// ----------------------------------------------------------------------------
//   Record a parameter list by evaluating all the arguments
// ----------------------------------------------------------------------------
{
    record(opcode, "Args %u", args.size());
    code.push_back(args.size());
    for (auto a : args)
        code.push_back(a);
}


void Bytecode::EnterArg(Tree *cst)
// ----------------------------------------------------------------------------
//   Record an opcode with a constant in the bytecode
// ----------------------------------------------------------------------------
{
    size_t size = constants.size();

    opcode_t index;
    for (index = 0; index < size; index++)
        if (cst == constants[index])
            break;
    if (index >= size)
    {
        constants.push_back(cst);
        XL_ASSERT(size < std::numeric_limits<opcode_t>::max());
    }
    record(opcode, "Constant %t = %u", cst, index);
    code.push_back(index);
}


void Bytecode::EnterArg(Rewrite *rw)
// ----------------------------------------------------------------------------
//   Record an opcode with a definition (rewrite) in the bytecode
// ----------------------------------------------------------------------------
{
    size_t size = rewrites.size();

    opcode_t index;
    for (index = 0; index < size; index++)
        if (rw == rewrites[index])
            break;
    if (index >= size)
    {
        rewrites.push_back(rw);
        XL_ASSERT(size < std::numeric_limits<opcode_t>::max());
    }
    record(opcode, "Rewrite %t = %u", rw, index);
    code.push_back(index);
}


template <typename Rep>
void Bytecode::EnterArg(const RepConstant<Rep> &rep)
// ----------------------------------------------------------------------------
//   Enter a constant for a given machine typeo
// ----------------------------------------------------------------------------
{
    typedef typename RepConstant<Rep>::literal_t literal_t;
    const size_t SIZE = sizeof(literal_t) / sizeof(opcode_t);
    union
    {
        literal_t       value;
        opcode_t        ops[SIZE];
    } u;
    u.value = rep.value;
    code.insert(code.end(), u.ops, u.ops + SIZE);
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


void Bytecode::PatchSuccesses(size_t where)
// ----------------------------------------------------------------------------
//   Patch the pending checks to the current position
// ----------------------------------------------------------------------------
{
    opaddr_t target = code.size();
    Patches &successes = compile->successes;
    Patches &jumps = compile->jumps;

    while (successes.size() > where)
    {
        opaddr_t success = successes.back();
        XL_ASSERT(code[success] == UNPATCHED);
        code[success] = target - (success + 1);
        successes.pop_back();
        jumps.push_back(success);
    }

    if (jumps.size() == 1 && jumps.back() == target - 1 &&
        code.size() >= 2 && code[code.size() - 2] == opcode_branch)
    {
        code.erase(code.end() - 2, code.end());
        jumps.pop_back();
    }
}


size_t Bytecode::SuccessesToPatch()
// ----------------------------------------------------------------------------
//   Return where we are in the successes list
// ----------------------------------------------------------------------------
{
    return compile->successes.size();
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
        code.push_back(opcode_ret);
    locals = compile->values.size();
    delete compile;
    compile = nullptr;
}


Tree_p Bytecode::Constant(opcode_t index)
// ----------------------------------------------------------------------------
//   Return a given constant
// ----------------------------------------------------------------------------
{
    XL_ASSERT(index < constants.size());
    return constants[index];
}


template<typename Rep>
inline Rep Bytecode::Constant(opaddr_t &pc)
// ----------------------------------------------------------------------------
//   Return a representation constant
// ----------------------------------------------------------------------------
{
    using literal_t = typename RepConstant<Rep>::literal_t;
    const size_t SIZE = sizeof(literal_t) / sizeof(opcode_t);
    union
    {
        literal_t value;
        opcode_t  ops[SIZE];
    } u;
    std::copy(code.begin() + pc, code.begin() + pc + SIZE, u.ops);
    pc += SIZE;
    return u.value;
}


Rewrite_p Bytecode::Definition(opcode_t index)
// ----------------------------------------------------------------------------
//   Return a given definition (rewrite)
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


void Bytecode::Bind(Name *name, Scope *scope, Tree *value)
// ----------------------------------------------------------------------------
//   Bind a parameter in the current bytecode
// ----------------------------------------------------------------------------
{
    opcode_t index = Evaluate(scope, value);
    parameters.push_back(name);
    arguments.push_back(index);
}


opcode_t Bytecode::Parameter(Name *name)
// ----------------------------------------------------------------------------
//    Find the index of a parameter in the current bytecode
// ----------------------------------------------------------------------------
{
    opaddr_t max = parameters.size();
    opcode_t index = max;
    while (index --> 0)
        if (parameters[index] == name)
            return index;
    return UNPATCHED;                   // Not a valid parameter index
}


inline NameList &Bytecode::Parameters()
// ----------------------------------------------------------------------------
//   Return current parameter list for the bytecode
// ----------------------------------------------------------------------------
{
    return parameters;
}


inline Bytecode::ArgList &Bytecode::Arguments()
// ----------------------------------------------------------------------------
//   Return current parameter list for the bytecode
// ----------------------------------------------------------------------------
{
    return arguments;
}


opcode_t Bytecode::Evaluate(Scope *scope, Tree *value)
// ----------------------------------------------------------------------------
//   Evaluate a value but only once
// ----------------------------------------------------------------------------
{
    Bytecode *bytecode = this;
    ValueMap &values = compile->values;
    opcode_t index;
    auto found = values.find(value);
    if (found != values.end())
    {
        // Already compiled and evaluated - Reload it
        Bytecode *bytecode = this;
        index = (*found).second;
        OP(load, Local(index));
    }
    else
    {
        // Compile the value
        XL::compile(scope, value, bytecode);
        index = values.size();
        values[value] = index;
        OP(store, Local(index));
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


bool Bytecode::IsTree(MachineType mtype)
// ----------------------------------------------------------------------------
//   Return true if this is a tree type
// ----------------------------------------------------------------------------
{
    switch(mtype)
    {
#define TREE_TYPE(Name, Rep, Code)      case  Name##_mtype:
#include "machine-types.tbl"
        return true;
    default:
        return false;
    }
}


bool Bytecode::IsNatural(MachineType mtype)
// ----------------------------------------------------------------------------
//   Return true if this is a natural machine type
// ----------------------------------------------------------------------------
{
    switch(mtype)
    {
#define NATURAL_TYPE(Name, Rep)         case  Name##_mtype:
#include "machine-types.tbl"
        return true;
    default:
        return false;
    }
}


bool Bytecode::IsInteger(MachineType mtype)
// ----------------------------------------------------------------------------
//   Return true if this is a integer machine type
// ----------------------------------------------------------------------------
{
    switch(mtype)
    {
#define INTEGER_TYPE(Name, Rep)         case  Name##_mtype:
#include "machine-types.tbl"
        return true;
    default:
        return false;
    }
}


bool Bytecode::IsReal(MachineType mtype)
// ----------------------------------------------------------------------------
//   Return true if this is a real machine type
// ----------------------------------------------------------------------------
{
    switch(mtype)
    {
#define REAL_TYPE(Name, Rep)         case  Name##_mtype:
#include "machine-types.tbl"
        return true;
    default:
        return false;
    }
}


bool Bytecode::IsScalar(MachineType mtype)
// ----------------------------------------------------------------------------
//   Return true if this is a scalar type (non-tree)
// ----------------------------------------------------------------------------
{
    return !IsTree(mtype);
}



// ============================================================================
//
//    Debug utilities
//
// ============================================================================

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
                out << it.second << ":\t" << it.first << "\n";
        }
        if (compile->types.size())
        {
            out << "Types:\n";
            for (auto &it : compile->types)
                out << it.first << "\tas\t" << it.second << "\n";
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


template <typename ...Args>
inline std::vector<Bytecode::OpcodeKind> Bytecode::OpcodeArgList(Args... args)
// ----------------------------------------------------------------------------
//   Build a list from the parameters given to OPCODE macro
// ----------------------------------------------------------------------------
{
    return std::vector<OpcodeKind>{ args... };
}


void Bytecode::Dump(std::ostream &out, opaddr_t &pcr)
// ----------------------------------------------------------------------------
//   Dump a single bytecode
// ----------------------------------------------------------------------------
{
    std::vector<OpcodeKind> args;
    opaddr_t pc = pcr;

    switch(code[pc++])
    {
#define OPCODE(Name, Parms, Body)                                       \
    case opcode_##Name:                                                 \
    {                                                                   \
        std::cout << std::right << std::setw(6) << pc-1 << ": "         \
                  << std::left  << std::setw(20) << #Name;              \
        args = OpcodeArgList Parms;                                     \
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
                out << "jump (not patched yet)";
            else
                out << "jump " << value << " to " << pc+value;
            break;
        case LOCAL:
            if (value < parameters.size())
                out << "arg " << value << "\t= " << parameters[value];
            else if (compile)
                out << "local " << value << "\t= " << Cached(value);
            else
                out << "local " << value;
            break;
        case CONSTANT:
            out << "constant " << value;
            if (value < constants.size())
                out << "=" << constants[value];
            break;
        case DEFINITION:
            out << "rewrite " << value;
            if (value < rewrites.size())
                out << "=" << rewrites[value];
            break;
        case ARGUMENTS:
            out << "with " << value << " arguments";
            for (size_t a = 0; a < value; a++)
            {
                opcode_t index = code[pc++];
                out << "\n  "
                    << std::right << std::setw(6) << a
                    << ":\tlocal " << index;
                if (index < parameters.size())
                    out << "\t= " << parameters[index];
                else if (compile)
                    out << " = " << Cached(index);
            }
            break;
#define TREE_TYPE(Name, Rep, Cast)
#define RUNTIME_TYPE(Name, Rep, BC)                                     \
        case CONSTANT_##Name:                                           \
            pc--;                                                       \
            out << "constant_" #Name "\t"                               \
                << *((RepConstant<Rep>::literal_t *) &code[pc]);        \
            pc += sizeof(Rep) / sizeof(opcode_t);                       \
            break;
#include "machine-types.tbl"
        default:
            out << "Unhandled parameter kind " << a;
            break;
        }
        sep = ", ";
    }

    out << "\n";
    pcr = pc;
}


Tree *Bytecode::Cached(opcode_t local)
// ----------------------------------------------------------------------------
//   Return the cached expression corresponding to an index
// ----------------------------------------------------------------------------
{
    for (const auto &it : compile->values)
        if (local == it.second)
            return it.first;
    return nullptr;
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
    OP(check_natural, Constant(what->value), CHECK);
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
    OP(check_real, Constant(what->value), CHECK);
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
    OP(check_text, Constant(what->value), CHECK);
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
        opcode_t index = bytecode->Parameter(what);
        XL_ASSERT(index != UNPATCHED);
        OP(loady, Local(index));
        OP(check_same, CHECK);
        return Possible();
    }

    // Otherwise, bind test value to name
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
        OP(constant, meta);
        OP(check_same, CHECK);
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
            OP(check_borrow);
            return Possible();
        }
        else if (Infix *typecast = binding->AsInfix())
        {
            Tree_p type = EvaluateType(typecast->right);
            bytecode->Type(test, type);
            Bind(name, test);
            if (type != bytecode->Type(test))
                OP(check_typed_borrow, type, CHECK);
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
    if ((Bytecode::IsInteger(want) ||
         Bytecode::IsNatural(want) ||
         Bytecode::IsReal(want)) &&
        (Bytecode::IsNatural(have) ||
         Bytecode::IsInteger(have)))
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

        // Need to match the left part with the converted value
        if (!outermost && !defined)
            defined = what;
        Tree_p value = test;
        what->left->Do(this);

        // Check hard incompatible types, e.g. integer vs text
        Tree_p have = bytecode->Type(test);
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
                bytecode->Op(cast, CHECK);
                return Possible();
            }
            return Perfect();
        }

        // Type check value against type
        if (outermost)
            type = want;
        else if (want != have)
            OP(check_input_type, want, CHECK);
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
        OP(check_guard, CHECK);

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
        OP(check_infix, what, CHECK);
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
    bytecode->Bind(name, evaluation, value);
    Context context(arguments);
    Context eval(evaluation);
    context.Define(name, eval.Enclose(value));
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
                opcode_t index = bytecode->Parameter(name);
                if (index != UNPATCHED)
                {
                    attempt.Fail();     // Cut any generated code
                    OP(load, Local(index));
                    done = true;
                }
                else if (IsSelf(decl->Definition()))
                {
                    attempt.Fail();     // Cut any generated code
                    OP(constant, name);
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
                OP(call_constant, pattern, bytecode->Arguments());
            }

            // Check the result type if we had one
            if (Tree *type = bindings.ResultType())
            {
                if (bytecode->Type(expr) != type)
                    OP(check_result_type, type, CHECK);
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
            OP(check_init_type, type, CHECK);
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
        if (n->IsSigned())
            OP(constant_integer, Constant((longlong) n->value));
        else
            OP(constant_natural, Constant(n->value));
        bytecode->Type(tree, natural_type);
        break;
    }
    case REAL:
    {
        OP(constant_real, Constant(((Real *) tree)->value));
        bytecode->Type(tree, real_type);
        break;
    }
    case TEXT:
    {
        OP(constant_text, Constant(((Natural *) tree)->value));
        bytecode->Type(tree, text_type);
        break;
    }
    default:
        OP(constant, tree);
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
        OP(nil);
        return true;
    }
    if (name->value == "true")
    {
        OP(true);
        return true;
    }
    if (name->value == "false")
    {
        OP(false);
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


const int IS_DEFINITION = 2;

// Array for builtins and native functions
static std::map<text, Opcode> builtins;
static std::map<text, Native::opcode_fn> natives;


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
        OP(make_matching, pattern);
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
        OP(constant, quote);
        return true;
    }

    // Filter out import statements (processed also during parsing)
    if (Name *import = prefix->left->AsName())
    {
        if (eval_fn callback = MAIN->Importer(import->value))
        {
            // Process the import statement
            callback(scope, prefix);
            OP(constant, import);
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
        Opcode opcode = builtins[name->value];
        if (!opcode)
        {
            record(opcode_error, "Nonexistent builtin %t", name);
            return false;
        }
        bytecode->Op(opcode);
        return true;
    }

    // Check if this is a native function
    if (Text *name = IsNative(prefix))
    {
        Native::opcode_fn function = natives[name->value];
        if (!function)
        {
            record(opcode_error, "Nonexistent native function %t", name);
            return false;
        }
        OP(native, Constant((Natural::value_t) function));
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
            OP(init_value, rewrite);
        }
        // Return the definition
        OP(rewrite, rewrite);
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
            OP(check_typecast, want, CHECK);
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
        natives[symbol] = native->opcode_function;
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
