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
#include "parser.h"
#include "tree.h"
#include "syntax.h"
#include "errors.h"
#include <iostream>
#include <sstream>
#include <cctype>

XL_BEGIN

// ============================================================================
// 
//   Renderer construction / initialization
// 
// ============================================================================

struct EnterFormatsAction : Action
// ----------------------------------------------------------------------------
//   Enter formats in a format table
// ----------------------------------------------------------------------------
{
    EnterFormatsAction (formats_table &fmt): formats(fmt) {}

    Tree *Do (Tree *what)                       { return what; }
    Tree *DoInfix(Infix *what)
    {
        static Block block(NULL);
        if (what->name == "=")
        {
            if (Name *nmt = dynamic_cast<Name *> (what->Left()))
            {
                text N = nmt->value;
                if (N == "cr")                  N = "\n";
                else if (N == "tab")            N = "\t";
                else if (N == "space")          N = " ";
                else if (N == "indent")         N = block.Opening();
                else if (N == "unindent")       N = block.Closing();
                else                            N += " ";
                formats[N] = what->Right();
                return what;
            }
            else if (Text *txt = dynamic_cast<Text *> (what->Left()))
            {
                formats[txt->value] = what->Right();
                return what;
            }
        }
        return Action::DoInfix(what);
    }

    formats_table &     formats;
};



Renderer::Renderer(std::ostream &out, text styleFile, Syntax &stx, uint ts)
// ----------------------------------------------------------------------------
//   Renderer constructor
// ----------------------------------------------------------------------------
    : Action(), output(out), syntax(stx),
      indent(0), tabsize(ts), need_space(""), parenthesize(false)
{
    Syntax defaultSyntax;
    Positions positions;
    Errors errors(&positions);
    defaultSyntax.ReadSyntaxFile("xl.syntax");
    Parser p(styleFile.c_str(), defaultSyntax, positions, errors);
    static Block block(NULL);

    // Some defaults
    formats[block.Opening()] = new Name("indent");
    formats[block.Closing()] = new Name("unindent");

    Tree *fmts = p.Parse();
    if (fmts)
    {
        EnterFormatsAction action(formats);
        fmts->Do(action);
    }
}


Renderer::Renderer(std::ostream &out, Renderer *from)
// ----------------------------------------------------------------------------
//   Clone a renderer from some existing one
// ----------------------------------------------------------------------------
    : Action(), output(out), syntax(from->syntax), formats(from->formats),
      indent(0), tabsize(from->tabsize), need_space(""), parenthesize(false)
{
}



// ============================================================================
// 
//   Rendering proper
// 
// ============================================================================

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
        output << need_space << open;

    need_space = "";
    what->child->Do(this);
        
    indent -= tabsize;
    if (close != iblk.Closing())
        output << close;

    return what;
}


Tree *Renderer::DoPrefix(Prefix *what)
// ----------------------------------------------------------------------------
//   Render a prefix tree
// ----------------------------------------------------------------------------
{
    if (parenthesize)
        output << "(";
    what->left->Do(this);
    if (parenthesize)
        output << ")";

    need_space = " ";
    if (parenthesize)
        output << "(";
    what->right->Do(this);
    if (parenthesize)
        output << ")";

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
        if (parenthesize)
            output << "(";
        (*c)->Do(this);
        if (parenthesize)
            output << ")";
        if (count--)
        {
            if (what->name == "\n" || what->name == "." ||
                what->name == "," || what->name == ":")
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


Renderer *Renderer::renderer = NULL;


XL_END

std::ostream& operator<< (std::ostream &out, XL::Tree *t)
// ----------------------------------------------------------------------------
//   Just in case you want to emit a tree using normal ostream interface
// ----------------------------------------------------------------------------
{
    XL::Renderer render(out);
    if (t)
        t->Do(render);
    else
        out << "NULL";
    return out;
}

void debug(XL::Tree *tree)
// ----------------------------------------------------------------------------
//    Emit for debugging purpose
// ----------------------------------------------------------------------------
{
    XL::Renderer render(std::cout);
    render.parenthesize = true;
    if (tree)
        tree->Do(render);
    else
        std::cout << "NULL\n";
}

