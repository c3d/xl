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
// 
// 
// 
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

#include <map>
#include <set>
#include <vector>
#include "base.h"

XL_BEGIN

struct Tree;
struct Variable;
struct Action;
struct Context;
struct Stack;
struct Errors;
struct Rewrite;

typedef std::map<text, Tree *>    symbol_table;
typedef std::set<Tree *>          active_set;
typedef std::map<ulong, Rewrite*> rewrite_table;
typedef std::vector<Tree *>       value_list;
typedef std::vector<ulong>        frame_list;
typedef std::map<Tree *, Tree *>  compile_cache;


struct Namespace
// ----------------------------------------------------------------------------
//   Holds the symbols in a given context
// ----------------------------------------------------------------------------
{
    Namespace(Namespace *p): parent(p), rewrites(NULL), numVars(0) {}
    ~Namespace();

    Namespace *         Parent()                { return parent; }

    // Symbol management
    Tree *              NamedTree (text name)
    {
        return names.count(name) > 0 ? names[name] : NULL;
    }
    Rewrite *           Rewrites()              { return rewrites; }

    // Entering symbols in the symbol table
    void                EnterName (text name, Tree *value);
    Rewrite *           EnterRewrite(Rewrite *r);
    Variable *          AllocateVariable (text name, ulong treepos);

    // Clearing symbol tables
    void                Clear();

public:
    Namespace *         parent;
    symbol_table        names;
    Rewrite *           rewrites;
    ulong               numVars;
};


struct Context : Namespace
// ----------------------------------------------------------------------------
//   The compilation context in which we evaluate trees
// ----------------------------------------------------------------------------
{
    // Constructors and destructors
    Context(Errors &err):
        Namespace(NULL),
        errors(err), gc_threshold(200),
        error_handler(NULL), run_stack(NULL), compiled() {}
    Context(Context *p):
        Namespace(p),
        errors(p->errors), gc_threshold(200),
        error_handler(NULL), run_stack(NULL), compiled() {}

    // Context properties
    Context *           Parent()                 { return (Context *) parent;}
    Tree *              ErrorHandler();
    void                SetErrorHandler(Tree *e) { error_handler = e; }
    ulong               Depth();
    
    // Garbage collection
    void                Root(Tree *t)           { roots.insert(t); }
    void                Mark(Tree *t)           { active.insert(t); }
    void                CollectGarbage();

    // Evaluation of trees
    Tree *              Name(text name);
    Tree *              Compile(Tree *source, bool nullIfBad = false);
    Tree *              Run(Tree *source);
    Rewrite *           EnterRewrite(Tree *from, Tree *to);
    Tree *              Error (text message,
                               Tree *a1=NULL, Tree *a2=NULL, Tree *a3=NULL);

public:
    static ulong        gc_increment;
    static ulong        gc_growth_percent;
    static Context *    context;

private:
    Errors &            errors;
    active_set          active;
    active_set          roots;
    ulong               gc_threshold;
    Tree *              error_handler;
    Stack *             run_stack;
    compile_cache       compiled;
};


struct Stack
// ----------------------------------------------------------------------------
//    A runtime stack holding args and locals
// ----------------------------------------------------------------------------
//   Ids of local variables start at 0 and grow up.
//   Ids of temporaries start at -1 and grow down
{
    Stack(Errors &err):
        values(), frames(), frame(~0UL), depth(0),
        error_handler(NULL), errors(err)
    {
        frames.resize(4);       // A good default
    }

    ulong Grow(ulong sz)
    {
        ulong oldFrame = frame;
        values.resize(values.size() + sz);
        frame = values.size() - 1;
        return oldFrame;
    }

    ulong Grow(value_list &args)
    {
        ulong oldFrame = frame;
        values.insert(values.end(), args.begin(), args.end());
        frame = values.size() - 1;
        return oldFrame;
    }

    void Shrink(ulong newFrame, ulong sz)
    {
        frame = newFrame;
        values.resize(values.size() - sz);
    }

    void AllocateLocals(long sz)
    {
        values.resize(values.size() + sz);
    }

    ulong EnterFrame(ulong newDepth)
    {
        if (frames.size() <= depth)
            frames.resize(depth+1);
        ulong oldFrame = frames[depth];
        frames[depth] = frame;
        depth = newDepth;
        return oldFrame;
    }

    void ExitFrame(ulong oldDepth, ulong oldFrame)
    {
        frames[oldDepth] = oldFrame;
        depth = oldDepth;
    }

    Tree *Get(ulong id)
    {
        Tree *value = values[frame - id];
        return value;
    }

    void Set(ulong id, Tree *v)
    {
        values[frame - id] = v;
    }

    Tree *GetLocal(ulong id)
    {
        Tree *value = values[values.size() + ~id];
        return value;
    }

    void SetLocal(ulong id, Tree *v)
    {
        values[values.size() + ~id] = v;
    }

    Tree *Get(long id, ulong depth)
    {
        ulong base = depth ? frames[frames.size() - depth] : frame;
        Tree *value = values[base - id];
        return value;
    }

    void Set(long id, Tree *v, ulong depth)
    {
        ulong base = depth ? frames[frames.size() - depth] : frame;
        values[base - id] = v;
    }

    Tree * Run(Tree *source);
    Tree * Error (text message, Tree *a1=NULL, Tree *a2=NULL, Tree *a3=NULL);

public:
    value_list values;
    frame_list frames;
    ulong      frame;
    ulong      depth;
    Tree *     error_handler;
    Errors &   errors;
};


struct Rewrite
// ----------------------------------------------------------------------------
//   Information about a rewrite, e.g fact N -> N * fact(N-1) 
// ----------------------------------------------------------------------------
{
    Rewrite (Context *c, Tree *f, Tree *t):
        context(c), from(f), to(t), hash() {}
    ~Rewrite();

    Rewrite *           Add (Rewrite *rewrite);
    Tree *              Do(Action &a);
    Tree *              Compile(void);

public:
    Context *           context;
    Tree *              from;
    Tree *              to;
    rewrite_table       hash;
};

XL_END

#endif // CONTEXT_H
