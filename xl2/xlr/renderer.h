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


struct Renderer : Action
// ----------------------------------------------------------------------------
//   Render a tree to some ostream
// ----------------------------------------------------------------------------
{
    Renderer(std::ostream &out, text styleFile, Syntax &stx, uint ts = 4);
    Renderer(std::ostream &out, Renderer *from = renderer);

    virtual Tree *      Do(Tree *what);
    virtual Tree *      DoInteger(Integer *what);
    virtual Tree *      DoReal(Real *what);
    virtual Tree *      DoText(Text *what);
    virtual Tree *      DoName(Name *what);
    virtual Tree *      DoPrefix(Prefix *what);
    virtual Tree *      DoPostfix(Postfix *what);
    virtual Tree *      DoInfix(Infix *what);
    virtual Tree *      DoBlock(Block *what);
    virtual Tree *      DoNative(Native *what);
    void                Indent(text t);

    static Renderer *   renderer;

    std::ostream &      output;
    Syntax &            syntax;
    formats_table       formats;
    uint                indent;
    uint                tabsize;
    kstring             need_space;
};

XL_END

extern std::ostream& operator<< (std::ostream&out, XL::Tree *t);

// For use in a debugger
extern "C" void debug(XL::Tree *);

#endif // RENDERER_H
