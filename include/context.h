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
//     That execution context is stored in an XL scope structure,
//     see http://c3d.github.io/xl/#scopes-and-maps.
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2012, Catherine Burvelle <catherine@taodyne.com>
// (C) 2003-2020, Christophe de Dinechin <christophe@dinechin.org>
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
  SYMBOL TABLE:

  A symbol table is represented as a prefix sequence of contexts:

     {
         X is 1
         Y is 2
     } {
         Z is 3
         T is "Hello"
     }

  The left of the prefix is the parent context.
  The right of the prefix is the current context.
  This prefix structure is typedefed as a "Scopes" (plural).
  Each of the levels is a Block surrounded by { }, and is
  typedefed as "Scope" (singular).

  Each of the levels contains either

  - An empty name if there is no definition in the scope (empty scope)
  - One Rewrite, which is an Infix "is" or ":="
  - One Rewrites sequence of rewrites, which is an infix "\n"

  In order to keep lookup O(log(N)) instead of O(N), the Infix ; are
  reordered based on a hash function, so that you may go left or right
  at each stage based on the lowest bit of the hash function.
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
//  The following express XL types that are not easy to express in C++:
//     scope: A prefix where left is nil or a block and right is a block
//     scopemap: a {} block containing onlyi rewrites
//     rewrite: An infix that is 'A is B' or 'A := B'
//     rewrites: An infix that is 'A \n B' or 'A ; B'

struct Context;                                 // Execution context

// Give names to components of a symbol table
typedef Prefix                          Scope;
typedef GCPtr<Scope>                    Scope_p;
typedef Block                           ScopeMap;
typedef GCPtr<ScopeMap>                 ScopeMap_p;
typedef Infix                           Rewrites;
typedef GCPtr<Rewrites>                 Rewrites_p;
typedef Infix                           Rewrite;
typedef GCPtr<Rewrite>                  Rewrite_p;
typedef Infix                           RewriteChildren;
typedef GCPtr<RewriteChildren>          RewriteChildren_p;

// API used to access a context
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
    Rewrite *           Enter(Rewrite *decl, bool overwrite=false);
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
    Tree *              DeclaredPattern(Tree *form);
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
    static void         Dump(std::ostream &out, Rewrite *rewrite);
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

inline Scope *EnclosingScope(Scope *scope)
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
        for (Scope *s = data.scope; s; s = EnclosingScope(s))
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
