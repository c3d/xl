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

  In general, the compiler generates only functions with the same prototype as
  eval_fn, i.e. Tree *(Tree *). The code is being generated on invokation
  of a form, and helps rewriting it, although attempts are made to leverage
  existing rewrites.

  In general, a specific rewrite can invoke multiple candidates. For example,
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
struct Compiler;                                // JIT compiler
struct Runtime;                                 // Runtime context
struct Errors;                                  // Error handlers

typedef std::map<text, Tree *>    symbol_table; // Symbol table in context
typedef std::set<Tree *>          active_set;   // Not to be garbage collected
typedef std::map<ulong, Rewrite*> rewrite_table;// Hashing of rewrites
typedef symbol_table::iterator    symbol_iter;  // Iterator over sym table



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
    Symbols(Context *c);
    Symbols(Symbols *s);
    ~Symbols();

    // Symbols properties
    Symbols *           Parent()                { return parent; }
    ulong               Depth();

    // Symbol management
    Tree *              Named (text name, bool deep=false);
    Rewrite *           Rewrites()              { return rewrites; }

    // Entering symbols in the symbol table
    void                EnterName (text name, Tree *value);
    Rewrite *           EnterRewrite(Rewrite *r);
    Tree *              Allocate(Name *varName);

    // Clearing symbol tables
    void                Clear();

public:
    Context *           context;
    Symbols *           parent;
    symbol_table        names;
    Rewrite *           rewrites;
    ulong               locals;
};


struct Context
// ----------------------------------------------------------------------------
//   The compile-time context in which we are currently evaluating
// ----------------------------------------------------------------------------
{
    // Constructors and destructors
    Context(Errors &err, Compiler *comp):
        symbols(NULL),                          // Symbols
        errors(err), error_handler(NULL),       // Error management
        compiler(comp),                         // Tree compilation
        active(), roots(), gc_threshold(200) {} // Garbage collection

    // Evaluation of trees (the killer feature of this class)
    Tree *              Run(Tree *t);

    // Context properties
    Tree *              ErrorHandler();
    void                SetErrorHandler(Tree *e){ error_handler = e; }

    // Garbage collection
    void                Root(Tree *t)           { roots.insert(t); }
    void                Mark(Tree *t)           { active.insert(t); }
    void                CollectGarbage();

    // Helpers for compilation of trees
    Tree *              Compile(Tree *source, bool nullIfBad = false);
    Rewrite *           EnterRewrite(Tree *from, Tree *to);
    Tree *              Error (text message,
                               Tree *a1=NULL, Tree *a2=NULL, Tree *a3=NULL);

public:
    static ulong        gc_increment;
    static ulong        gc_growth_percent;
    static Context *    context;

    Symbols *           symbols;
    Errors &            errors;
    Tree *              error_handler;
    Compiler *          compiler;
    active_set          active;
    active_set          roots;
    ulong               gc_threshold;
};


struct Rewrite
// ----------------------------------------------------------------------------
//   Information about a rewrite, e.g fact N -> N * fact(N-1)
// ----------------------------------------------------------------------------
//   Note that a rewrite with 'to' = NULL is used for 'data' statements
{
    Rewrite (Symbols *s, Tree *f, Tree *t):
        symbols(s), from(f), to(t), hash() {}
    ~Rewrite();

    Rewrite *           Add (Rewrite *rewrite);
    Tree *              Do(Action &a);
    Tree *              Compile(void);

public:
    Symbols *           symbols;
    Tree *              from;
    Tree *              to;
    rewrite_table       hash;
};


struct Runtime
// ----------------------------------------------------------------------------
//   Runtime execution environment (one per thread)
// ----------------------------------------------------------------------------
{
    Tree *      pc;                             // Program counter
    Tree **     fp;                             // Frame pointer
};



// ============================================================================
// 
//   Inline functions
// 
// ============================================================================

inline Symbols::Symbols(Context *c)
// ----------------------------------------------------------------------------
//   Links the symbol table being created into the context
// ----------------------------------------------------------------------------
    : context(c), parent(c->symbols), rewrites(NULL), locals(0)
{
    c->symbols = this;
}


inline Symbols::Symbols(Symbols *s)
// ----------------------------------------------------------------------------
//   Create a "child" symbol table
// ----------------------------------------------------------------------------
    : context(s->context), parent(s), rewrites(NULL), locals(0)
{}


inline Symbols::~Symbols()
// ----------------------------------------------------------------------------
//   Delete all included rewrites if necessary and unlink from context
// ----------------------------------------------------------------------------
{
    if (rewrites)
        delete rewrites;
    if (context->symbols == this)
        context->symbols = parent;
}


inline Tree *Symbols::Named(text name, bool deep)
// ----------------------------------------------------------------------------
//   Find the name in the current context
// ----------------------------------------------------------------------------
{
    for (Symbols *s = this; s && deep; s = s->parent)
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

#endif // CONTEXT_H
