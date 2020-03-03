#ifndef CONTEXT_H
#define CONTEXT_H
// *****************************************************************************
// context.h                                                          XL project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2012, Catherine Burvelle <catherine@taodyne.com>
// (C) 2003-2004,2009-2012,2014-2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2010-2011,2013, Jérôme Forissier <jerome@taodyne.com>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
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

  The XL type system is itself based on tree shapes. For example,
  "integer" is a type that covers all Integer trees. Verifying if X
  has type Y is performed by evaluting the value X:Y.

  - In the direct type match case, this evaluates to X.
    For example, '1:integer' will evaluate directly to 1.

  - The X:Y expression may perform an implicit type conversion.
    For example, '1:real' will evaluate to 1.0 (converting 1 to real)

  - Finally, in case of type mismatch, X:Y evaluates to the same X:Y tree.

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
  prototype as native_fn, i.e. Tree * (Context *, Tree *). The code is being
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
  doesn't have the expected signature for evaluation. Specifically,
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
  a closure retaining the value of X in 'foo'.

  In the symbol table, closures are simply implemented as additional
  declarations that precede the original code. For example, in teh
  above example, if X is 17 in the environment, the closure would be
  created as a scope containing { X -> 17 }, and would be applied as a
  prefix, i.e. { X -> 17 } { write X+1 }.

  In boxed formats, closures take a slightly more complicated form.


  LAZY EVALUATION:

  Trees are evaluated as late as possible (lazy evaluation). In order to
  achieve that, the compiler maintains a boolean variable per tree that may
  be evaluated lazily. This variable is allocated by CompilerUnit::NeedLazy.
  It is set when the value is evaluated, and initially cleared.
 */

#include "base.h"
#include "tree.h"

#include <recorder/recorder.h>
#include <map>
#include <set>
#include <vector>
#include <iostream>


XL_BEGIN

// ============================================================================
//
//    Forward type declarations
//
// ============================================================================

struct Context;                                 // Execution context

// Give names to components of a symbol table
typedef Prefix                          Scope;
typedef GCPtr<Scope>                    Scope_p;
typedef Infix                           Rewrite;
typedef GCPtr<Rewrite>                  Rewrite_p;
typedef Infix                           RewriteChildren;
typedef GCPtr<RewriteChildren>          RewriteChildren_p;

typedef GCPtr<Context>                  Context_p;
typedef std::vector<Infix_p>            RewriteList;
typedef std::map<Tree_p, Tree_p>        tree_map;
typedef Tree *                          (*eval_fn) (Scope *, Tree *);
typedef std::map<Tree_p, eval_fn>       code_map;



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
    Context(Context *parent, TreePosition pos = Tree::NOWHERE);
    Context(const Context &source);
    Context(Scope *symbols);
    ~Context();

public:
    // Create and delete a local scope
    Scope *             CreateScope(TreePosition pos = Tree::NOWHERE);
    void                PopScope();
    Scope *             Symbols()               { return symbols; }
    void                SetSymbols(Scope *s)    { symbols = s; }
    Context *           Parent();
    Context *           Pointer()               { return this; }
    operator Scope *()                          { return symbols; }

    // Special forms of evaluation
    Tree *              Call(text prefix, TreeList &args);

    // Phases of evaluation
    bool                ProcessDeclarations(Tree *what);

    // Adding definitions to the context
    Rewrite *           Enter(Infix *decl, bool overwrite=false);
    Rewrite *           Define(Tree *from, Tree *to, bool overwrite=false);
    Rewrite *           Define(text name, Tree *to, bool overwrite=false);
    Tree *              Assign(Tree *target, Tree *source);

    // Set and get per-context tree info
    Tree *              Info(text key, Tree *what, bool recurse = false);
    Rewrite *           SetInfo(text key, Tree *what, Tree *value);

    Tree *              Type(Tree *what);
    Rewrite *           SetType(Tree *what, Tree *type);

    // Set context attributes
    Rewrite *           SetOverridePriority(double priority);
    Rewrite *           SetModulePath(text name);
    Rewrite *           SetModuleDirectory(text name);
    Rewrite *           SetModuleFile(text name);
    Rewrite *           SetModuleName(text name);

    Rewrite *           SetAttribute(text attr, Tree *value, bool ovwr=false);
    Rewrite *           SetAttribute(text attr, longlong value, bool ow=false);
    Rewrite *           SetAttribute(text attr, double value, bool ow=false);
    Rewrite *           SetAttribute(text attr, text value, bool ow=false);

    // Path management
    void                SetPrefixPath(text prefix, text path);
    text                ResolvePrefixedPath(text path);

    // Looking up definitions in a context
    typedef Tree *      (*lookup_fn)(Scope *evalContext, Scope *declContext,
                                     Tree *form, Infix *decl, void *info);
    Tree *              Lookup(Tree *what,
                               lookup_fn lookup, void *info,
                               bool recurse=true);
    Rewrite *           Reference(Tree *form, bool recurse=true);
    Tree *              DeclaredForm(Tree *form);
    Tree *              Bound(Tree *form,bool recurse=true);
    Tree *              Bound(Tree *form, bool rec, Rewrite_p *rw,Scope_p *ctx);
    Tree *              Named(text name, bool recurse=true);
    bool                IsEmpty();
    bool                HasRewritesFor(kind k);
    void                HasOneRewriteFor(kind k);

    // List rewrites of a given type
    ulong               ListNames(text begin, RewriteList &list,
                                  bool recurses = true,
                                  bool includePrefixes = false);

    // The hash code used in the rewrite table
    static ulong        Hash(Tree *input);
    static inline ulong Rehash(ulong h) { return (h>>1) ^ (h<<31); }

    // Clear the symbol table
    void                Clear();

    // Dump symbol tables
    static void         Dump(std::ostream &out, Scope *symbols, bool recurse);
    static void         Dump(std::ostream &out, Rewrite *locals);
    void                Dump(std::ostream &out) { Dump(out, symbols, true); }

public:
    Scope_p             symbols;
    static uint         hasRewritesForKind;
    GARBAGE_COLLECT(Context);
};


// ============================================================================
//
//    Meaning adapters - Make it more explicit what happens in code
//
// ============================================================================

inline Scope *Enclosing(Scope *scope)
// ----------------------------------------------------------------------------
//   Find parent for a given scope
// ----------------------------------------------------------------------------
{
    return scope->left->As<Scope>();
}


inline Tree_p &ScopeLocals(Scope *scope)
// ----------------------------------------------------------------------------
//   Return the place where we store the parent for a scope
// ----------------------------------------------------------------------------
{
    return scope->right;
}


inline Rewrite *ScopeRewrites(Scope *scope)
// ----------------------------------------------------------------------------
//   Find top rewrite for a given scope
// ----------------------------------------------------------------------------
{
    return scope->right->As<Rewrite>();
}


inline Infix *RewriteDeclaration(Rewrite *rw)
// ----------------------------------------------------------------------------
//   Find what a rewrite declares
// ----------------------------------------------------------------------------
{
    return rw->left->AsInfix();
}


inline RewriteChildren *RewriteNext(Rewrite *rw)
// ----------------------------------------------------------------------------
//   Find the children of a rewrite during lookup
// ----------------------------------------------------------------------------
{
    return rw->right->AsInfix();
}


struct ContextStack
// ----------------------------------------------------------------------------
//   For debug purpose: display the context stack on a std::ostream
// ----------------------------------------------------------------------------
{
    ContextStack(Scope *scope) : scope(scope) {}
    friend std::ostream &operator<<(std::ostream &out, const ContextStack &data)
    {
        out << "[ ";
        for (Scope *s = data.scope; s; s = Enclosing(s))
            out << (void *) s << " ";
        out << "]";
        return out;
    }
    Scope *scope;
};



// ============================================================================
//
//    Helper functions to extract key elements from a rewrite
//
// ============================================================================

inline Rewrite *Context::SetOverridePriority(double priority)
// ----------------------------------------------------------------------------
//   Set the override_priority attribute
// ----------------------------------------------------------------------------
{
    return SetAttribute("override_priority", priority);
}


inline Rewrite *Context::SetModulePath(text path)
// ----------------------------------------------------------------------------
//   Set the module_path attribute
// ----------------------------------------------------------------------------
{
    return SetAttribute("module_path", path);
}


inline Rewrite *Context::SetModuleDirectory(text directory)
// ----------------------------------------------------------------------------
//   Set the module_directory attribute
// ----------------------------------------------------------------------------
{
    return SetAttribute("module_directory", directory);
}


inline Rewrite *Context::SetModuleFile(text file)
// ----------------------------------------------------------------------------
//   Set the module_file attribute
// ----------------------------------------------------------------------------
{
    return SetAttribute("module_file", file);
}


inline Rewrite *Context::SetModuleName(text name)
// ----------------------------------------------------------------------------
//   Set the module_name attribute
// ----------------------------------------------------------------------------
{
    return SetAttribute("module_name", name);
}


inline bool Context::HasRewritesFor(kind k)
// ----------------------------------------------------------------------------
//    Check if we detected rewrites for a specific kind
// ----------------------------------------------------------------------------
//    This is used to avoid useless lookups for reals, integers, etc.
{
    return (hasRewritesForKind & (1<<k)) != 0;
}


inline void Context::HasOneRewriteFor(kind k)
// ----------------------------------------------------------------------------
//    Record that we have a new rewrite for a given kind
// ----------------------------------------------------------------------------
{
    hasRewritesForKind |= 1<<k;
}


// ============================================================================
//
//   Inline helpers defining the shape of trees in the context
//
// ============================================================================

inline bool IsTypeAnnotation(Infix *infix)
// ----------------------------------------------------------------------------
//   Check if an infix is a type annotation
// ----------------------------------------------------------------------------
{
    return infix->name == ":" || infix->name == "as";
}


inline Infix *IsTypeAnnotation(Tree *tree)
// ----------------------------------------------------------------------------
//    Check if a type is a type annotation
// ----------------------------------------------------------------------------
{
    if (Infix *infix = tree->AsInfix())
        if (IsTypeAnnotation(infix))
            return infix;
    return nullptr;
}


inline bool IsAssignment(Infix *infix)
// ----------------------------------------------------------------------------
//   Check if an infix is an assignment
// ----------------------------------------------------------------------------
{
    return infix->name == ":=";
}


inline Infix *IsAssignment(Tree *tree)
// ----------------------------------------------------------------------------
//   Check if a tree is an assignment
// ----------------------------------------------------------------------------
{
    if (Infix *infix = tree->AsInfix())
        if (IsAssignment(infix))
            return infix;
    return nullptr;
}


inline bool IsDefinition(Infix *infix)
// ----------------------------------------------------------------------------
//   Check if an infix is a constant declaration
// ----------------------------------------------------------------------------
{
    return infix->name == "is";
}


inline Infix *IsDefinition(Tree *tree)
// ----------------------------------------------------------------------------
//   Check if a tree is an assignment
// ----------------------------------------------------------------------------
{
    if (Infix *infix = tree->AsInfix())
        if (IsDefinition(infix))
            return infix;
    return nullptr;
}


inline bool IsVariableDeclaration(Infix *infix)
// ----------------------------------------------------------------------------
//   Check if an infix is a variable declaration [Name : Type := Init]
// ----------------------------------------------------------------------------
{
    return IsAssignment(infix) && IsTypeAnnotation(infix->left);
}


inline Infix *IsVariableDeclaration(Tree *tree)
// ----------------------------------------------------------------------------
//   Check if a tree is a variable declaration
// ----------------------------------------------------------------------------
{
    if (Infix *infix = tree->AsInfix())
        if (IsVariableDeclaration(infix))
            return infix;
    return nullptr;
}


inline bool IsDeclaration(Infix *infix)
// ----------------------------------------------------------------------------
//   Check if an infix is a declaration
// ----------------------------------------------------------------------------
{
    return (IsDefinition(infix) ||
            IsTypeAnnotation(infix) ||
            IsVariableDeclaration(infix));
}


inline Infix *IsDeclaration(Tree *tree)
// ----------------------------------------------------------------------------
//   Check if a tree is a declaration
// ----------------------------------------------------------------------------
{
    if (Infix *infix = tree->AsInfix())
        if (IsDeclaration(infix))
            return infix;
    return nullptr;
}


inline bool IsSequence(Infix *infix)
// ----------------------------------------------------------------------------
//   Check if an infix represents a sequence, i.e. "A;B" or newline
// ----------------------------------------------------------------------------
{
    return infix->name == ";" || infix->name == "\n";
}


inline Infix *IsSequence(Tree *tree)
// ----------------------------------------------------------------------------
//   Check if a tree represents a sequence
// ----------------------------------------------------------------------------
{
    if (Infix *infix = tree->AsInfix())
        if (IsSequence(infix))
            return infix;
    return nullptr;
}


inline bool IsCommaList(Infix *infix)
// ----------------------------------------------------------------------------
//    Check if the infix is a comma operator
// ----------------------------------------------------------------------------
{
    return infix->name == ",";
}


inline Infix *IsCommaList(Tree *tree)
// ----------------------------------------------------------------------------
//   Check if a tree is a comma infix
// ----------------------------------------------------------------------------
{
    if (Infix *infix = tree->AsInfix())
        if (IsCommaList(infix))
            return infix;
    return nullptr;
}


inline bool IsPatternCondition(Infix *infix)
// ----------------------------------------------------------------------------
//   Check if an infix marks a condition
// ----------------------------------------------------------------------------
{
    return infix->name == "when";
}


inline Infix *IsPatternCondition(Tree *tree)
// ----------------------------------------------------------------------------
//   Check if a tree marks a condition
// ----------------------------------------------------------------------------
{
    if (Infix *infix = tree->AsInfix())
        if (IsPatternCondition(infix))
            return infix;
    return nullptr;
}


inline bool IsPatternMatchingType(Prefix *prefix)
// ----------------------------------------------------------------------------
//   Check if a prefix is [matching Pattern]
// ----------------------------------------------------------------------------
{
    if (Name *matching = prefix->left->AsName())
        if (matching->value == "matching")
            return true;
    return false;
}


inline Prefix *IsPatternMatchingType(Tree *tree)
// ----------------------------------------------------------------------------
//   Check if a tree is a [matching Pattern] prefix
// ----------------------------------------------------------------------------
{
    if (Prefix *prefix = tree->AsPrefix())
        if (IsPatternMatchingType(prefix))
            return prefix;
    return nullptr;
}


inline bool IsSelf(Name *name)
// ----------------------------------------------------------------------------
//   Return true if this is a [self] name
// ----------------------------------------------------------------------------
{
    return name->value == "self";
}


inline Name *IsSelf(Tree *tree)
// ----------------------------------------------------------------------------
//   Check if a definition is the 'self' tree
// ----------------------------------------------------------------------------
{
    if (Name *name = tree->AsName())
        if (IsSelf(name))
            return name;
    return nullptr;
}


inline Tree * PatternBase(Tree *form)
// ----------------------------------------------------------------------------
//   Find what we actually define based on the shape of the left of a 'is'
// ----------------------------------------------------------------------------
{
    Tree *old;
    do
    {
        old = form;

        // Check 'X as integer', we define X
        if (Infix *typeDecl = form->AsInfix())
            if (IsTypeAnnotation(typeDecl))
                form = typeDecl->left;

        // Check 'X when Condition', we define X
        if (Infix *typeDecl = form->AsInfix())
            if (IsPatternCondition(typeDecl))
                form = typeDecl->left;

        // Check outermost (X): we define X
        if (Block *block = form->AsBlock())
            form = block->child;

    } while (form != old);

    return form;
}


inline Tree * AnnotatedType(Tree *what)
// ----------------------------------------------------------------------------
//   If we have a declaration with 'X as Type', return Type
// ----------------------------------------------------------------------------
{
    if (Infix *typeDecl = what->AsInfix())
        if (IsTypeAnnotation(typeDecl))
            return typeDecl->right;
    return nullptr;
}


inline std::ostream &operator<< (std::ostream &out, Context *c)
// ----------------------------------------------------------------------------
//   Dump a context
// ----------------------------------------------------------------------------
{
    out << "Context " << (void *) c->Symbols() << ":\n";
    Context::Dump(out, ScopeRewrites(c->symbols));
    return out;
}


RECORDER_DECLARE(xl2c);

XL_END

#endif // CONTEXT_H
