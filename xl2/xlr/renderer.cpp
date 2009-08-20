// ****************************************************************************
//  renderer.cpp                    (C) 1992-2009 Christophe de Dinechin (ddd) 
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

#include "renderer.h"
#include <iostream>
#include <cctype>

XL_BEGIN

void Renderer::Indent(text t)
// ----------------------------------------------------------------------------
//   Emit the given text with proper indentation
// ----------------------------------------------------------------------------
{
    for (text::iterator i = t.begin(); i != t.end(); i++)
    {
        char c = *i;
        if (isspace(c))
            output << c;
        else
            output << need_space << c;
        need_space = "";
        if (c == '\n')
            for (uint i = 0; i < indent; i++)
                output << ' ';
    }
}


Tree *Renderer::Do(Tree *what)
// ----------------------------------------------------------------------------
//   Render the normalized trees
// ----------------------------------------------------------------------------
{
    if (!what)
    {
        output << need_space << "NULL";
    }
    else
    {
        Tree *any = what->Normalize();
        if (any && any != what)
            Do(any);
        else
            output << "DENORM@" << (void *) what;
    }
    return what;
}


Tree *Renderer::DoInteger(Integer *what)
// ----------------------------------------------------------------------------
//    Render integer
// ----------------------------------------------------------------------------
{
    output << need_space << what->value;
    return what;
}


Tree *Renderer::DoReal(Real *what)
// ----------------------------------------------------------------------------
//    Render real
// ----------------------------------------------------------------------------
{
    output << need_space << what->value;
    return what;
}


Tree *Renderer::DoText(Text *what)
// ----------------------------------------------------------------------------
//   Render text
// ----------------------------------------------------------------------------
{
    output << need_space << what->Opening();
    need_space = "";
    Indent (what->value);
    output << what->Closing();
    return what;
}


Tree *Renderer::DoName (Name *what)
// ----------------------------------------------------------------------------
//   Render name
// ----------------------------------------------------------------------------
{
    output << need_space << what->value;
    return what;
}


Tree *Renderer::DoBlock (Block *what)
// ----------------------------------------------------------------------------
//   Render block
// ----------------------------------------------------------------------------
{
    static Block iblk(NULL);
    text open = what->Opening(), close = what->Closing();

    indent += tabsize;
    if (open == iblk.Opening())
        Indent("\n");
    else
        output << open;
    
    what->child->Do(this);
        
    indent -= tabsize;
    if (close == iblk.Closing())
        Indent("\n");
    else
        output << close;

    return what;
}


Tree *Renderer::DoPrefix(Prefix *what)
// ----------------------------------------------------------------------------
//   Render a prefix tree
// ----------------------------------------------------------------------------
{
    what->left->Do(this);
    need_space = " ";
    what->right->Do(this);
    return what;
}


Tree *Renderer::DoPostfix(Postfix *what)
// ----------------------------------------------------------------------------
//   Render a postfix tree
// ----------------------------------------------------------------------------
{
    return DoPrefix(what);
}


Tree *Renderer::DoInfix (Infix *what)
// ----------------------------------------------------------------------------
//   Render an infix tree
// ----------------------------------------------------------------------------
{
    tree_list &list = what->list;
    uint count = list.size() - 1;
    for (tree_list::iterator c = list.begin(); c != list.end(); c++)
    {
        (*c)->Do(this);
        if (count--)
        {
            if (what->name == "\n" || what->name == "." || what->name == ",")
                need_space = "";
            else
                need_space = " ";
            Indent(what->name);
            if (what->name == "\n" || what->name == ".")
                need_space = "";
            else
                need_space = " ";
        }
    }
    return what;
}


Tree *Renderer::DoNative(Native *what)
// ----------------------------------------------------------------------------
//   What does it mean to render a native tree?
// ----------------------------------------------------------------------------
{
    output << "NATIVE@" << (void *) what;
    return what;
}


void Renderer::Render(std::ostream &out, Tree *t)
// ----------------------------------------------------------------------------
//   Render a tree
// ----------------------------------------------------------------------------
{
    Renderer renderer(out);
    t->Do(&renderer);
}


XL_END

void debug(XL::Tree *tree)
// ----------------------------------------------------------------------------
//    Emit for debugging purpose
// ----------------------------------------------------------------------------
{
    std::cout << tree << "\n";
}

