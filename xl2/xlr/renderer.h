#ifndef RENDERER_H
#define RENDERER_H
// ****************************************************************************
//  renderer.h                      (C) 1992-2009 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//     Rendering of XL trees
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
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

#include "base.h"
#include "tree.h"
#include <ostream>

XL_BEGIN

class Syntax;
typedef std::map<text,Tree_p>    formats_table;
typedef std::map<Tree_p,text>    highlights_table;


struct Renderer
// ----------------------------------------------------------------------------
//   Render a tree to some ostream
// ----------------------------------------------------------------------------
{
    // Construction
    Renderer(std::ostream &out, text styleFile, Syntax &stx);
    Renderer(std::ostream &out, Renderer *from = renderer);

    // Selecting the style sheet file
    void                SelectStyleSheet(text styleFile,
                                         text syntaxFile = "xl.syntax");

    // Rendering proper
    void                Render (Tree *what);
    void                RenderOne(Tree *what);
    void                DoRenderOne(Tree *what);
    void                RenderText(text format);
    void                RenderFormat(Tree *format);
    void                RenderFormat(text self, text format);
    void                RenderFormat(text self, text format, text generic);
    void                RenderFormat(text self, text f, text g1, text g2);
    Tree *              ImplicitBlock(Tree *t);
    bool                IsAmbiguousPrefix(Tree *test, bool testL, bool testR);
    bool                IsSubFunctionInfix(Tree *t);
    int                 InfixPriority(Tree *test);

    std::ostream &      output;
    Syntax &            syntax;
    formats_table       formats;
    highlights_table    highlights;
    uint                indent;
    text                self;
    Tree_p              left;
    Tree_p              right;
    text                current_quote;
    int                 priority;
    bool                had_space;
    bool                had_punctuation;
    bool                need_separator;

    static Renderer *   renderer;
};

std::ostream& operator<< (std::ostream&out, XL::Tree *t);

XL_END

// For use in a debugger
extern "C" void debug(XL::Tree *);
extern "C" void debugp(XL::Tree *);

#endif // RENDERER_H
