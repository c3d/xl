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

struct Renderer : Action
// ----------------------------------------------------------------------------
//   Render a tree to some ostream
// ----------------------------------------------------------------------------
{
    Renderer(std::ostream &out, uint ts = 4):
        Action(), output(out), indent(0), tabsize(ts), need_space("") {}

    virtual Tree *      Run(Tree *what);
    void                Indent(text t);
    static void         Render(std::ostream &out, Tree *t);

    std::ostream &      output;
    uint                indent;
    uint                tabsize;
    kstring             need_space;
};

XL_END

inline std::ostream& operator<< (std::ostream&out, XL::Tree *t)
// ----------------------------------------------------------------------------
//   Just in case you want to emit a tree using normal ostream interface
// ----------------------------------------------------------------------------
{
    XL::Renderer::Render(out, t);
    return out;
}

// For use in a debugger
extern "C" void debug(XL::Tree *);

#endif // RENDERER_H
