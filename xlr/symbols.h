#ifndef SYMBOLS_H
#define SYMBOLS_H
// ****************************************************************************
//  symbols.h                                                       XLR project
// ****************************************************************************
// 
//   File Description:
// 
//     The "older" compiler technology
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
/*
   This compilation technology is less efficient than the new one, but
   we need it in order to be able to release a compatible version of Tao.
   So we import it wholesale. It will be discarded once the new compiler
   takes over entirely.
*/

#include "context.h"
#include "action.h"
#include "compiler.h"

XL_BEGIN

// ============================================================================
// 
//    Forward type declarations
// 
// ============================================================================

struct Rewrite;                                 // Tree rewrite data
struct Runtime;                                 // Runtime context
struct Errors;                                  // Error handlers
struct Compiler;                                // JIT compiler
struct OCompiledUnit;                           // Compilation unit (old style)

typedef GCPtr<Rewrite>             Rewrite_p;
  
typedef std::map<text, Tree_p>     symbol_table; // Symbol table in context
typedef std::set<Tree_p>           active_set;   // Not to be garbage collected
typedef std::set<Symbols_p>        symbols_set;  // Set of symbol tables
typedef std::vector<Symbols_p>     symbols_list; // List of symbols table
typedef symbol_table::iterator     symbol_iter;  // Iterator over sym table
typedef std::map<Name_p, Tree_p>   capture_table;// Symbol capture table
typedef std::map<Tree_p, Tree_p>   value_table;  // Used for value caching
typedef value_table::iterator      value_iter;   // Used to iterate over values
typedef Tree * (*typecheck_fn) (Context *context, Tree *src, Tree *value);
typedef Tree * (*decl_fn) (Symbols *, Tree *source, bool execute);
typedef std::map<text, decl_fn>    declarator_table; // To call at decl time



// ============================================================================
// 
//    Compile-time symbols and rewrites management
// 
// ============================================================================

struct Symbols
// ----------------------------------------------------------------------------
//   Holds the symbols in a given context
// ----------------------------------------------------------------------------
{
    Symbols(Symbols *s);
    ~Symbols();

    // Symbols properties
    Symbols *           Parent()                { return parent; }
    ulong               Depth()                 { return depth; }
    ulong               Deepen()                { return ++depth; }
    void                Import (Symbols *other) { imported.insert(other); }

    // Symbol management
    Tree *              Named (text name, bool deep = true);
    Rewrite *           Rewrites()              { return rewrites; }

    // Entering symbols in the symbol table
    void                EnterName (text name, Tree *value, Rewrite::Kind k);
    void                ExtendName (text name, Tree *value);
    Rewrite *           EnterRewrite(Rewrite *r);
    Rewrite *           EnterRewrite(Tree *from, Tree *to);
    Name *              Allocate(Name *varName);
    ulong               Count(ulong kinds /* bit mask of Rewrite::Kind */,
                              Rewrite *top = NULL);

    // Enter properties, return number of properties found
    uint                EnterProperty(Context *context,
                                      Tree *self, Tree *storage, Tree *decls);
    Rewrite *           Entry(text name, bool create);
    Rewrite *           Entry(Tree *form, bool create);
    Rewrite *           LookupEntry(text name, bool create);
    Rewrite *           LookupEntry(Tree *form, bool create);

    // Performing the declarations on a given tree
    Tree *              ProcessDeclarations(Tree *tree);

    // Clearing symbol tables
    void                Clear();

    // Compiling and evaluating a tree in scope defined by these symbols
    Tree *              Compile(Tree *s, OCompiledUnit &,
                                bool nullIfBad = false,
                                bool keepOtherConstants = false,
                                bool noDataForms = false);
    Tree *              CompileAll(Tree *s,
                                   bool nullIfBad = false,
                                   bool keepOtherConstants = false,
                                   bool noDataForms = false);
    Tree *              CompileCall(text callee, TreeList &args,
                                    bool nullIfBad=false, bool cached = true);
    Infix *             CompileTypeTest(Tree *type);
    Tree *              Run(Context *, Tree *t);

    // Error handling
    Tree *              Ooops (text message,
                               Tree *a1=NULL, Tree *a2=NULL, Tree *a3=NULL);

public:
    Tree_p              source;
    Symbols_p           parent;
    Rewrite_p           rewrites;
    symbol_table        calls;
    value_table         type_tests;
    symbols_set         imported;
    Tree_p              error_handler;
    ulong               depth;
    bool                has_rewrites_for_constants;
    bool                is_global;

    static declarator_table declarators;

    GARBAGE_COLLECT(Symbols);
};



// ============================================================================
// 
//   Inline functions
// 
// ============================================================================

inline Symbols::Symbols(Symbols *s)
// ----------------------------------------------------------------------------
//   Create a "child" symbol table
// ----------------------------------------------------------------------------
    : source(NULL), parent(s), rewrites(NULL), error_handler(NULL),
      depth(s ? s->depth : 0),
      has_rewrites_for_constants(false), is_global(false)
{}


inline Symbols::~Symbols()
// ----------------------------------------------------------------------------
//   Delete all included rewrites if necessary and unlink from context
// ----------------------------------------------------------------------------
{}



// ============================================================================
// 
//    Old compilation unit
// 
// ============================================================================

struct OCompiledUnit
// ----------------------------------------------------------------------------
//  A compilation unit, which typically corresponds to an expression
// ----------------------------------------------------------------------------
{
    OCompiledUnit(Compiler *comp, Tree *source, TreeList parms, bool closure);
    ~OCompiledUnit();

    bool                IsForwardCall()         { return entrybb == NULL; }
    eval_fn             Finalize();

    enum { knowAll = -1, knowGlobals = 1, knowLocals = 2, knowValues = 4 };

    llvm::Value *       NeedStorage(Tree *tree, Tree *source = NULL);
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
    llvm::Value *       CallFillBlock(Block *);
    llvm::Value *       CallFillPrefix(Prefix *);
    llvm::Value *       CallFillPostfix(Postfix *);
    llvm::Value *       CallFillInfix(Infix *);
    llvm::Value *       CallArrayIndex(Tree *self, Tree *l, Tree *r);
    llvm::Value *       CreateClosure(Tree *callee,
                                      TreeList &parms, TreeList &args,
                                      llvm::Function *);
    llvm::Value *       CallClosure(Tree *callee, uint ntrees);
    llvm::Value *       CallTypeError(Tree *what);

    llvm::BasicBlock *  TagTest(Tree *code, ulong tag);
    llvm::BasicBlock *  IntegerTest(Tree *code, longlong value);
    llvm::BasicBlock *  RealTest(Tree *code, double value);
    llvm::BasicBlock *  TextTest(Tree *code, text value);
    llvm::BasicBlock *  ShapeTest(Tree *code, Tree *other);
    llvm::BasicBlock *  InfixMatchTest(Tree *code, Infix *ref);
    llvm::BasicBlock *  TypeTest(Tree *code, Tree *type);

public:
    Compiler *          compiler;       // The compiler environment we use
    llvm::LLVMContext * llvm;           // The LLVM context we got from compiler
    Tree_p              source;         // The original source we compile

    llvm::IRBuilder<> * code;           // Instruction builder for code
    llvm::IRBuilder<> * data;           // Instruction builder for data
    llvm::Function *    function;       // Function we generate

    llvm::BasicBlock *  allocabb;       // Function entry point, allocas
    llvm::BasicBlock *  entrybb;        // Entry point for that code
    llvm::BasicBlock *  exitbb;         // Exit point for that code
    llvm::BasicBlock *  failbb;         // Where we go if tests fail
    llvm_value          contextPtr;     // Storage for context pointer

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
    ExpressionReduction(OCompiledUnit &unit, Tree *source);
    ~ExpressionReduction();

    void                NewForm();
    void                Succeeded();
    void                Failed();

public:
    OCompiledUnit &     unit;           // Compilation unit we use
    Tree *              source;         // Tree we build (mostly for debugging)
    llvm::LLVMContext * llvm;           // Inherited context

    llvm::Value *       storage;        // Storage for expression value
    llvm::Value *       computed;       // Flag telling if value was computed

    llvm::BasicBlock *  savedfailbb;    // Saved location of failbb

    llvm::BasicBlock *  entrybb;        // Entry point to subcase
    llvm::BasicBlock *  savedbb;        // Saved position before subcase
    llvm::BasicBlock *  successbb;      // Successful completion of expression

    value_map           savedvalue;     // Saved compile unit value map
};



// ============================================================================
// 
//   Stack depth management
// 
// ============================================================================

struct StackDepthCheck
// ----------------------------------------------------------------------------
//   Verify that we don't go too deep into the stack
// ----------------------------------------------------------------------------
{
    StackDepthCheck(Tree *what)
    {
        stack_depth++;
        if (stack_depth > max_stack_depth)
            StackOverflow(what);
    }
    ~StackDepthCheck()
    {
        stack_depth--;
        if (stack_depth == 0 && !in_error_handler)
            in_error = false;
    }
    operator bool()
    {
        return in_error && !in_error_handler;
    }
    void StackOverflow(Tree *what);

protected:
    static uint         stack_depth;
    static uint         max_stack_depth;
    static bool         in_error_handler;
    static bool         in_error;
};



// ============================================================================
// 
//    Compilation actions
// 
// ============================================================================

struct DeclarationAction : Action
// ----------------------------------------------------------------------------
//   Record data and rewrite declarations in the input tree
// ----------------------------------------------------------------------------
{
    DeclarationAction (Symbols *c): symbols(c) {}

    virtual Tree *Do(Tree *what);
    virtual Tree *DoInteger(Integer *what);
    virtual Tree *DoReal(Real *what);
    virtual Tree *DoText(Text *what);
    virtual Tree *DoName(Name *what);
    virtual Tree *DoPrefix(Prefix *what);
    virtual Tree *DoPostfix(Postfix *what);
    virtual Tree *DoInfix(Infix *what);
    virtual Tree *DoBlock(Block *what);

    void        EnterRewrite(Tree *defined, Tree *definition);

    Symbols_p symbols;
};


struct CompileAction : Action
// ----------------------------------------------------------------------------
//   Compute the input tree in the given compiled unit
// ----------------------------------------------------------------------------
{
    CompileAction (Symbols *s, OCompiledUnit &,
                   bool nullIfBad, bool keepAlt, bool noDataForms);

    virtual Tree *Do(Tree *what);
    virtual Tree *DoInteger(Integer *what);
    virtual Tree *DoReal(Real *what);
    virtual Tree *DoText(Text *what);
    virtual Tree *DoName(Name *what);
    virtual Tree *DoPrefix(Prefix *what);
    virtual Tree *DoPostfix(Postfix *what);
    virtual Tree *DoInfix(Infix *what);
    virtual Tree *DoBlock(Block *what);

    // Evaluation of names is lazy, except in sequences where it's forced
    Tree *        DoName(Name *what, bool forceEval);

    // Build code selecting among rewrites in current context
    Tree *        Rewrites(Tree *what);

    // Build code for children
    Tree *        RewriteChildren(Tree *what);

    Symbols_p     symbols;
    OCompiledUnit &unit;
    bool          nullIfBad;
    bool          keepAlternatives;
    bool          noDataForms;
    char          debugRewrites;
};


struct ParameterMatch : Action
// ----------------------------------------------------------------------------
//   Collect parameters on the left of a rewrite
// ----------------------------------------------------------------------------
{
    ParameterMatch (Symbols *s)
        : symbols(s), defined(NULL) {}

    virtual Tree *Do(Tree *what);
    virtual Tree *DoInteger(Integer *what);
    virtual Tree *DoReal(Real *what);
    virtual Tree *DoText(Text *what);
    virtual Tree *DoName(Name *what);
    virtual Tree *DoPrefix(Prefix *what);
    virtual Tree *DoPostfix(Postfix *what);
    virtual Tree *DoInfix(Infix *what);
    virtual Tree *DoBlock(Block *what);

    Symbols_p symbols;          // Symbols in which we test
    Tree_p    defined;          // Tree beind defined, e.g. 'sin' in 'sin X'
    TreeList  order;            // Record order of parameters
};


struct ArgumentMatch : Action
// ----------------------------------------------------------------------------
//   Check if a tree matches the form of the left of a rewrite
// ----------------------------------------------------------------------------
{
    ArgumentMatch (Tree *t,
                   Symbols *s, Symbols *l, Symbols *r,
                   CompileAction *comp, bool data):
        symbols(s), locals(l), rewrite(r),
        test(t), defined(NULL), compile(comp), unit(comp->unit), data(data) {}

    // Action callbacks
    virtual Tree *Do(Tree *what);
    virtual Tree *DoInteger(Integer *what);
    virtual Tree *DoReal(Real *what);
    virtual Tree *DoText(Text *what);
    virtual Tree *DoName(Name *what);
    virtual Tree *DoPrefix(Prefix *what);
    virtual Tree *DoPostfix(Postfix *what);
    virtual Tree *DoInfix(Infix *what);
    virtual Tree *DoBlock(Block *what);

    // Compile a tree
    Tree *         Compile(Tree *source, bool noData);
    Tree *         CompileValue(Tree *source, bool noData);
    Tree *         CompileClosure(Tree *source);

public:
    Symbols_p      symbols;     // Context in which we evaluate values
    Symbols_p      locals;      // Symbols where we declare arguments
    Symbols_p      rewrite;     // Symbols in which the rewrite was declared
    Tree_p         test;        // Tree we test
    Tree_p         defined;     // Tree beind defined, e.g. 'sin' in 'sin X'
    CompileAction *compile;     // Action in which we are compiling
    OCompiledUnit &unit;        // JIT compiler compilation unit
    bool           data;        // Is a data form
};


struct EnvironmentScan : Action
// ----------------------------------------------------------------------------
//   Collect variables in the tree that are imported from environment
// ----------------------------------------------------------------------------
{
    EnvironmentScan (Symbols *s): symbols(s) {}

    virtual Tree *Do(Tree *what);
    virtual Tree *DoInteger(Integer *what);
    virtual Tree *DoReal(Real *what);
    virtual Tree *DoText(Text *what);
    virtual Tree *DoName(Name *what);
    virtual Tree *DoPrefix(Prefix *what);
    virtual Tree *DoPostfix(Postfix *what);
    virtual Tree *DoInfix(Infix *what);
    virtual Tree *DoBlock(Block *what);

public:
    Symbols_p           symbols;        // Symbols in which we test
    capture_table       captured;       // Captured symbols
};


struct EvaluateChildren
// ----------------------------------------------------------------------------
//   Build a clone of a tree, evaluating its children
// ----------------------------------------------------------------------------
{
    EvaluateChildren(CompileAction *c): compile(c) {}
    ~EvaluateChildren()                         {}
    typedef Tree *value_type;

    virtual Tree *DoInteger(Integer *what);
    virtual Tree *DoReal(Real *what);
    virtual Tree *DoText(Text *what);
    virtual Tree *DoName(Name *what);
    virtual Tree *DoPrefix(Prefix *what);
    virtual Tree *DoPostfix(Postfix *what);
    virtual Tree *DoInfix(Infix *what);
    virtual Tree *DoBlock(Block *what);

public:
    CompileAction *     compile;
};

extern "C"
{
    void debugsy(XL::Symbols *s);
    void debugsym(XL::Symbols *s);
}


XL_END

#endif // SYMBOLS_H
