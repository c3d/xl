#ifndef CONTEXT_H
#define CONTEXT_H
// ****************************************************************************
//  context.h                       (C) 1992-2003 Christophe de Dinechin (ddd)
//                                                                 XL2 project
// ****************************************************************************
//
//   File Description:
//
//     The execution environment for XL
//
//     This defines both the compile-time environment (Context), where we
//     keep symbolic information, e.g. how to rewrite trees, and the
//     runtime environment (Runtime), which we use while executing trees
//
//
//
//
// ****************************************************************************
// This program is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html for details
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************
/*
  COMPILATION STRATEGY:
  
  The version of XL implemented here is a very simple language based
  on tree rewrites, designed to serve as a dynamic document description
  language (DDD), as well as a tool to implement the "larger" XL in a more
  dynamic way. Both usage models imply that the language is compiled on the
  fly, not statically.

  Also, because the language is designed to manipulate program trees, which
  server as the primary data structure, this implies that the program trees
  will exist at run-time as well. There needs to be a garbage collection
  phase. The chosen garbage collection technique is mark and sweep, so that we
  can deal with cyclic data structures. This allows us to replace a name with
  what it references, even in cases such as X->1,X, an infinite
  comma-separated tree of 1s.

  The chosen approach is to add an evaluation function pointer to each tree,
  the field being called 'code' in struct Tree. This function pointer is
  filled in by the compiler as a result of compilation. This means that it is
  possible to render the source tree as well as to execute optimized code
  when we evaluate it.

  In some cases, the compiler may want to choose a more efficient data
  structure layout for the tree being represented. For example, we may decide
  to use an memory-contiguous array to represent [ 1, 2, 3, 4 ], even if the
  native tree representation not memory contiguous nor easy to access. The
  above technique makes this possible too. For such cases, we generate the
  data, and prefix it with a very small code thunk that simply returns the
  result of a call to xl_data(). xl_data() can get the address of the data
  from its return address. See xl_data() for details.

  
  PREDEFINED FORMS:

  XL is really built on a very small number of predefined forms recognized by
  the compilation phase.

    "A->B" defines a rewrite rule, rewriting A as B. The form A can be
           "guarded" by a "when" clause, e.g. "N! when N>0 -> N * (N-1)!"
    "A:B" is a type annotation, indicating that the type of A is B
    "(A)" is the same as A, allowing to override default precedence,
          and the same holds for A in an indentation (indent block)
    "A;B" is a sequence, evaluating A, then B, the value being B.
          The newline infix operator plays the same role.
    "data A" declares A as a form that cannot be reduced further

  The XL type system is itself based on tree shapes. For example, "integer" is
  a type that covers all Integer trees. "integer+integer" is a type that
  covers all Infix trees with "+" as the name and an Integer node in the left
  and right nodes. "integer+0" covers "+" infix trees with an Integer on the
  left and the exact value 0 on the right.

  The compiler is allowed to special-case any such form. For example, if it
  determines that the type of a tree is "integer", it may represent it using a
  machine integer. It must however convert to/from tree when connecting to
  code that deals with less specialized trees.

  The compiler is also primed with a number of declarations such as:
     x:integer+y:integer -> native addint:integer
  This tells the compiler to lookup a native compilation function called
  "xl_addint" and invoke that.

  
  RUNTIME EXECUTION:

  The evaluation functions pointed to by 'code' in the Tree structure are
  designed to be invoked by normal C code using the normal C stack. This
  facilitate the interaction with other code.

  At top-level, the compiler generates only functions with the same
  prototype as eval_fn, i.e. Tree *(Tree *). The code is being
  generated on invokation of a form, and helps rewriting it, although
  attempts are made to leverage existing rewrites. This is implemented
  in Context::CompileAll.

  Compiling such top-level forms invokes a number of rewrites. A
  specific rewrite can invoke multiple candidates. For example,
  consider the following factorial declaration:
     0! -> 1
     N! when N>0 -> N * (N-1)!

  In that case, code invoking N! will generate a test to check if N is 0, and
  if so invoke the first rule, otherwise invoke the second rule. The same
  principle applies when there are guarded rules, i.e. the "when N>0" clause
  will cause a test N>0 to be added guarding the second rule.

  If all these invokations fail, then the input tree is in "reduced form",
  i.e. it cannot be reduced further. If it is a non-leaf tree, then an attempt
  is made to evaluate its children.

  If no evaluation can be found for a given form that doesn't match a 'data'
  clause, the compiler will emit a diagnostic. This is not a fatal condition,
  however, and code will be generated that simply leaves the tree as is when
  that happens.
 */

#include <map>
#include <set>
#include <vector>
#include "base.h"
#include "tree.h"

XL_BEGIN

// ============================================================================
// 
//    Forward type declarations
// 
// ============================================================================

struct Tree;                                    // Abstract syntax tree
struct Name;                                    // Name node, e.g. ABC or +
struct Action;                                  // Action on trees
struct Context;                                 // Compile-time context
struct Rewrite;                                 // Tree rewrite data
struct Runtime;                                 // Runtime context
struct Errors;                                  // Error handlers
struct Compiler;                                // JIT compiler
struct CompiledUnit;                            // Compilation unit

typedef std::map<text, Tree *>    symbol_table; // Symbol table in context
typedef std::set<Tree *>          active_set;   // Not to be garbage collected
typedef std::set<Symbols*>        active_syms;  // Not to be garbage collected
typedef std::map<ulong, Rewrite*> rewrite_table;// Hashing of rewrites
typedef symbol_table::iterator    symbol_iter;  // Iterator over sym table
typedef std::vector<Tree **>      globals_table;// Table of LLVM globals
typedef std::map<Tree*, Symbols*> capture_table;// Symbol capture table



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
    ulong               Depth();

    // Symbol management
    Tree *              Named (text name, bool deep = true);
    Rewrite *           Rewrites()              { return rewrites; }

    // Entering symbols in the symbol table
    void                EnterName (text name, Tree *value);
    Rewrite *           EnterRewrite(Rewrite *r);
    Rewrite *           EnterRewrite(Tree *from, Tree *to);
    Name *              Allocate(Name *varName);

    // Clearing symbol tables
    void                Clear();

    // Compiling and evaluating a tree in scope defined by these symbols
    Tree *              Compile(Tree *s, CompiledUnit &, bool nullIfBad=false);
    Tree *              CompileAll(Tree *s);
    Tree *              Run(Tree *t);

public:
    Symbols *           parent;
    symbol_table        names;
    Rewrite *           rewrites;
};


struct Context : Symbols
// ----------------------------------------------------------------------------
//   The compile-time context in which we are currently evaluating
// ----------------------------------------------------------------------------
{
    // Constructors and destructors
    Context(Errors &err, Compiler *comp):
        Symbols(NULL),
        errors(err), error_handler(NULL),       // Error management
        compiler(comp),                         // Tree compilation
        active(), roots(),                      // Garbage collection
        active_symbols(), gc_threshold(200) {}  // More garbage collection
    ~Context();

    // Context properties
    Tree *              ErrorHandler();
    void                SetErrorHandler(Tree *e){ error_handler = e; }

    // Garbage collection
    void                Root(Tree *t)           { roots.insert(t); }
    void                Mark(Tree *t)           { active.insert(t); }
    void                CollectGarbage();

    // Helpers for compilation of trees
    Tree *              Error (text message,
                               Tree *a1=NULL, Tree *a2=NULL, Tree *a3=NULL);
    Tree **             AddGlobal(Tree *value);

public:
    static ulong        gc_increment;
    static ulong        gc_growth_percent;
    static Context *    context;

    Errors &            errors;
    Tree *              error_handler;
    Compiler *          compiler;
    active_set          active;
    active_set          roots;
    active_syms         active_symbols;
    ulong               gc_threshold;
    globals_table       globals;
};


struct Rewrite
// ----------------------------------------------------------------------------
//   Information about a rewrite, e.g fact N -> N * fact(N-1)
// ----------------------------------------------------------------------------
//   Note that a rewrite with 'to' = NULL is used for 'data' statements
{
    Rewrite (Symbols *s, Tree *f, Tree *t):
        symbols(s), from(f), to(t), hash(), parameters() {}
    ~Rewrite();

    Rewrite *           Add (Rewrite *rewrite);
    Tree *              Do(Action &a);
    Tree *              Compile(void);

public:
    Symbols *           symbols;
    Tree *              from;
    Tree *              to;
    rewrite_table       hash;
    tree_list           parameters;
};



// ============================================================================
// 
//    Compilation actions
// 
// ============================================================================

typedef std::map<Tree *, ulong> eval_cache;


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

    Symbols *symbols;
};


struct CompileAction : Action
// ----------------------------------------------------------------------------
//   Compute the input tree in the given compiled unit
// ----------------------------------------------------------------------------
{
    CompileAction (Symbols *s, CompiledUnit &, bool nullIfBad);

    virtual Tree *Do(Tree *what);
    virtual Tree *DoInteger(Integer *what);
    virtual Tree *DoReal(Real *what);
    virtual Tree *DoText(Text *what);
    virtual Tree *DoName(Name *what);
    virtual Tree *DoPrefix(Prefix *what);
    virtual Tree *DoPostfix(Postfix *what);
    virtual Tree *DoInfix(Infix *what);
    virtual Tree *DoBlock(Block *what);

    // Build code selecting among rewrites in current context
    Tree *        Rewrites(Tree *what);

    Symbols *     symbols;
    CompiledUnit &unit;
    eval_cache    needed;
    bool          nullIfBad;
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

    Symbols * symbols;          // Symbols in which we test
    Tree *    defined;          // Tree beind defined, e.g. 'sin' in 'sin X'
    tree_list order;            // Record order of parameters
};


struct ArgumentMatch : Action
// ----------------------------------------------------------------------------
//   Check if a tree matches the form of the left of a rewrite
// ----------------------------------------------------------------------------
{
    ArgumentMatch (Tree *t,
                   Symbols *s, Symbols *l, Symbols *r,
                   CompileAction *comp, eval_cache &evals):
        symbols(s), locals(l), rewrite(r),
        test(t), defined(NULL), compile(comp), unit(comp->unit) {}

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
    Tree *        Compile(Tree *source);
    Tree *        CompileValue(Tree *source);
    Tree *        CompileClosure(Tree *source);

public:
    Symbols *     symbols;      // Context in which we evaluate values
    Symbols *     locals;       // Symbols where we declare arguments
    Symbols *     rewrite;      // Symbols in which the rewrite was declared
    Tree *        test;         // Tree we test
    Tree *        defined;      // Tree beind defined, e.g. 'sin' in 'sin X'
    CompileAction *compile;     // Action in which we are compiling
    CompiledUnit &unit;         // JIT compiler compilation unit
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
    Symbols *           symbols;        // Symbols in which we test
    capture_table       captured;       // Captured symbols
};


struct BuildChildren : Action
// ----------------------------------------------------------------------------
//   Build a clone of a tree, evaluating its children
// ----------------------------------------------------------------------------
{
    BuildChildren(CompileAction *comp);
    ~BuildChildren();

    virtual Tree *Do(Tree *what)                { return what; }
    virtual Tree *DoInteger(Integer *what)      { return what; }
    virtual Tree *DoReal(Real *what)            { return what; }
    virtual Tree *DoText(Text *what)            { return what; }
    virtual Tree *DoName(Name *what)            { return what; }
    virtual Tree *DoPrefix(Prefix *what);
    virtual Tree *DoPostfix(Postfix *what);
    virtual Tree *DoInfix(Infix *what);
    virtual Tree *DoBlock(Block *what);
 
public:
    CompileAction *compile;             // Compilation in progress
    CompiledUnit & unit;                // JIT compiler compilation unit
    bool           saveNullIfBad;       // Unit original "nib" settings
};



// ============================================================================
// 
//   LocalSave helper class
// 
// ============================================================================

template <class T>
struct LocalSave
// ----------------------------------------------------------------------------
//    Save a variable locally
// ----------------------------------------------------------------------------
{
    LocalSave (T &source, T value): reference(source), original(source)
    {
        reference = value;
    }
    ~LocalSave()
    {
        reference = original;
    }
    T&  reference;
    T   original;
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
    : parent(s), rewrites(NULL)
{}


inline Symbols::~Symbols()
// ----------------------------------------------------------------------------
//   Delete all included rewrites if necessary and unlink from context
// ----------------------------------------------------------------------------
{
    if (rewrites)
        delete rewrites;
 }


inline Tree *Symbols::Named(text name, bool deep)
// ----------------------------------------------------------------------------
//   Find the name in the current context
// ----------------------------------------------------------------------------
{
    for (Symbols *s = this; s; s = deep ? s->parent : NULL)
        if (s->names.count(name) > 0)
            return s->names[name];
    return NULL;
}


inline ulong Symbols::Depth()
// ----------------------------------------------------------------------------
//    Return the depth for the current symbol table
// ----------------------------------------------------------------------------
{
    ulong depth = 0;
    for (Symbols *s = this; s; s = s->parent)
        depth++;
    return depth;
}

XL_END

extern "C"
{
    void debugs(XL::Symbols *s);
    void debugsc(XL::Symbols *s);
}

#endif // CONTEXT_H
