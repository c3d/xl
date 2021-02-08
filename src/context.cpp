// *****************************************************************************
// context.cpp                                                        XL project
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
//     See comments at beginning of context.h for details
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2003-2004,2009-2012,2014-2020, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2010-2013, Jérôme Forissier <jerome@taodyne.com>
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

#include "context.h"
#include "tree.h"
#include "errors.h"
#include "options.h"
#include "renderer.h"
#include "builtins.h"
#include "runtime.h"
#include "main.h"
#include "save.h"
#include "cdecls.h"
#include "interpreter.h"

#ifndef INTERPRETER_ONLY
#include "compiler.h"
#endif // INTERPRETER_ONLY

#include <iostream>
#include <cstdlib>
#include <sstream>
#include <sys/stat.h>



RECORDER(context,        32, "Context: wrapper for XL symbol table");
RECORDER(symbols,        32, "XL symbol table");
RECORDER(symbols_errors, 32, "Errors running symbol table");
RECORDER(symbols_sort,   32, "Sorting entries in the symbol table");
RECORDER(symbols_lookup, 32, "Lookup in symbol table");


XL_BEGIN

// ============================================================================
//
//   Context: Representation of execution context
//
// ============================================================================

Context::Context()
// ----------------------------------------------------------------------------
//   Constructor for a top-level evaluation context
// ----------------------------------------------------------------------------
    : symbols()
{
    symbols = new Scope();
    record(context, "Created context %p with new symbols %t", this, symbols);
}


Context::Context(Context *parent, TreePosition pos)
// ----------------------------------------------------------------------------
//   Constructor creating a child context in a parent context
// ----------------------------------------------------------------------------
    : symbols(parent->symbols)
{
    CreateScope(pos);
    record(context, "Created context %p from %p with symbols %t parent %t",
           this, parent, symbols, parent->symbols);
}


Context::Context(const Context &source)
// ----------------------------------------------------------------------------
//   Constructor for a top-level evaluation context
// ----------------------------------------------------------------------------
    : symbols(source.symbols)
{
    record(context, "Copy context %p into %p symbols %t",
           this, &source, symbols);
}


Context::Context(Scope *symbols)
// ----------------------------------------------------------------------------
//   Constructor from a known symbol table
// ----------------------------------------------------------------------------
    : symbols(symbols)
{
    record(context, "Create context %p from symbols %t", this, symbols);
}


Context::~Context()
// ----------------------------------------------------------------------------
//   Destructor for execution context
// ----------------------------------------------------------------------------
{
    record(context, "Destroyed context %p");
}



// ============================================================================
//
//    High-level evaluation functions
//
// ============================================================================

Scope *Context::CreateScope(TreePosition pos)
// ----------------------------------------------------------------------------
//    Add a local scope to the current context
// ----------------------------------------------------------------------------
{
    symbols = new Scope(new Scopes(symbols, new Scope()), pos);
    record(context, "In context %p created scope %t", this, symbols);
    return symbols;
}


void Context::PopScope()
// ----------------------------------------------------------------------------
//   Remove the innermost local scope
// ----------------------------------------------------------------------------
{
    Scope *enclosing = symbols->Enclosing();
    record(context, "In context %p popped scope %t enclosing %t",
           this, symbols, enclosing);
    XL_ASSERT(enclosing && "Context::PopScope called on top scope");
    symbols = enclosing;
}


Context *Context::Parent()
// ----------------------------------------------------------------------------
//   Return the parent context
// ----------------------------------------------------------------------------
{
    Scope *psyms = symbols->Enclosing();
    record(context, "In context %p parent from enclosing symbols %t",
           this, psyms);
    if (psyms)
        return new Context(psyms);
    return nullptr;
}



// ============================================================================
//
//    Entering symbols
//
// ============================================================================

bool Context::ProcessDeclarations(Tree *what, RewriteList &inits)
// ----------------------------------------------------------------------------
//   Process all declarations, return true if there are instructions
// ----------------------------------------------------------------------------
{
    Tree_p next   = nullptr;
    bool   result = false;

    record(context, "In %p process declarations for %t", this, what);
    while (Block *block = what->AsBlock())
        what = block->child;

    while (what)
    {
        bool isInstruction = true;
        if (Infix *infix = what->AsInfix())
        {
            if (IsDefinition(infix))
            {
                record(context, "In %p enter declaration %t", this, infix);
                Enter(infix, inits);
                isInstruction = false;
            }
            else if (IsSequence(infix))
            {
                // Chain of declarations, avoiding recursing if possible.
                if (Infix *left = infix->left->AsInfix())
                {
                    isInstruction = false;
                    if (IsDefinition(left))
                        Enter(left, inits);
                    else
                        isInstruction = ProcessDeclarations(left, inits);
                }
                next = infix->right;
            }
        }
        else if (Prefix *prefix = what->AsPrefix())
        {
            if (IsDeclaration(prefix))
            {
                CDeclaration *pcd = new CDeclaration;
                Infix *normalForm = pcd->Declaration(prefix->right);
                record(context, "C: %t is XL: %t", prefix, normalForm);
                if (normalForm)
                {
                    // Process C declarations only in optimized mode
                    Define(normalForm->left, normalForm->right);
                    prefix->SetInfo<CDeclaration>(pcd);
                    isInstruction = false;
                }
                else
                {
                    delete pcd;
                }
            }

            // Check if this prefix is some [import X.Y.Z] statement
            if (Name *import = prefix->left->AsName())
                if (eval_fn callback = MAIN->Declarator(import->value))
                    callback(symbols, prefix);
        }

        // Check if we see instructions
        result |= isInstruction;

        // Consider next in chain
        what = next;
        next = nullptr;
    }
    if (RECORDER_TRACE(symbols_sort))
        Dump(std::cerr, symbols, false);
    return result;
}


Scope *Context::ProcessScope(Tree *declarations, RewriteList &inits)
// ----------------------------------------------------------------------------
//   If 'declarations' contains only declarations, return scope for them
// ----------------------------------------------------------------------------
{
    // If the left is already identified as a scope, return that
    if (Scope *scope = declarations->As<Scope>())
        return scope;

    // For blocks, process declarations and return scope if any
    if (Block *block = declarations->AsBlock())
    {
        Context child(this);
        RewriteList inits;
        if (!child.ProcessDeclarations(block->child, inits))
            return child.Symbols();
    }
    return nullptr;
}


Tree *Context::ValidatePattern(Tree *pattern)
// ----------------------------------------------------------------------------
//   Check that we have only valid names in the pattern, evaluate metaboxen
// ----------------------------------------------------------------------------
{
    switch(pattern->Kind())
    {
    case NATURAL:
    case REAL:
    case TEXT:
        break;
    case NAME:
    {
        Name *name = (Name *) pattern;
        if (name->value.length() && !isalpha(name->value[0]))
            Ooops("The pattern variable $1 is not a name", name);
        break;
    }
    case INFIX:
    {
        Infix *infix = (Infix *) pattern;
        Tree *left = ValidatePattern(infix->left);
        Tree *right = ValidatePattern(infix->right);
        if (left != infix->left || right != infix->right)
            pattern = new Infix(infix, left, right);
        break;
    }
    case PREFIX:
    {
        Prefix *prefix = (Prefix *) pattern;
        Tree *left = prefix->left;
        if (prefix->left->Kind() != NAME)
            left = ValidatePattern(prefix->left);
        Tree *right = ValidatePattern(prefix->right);
        if (left != prefix->left || right != prefix->right)
            pattern = new Prefix(prefix, left, right);
        break;
    }
    case POSTFIX:
    {
        Postfix *postfix = (Postfix *) pattern;
        Tree *right = postfix->right;
        if (postfix->right->Kind() != NAME)
            right = ValidatePattern(postfix->right);
        Tree *left = ValidatePattern(postfix->left);
        if (left != postfix->left || right != postfix->right)
            pattern = new Postfix(postfix, left, right);
        break;
    }
    case BLOCK:
    {
        Block *block = (Block *) pattern;
        if (!block->IsMetaBox())
        {
            Tree *child = ValidatePattern(block->child);
            if (child != block-> child)
                pattern = child;
        }
        break;
    }
    }
    return pattern;
}


Rewrite *Context::Define(Tree *pattern, Tree *definition, bool overwrite)
// ----------------------------------------------------------------------------
//   Enter a rewrite in the current context
// ----------------------------------------------------------------------------
{
    Rewrite_p rewrite = new Rewrite(pattern, definition);
    return Enter(rewrite, overwrite);
}


Rewrite *Context::Define(text spelling, Tree *value, bool overwrite)
// ----------------------------------------------------------------------------
//   Enter a rewrite in the current context
// ----------------------------------------------------------------------------
{
    text name = Normalize(spelling);
    Name *pattern = new Name(name, spelling, value->Position());
    return Define(pattern, value, overwrite);
}


template<typename T> int Sort(T x, T y)
// ----------------------------------------------------------------------------
//   Return sort order between the two values
// ----------------------------------------------------------------------------
{
    return x < y ? -1 : x > y ? 1 : 0;
}


enum SortMode
// ----------------------------------------------------------------------------
//   Indicate how we process the tree during sorting
// ----------------------------------------------------------------------------
{
    INSERT,                     // Sort for insertion of a pattern
    INSERT_BIND,                // Sort for insertion, all names identical
    INSERT_INFIX,               // Sort for insertion, matched infix
    SEARCH,                     // Sort for search of top pattern
    SEARCH_BIND,                // Sort for search, all names identical
    SEARCH_INFIX,               // Sort for search, matched top infix
    TYPE,                       // Sort for type annotations
    EXPRESSION                  // Sort for expression (when clauses)
};


inline SortMode SortIgnoreNames(SortMode mode)
// ----------------------------------------------------------------------------
//   Once we have recognized a top-level name, ignore other names
// ----------------------------------------------------------------------------
//   This is so that [cos X] < [sin X] but [cos X] = [cos Y]
{
    if (mode == INSERT)
        mode = INSERT_BIND;
    else if (mode == SEARCH)
        mode = SEARCH_BIND;
    return mode;
}


inline SortMode SortIgnoreNamesInInfix(SortMode mode)
// ----------------------------------------------------------------------------
//   Once we have recognized a top-level name, ignore other names
// ----------------------------------------------------------------------------
//   This is so that [cos X] < [sin X] but [cos X] = [cos Y]
{
    if (mode == INSERT)
        mode = INSERT_INFIX;
    else if (mode == SEARCH)
        mode = SEARCH_INFIX;
    return mode;
}


inline SortMode SortCheckNamesInInfix(SortMode mode)
// ----------------------------------------------------------------------------
//   Once we have recognized a top-level name, ignore other names
// ----------------------------------------------------------------------------
//   This is so that [cos X] < [sin X] but [cos X] = [cos Y]
{
    if (mode == INSERT_INFIX)
        mode = INSERT;
    else if (mode == SEARCH_INFIX)
        mode = SEARCH;
    return mode;
}


inline bool SortNames(SortMode mode)
// ----------------------------------------------------------------------------
//   Return true if we want to sort names in this mode
// ----------------------------------------------------------------------------
{
    return
        mode != INSERT_BIND &&
        mode != INSERT_INFIX &&
        mode != SEARCH_BIND &&
        mode != SEARCH_INFIX;
}


inline bool SortInserting(SortMode mode)
// ----------------------------------------------------------------------------
//   Return true if we are inserting
// ----------------------------------------------------------------------------
{
    return mode == INSERT || mode == INSERT_BIND || mode == INSERT_INFIX;
}


inline bool SortSearching(SortMode mode)
// ----------------------------------------------------------------------------
//   Return true if we are searching
// ----------------------------------------------------------------------------
{
    return mode == SEARCH || mode == SEARCH_BIND || mode == SEARCH_INFIX;
}


static int ISort(Tree *pat, Tree *val, SortMode mode);
static int Sort(Tree *pat, Tree *val, SortMode mode)
// ----------------------------------------------------------------------------
//   For instrumentation purpose
// ----------------------------------------------------------------------------
{
    int result = ISort(pat, val, mode);
    static const char *modename[] =
    {
        "INSERT",
        "INSERT_BIND",
        "INSERT_INFIX",
        "SEARCH",
        "SEARCH_BIND",
        "SEARCH_INFIX",
        "TYPE",
        "EXPRESSION"
    };
    record(symbols_sort, "%+12s %t %+s %t",
           modename[mode], pat, result<0 ? "<" : result>0 ? ">" : "=", val);
    return result;
}


static int ISort(Tree *pat, Tree *val, SortMode mode)
// ----------------------------------------------------------------------------
//   Return the sorting order while inserting patterns
// ----------------------------------------------------------------------------
//   The sort order implements the "largest and more specialized first" order
//   required for the XL semantics. In other words, a pattern precedes another
//   if it should be considered first during lookup.
//
//   For example:
//      [X,Y] < [X] because it's larger
//      [0] < [X] because it's a more specialized case
//      [X] < [lambda X] when we match [X], because it's more specialized
//
//   The sorting order while inserting a pattern considers the whole pattern,
//   and does not attempt to interpret it, except for blocks that are ignored.
//   Therefore, the sorting order should be:
//       Infix < Prefix < Postfix < Natural < Real < Text < Name
//   This order is chosen with the assumption that it's not unlikely to add
//   implicit conversions between Natural and Real, so that 0 is more
//   specialized than 0.0.
//   Within a given category, items are sorted based on value, so that
//   [A+B] precedes [A<B], and [cos X] precedes [sin Y]. However, named
//   parameters are all considered the same if sortNames == false.
{
    kind patk = pat->Kind();
    kind valk = val->Kind();

    // Eliminate blocks
    if (patk == BLOCK)
    {
        Block *patb = pat->AsBlock();
        if (Tree *patm = patb->IsMetaBox())
        {
            if (SortSearching(mode))
                return 0;
            if (SortInserting(mode))
                return valk == NAME ? -1 : Sort(patm, val, EXPRESSION);
            if (valk == BLOCK)
            {
                Block *valb = (Block *) val;
                if (Tree *valm = valb->IsMetaBox())
                    return Sort(patm, valm, EXPRESSION);
            }
        }
        else
        {
            return Sort(patb->child, val, mode);
        }
    }
    if (valk == BLOCK)
    {
        Block *valb = val->AsBlock();
        // A metabox comes before names, is
        if (Tree *valm = valb->IsMetaBox())
            if (SortInserting(mode))
                return patk == NAME ? 1 : Sort(pat, valm, EXPRESSION);
        return Sort(pat, valb->child, mode);
    }
    if (patk == PREFIX)
    {
        Prefix *patp = (Prefix *) pat;

        // Put lambda behind anything but other lambdas and parameter names
        if (IsLambda(patp))
        {
            if (IsLambda(val))
                return 0;
            if (valk == NAME)
                return SortNames(mode) ? 1 : 0;
            return 1;
        }
    }
    if (valk == PREFIX)
    {
        Prefix *valp = (Prefix *) val;

        // Put lambdas last
        if (IsLambda(valp))
        {
            if (patk == NAME)
                return SortNames(mode) ? -1 : 0;
            return -1;
        }
    }

    // Check case where kinds are different: use sorting rank described above
    if (mode == SEARCH_BIND || mode == SEARCH_INFIX)
    {
        // Catch wildcards, and consider that binding is "upper" problem
        return 0;
    }
    if (patk == INFIX)
    {
        Infix *pati = (Infix *) pat;
        if (IsTypeAnnotation(pati))
        {
            if (SortInserting(mode))
            {
                // Inserting [X + Y as integer] vs. [X * Y as integer]
                if (Infix *vali = IsTypeAnnotation(val))
                {
                    if (int pats = Sort(pati->left, vali->left, mode))
                        return pats;
                    return Sort(pati->right, vali->right, TYPE);
                }
            }
            else if (mode == SEARCH &&
                     IsTypeCastDeclaration(pati) &&
                     IsTypeCast(val))
            {
                // Searching [X+Y as integer] vs [lambda N as integer]
                return Sort(pati->right, ((Infix *) val)->right, TYPE);
            }

            // For expressions and types, sort left first, then right
            if (int pats = Sort(pati->left, val, mode))
                return pats;

            // Same left: if searching, stop here. Otherwise [X:Y] < [X]
            return SortSearching(mode) ? 0 : -1;

        }
        else if (IsPatternCondition(pati))
        {
            if (SortInserting(mode))
            {
                // When inserting, compare guard clauses as well
                if (Infix *vali = IsPatternCondition(val))
                {
                    if (int pats = Sort(pati->left, vali->left, mode))
                        return pats;
                    return Sort(pati->right, vali->right, EXPRESSION);
                }
            }

            // For expressions and types, sort left first, then right
            if (int pats = Sort(pati->left, val, mode))
                return pats;

            // Same left: if searching, stop here. Otherwise [X when Y] < [X]
            return SortSearching(mode) ? 0 : -1;
        }
        else if (IsVariableDefinition(pati))
        {
            // Treat [X:integer := 1] like [X as integer]
            return Sort(pati->left, val, mode);
        }
    }
    if (valk == INFIX)
    {
        Infix *vali = (Infix *) val;

        if (IsTypeAnnotation(vali))
        {
            // The case annotation vs. annotation is handled above.
            // So here, we have [Expr] vs. [X+Y as integer]
            if (SortSearching(mode))
            {
                // Searching [A+B as integer] looks for [lambda N as integer]
                return -1;
            }

            if (int vals = Sort(pat, vali->left, mode))
                return vals;
            return SortSearching(mode) ? 0 : 1;
        }
        else if (IsPatternCondition(vali))
        {
            if (int vals = Sort(val, vali->left, mode))
                return vals;
            return SortSearching(mode) ? 0 : 1;
        }
        else if (SortSearching(mode) && IsVariableDefinition(vali))
        {
            return Sort(pat, vali->left, mode);
        }
    }
    if (patk != valk)
    {
        static const unsigned SortRank[KIND_COUNT] = {
            [NATURAL]   = 3,    // Bogus order due to gcc 'not implemented'
            [REAL]      = 4,
            [TEXT]      = 5,
            [NAME]      = 6,
            [BLOCK]     = 7,
            [PREFIX]    = 1,
            [POSTFIX]   = 2,
            [INFIX]     = 0
        };

        return SortRank[patk] < SortRank[valk] ? -1 : 1;
    }

    // Otherwise, sort by values if possible
    switch(patk)
    {
    case NATURAL:
        return Sort(((Natural *) pat)->value,
                    ((Natural *) val)->value);
    case REAL:
        return Sort(((Real *) pat)->value,
                    ((Real *) val)->value);
    case TEXT:
        return Sort(((Text *) pat)->value,
                    ((Text *) val)->value);
    case NAME:
        return SortNames(mode)
            ? Sort(((Name *) pat)->value,
                   ((Name *) val)->value)
            : 0;
    case BLOCK:
        // Otherwise, sort based on block contents
        return Sort(((Block *) pat)->child,
                    ((Block *) val)->child,
                    mode);
    case PREFIX:
        mode = SortCheckNamesInInfix(mode);
        if (int pats = Sort(((Prefix *) pat)->left,
                            ((Prefix *) val)->left,
                            mode))
            return pats;
        return Sort(((Prefix *) pat)->right,
                    ((Prefix *) val)->right,
                    SortIgnoreNames(mode));
    case POSTFIX:
        if (int vals = Sort(((Postfix *) pat)->right,
                            ((Postfix *) val)->right,
                            mode))
            return vals;
        return Sort(((Postfix *) pat)->left,
                    ((Postfix *) val)->left,
                    SortIgnoreNames(mode));
    case INFIX:
        // Otherwise, compare the operator first
        if (int ns = Sort(((Infix *) pat)->name,
                          ((Infix *) val)->name))
            return ns;
        if (int pats = Sort(((Infix *) pat)->left,
                            ((Infix *) val)->left,
                            SortIgnoreNamesInInfix(mode)))
            return pats;
        return Sort(((Infix *) pat)->right,
                    ((Infix *) val)->right,
                    SortIgnoreNames(mode));
    }

    // Should never happen
    return 0;
}


Rewrite *Context::Enter(Infix *infix, RewriteList &inits)
// ----------------------------------------------------------------------------
//   Make sure we produce a real rewrite in the symbol table
// ----------------------------------------------------------------------------
{
    XL_ASSERT(IsDefinition(infix) && "Context::Enter only accepts definitions");
    Rewrite_p rewrite = new Rewrite(infix);
    rewrite = Enter(rewrite);
    if (IsVariableDefinition(infix))
        inits.push_back(rewrite);
    return rewrite;
}


static inline void Redefined(Tree *pattern, Rewrite *old)
// ----------------------------------------------------------------------------
//   Emit a message if we redefined a patterb
// ----------------------------------------------------------------------------
{
    Ooops("Redefinition of $1", pattern);
    Ooops("Previous definition is $1", old->Pattern());
}


Rewrite *Context::Enter(Rewrite *rewrite, bool overwrite)
// ----------------------------------------------------------------------------
//   Enter a known declaration
// ----------------------------------------------------------------------------
{
    // If the rewrite is not good, just exit
    if (!IsDefinition(rewrite))
    {
        record(symbols_errors, "In %p, malformed rewrite %t", this, rewrite);
        return nullptr;
    }

    // Find pattern from the rewrite
    Tree *pattern = rewrite->Pattern();

    // Validate form names, replace metaboxes, emit errors in case of problem.
    pattern = ValidatePattern(pattern);

    // Find locals symbol table, populate it
    Scope   *scope  = symbols;
    Tree_p  &locals = scope->Locals();

    // If empty scope, insert the rewrite
    if (locals == xl_nil)
    {
        record(symbols, "In %p insert top-level rewrite %t", this, rewrite);
        locals = rewrite;
        return rewrite;
    }

    // Now we process multiple rewrites
    Tree_p  *entry = &locals;
    while (true)
    {
        // Reached single rewrite: decide if we add new one left or right
        if (Rewrite *old = (*entry)->As<Rewrite>())
        {
            int sort = Sort(old->Pattern(), pattern, INSERT);
            record(symbols, "In %p old %t rewrite %t sort %d",
                   this, old, rewrite, sort);
            if (sort < 0)
            {
                *entry = new Rewrites(old, rewrite);
            }
            else if (sort > 0)
            {
                *entry = new Rewrites(rewrite, old);
            }
            else if (overwrite)
            {
                *entry = rewrite;
            }
            else
            {
                Redefined(pattern, old);
            }
            return rewrite;
        }

        // If multiple rewrites, select if we move left or right
        Rewrites *rewrites = (*entry)->As<Rewrites>();
        XL_ASSERT(rewrites && "Malformed symbol table: expected rewrites");

        // Check the payload for this entry
        Rewrite *left = rewrites->Payload();
        XL_ASSERT(left && "Malformed symbol table: expected rewrite");

        // Check if the right item is a single rewrite: 3 items, reorder
        if (Rewrite *right = rewrites->Second())
        {
            int cmpleft = Sort(pattern, left->Pattern(), INSERT);
            if (cmpleft == 0 && !overwrite)
                Redefined(pattern, left);
            if (cmpleft < 0)
            {
                // Here pattern < left < right
                rewrites->right = new Rewrites(rewrite, right);
                return rewrite;
            }

            int cmpright = Sort(pattern, right->Pattern(), INSERT);
            if (cmpright == 0 && !overwrite)
                Redefined(pattern, right);
            if (cmpright < 0)
            {
                // Here left <= pattern < right: reorder
                rewrites->left = rewrite;
                rewrites->right = new Rewrites(left, right);
                return rewrite;
            }

            // Here, left <= right <= pattern: reorder
            rewrites->left = right;
            rewrites->right = new Rewrites(left, rewrite);
            return rewrite;
        }

        // The right should be a Rewrites as well
        Rewrites *right = rewrites->Children();
        XL_ASSERT(right && "Malformed symbol table: unknown type on the right");

        // Compare with current node to decide if we go left or right
        int cmpleft = Sort(pattern, left->Pattern(), INSERT);
        entry = cmpleft < 0 ? &right->left : &right->right;
    }

    // Never happens
    return nullptr;
}


Tree *Context::Assign(Tree *ref, Tree *value)
// ----------------------------------------------------------------------------
//   Perform an assignment in the given context
// ----------------------------------------------------------------------------
{
    // Check if the reference already exists
    Rewrite *declaration = Reference(ref);
    if (!declaration)
    {
        // The reference does not exist: this is an error
        Ooops("Assigning $1 to unknown reference $2", value, ref);
        return value;
    }

    // Check if the declaration has a type, i.e. it is 'X as natural'
    if (Tree *type = AnnotatedType(declaration->left))
    {
        Scope *scope = Symbols();
        Tree *cast = xl_typecheck(scope, type, value);
        if (!cast)
        {
            Ooops("New value $1 does not match type $2", value, type);
            Ooops("required from declaration $1", declaration);
            return declaration->right;
        }

        value = cast;
    }

    // Update existing value with assigned value
    declaration->right = value;

    // Return evaluated assigned value
    return value;
}



// ============================================================================
//
//    Tree attributes
//
// ============================================================================

// Set and get per-context tree attributes
Tree *Context::Info(text key, Tree *what, bool recurse)
// ----------------------------------------------------------------------------
//    Return the information associated with the given attribute
// ----------------------------------------------------------------------------
{
    std::ostringstream out;
    out << key << "@" << (void *) what;
    return Named(out.str(), recurse);
}


Rewrite *Context::SetInfo(text key, Tree *what, Tree *value)
// ----------------------------------------------------------------------------
//   Set the information associated with the tree
// ----------------------------------------------------------------------------
{
    std::ostringstream out;
    out << key << "@" << (void *) what;
    return Define(out.str(), value, false);
}


Tree *Context::Type(Tree *what)
// ----------------------------------------------------------------------------
//   Return the type associated with the value
// ----------------------------------------------------------------------------
{
    return Info("type", what);
}


Rewrite *Context::SetType(Tree *what, Tree *type)
// ----------------------------------------------------------------------------
//   Set the type associated with the value
// ----------------------------------------------------------------------------
{
    return SetInfo("type", what, type);
}



// ============================================================================
//
//    Context attributes
//
// ============================================================================

Rewrite *Context::SetAttribute(text attribute, Tree *value, bool owr)
// ----------------------------------------------------------------------------
//   Set an attribute for the innermost context
// ----------------------------------------------------------------------------
{
    return Define(attribute, value, owr);
}


Rewrite *Context::SetAttribute(text attribute, longlong value, bool owr)
// ----------------------------------------------------------------------------
//   Set an attribute for the innermost context
// ----------------------------------------------------------------------------
{
    return SetAttribute(attribute,new Natural(value,symbols->Position()),owr);
}


Rewrite *Context::SetAttribute(text attribute, double value, bool owr)
// ----------------------------------------------------------------------------
//   Set the override priority for the innermost scope in the context
// ----------------------------------------------------------------------------
{
    return Define(attribute, new Real(value, symbols->Position()), owr);
}


Rewrite *Context::SetAttribute(text attribute, text value, bool owr)
// ----------------------------------------------------------------------------
//   Set the file name the innermost scope in the context
// ----------------------------------------------------------------------------
{
    return Define(attribute, new Text(value, symbols->Position()), owr);
}



// ============================================================================
//
//    Looking up symbols
//
// ============================================================================

static Tree *LookupEntry(Scope              *symbols,
                         Scope              *scope,
                         Tree_p             *entry,
                         Tree               *what,
                         Context::lookup_fn  lookup,
                         void               *info)
// ----------------------------------------------------------------------------
//   Internal lookup helper
// ----------------------------------------------------------------------------
{
    // If we have found a nil or empty spot, done with current scope
    if (*entry == xl_nil)
        return nullptr;
    if (Name *name = (*entry)->AsName())
        if (name->value == "")
            return nullptr;

    while (true)
    {
        // Quick exit in case we are dropping evaluation
        if (Errors::Aborting())
            return nullptr;

        // If we have a definition in this entry, check it
        if (Rewrite *rewrite = (*entry)->As<Rewrite>())
        {
            int cmp = Sort(rewrite->Pattern(), what, SEARCH);
            if (cmp == 0)
            {
                Tree *result = lookup(symbols, scope, what, rewrite, info);
                record(symbols_lookup, "entry %t = %t",
                       rewrite->Pattern(), result);
                if (result)
                    return result;
            }
            return nullptr;
        }

        // Check rewrites at that level
        Rewrites *rewrites = (*entry)->As<Rewrites>();
        XL_ASSERT(rewrites && "Missing Rewrites entry during lookup");

        // Check payload in the rewrites
        Rewrite *left = rewrites->Payload();
        XL_ASSERT(left && "Missing Rewrite payload during lookup");

        // Check how we compare to the current entry
        int cmpleft = Sort(left->Pattern(), what, SEARCH);

        // If right is a rewrite, test left then right
        if (rewrites->Second())
        {
            // left <= right, matching left: test left then right
            if (cmpleft == 0)
            {
                Tree *result = lookup(symbols, scope, what, left, info);
                record(symbols_lookup, "left %t = %t", left->Pattern(), result);
                if (result)
                    return result;
            }
            entry = &rewrites->right;
            continue;
        }

        // Multiple rewrites, sorted as right->left <= left <= right->right
        Rewrites *right = rewrites->Children();
        XL_ASSERT(right && "Missing right Rewrites during lookup");

        // If we need to explore only left or right branch
        if (cmpleft)
        {
            entry = cmpleft > 0 ? &right->left : &right->right;
            continue;
        }

        // Need to explore left branch first
        if (Tree *result = LookupEntry(symbols, scope,
                                       &right->left, what, lookup, info))
            return result;

        // Since payload matches, need to check it too
        Tree *result = lookup(symbols, scope, what, left, info);
        record(symbols_lookup, "mid %t = %t", left->Pattern(), result);
        if (result)
            return result;

        // Then need to check right branch
        entry = &right->right;
    }
}


Tree *Context::Lookup(Tree *what, lookup_fn lookup, void *info, bool recurse)
// ----------------------------------------------------------------------------
//   Lookup a tree using the given lookup function
// ----------------------------------------------------------------------------
{
    Scope * scope = symbols;
    record(symbols_lookup, "Looking up %t in %t", what, scope);
    while (scope)
    {
        // Initialize local scope
        Tree_p &locals = scope->Locals();
        if (Tree *result = LookupEntry(symbols, scope,
                                       &locals, what, lookup, info))
            return result;

        // Not found in this scope. Keep going with next scope if recursing
        // The last top-level global will be nil, so we will end with scope=NULL
        scope = recurse ? scope->Enclosing() : nullptr;
    }

    // Return NULL if all evaluations failed
    return nullptr;
}


static Tree *findReference(Scope *, Scope *, Tree *what, Rewrite *decl, void *)
// ----------------------------------------------------------------------------
//   Return the reference we found
// ----------------------------------------------------------------------------
{
    return decl;
}


Rewrite *Context::Reference(Tree *pattern, bool recurse)
// ----------------------------------------------------------------------------
//   Find an existing definition in the symbol table that matches the pattern
// ----------------------------------------------------------------------------
{
    if (Tree *result = Lookup(pattern, findReference, nullptr, recurse))
        if (Rewrite *decl = result->As<Rewrite>())
            return decl;
    return nullptr;
}


Tree *Context::DeclaredPattern(Tree *pattern)
// ----------------------------------------------------------------------------
//   Find the original declaration associated to a given form
// ----------------------------------------------------------------------------
{
    if (Tree *result = Lookup(pattern, findReference, nullptr, true))
        if (Rewrite *decl = result->As<Rewrite>())
            return PatternBase(decl->left);
    return nullptr;
}


static Tree *findValue(Scope *, Scope *, Tree *what, Rewrite *decl, void *info)
// ----------------------------------------------------------------------------
//   Return the value bound to a given pattern
// ----------------------------------------------------------------------------
{
    if (what->IsLeaf())
        if (!Tree::Equal(what, PatternBase(decl->left)))
            return nullptr;
    return decl->right;
}


static Tree *findValueX(Scope *, Scope *scope,
                        Tree *what, Rewrite *decl, void *info)
// ----------------------------------------------------------------------------
//   Return the value bound to a given pattern, as well as its scope and decl
// ----------------------------------------------------------------------------
{
    if (what->IsLeaf())
        if (!Tree::Equal(what, PatternBase(decl->left)))
            return nullptr;
    Prefix *rewriteInfo = (Prefix *) info;
    rewriteInfo->left = scope;
    rewriteInfo->right = decl;
    return decl->right;
}


Tree *Context::Bound(Tree *pattern, bool recurse)
// ----------------------------------------------------------------------------
//   Return the value bound to a given declaration
// ----------------------------------------------------------------------------
{
    Tree *result = Lookup(pattern, findValue, nullptr, recurse);
    return result;
}


Tree *Context::Bound(Tree *pattern,
                     bool recurse,
                     Rewrite_p *rewrite,
                     Scope_p *ctx)
// ----------------------------------------------------------------------------
//   Return the value bound to a given declaration
// ----------------------------------------------------------------------------
{
    Prefix info(nullptr, nullptr);
    Tree *result = Lookup(pattern, findValueX, &info, recurse);
    if (ctx)
        *ctx = info.left->As<Scope>();
    if (rewrite)
        *rewrite = info.right->As<Rewrite>();
    return result;
}


Tree *Context::Named(text name, bool recurse)
// ----------------------------------------------------------------------------
//   Return the value bound to a given name
// ----------------------------------------------------------------------------
{
    Name nameTree(name);
    return Bound(&nameTree, recurse);
}


bool Context::IsEmpty()
// ----------------------------------------------------------------------------
//   Return true if the context has no declaration inside
// ----------------------------------------------------------------------------
{
    return symbols->Inner()->child == xl_nil;
}


static ulong listNames(Tree_p *entry, text begin, RewriteList &list, bool pfx)
// ----------------------------------------------------------------------------
//   List names in the given tree
// ----------------------------------------------------------------------------
{
    ulong count = 0;
    while (entry)
    {
        if (Rewrite *rewrite = (*entry)->As<Rewrite>())
        {
            Tree *declared = rewrite->left;
            Name *name = declared->AsName();
            if (!name && pfx)
            {
                Prefix *prefix = declared->AsPrefix();
                if (prefix)
                    name = prefix->left->AsName();
            }
            if (name && name->value.find(begin) == 0)
            {
                list.push_back(rewrite);
                count++;
            }
            entry = nullptr;
        }
        else if (Rewrites *rewrites = (*entry)->As<Rewrites>())
        {
            count += listNames(&rewrites->left, begin, list, pfx);
            entry = &rewrites->right;
        }
        else
        {
            entry = nullptr;
        }
    }
    return count;
}


ulong Context::ListNames(text begin, RewriteList &list,
                         bool recurse, bool includePrefixes)
// ----------------------------------------------------------------------------
//    List names in a context
// ----------------------------------------------------------------------------
{
    Scope *scope = symbols;
    ulong count = 0;
    while (scope)
    {
        Tree_p &locals = scope->Locals();
        count += listNames(&locals, begin, list, includePrefixes);
        scope = recurse ? scope->Enclosing() : nullptr;
    }
    return count;
}



// ============================================================================
//
//    Utility functions
//
// ============================================================================

void Context::Clear()
// ----------------------------------------------------------------------------
//   Clear the symbol table
// ----------------------------------------------------------------------------
{
    symbols->child = xl_nil;
}


void Context::Dump(std::ostream &out, Scope *scope, bool recurse)
// ----------------------------------------------------------------------------
//   Dump the symbol table to the given stream
// ----------------------------------------------------------------------------
{
    while (scope)
    {
        Scope *parent = scope->Enclosing();
        Tree_p &locals = scope->Locals();
        Dump(out, &locals);
        if (parent)
            out << "// Parent " << (void *) parent << "\n";
        scope = recurse ? parent : nullptr;
    }
}


void Context::Dump(std::ostream &out, Tree_p *entry)
// ----------------------------------------------------------------------------
//   Dump the symbol table to the given stream
// ----------------------------------------------------------------------------
{
    while (entry)
    {
        if (Rewrite *rewrite = (*entry)->As<Rewrite>())
        {
            out << rewrite << "\n";
            entry = nullptr;
        }
        else if (Rewrites *rewrites = (*entry)->As<Rewrites>())
        {
            if (Rewrites *right = rewrites->Children())
            {
                Dump(out, &right->left);
                out << rewrites->left << "\n";
                entry = &right->right;
            }
            else
            {
                out << rewrites << "\n";
                entry = nullptr;
            }
        }
        else
        {
            out << "Empty scope: " << *entry << "\n";
            entry = nullptr;
        }
    }
}


XL_END

extern XL::Tree *xldebug(XL::Tree *tree);

XL::Scope *xldebug(XL::Scope *scope)
// ----------------------------------------------------------------------------
//    Helper to show a global scope in a symbol table
// ----------------------------------------------------------------------------
{
    if (XL::Allocator<XL::Scope>::IsAllocated(scope))
    {
        XL::Context::Dump(std::cerr, scope, false);
        scope = scope->Enclosing();
    }
    else
    {
        std::cerr << "Cowardly refusing to render unknown scope pointer "
                  << (void *) scope << "\n";
    }
    return scope;
}


XL::Scope *xldebug(XL::Closure *closure)
// ----------------------------------------------------------------------------
//    Helper to show a global scope in a symbol table
// ----------------------------------------------------------------------------
{
    if (XL::Allocator<XL::Closure>::IsAllocated(closure))
    {
        XL::Scope *scope = closure->CapturedScope();
        std::cerr << "Closure scope " << (void *) scope << ":\n";
        XL::Context::Dump(std::cerr, scope, false);

        std::cerr << "Closure value: " << closure->Value() << "\n";
        return scope->Enclosing();
    }
    else
    {
        std::cerr << "Cowardly refusing to render unknown closure pointer "
                  << (void *) closure << "\n";
    }
    return nullptr;
}


XL::Scopes *xldebug(XL::Scopes *scopes)
// ----------------------------------------------------------------------------
//    Helper to show a global scopes in a symbol table
// ----------------------------------------------------------------------------
{
    if (XL::Allocator<XL::Scope>::IsAllocated(scopes))
    {
        XL::Context::Dump(std::cerr, scopes->left->As<XL::Scope>(), true);
        XL::Context::Dump(std::cerr, scopes->right->As<XL::Scope>(), true);
    }
    else
    {
        std::cerr << "Cowardly refusing to render unknown scope pointer "
                  << (void *) scopes << "\n";
    }
    return scopes;
}


XL::Rewrite *xldebug(XL::Rewrite *rw)
// ----------------------------------------------------------------------------
//   Helper to show a rewrite for debugging purpose
// ----------------------------------------------------------------------------
//   The infix can be shown using debug(), but it's less convenient
{
    if (XL::Allocator<XL::Rewrite>::IsAllocated(rw))
    {
        std::cerr << rw << "\n";
    }
    else
    {
        std::cerr << "Cowardly refusing to render unknown rewrite pointer "
                  << (void *) rw << "\n";
    }
    return rw;
}


XL::Rewrites *xldebug(XL::Rewrites *rws)
// ----------------------------------------------------------------------------
//   Helper to show a rewrite for debugging purpose
// ----------------------------------------------------------------------------
//   The infix can be shown using debug(), but it's less convenient
{
    if (XL::Allocator<XL::Rewrites>::IsAllocated(rws))
    {
        std::cerr << rws << "\n";
    }
    else
    {
        std::cerr << "Cowardly refusing to render unknown rewrites pointer "
                  << (void *) rws << "\n";
    }
    return rws;
}


XL::Context *xldebug(XL::Context *context)
// ----------------------------------------------------------------------------
//   Helper to show a context for debugging purpose
// ----------------------------------------------------------------------------
{
    if (XL::Allocator<XL::Context>::IsAllocated(context))
    {
        XL::Scope *scope = context->Symbols();
        xldebug(scope);
        return context;
    }
    else
    {
        std::cerr << "Cowardly refusing to render unknown context pointer "
                  << (void *) context << "\n";
        return nullptr;
    }
}
XL::Context *xldebug(XL::Context_p c)
{
    return xldebug((XL::Context *) c);
}
