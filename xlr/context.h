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
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************
/*
  COMPILATION STRATEGY:

  The version of XL implemented here is a very simple language based
  on tree rewrites, designed to serve as a dynamic document description
  language (DDD), as well as a tool to implement the "larger" XL in a more
  dynamic way. Both usage models imply that the language is compiled on the
  fly, not statically. We use LLVM as a back-end, see compiler.h.

  Also, because the language is designed to manipulate program trees, which
  serve as the primary data structure, this implies that the program trees
  will exist at run-time as well. As a resut, there needs to be a
  garbage collection phase. The chosen garbage collection technique is
  based on reference counting because trees are not cyclic, so that
  ref-counting is both faster and simpler.


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
    "data A" declares A as a form that cannot be reduced further.
          This can be used to declare data structures.

  The XL type system is itself based on tree shapes. For example, "integer" is
  a type that covers all Integer trees. Verifying if X has type Y is performed
  by evaluting the value X:Y.

  - In the direct type match case, this evaluates to X.
    For example, '1:integer' will evaluate directly to 1.

  - The X:Y expression may perform an implicit type conversion.
    For example, '1:real' will evaluate to 1.0 (converting 1 to real)

  - Finally, in case of type mismatch, X:Y evaluates to the same X:Y tree.

  At run-time, this is checked by xl_type_check, which returns NULL in case
  of mismatch, and the converted value otherwise.

  The compiler is allowed to special-case any such form. For example, if it
  determines that the type of a tree is "integer", it may represent it using a
  machine integer. It must however convert to/from tree when connecting to
  code that deals with less specialized trees.

  The compiler is also primed with a number of declarations such as:
     x:integer+y:integer -> [builtin]
  This tells the compiler to lookup a native compilation function. These
  functions are declared in basics.tbl (and possibly additional .tbl files)


  RUNTIME EXECUTION:

  The evaluation functions pointed to by 'code' in the Tree structure are
  designed to be invoked by normal C code using the normal C stack. This
  facilitate the interaction with other code.

  At top-level, the compiler generates only functions with the same
  prototype as eval_fn, i.e. Tree * (Tree *). The code is being
  generated on invokation of a form, and helps rewriting it, although
  attempts are made to leverage existing rewrites.

  Compiling such top-level forms invokes a number of rewrites. A
  specific rewrite can invoke multiple candidates. For example,
  consider the following factorial declaration:
     0! -> 1
     N! where N>0 -> N * (N-1)!

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


  GENERATING CODE FOR REWRITES:

  The code for a rewrite is kept in the 'code' field of the definition.
  This definition should never be evaluated directly, because the code field
  doesn't have the expected signature for xl_evaluate(). Specifically,
  it has additional Tree * parameters, corresponding to the variables
  in the pattern (the left-hand side of the rewrite).

  For example, the rewrite 'bar X:integer, Y -> foo X, Y+1' has two variables
  in its pattern, X and Y. Therefore, the code field for 'foo X, Y+1' will
  have the following signature: Tree *(Tree *self, Tree *X, Tree *Y).
  Note that bar is not a variable for the pattern. While the name 'bar' is
  being defined partially by this rewrite, it is not a variable in the pattern.

  The generated code will only be invoked if the conditions for invoking it
  are fulfilled. In the example, it will not be invoked if X is not an integer
  or for a form such as 'bar 3' because 3 doesn't match 'X:integer, Y'.


  CLOSURES:

  Closures are a way to "embed" the value of variables in a tree so that
  it can be safely passed around. For example, consider
      AtoB T ->
         A; do T; B
      foo X ->
         AtoB { write X+1 }

  In this example, the rules of the language state that 'write X+1' is not
  evaluated before being passed to 'AtoB', because nothing in the 'AtoB'
  pattern requires evaluation. However, 'write X+1' needs the value of 'X'
  to evaluate properly in 'AtoB'. Therefore, what is passed to 'AtoB' is
  a closure retaining the value of X in 'foo'. Assuming the value of X is 17,
  the closure will take the form of a prefix with the original expression
  as its first element, and the values to set as the next arguments.
  In our example, it will take the form '{ write X+1 } 17 false'.

  The code evaluating the closure is generated by CompiledUnit::CallClosure.
  To simplify the generated code, arguments are always on the left of
  the prefix, and we descend on the right.  This is why there is an arbitrary
  final 'false' that occupies the 'right' field of the last prefix.

  At runtime, closures are generated by CompiledUnit::CreateClosure,
  which is called by ArgumentMatch::CompileClosure(), a function called
  when passing arguments that may need to be embedded in a closure.


  LAZY EVALUATION:

  Trees are evaluated as late as possible (lazy evaluation). In order to
  achieve that, the compiler maintains a boolean variable per tree that may
  be evaluated lazily. This variable is allocated by CompiledUnit::NeedLazy.
  It is set when the value is evaluated, and initially cleared.
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

struct Context;                                 // Execution context
struct Rewrite;                                 // Tree rewrite data
struct Errors;                                  // Error handlers

typedef GCPtr<Context>             Context_p;
typedef GCPtr<Rewrite>             Rewrite_p;
typedef std::map<ulong, Rewrite_p> rewrite_table;// Hashing of rewrites
typedef std::vector<Rewrite_p>     rewrite_list;
typedef std::set<Context_p>        context_set;
typedef std::vector<Context_p>     context_list;
typedef std::map<Tree_p, Tree_p>   tree_map;
typedef Tree *(*native_fn) (Context *ctx, Tree *self);



// ============================================================================
//
//    Compile-time symbols and rewrites management
//
// ============================================================================

struct Context
// ----------------------------------------------------------------------------
//   The evaluation context for a given tree
// ----------------------------------------------------------------------------
{
    Context(Context *scope, Context *stack);
    ~Context();

    // Type of lookup
    enum lookup_mode
    {
        LOCAL_LOOKUP    = 0,    // Only in the local context
        SCOPE_LOOKUP    = 1,    // Lexical lookup
        STACK_LOOKUP    = 2,    // Along the execution stack
        IMPORTED_LOOKUP = 4,    // Lookup in imported scopes
        AVOID_ERRORS    = 8,    // During binding, avoid errors

        NORMAL_LOOKUP   = SCOPE_LOOKUP | IMPORTED_LOOKUP,
        BIND_LOOKUP     = NORMAL_LOOKUP | AVOID_ERRORS,
        ANY_LOOKUP      = NORMAL_LOOKUP | STACK_LOOKUP
    };

    // Process declarations for a given context, return non-declarations
    Tree *              ProcessDeclarations(Tree *what);

    // Adding definitions to the context
    Rewrite *           Define(Tree *form, Tree *value);
    Rewrite *           DefineData(Tree *form);
    Tree *              Assign(Tree *target, Tree *source,
                               lookup_mode mode = SCOPE_LOOKUP);

    // Rewriting things in the context
    Tree *              Evaluate(Tree *what, lookup_mode mode = NORMAL_LOOKUP);
    Tree *              Evaluate(Tree *what,
                                 tree_map &valueCache,
                                 lookup_mode mode = NORMAL_LOOKUP,
                                 Context **tailContext = NULL,
                                 Tree **tailTree = NULL);
    Tree *              EvaluateBlock(Tree *child);

    // The hash code used in the rewrite table
    static ulong        Hash(Tree *input);

    // Bind parameters in context based on arguments in form
    bool                Bind(Tree *form, Tree *value, tree_map &values,
                             TreeList *args = NULL);

    // Find the value that a name is bound to, or returns NULL
    Tree *              Bound(Name *name, lookup_mode mode = SCOPE_LOOKUP);

    // Create a closure in this context
    Tree *              CreateCode(Tree *value);
    Tree *              EvaluateCode(Tree *closure, Tree *value);
    Tree *              CreateLazy(Tree *value);
    Tree *              EvaluateLazy(Tree *closure, Tree *value);

    // List rewrites of a given type
    void                ListNames(text begin, rewrite_list &list,
                                  lookup_mode lookup = SCOPE_LOOKUP);

    // List the set of contexts to lookup (necessary for imported case)
    void                Contexts(lookup_mode, context_set &, context_list &);

    // Clear the symbol table
    void                Clear();

public:
    Context_p           scope;
    Context_p           stack;
    rewrite_table       rewrites;
    context_set         imported;
    bool                hasConstants;

    GARBAGE_COLLECT(Context);
};


struct Rewrite
// ----------------------------------------------------------------------------
//   Information about a rewrite, e.g fact N -> N * fact(N-1)
// ----------------------------------------------------------------------------
//   Note that a rewrite with 'to' = NULL is used for 'data' statements
{
    Rewrite (Tree *f, Tree *t): from(f), to(t), hash(), native(NULL) {}
    ~Rewrite() {}

public:
    Tree_p              from;
    Tree_p              to;
    rewrite_table       hash;
    native_fn           native;

    GARBAGE_COLLECT(Rewrite);
};


struct ClosureInfo : Info
// ----------------------------------------------------------------------------
//    Information recording the context in which we evaluate a given tree
// ----------------------------------------------------------------------------
{
    typedef Context_p data_t;
    ClosureInfo(Context *context): context(context) {}
    operator data_t() { return context; }
    Context_p context;
};


struct PrefixDefinitionsInfo : Info
// ----------------------------------------------------------------------------
//   Information recording definitions associated with a prefix
// ----------------------------------------------------------------------------
{
    PrefixDefinitionsInfo(): last(NULL) {}
    Infix_p   last;
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
    LocalSave (T &source, T value): reference(source), saved(source)
    {
        reference = value;
    }
    LocalSave(const LocalSave &o): reference(o.reference), saved(o.saved) {}
    LocalSave (T &source): reference(source), saved(source)
    {
    }
    ~LocalSave()
    {
        reference = saved;
    }
    operator T()        { return saved; }

    T&  reference;
    T   saved;
};

XL_END

extern "C"
{
    void debugs(XL::Context *s);
    void debugsc(XL::Context *s);
    void debugst(XL::Context *s);
}

#endif // CONTEXT_H
