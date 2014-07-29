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
  prototype as native_fn, i.e. Tree * (Tree *). The code is being
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
struct Action;                                  // Actions applied on trees

typedef GCPtr<Context>             Context_p;
typedef std::vector<Infix_p>       rewrite_list;
typedef std::map<Tree_p, Tree_p>   tree_map;
typedef Tree *(*native_fn) (Context *ctx, Tree *self);



// ============================================================================
//
//    Compile-time symbols and rewrites management
//
// ============================================================================

struct Context
// ----------------------------------------------------------------------------
//   Representation of the evaluation context
// ----------------------------------------------------------------------------
// A context is a represented by a sequence of the form L;E,
// where L is the local scope, and E is the enclosing context
// (which has itself a similar form).

// The local scope is made of nodes that are equivalent to the old Rewrite class
// They have the following structure: D \n L ; R, where D is the local
// declaration for that node, and L and R are the left and right child when
// walking through the local symbol table using a tree hash.
// This makes it possible to implement tree balancing and O(log N) lookups
// in a local symbol table.

// A declaration generally has the form From->To, where From is the
// form we match, and To is the implementation. There are variants in From:
// - Guard clauses will have the form From when Condition
// - Return type declarations will have the form From as Type
{
    Context();
    Context(Context *parent);
    Context(const Context &source);
    Context(Infix *symbols);
    ~Context();

public:
    // Create and delete a local scope
    void                CreateScope();
    void                PopScope();
    Infix *             Scope()         { return symbols; }
    Context *           Parent();

    // Evaluate a tree entirely (compile then run)
    Tree *              Evaluate(Tree *what);
    Tree *              Call(text prefix, TreeList &args);

    // Process declarations for a given context, return non-declarations
    Tree *              ProcessDeclarations(Tree *what);

    // Adding definitions to the context
    Infix *             Enter(Infix *decl);
    Infix *             Define(Tree *from, Tree *to);
    Infix *             Define(text name, Tree *to);
    Tree *              Assign(Tree *target, Tree *source);

    // Set context attributes
    Infix *             SetOverridePriority(double priority);
    Infix *             SetFileName(text filename);

    // Path management
    text                ResolvePrefixedPath(text path);

    // Looking up definitions in a context
    typedef Tree *      (*lookup_fn)(Infix *sc,Tree *frm,Infix *dcl,void *info);
    Tree *              Lookup(Tree *what,
                               lookup_fn lookup, void *info,
                               bool recurse=true);
    Infix *             Reference(Tree *form);
    Tree *              Bound(Tree *form,bool recurse=true);
    Tree *              Bound(Tree *form, bool rec, Infix_p *rw, Infix_p *ctx);
    Tree *              Named(text name, bool recurse=true);

    // List rewrites of a given type
    ulong               ListNames(text begin,rewrite_list &list,
                                  bool recurses = true,
                                  bool includePrefixes = false);

    // The hash code used in the rewrite table
    static ulong        Hash(text name);
    static ulong        Hash(Tree *input, bool inDecl);
    static inline ulong Rehash(ulong h) { return (h>>1) | (h<<31); }

    // Clear the symbol table
    void                Clear();

public:
    Infix_p             symbols;
    GARBAGE_COLLECT(Context);
};



// ============================================================================
// 
//    Helper functions to extract key elements from a rewrite
// 
// ============================================================================

// Check what is actually defined based on the left of a ->
inline Tree * RewriteDefined(Tree *form)
// ----------------------------------------------------------------------------
//   Find what we actually define based on the shape of the left of a ->
// ----------------------------------------------------------------------------
{
    // Check 'X as integer', we define X
    if (Infix *typeDecl = form->AsInfix())
        if (typeDecl->name == "as" || typeDecl->name == ":")
            form = typeDecl->left;

    // Check 'X when Condition', we define X
    if (Infix *typeDecl = form->AsInfix())
        if (typeDecl->name == "when")
            form = typeDecl->left;

    // Check outermost (X): we define X
    if (Block *block = form->AsBlock())
        form = block->child;

    return form;
}


inline Tree * RewriteType(Tree *what)
// ----------------------------------------------------------------------------
//   If we have a declaration with 'X as Type', return Type
// ----------------------------------------------------------------------------
{
    if (Infix *typeDecl = what->AsInfix())
        if (typeDecl->name == "as")
            return typeDecl->right;
    return NULL;
}

XL_END

#endif // CONTEXT_H
