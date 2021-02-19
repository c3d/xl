#ifndef CONTEXT_H
#define CONTEXT_H
// *****************************************************************************
// context.h                                                          XL project
// *****************************************************************************
//
// File description:
//
//     Defines the evaluation context for XL programs
//
//     The evaluation context is represented by a standard XL parse tree.
//     The Context C++ class is a simple wrapper around such parse trees
//     that makes it a bit easier to manipulate the trees from C++ programs.
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2012, Catherine Burvelle <catherine@taodyne.com>
// (C) 2003-2004,2009-2012,2014-2020, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2010-2011,2013, Jérôme Forissier <jerome@taodyne.com>
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
/*
  A scope is defined as a block that contains bindings.

  For example, if you have an operator [foo X,Y] defined as [foo X,Y is X+Y],
  and if you invoke that operator as [foo 3, 4], then it is evaluated in a
  scope that looks like [{X is 3; Y is 4}]

  The scope is exactly similar to how you would define it in the language
  itself, but for efficienty, it is balanced based on size and specialization of
  the trees, to make sure that the largest and most specialized definition is
  found. For example, consider the following definitions:

      N! is N * (N-1)!
      0! is 1

  In that case, the definition [0!] is more specialized than [N!]. As a result,
  the sorted scope after insertions will have the two definitions reversed. This
  ensures that the behavior of a program does not depend on the order of
  declarations.

  If definitions have the same size and specialization, then they are kept in
  program order, so that the first one is selected. For example, the two
  following definitions would be kept in order:

      foo X is 1
      foo Y is 2

   Consider a context containing an outer scope and an inner scope as follows:

      // Outer scope
      min X in X
      min X, Y is { mX is min X; mY is min Y; if mX < mY then mX else mY }

      // Inner scope
      A is 3
      B is 4
      printf "Min of %d and %d is %d\n", A, B, min(A, B)

   In that scenario, the context structure while executing [min(A,B)] will look
   like a sequence of prefixes, which is searched from right to left, where the
   outer contexxt was sorted "largest and most specialized first":

      {  min X, Y is ... ; min X is X } { A is 3; B is 4 } { X is A; Y is B }

   The following C++ classes wrap specific structures in this symbol table:

   - The Rewrite class wraps a declaration, like [A is 3]
   - The Rewrites class wraps a sequence, like [A is 3; B is 4]
   - The Scope class wraps a single scope, like [{A is 3; B is 4}]
   - The Scopes class wraps a nested scopes, like [{A is 3} {B is 4}]

   To avoid a linear search on the symbol table, entries are organized in the
   symbol table to allow a more efficient binary search. This works as follows,
   where [A < B] means that A is larger or more specialized than B:

   - A Rewrites always has a Rewrite on the left, its "payload".
   - A Rewrites can have either:
     + A Rewrite on the right, in which case left < right
     + A Rewrites on the right, in which case right.left < left < right.right

   For example, if we insert { A is 3; B is 4; C is 5 }, we end up with the
   followign structure:

      { B is 4; ( A is 3; C is 5 ) }

   which corresponds to the order A, B, C, since the outer node has [B is 4] on
   its right, and assumes that the node on the left of its left precedes it, and
   the node on the right follows it.

   To avoid skewing of the binary tree constructed that way, we may later follow
   the red-black tree algorithm for rebalancing. The convention could be that a
   an infix ; corresponds to black, and an infix \n corresponds to red.
 */

#include "base.h"
#include "tree.h"
#include "builtins.h"

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
typedef GCPtr<Context>                  Context_p;
struct Rewrites;


// ============================================================================
//
//   C++ type safe wrappers for elements in a symbol table
//
// ============================================================================

struct Scope : Block
// ----------------------------------------------------------------------------
//   A scope is simply a {} block
// ----------------------------------------------------------------------------
//   It contains either
//   - xl_nil for an empty scope,
//   - A declaration tree made of Rewrites/Rewrites for a top-level scope
//   - A Scopes giving access to the enclosing scopes
//   This corresponds to a source structure like [{X is 0; Y is 1}], except
//   that the entries are sorted, larger and more specialized first
{
    Scope(Tree *child = xl_nil, TreePosition pos = NOWHERE)
        : Block(child, "{", "}", pos) {}
    Tree   *Entries()   { return child; }
    Scope  *Enclosing();
    Scope  *Inner();
    Tree_p &Locals();
    void    Reparent(Scope *enclosing);
    void    Clear();
    bool    IsEmpty();
    void    Import(Prefix *import);
    Prefix *Import();
    GARBAGE_COLLECT(Scope);
};
typedef GCPtr<Scope> Scope_p;


struct Scopes : Prefix
// ----------------------------------------------------------------------------
//   A sequence of scopes
// ----------------------------------------------------------------------------
//   This is used to represent a hierarchy of scopes.
//   The enclosing scope is on the left, the child scope on the right
//   This corresponds to the source structure [(X is 1) (Y is 2)]
//   Import statements are inserted at the end of the enclosing list
{
    Scopes(Scope *enclosing, Scope *inner, TreePosition pos = NOWHERE)
        : Prefix(enclosing, inner, pos) {}
    Scopes(Scope *enclosing, Prefix *import, TreePosition pos = NOWHERE)
        : Prefix(enclosing, import, pos) {}
    Scope *Enclosing();
    void   Reparent(Scope *enclosing);
    void   Import(Prefix *import);
    Prefix *Import();
    Scope *Inner();
    GARBAGE_COLLECT(Scopes);
};
typedef GCPtr<Scopes> Scopes_p;


struct Rewrite : Infix
// ----------------------------------------------------------------------------
//   A rewrite is an infix "is"
// ----------------------------------------------------------------------------
//   This corresponds to a source structure like [X is 1]
{
    Rewrite(Tree *pattern, Tree *definition)
        : Infix("is", pattern, definition, pattern->Position()) {}
    Rewrite(Infix *infix)
        : Infix(infix, infix->left, infix->right) {}
    Tree *Pattern() { return left; }
    Tree *BasePattern();
    Tree *Definition()  { return right; }
    GARBAGE_COLLECT(Rewrite);
};
typedef GCPtr<Rewrite> Rewrite_p;


struct Rewrites : Infix
// ----------------------------------------------------------------------------
//   Sequence of rewrites are separated by a \n
// ----------------------------------------------------------------------------
//   This corresponds to a source structure like [X is 1 \n Y is 2]
//   It is sorted as follows:
//   - Payload on the left, which is always a Rewrite
//   - Rewrite or Rewrites on the right
//   - The Rewrites form a sorted tree of declarations
//   - The payload can be an import statement
{
    Rewrites(Rewrite *left, Rewrite *right)
        : Infix("\n", left, right, left->Position()) {}
    Rewrites(Rewrite *left, Rewrites *right)
        : Infix("\n", left, right, left->Position()) {}
    Rewrite  *Payload()         { return (Rewrite *) (Tree *) left; }
    Rewrite  *Second();
    Rewrites *Children();
    GARBAGE_COLLECT(Rewrites);
};
typedef GCPtr<Rewrites> Rewrites_p;


struct Closure : Prefix
// ----------------------------------------------------------------------------
//   An XL closure is represented by a prefix with a scope on the left
// ----------------------------------------------------------------------------
//   This corresponds to the source structure [(X is 1) X]
{
    Closure(Scope *scope, Tree *value)
        : Prefix(scope, value, value->Position()) {}
    Scope *CapturedScope()      { return (Scope *) (Tree *) left; }
    Tree  *Value()              { return right; }
    GARBAGE_COLLECT(Closure);
};
typedef GCPtr<Closure> Closure_p;


typedef std::vector<Rewrite_p>          RewriteList;
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
    Scope *             PopScope();
    Scope *             Symbols()               { return symbols; }
    void                SetSymbols(Scope *s)    { symbols = s; }
    Context *           Parent();
    operator Scope *()                          { return symbols; }

    // Special forms of evaluation
    Tree *              Call(text prefix, TreeList &args);

    // Phases of evaluation
    bool                ProcessDeclarations(Tree *what, RewriteList &inits);
    Scope *             ProcessScope(Tree *declarations, RewriteList &inits);
    Tree *              Enclose(Tree *value);
    static Scope *      IsClosure(Tree *value);

    // Adding definitions to the context
    Rewrite *           Enter(Infix *infix, RewriteList &inits);
    Rewrite *           Enter(Rewrite *rewrite, bool overwrite=false);
    Rewrite *           Define(Tree *pattern, Tree *def, bool overwrite=false);
    Rewrite *           Define(text name, Tree *def, bool overwrite=false);
    Tree *              Assign(Tree *target, Tree *source);
    Tree *              ValidatePattern(Tree *pattern);

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

    // Looking up definitions in a context
    typedef Tree *      (*lookup_fn)(Scope *evalContext, Scope *declContext,
                                     Tree *form, Rewrite *decl, void *info);
    Tree *              Lookup(Tree *what,
                               lookup_fn lookup, void *info,
                               bool recurse=true);
    Rewrite *           Reference(Tree *form, bool recurse=true);
    Tree *              DeclaredPattern(Tree *form);
    Tree *              Bound(Tree *form,bool recurse=true);
    Tree *              Bound(Tree *form, bool rec, Rewrite_p *rw,Scope_p *ctx);
    Tree *              Named(text name, bool recurse=true);
    bool                IsEmpty();

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
    static void         Dump(std::ostream &out, Tree_p *locals);
    void                Dump(std::ostream &out) { Dump(out, symbols, true); }

public:
    Scope_p             symbols;
    GARBAGE_COLLECT(Context);
};


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


inline bool IsTypeCast(Infix *infix)
// ----------------------------------------------------------------------------
//   Check if an infix is a type cast
// ----------------------------------------------------------------------------
{
    return infix->name == "as";
}


inline Infix *IsTypeCast(Tree *tree)
// ----------------------------------------------------------------------------
//    Check if a type is a type cast
// ----------------------------------------------------------------------------
{
    if (Infix *infix = tree->AsInfix())
        if (IsTypeCast(infix))
            return infix;
    return nullptr;
}


inline bool IsAssignment(Infix *infix)
// ----------------------------------------------------------------------------
//   Check if an infix is an assignment
// ----------------------------------------------------------------------------
{
    return infix->name == ":=" || infix->name == ":<" || infix->name == ":+";
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


inline bool IsConstantDefinition(Infix *infix)
// ----------------------------------------------------------------------------
//   Check if an infix is a constant definition, i.e. [Pattern is Impl]
// ----------------------------------------------------------------------------
{
    return infix->name == "is";
}


inline Infix *IsConstantDefinition(Tree *tree)
// ----------------------------------------------------------------------------
//   Check if a tree is a constant definition
// ----------------------------------------------------------------------------
{
    if (Infix *infix = tree->AsInfix())
        if (IsConstantDefinition(infix))
            return infix;
    return nullptr;
}


inline bool IsVariableDefinition(Infix *infix)
// ----------------------------------------------------------------------------
//   Check if an infix is a variable definition [Name : Type := Init]
// ----------------------------------------------------------------------------
{
    return IsAssignment(infix) && IsTypeAnnotation(infix->left);
}


inline Infix *IsVariableDefinition(Tree *tree)
// ----------------------------------------------------------------------------
//   Check if a tree is a variable declaration
// ----------------------------------------------------------------------------
{
    if (Infix *infix = tree->AsInfix())
        if (IsVariableDefinition(infix))
            return infix;
    return nullptr;
}


inline bool IsConstantDeclaration(Infix *infix)
// ----------------------------------------------------------------------------
//   Check if an infix is a constant declaration [Name as Type]
// ----------------------------------------------------------------------------
{
    return infix->name == "as";
}


inline Infix *IsConstantDeclaration(Tree *tree)
// ----------------------------------------------------------------------------
//   Check if a tree is a constant declaration
// ----------------------------------------------------------------------------
{
    if (Infix *infix = tree->AsInfix())
        if (IsConstantDeclaration(infix))
            return infix;
    return nullptr;
}


inline bool IsVariableDeclaration(Infix *infix)
// ----------------------------------------------------------------------------
//   Check if an infix is a variable declaration [Name : Type]
// ----------------------------------------------------------------------------
{
    return infix->name == ":";
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
    return IsConstantDeclaration(infix) || IsVariableDeclaration(infix);
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


inline bool IsDefinition(Infix *infix)
// ----------------------------------------------------------------------------
//   Check if an infix is a definition
// ----------------------------------------------------------------------------
{
    return IsConstantDefinition(infix) || IsVariableDefinition(infix);
}


inline bool IsDefinition(Prefix *prefix)
// ----------------------------------------------------------------------------
//   Check if a prefix is a definition
// ----------------------------------------------------------------------------
//   Something like [extern int foo(bar)] is a definition, since it's
//   implicitly [foo X:bar as integer is C foo]
{
    if (Name *name = prefix->left->AsName())
        if (name->value == "extern")
            return true;
    return false;
}


inline Infix *IsDefinition(Tree *tree)
// ----------------------------------------------------------------------------
//   Check if a tree is a definition
// ----------------------------------------------------------------------------
{
    if (Infix *infix = tree->AsInfix())
        if (IsDefinition(infix))
            return infix;
    return nullptr;
}


inline Name *IsLambda(Prefix *prefix)
// ----------------------------------------------------------------------------
//   Check if a prefix is a lambda form
// ----------------------------------------------------------------------------
{
    if (Name *name = prefix->left->AsName())
        if (name->value == "lambda" || name->value == "\\")
            if (Name *defined = prefix->right->AsName())
                return defined;
    return nullptr;
}


inline Name *IsLambda(Tree *what)
// ----------------------------------------------------------------------------
//   Check if a tree is an error form
// ----------------------------------------------------------------------------
{
    if (Prefix *prefix = what->AsPrefix())
        if (Name *name = IsLambda(prefix))
            return name;
    return nullptr;
}


inline Name *IsTypeCastDeclaration(Infix *infix)
// ----------------------------------------------------------------------------
//   Check if an infix is a type cast declaration [lambda N as type]
// ----------------------------------------------------------------------------
{
    if (IsTypeCast(infix))
        if (Name *name = IsLambda(infix->left))
            return name;
    return nullptr;
}


inline Name *IsTypeCastDeclaration(Tree *tree)
// ----------------------------------------------------------------------------
//    Check if a type is a type cast declaration [lambda N as type]
// ----------------------------------------------------------------------------
{
    if (Infix *infix = tree->AsInfix())
        if (Name *name = IsTypeCastDeclaration(infix))
            return name;
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


inline bool IsError(Prefix *prefix)
// ----------------------------------------------------------------------------
//   Check if a prefix is an error form
// ----------------------------------------------------------------------------
{
    if (Name *name = prefix->left->AsName())
        if (name->value == "error")
            return true;
    return false;
}


inline Prefix *IsError(Tree *what)
// ----------------------------------------------------------------------------
//   Check if a tree is an error form
// ----------------------------------------------------------------------------
{
    if (Prefix *prefix = what->AsPrefix())
        if (IsError(prefix))
            return prefix;
    return nullptr;
}


inline Tree *IsQuote(Prefix *prefix)
// ----------------------------------------------------------------------------
//   Check if a prefix is a quote form
// ----------------------------------------------------------------------------
{
    if (Name *name = prefix->left->AsName())
        if (name->value == "quote")
            return prefix->right;
    return nullptr;
}


inline Prefix *IsQuote(Tree *what)
// ----------------------------------------------------------------------------
//   Check if a tree is a quote form
// ----------------------------------------------------------------------------
{
    if (Prefix *prefix = what->AsPrefix())
        if (IsQuote(prefix))
            return prefix;
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


inline bool IsDot(Infix *infix)
// ----------------------------------------------------------------------------
//    Check if the infix is a dot operator
// ----------------------------------------------------------------------------
{
    return infix->name == ".";
}


inline Infix *IsDot(Tree *tree)
// ----------------------------------------------------------------------------
//   Check if a tree is a dot notation
// ----------------------------------------------------------------------------
{
    if (Infix *infix = tree->AsInfix())
        if (IsDot(infix))
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


inline Tree *IsPatternMatchingType(Prefix *prefix)
// ----------------------------------------------------------------------------
//   Check if a prefix is [matching Pattern]
// ----------------------------------------------------------------------------
{
    if (prefix->left == xl_matching)
        return prefix->right;
    if (Name *matching = prefix->left->AsName())
        if (matching->value == "matching")
            return prefix->right;
    return nullptr;
}


inline Tree *IsPatternMatchingType(Tree *tree)
// ----------------------------------------------------------------------------
//   Check if a tree is a [matching Pattern] prefix
// ----------------------------------------------------------------------------
{
    if (Prefix *prefix = tree->AsPrefix())
        if (Tree *pattern = IsPatternMatchingType(prefix))
            return pattern;
    return nullptr;
}


inline Text *IsBuiltin(Prefix *prefix)
// ----------------------------------------------------------------------------
//   Check if a prefix is special (extern, builtin, C)
// ----------------------------------------------------------------------------
{
    if (Name *builtin = prefix->left->AsName())
        if (builtin->value == "builtin")
            if (Text *name = prefix->right->AsText())
                return name;
    return nullptr;
}


inline Text *IsBuiltin(Tree *tree)
// ----------------------------------------------------------------------------
//   Return a builtin prefix if this is one
// ----------------------------------------------------------------------------
{
    if (Prefix *prefix = tree->AsPrefix())
        if (Text *name = IsBuiltin(prefix))
            return name;
    return nullptr;
}


inline Text *IsNative(Prefix *prefix)
// ----------------------------------------------------------------------------
//   Check if a prefix is for a native function, e.g. [C mallloc]
// ----------------------------------------------------------------------------
{
    if (Name *C = prefix->left->AsName())
        if (C->value == "C" || C->value == "c")
            if (Text *name = prefix->right->AsText())
                return name;
    return nullptr;
}


inline Text *IsNative(Tree *tree)
// ----------------------------------------------------------------------------
//   Return a builtin prefix if this is one
// ----------------------------------------------------------------------------
{
    if (Prefix *prefix = tree->AsPrefix())
        if (Text *name = IsNative(prefix))
            return name;
    return nullptr;
}


inline Name *IsSelf(Name *name)
// ----------------------------------------------------------------------------
//   Return true if this is a [self] name
// ----------------------------------------------------------------------------
{
    return name->value == "self" ? xl_self : nullptr;
}


inline Name *IsSelf(Tree *tree)
// ----------------------------------------------------------------------------
//   Check if a definition is the 'self' tree
// ----------------------------------------------------------------------------
{
    if (tree == xl_self)
        return xl_self;
    if (Name *name = tree->AsName())
        if (Name *self = IsSelf(name))
            return self;
    return nullptr;
}


inline Name *IsMatching(Name *name)
// ----------------------------------------------------------------------------
//   Return true if this is a [self] name
// ----------------------------------------------------------------------------
{
    return name->value == "matching" ? xl_matching : nullptr;
}


inline Name *IsMatching(Tree *tree)
// ----------------------------------------------------------------------------
//   Check if a definition is the 'self' tree
// ----------------------------------------------------------------------------
{
    if (Name *name = tree->AsName())
        if (Name *matching = IsMatching(name))
            return matching;
    return nullptr;
}


inline Block *IsMetabox(Tree *tree)
// ----------------------------------------------------------------------------
//  Check if a given tree is a metabox
// ----------------------------------------------------------------------------
{
    if (Block *block = tree->AsBlock())
        if (block->IsMetaBox())
            return block;
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

        // Check 'X as natural', we define X
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
    Context::Dump(out, c->Symbols(), true);
    return out;
}



// ============================================================================
//
//   Specialization for Tree::As<T>
//
// ============================================================================

template<> inline Scope *Tree::As<Scope>(Scope *)
// ----------------------------------------------------------------------------
//   Check that we have an actual scope
// ----------------------------------------------------------------------------
{
    if (Block *block = AsBlock())
        // Check if this was allocated as a scope - Then it's a scope
        if (XL::Allocator<XL::Scope>::IsAllocated(this))
            return (Scope *) block;
    return nullptr;
}


template<> inline Scopes *Tree::As<Scopes>(Scope *)
// ----------------------------------------------------------------------------
//   Check that we have an actual prefix, sequence of scopes
// ----------------------------------------------------------------------------
{
    if (Prefix *prefix = AsPrefix())
        // Check if this was allocated as a scope - Then it's a scope
        if (XL::Allocator<XL::Scopes>::IsAllocated(this))
            return (Scopes *) prefix;
    return nullptr;
}


template<> inline Rewrite *Tree::As<Rewrite>(Scope *)
// ----------------------------------------------------------------------------
//   Check that the name identifies a rewrite
// ----------------------------------------------------------------------------
{
    if (Infix *definition = IsDefinition(this))
        return (Rewrite *) definition;
    return nullptr;
}


template<> inline Rewrites *Tree::As<Rewrites>(Scope *)
// ----------------------------------------------------------------------------
//   Check that the name identifies rewrites
// ----------------------------------------------------------------------------
{
    if (Infix *sequence = IsSequence(this))
        return (Rewrites *) sequence;
    return nullptr;
}


template<> inline Closure *Tree::As<Closure>(Scope *)
// ----------------------------------------------------------------------------
//   Check that we have an actual closure
// ----------------------------------------------------------------------------
{
    if (Prefix *prefix = AsPrefix())
        // Check if this was allocated as a closure
        if (XL::Allocator<XL::Closure>::IsAllocated(this))
            return (Closure *) prefix;
    return nullptr;
}



// ============================================================================
//
//    Inline functions that cannot be defined before specialization
//
// ============================================================================

inline Rewrite *Rewrites::Second()
// ----------------------------------------------------------------------------
//   Return the second entry in a Rewrites if it's a Rewrite
// ----------------------------------------------------------------------------
{
    return right->As<Rewrite>();
}


inline Rewrites *Rewrites::Children()
// ----------------------------------------------------------------------------
//   Return the second entry in a Rewrites if it's another Rewrites
// ----------------------------------------------------------------------------
{
    return right->As<Rewrites>();
}


inline Tree *Rewrite::BasePattern()
// ----------------------------------------------------------------------------
//   Return the base pattern
// ----------------------------------------------------------------------------
{
    return PatternBase(left);
}



// ============================================================================
//
//    Meaning adapters - Make it more explicit what happens in code
//
// ============================================================================

inline Scope *Scopes::Enclosing()
// ----------------------------------------------------------------------------
//   Return the enclosing scope
// ----------------------------------------------------------------------------
{
    // Normal case: left is a Scope
    if (Scope *scope = left->As<Scope>())
        return scope;

    // Import case: left is a Scopes
    if (Scopes *scopes = left->As<Scopes>())
        return scopes->Enclosing();

    // Unexpected
    return nullptr;
}


inline void Scopes::Reparent(Scope *enclosing)
// ----------------------------------------------------------------------------
//   Change the enclosing scope
// ----------------------------------------------------------------------------
{
    left = enclosing;
}


inline Scope *Scopes::Inner()
// ----------------------------------------------------------------------------
//   Return the inner scope
// ----------------------------------------------------------------------------
//   This should not be called when there is an import
{
    return right->As<Scope>();
}


inline void Scopes::Import(Prefix *import)
// ----------------------------------------------------------------------------
//   Add an import statement to the scopes
// ----------------------------------------------------------------------------
{
    left = new Scopes(Enclosing(), import);
}


inline Prefix *Scopes::Import()
// ----------------------------------------------------------------------------
//   Return the last import
// ----------------------------------------------------------------------------
{
    if (Scopes *scopes = left->As<Scopes>())
        if (Prefix *prefix = scopes->right->AsPrefix())
            return prefix;
    return nullptr;
}


inline Scope *Scope::Enclosing()
// ----------------------------------------------------------------------------
//   Find parent for a given scope
// ----------------------------------------------------------------------------
{
    if (Scopes *scopes = child->As<Scopes>())
        return scopes->Enclosing();
    return nullptr;
}


inline Scope *Scope::Inner()
// ----------------------------------------------------------------------------
//   Return the inner scope
// ----------------------------------------------------------------------------
{
    Scope *scope = this;
    if (Scopes *scopes = child->As<Scopes>())
        scope = scopes->Inner();
    return scope;
}


inline Tree_p &Scope::Locals()
// ----------------------------------------------------------------------------
//   Return the place where we store the parent for a scope
// ----------------------------------------------------------------------------
{
    Scope *scope = Inner();
    return scope->child;
}


inline void Scope::Reparent(Scope *enclosing)
// ----------------------------------------------------------------------------
//   Change the enclosing context for this scope
// ----------------------------------------------------------------------------
{
    if (Scopes *scopes = child->As<Scopes>())
        scopes->Reparent(enclosing);
}


inline void Scope::Clear()
// ----------------------------------------------------------------------------
//  Clear the local symbol table
// ----------------------------------------------------------------------------
{
    Tree_p &locals = Locals();
    locals = xl_nil;
}


inline bool Scope::IsEmpty()
// ----------------------------------------------------------------------------
//   Return true if the scope is empty
// ----------------------------------------------------------------------------
{
    Tree_p &locals = Locals();
    return locals == xl_nil;
}


inline void Scope::Import(Prefix *import)
// ----------------------------------------------------------------------------
//   Add an import to the current scope
// ----------------------------------------------------------------------------
{
    Scopes *scopes = child->As<Scopes>();
    if (!scopes)
    {
        // Import into top-level scope
        scopes = new Scopes(new Scope(), new Scope(child));
        child = scopes;
    }
    scopes->Import(import);
}


inline Prefix *Scope::Import()
// ----------------------------------------------------------------------------
//   Return last import, or null
// ----------------------------------------------------------------------------
{
    if (Scopes *scopes = child->As<Scopes>())
        return scopes->Import();
    return nullptr;
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
        for (Scope *s = data.scope; s; s = s->Enclosing())
            out << (void *) s << " ";
        out << "]";
        return out;
    }
    Scope *scope;
};



// ============================================================================
//
//   Closure management
//
// ============================================================================

inline Tree *Context::Enclose(Tree *value)
// ----------------------------------------------------------------------------
//   Prefix the value wiht the current symbols - Unwrapped by evaluate()
// ----------------------------------------------------------------------------
{
    if (!IsClosure(value))
    {
        Scope *scope = Symbols();
        while (scope && scope->IsEmpty())
            scope = scope->Enclosing();
        if (scope)
            value = new Closure(scope, value);
    }
    return value;
}


inline Scope *Context::IsClosure(Tree *value)
// ----------------------------------------------------------------------------
//   Check if this looks like a closure
// ----------------------------------------------------------------------------
{
    if (Closure *prefix = value->As<Closure>())
        if (Scope *scope = prefix->left->As<Scope>())
            return scope;
    return nullptr;
}


inline bool Context::IsEmpty()
// ----------------------------------------------------------------------------
//   Return true if the context is empty
// ----------------------------------------------------------------------------
{
    return symbols->IsEmpty();
}



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

XL_END


RECORDER_DECLARE(context);
RECORDER_DECLARE(symbols);
RECORDER_DECLARE(symbols_errors);
RECORDER_DECLARE(symbols_sort);

#endif // CONTEXT_H
