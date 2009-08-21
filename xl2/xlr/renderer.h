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
typedef std::map<text,Tree*>    formats_table;


struct Renderer
// ----------------------------------------------------------------------------
//   Render a tree to some ostream
// ----------------------------------------------------------------------------
{
    // Construction
    Renderer(std::ostream &out, text styleFile, Syntax &stx);
    Renderer(std::ostream &out, Renderer *from = renderer);

    // Selecting the style sheet file
    void                SelectStyleSheet(text styleFile);

    // Rendering proper
    void                Render (Tree *what);
    void                RenderOne(Tree *what);
    void                RenderText(text format);
    void                RenderFormat(Tree *format);
    void                RenderFormat(text self, text format);
    void                RenderFormat(text self, text format, text generic);
    void                RenderFormat(text self, text f, text g1, text g2);
    Tree *              ImplicitBlock(Tree *t);
    bool                IsAmbiguousPrefix(Tree *test, bool testL, bool testR);
    bool                IsSubFunctionInfix(Tree *t);
    int                 InfixPriority(Tree *test);

    static Renderer *   renderer;

    std::ostream &      output;
    Syntax &            syntax;
    formats_table       formats;
    uint                indent;
    text                self;
    Tree *              left;
    Tree *              right;
    text                current_quote;
    int                 priority;
    bool                had_space;
    bool                had_punctuation;
    bool                need_separator;
    bool                force_parentheses;
};

XL_END

extern std::ostream& operator<< (std::ostream&out, XL::Tree *t);

// For use in a debugger
extern "C" void debug(XL::Tree *);
extern "C" void debugp(XL::Tree *);
extern "C" void debugc(XL::Tree *);

#endif // RENDERER_H
