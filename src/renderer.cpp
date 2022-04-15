// *****************************************************************************
// renderer.cpp                                                       XL project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2009-2010,2013,2015-2020, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2010-2012, Jérôme Forissier <jerome@taodyne.com>
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

#include "renderer.h"
#include "parser.h"
#include "tree.h"
#include "syntax.h"
#include "errors.h"
#include "save.h"
#include "action.h"
#include "options.h"
#include "interpreter.h"
#include "context.h"

#include <recorder/recorder.h>
#include <iostream>
#include <sstream>
#include <cctype>

RECORDER(renderer, 16, "Rendering parse trees as source code");

XL_BEGIN

// ============================================================================
//
//   Renderer construction / initialization
//
// ============================================================================

Renderer *Renderer::renderer = nullptr;

struct EnterFormatsAction
// ----------------------------------------------------------------------------
//   Enter formats in a format table
// ----------------------------------------------------------------------------
{
    typedef Tree *value_type;
    EnterFormatsAction (formats_table &fmt): formats(fmt) {}

    Tree *Do (Tree *what)                       { return what; }
    Tree *Do(Infix *what)
    {
        record(renderer, "Enter format %t", what);
        if (what->name == "=")
        {
            if (Name *nmt = what->left->AsName())
            {
                text N = nmt->value;
                if (N == "cr")                  N = "\n";
                else if (N == "tab")            N = "\t";
                else if (N == "space")          N = " ";
                else if (N == "indent")         N = Block::indent;
                else if (N == "unindent")       N = Block::unindent;
                else                            N += " ";
                formats[N] = what->right;
                return what;
            }
            else if (Text *txt = what->left->AsText())
            {
                formats[txt->value] = what->right;
                return what;
            }
        }
        else
        {
            what->left->Do(this);
            what->right->Do(this);
        }
        return Do((Tree *) what);
    }

    formats_table &     formats;
};


Renderer::Renderer(std::ostream &out, text styleFile, Syntax &stx)
// ----------------------------------------------------------------------------
//   Renderer constructor
// ----------------------------------------------------------------------------
    : output(out), syntax(stx), formats(),
      indent(0), self(""), left(nullptr), right(nullptr), current_quote("\""),
      priority(0),
      had_space(true), had_newline(false), had_punctuation(false),
      need_separator(false), need_newline(false), no_indents(false)
{
    SelectStyleSheet(styleFile);
}


Renderer::Renderer(std::ostream &out, Renderer *from)
// ----------------------------------------------------------------------------
//   Clone a renderer from some existing one
// ----------------------------------------------------------------------------
    : output(out), syntax(from->syntax), formats(from->formats),
      indent(from->indent), self(from->self),
      left(from->left), right(from->right),
      current_quote(from->current_quote), priority(from->priority),
      had_space(from->had_space),
      had_newline(from->had_newline),
      had_punctuation(from->had_punctuation),
      need_separator(from->need_separator),
      need_newline(from->need_newline),
      no_indents(from->no_indents)
{}


void Renderer::SelectStyleSheet(text styleFile)
// ----------------------------------------------------------------------------
//   Select an arbitrary style sheet
// ----------------------------------------------------------------------------
{
    Syntax defaultSyntax = syntax;
    Syntax emptySyntax;
    Positions positions;
    Errors errors;
    Parser p(styleFile.c_str(), defaultSyntax, positions, errors);

    // Some defaults
    formats.clear();
    formats[Block::indent]   = new Name("indent");
    formats[Block::unindent] = new Name("unindent");

    Tree *fmts = p.Parse();
    if (fmts)
    {
        EnterFormatsAction action(formats);
        fmts->Do(action);
    }
}



// ============================================================================
//
//   Rendering proper
//
// ============================================================================

void Renderer::RenderSeparators(char c)
// ----------------------------------------------------------------------------
//   Render space and newline separators as required before char c
// ----------------------------------------------------------------------------
{
    if (need_newline)
    {
        had_newline = true;
        need_newline = false;

        text cr = "\n";
        if (formats.count(cr) > 0)
            RenderFormat(formats[cr]);
        else
            output << cr;
    }

    if (c != '\n')
    {
        if (had_newline && c != 0)
        {
            had_newline = false;
            need_separator = false;
            if (!no_indents)
                RenderIndents();
        }

        if (need_separator)
        {
            need_separator = false;
            if (!had_space && !isspace(c))
            {
                if (had_punctuation == ispunct(c))
                {
                    text space = " ";
                    if (formats.count(space) > 0)
                        RenderFormat(formats[space]);
                    else
                        output << ' ';
                }
            }
        }
    }
}


void Renderer::RenderText(text format)
// ----------------------------------------------------------------------------
//   Render as text, reformatting character as required by style sheet
// ----------------------------------------------------------------------------
{
    char c;
    uint i;
    uint length   = format.length();
    bool quoted   = false;

    for (i = 0; i < length; i++)
    {
        c = format[i];
        if (need_newline || need_separator || had_newline)
        {
            RenderSeparators(c);
            if (had_newline && i == 0 && c == '\n')
                continue;
        }

        if (c == '\n')
        {
            need_newline = true;
            need_separator = false;
        }
        else
        {
            text t = text(1, c);
            quoted = i > 0 && i < length-1 && t == current_quote;
            if (quoted)
                t += " quoted";
            if (formats.count(t) > 0)
                RenderFormat(formats[t]);
            else if (!quoted)
                output << c;
            else
                output << c << c;   // Quoted char, as in  """Hello"""
        }
        had_space = isspace(c);
        had_punctuation = ispunct(c);
    }
}


void Renderer::RenderIndents()
// ----------------------------------------------------------------------------
//   Render the indents at the beginning of a line
// ----------------------------------------------------------------------------
{
    text k0 = "indents ";
    if (formats.count(k0) > 0)
    {
        Tree *fmt = formats[k0];
        for (uint i = 0; i < indent; i++)
            RenderFormat(fmt);
    }
    else
    {
        for (uint i = 0; i < indent; i++)
            RenderText(" ");
    }
}


void Renderer::RenderFormat(Tree *format)
// ----------------------------------------------------------------------------
//   Render a format read from the style sheet
// ----------------------------------------------------------------------------
{
    if (Text *tf = format->AsText())
    {
        text t = tf->value;
        if (tf->opening == Text::textQuote)
        {
            if (need_newline && t != "")
                RenderSeparators(t[0]);
            output << t;                        // As is, no formatting
        }
        else
            RenderText(t);                      // Format contents
    }
    else if (Name *nf = format->AsName())
    {
        text n = nf->value;
        text m = n + " ";
        if (n == "cr")
        {
            m = "\n";
            need_newline = true;
        }

        if (n == "indent")
        {
            indent += 1;
        }
        else if (n == "unindent")
        {
            indent -= 1;
        }
        else if (n == "indents")
        {
            RenderIndents();
        }
        else if (n == "self")
        {
            RenderText(self);
        }
        else if (n == "quoted_self")
        {
            text escaped;
            uint i;
            uint length = self.length();
            for (i = 0; i < length; i++)
            {
                text t = text(1, self[i]);
                escaped += t;
                if (t == current_quote)
                    escaped += t;
            }
            RenderText(escaped);
        }
        else if (n == "unindented_self")
        {
            Save<bool> saveNoIndents(no_indents, true);
            RenderText(self);
        }
        else if (n ==  "left" || n == "child")
        {
            Render(left);
        }
        else if (n == "right")
        {
            Render(right);
        }
        else if (n == "opening")
        {
            Block *b = right->AsBlock();
            if (b)
                RenderText(b->opening);
        }
        else if (n == "closing")
        {
            Block *b = right->AsBlock();
            if (b)
                RenderText(b->closing);
        }
        else if (n == "space")
        {
            if (!had_space)
                RenderText(" ");
        }
        else if (n == "separator")
        {
            need_separator = true;
        }
        else if (n == "newline" || n == "cr")
        {
            need_newline = true;
        }
        else if (formats.count(m) > 0)
        {
            RenderFormat (formats[m]);
        }
        else
        {
            output << "** Undeclared format directive " << n << "**\n";
        }
    }
    else if (Prefix *pf = format->AsPrefix())
    {
        RenderFormat (pf->left);
        RenderFormat (pf->right);
    }
    else if (Block *bf = format->AsBlock())
    {
        RenderFormat(bf->child);
    }
    else
    {
        output << "** Unkown kind of format directive **\n";
    }
}


void Renderer::RenderFormat(text self, text format)
// ----------------------------------------------------------------------------
//    Render format if it exists, otherwise text as is
// ----------------------------------------------------------------------------
{
    this->self = self;
    if (formats.count(format) > 0)
        RenderFormat (formats[format]);
    else
        RenderText(self);
}


void Renderer::RenderFormat(text self, text format, text generic)
// ----------------------------------------------------------------------------
//    Render format if it exists, otherwise try generic, otherwise as is
// ----------------------------------------------------------------------------
{
    this->self = self;
    if (formats.count(format) > 0)
        RenderFormat (formats[format]);
    else if (formats.count(generic) > 0)
        RenderFormat (formats[generic]);
    else
        RenderText(self);
}


void Renderer::RenderFormat(text self, text format,
                            text generic1, text generic2)
// ----------------------------------------------------------------------------
//    Render format if it exists, otherwise try generic, otherwise as is
// ----------------------------------------------------------------------------
{
    this->self = self;
    if (formats.count(format) > 0)
        RenderFormat (formats[format]);
    else if (formats.count(generic1) > 0)
        RenderFormat (formats[generic1]);
    else if (formats.count(generic2) > 0)
        RenderFormat (formats[generic2]);
    else
        RenderText(self);
}


Tree *Renderer::ImplicitBlock(Tree *t)
// ----------------------------------------------------------------------------
//   Return an implicit block when precedence requires it
// ----------------------------------------------------------------------------
{
    // Notice the spaces, which are used for distinct HTML color
    return new Block(t, " (", ") ", t->Position());
}


bool Renderer::IsAmbiguousPrefix(Tree *test, bool testL, bool testR)
// ----------------------------------------------------------------------------
//   Return true if tree might be seen as infix despite being a prefix
// ----------------------------------------------------------------------------
{
    if (Prefix *t = test->AsPrefix())
    {
        Tree *l = t->left;
        Tree *r = t->right;
        if (testL)
            if (Name *n = l->AsName())
                if (syntax.infix_priority.count(n->value) > 0)
                    return true;
        if (testR)
            if (Name *n = r->AsName())
                if (syntax.infix_priority.count(n->value) > 0)
                    return true;
    }
    return false;
}


bool Renderer::IsSubFunctionInfix(Tree *test)
// ----------------------------------------------------------------------------
//    Return true if tree is an infix with priority below function
// ----------------------------------------------------------------------------
{
    if (Infix *it = test->AsInfix())
    {
        if (syntax.infix_priority.count(it->name) <= 0)
            return true;
        else if (syntax.infix_priority[it->name] < syntax.function_priority)
            return true;
    }
    return false;
}


int Renderer::InfixPriority(Tree *test)
// ----------------------------------------------------------------------------
//    Return infix priority for infix trees, "infinity" otherwise
// ----------------------------------------------------------------------------
{
    if (Infix_p it = test->AsInfix())
        if (syntax.infix_priority.count(it->name) > 0)
            return syntax.infix_priority[it->name];
    return 9997;                                // Approximate infinity
}


void Renderer::Render(Tree *what)
// ----------------------------------------------------------------------------
//   Render to given stream (taking care of selected items highlighting)
// ----------------------------------------------------------------------------
{
    text hname;
    bool highlight = what && highlights.count(what) > 0;
    if (highlight)
        hname = highlights[what];
    std::streampos hbegin, hend;
    CommentsInfo *cinfo = what ? what->GetInfo<CommentsInfo>() : nullptr;
    CommentsList::iterator ci;

    if (highlight)
    {
        hbegin = output.tellp();
        RenderFormat("", "highlight_begin_" + hname + " ");
    }

    if (cinfo)
    {
        text saveSelf = self;
        for (ci = cinfo->before.begin(); ci != cinfo->before.end(); ci++)
            RenderFormat(*ci, "comment_before ", "comment ");
        self = saveSelf;
    }

    RenderBody(what);

    if (cinfo)
    {
        text saveSelf = self;
        for (ci = cinfo->after.begin(); ci != cinfo->after.end(); ci++)
            RenderFormat(*ci, "comment_after ", "comment ");
        self = saveSelf;
    }

    if (highlight)
    {
        hend = output.tellp();
        RenderFormat("", "highlight_end_" + hname + " ");

        stream_range r(hbegin, hend);
        highlighted[hname].push_back(r);
    }
}


void Renderer::RenderBody(Tree *what)
// ----------------------------------------------------------------------------
//   Render to given stream
// ----------------------------------------------------------------------------
//   The real complication here is to figure out when to emit
//   implicit parentheses (which we do for blocks). We need that as some
//   plug-ins may generate trees which would not be generable from
//   the parser directly. For instance, you need to write
//   exp (2*X), but it is possible to generate
//   prefix(name("exp"), infix("*", natural(2), name("X")))
//   which is the same thing without the parentheses.
//   We insert the implicit parentheses here (creating blocks on the fly)
//   They are bracketed between [ and ] in the examples below
//
//   The cases where we need that are:
//   - When an infix child is an infix with a lower priority
//     (strictly lower/lower-or-equal depending on associativity)
//     Example: "A * [B + C]" or "A + [B + C]" or "[A else B] else C"
//   - When an infix or prefix has a prefix child with a tail that is
//     itself a valid infix symbol (it would be parsed as infix)
//     Example: "A * [A *]" or "[A and] + B" or "not [A and]"
//   - When an prefix child has a child which is an infix with a priority
//     below "function_priority". Example: "exp [A + B] " (but not "A.B C")
//   - When a prefix tail is a prefix with a left that is itself a
//     valid infix. Example: "A [+ B]"
{
    text  old_self     = this->self;
    Tree *old_left     = this->left;
    Tree *old_right    = this->right;
    int   old_priority = this->priority;
    text  t;
    std::ostringstream toText;
    static uint recursionCount = 0;

    recursionCount++;
    if (recursionCount < 300)
    {
        if (!what)
        {
            RenderFormat("?null?", "?null?", "error ");
        }
        else switch (what->Kind())
             {
             case NATURAL:
                 // HACK: For now consider it as natural for printout
                 toText << (longlong) what->AsNatural()->value;
                 t = toText.str();
                 RenderFormat (t, t, "natural ");
                 break;
             case REAL:
                 toText << what->AsReal()->value;
                 t = toText.str();
                 if (t.find(".") == t.npos)
                 {
                     size_t exponent = t.find("e");
                     if (exponent == t.npos)
                         t += ".0";
                     else
                         t.insert(exponent, ".0");
                 }
                 RenderFormat (t, t, "real ");
                 break;
             case TEXT: {
                 Text *w = what->AsText();
                 t = w->value;
                 text q0 = t.find("\n") != t.npos ? "longtext " : "text ";
                 text q1 = q0 + w->opening;
                 text q2 = q1 + " " + w->closing;
                 text saveq = this->current_quote;
                 this->current_quote = w->opening;

                 if (formats.count(q2) > 0)
                 {
                     this->self = t;
                     RenderFormat(formats[q2]);
                 }
                 else if (formats.count(q1) > 0)
                 {
                     this->self = t;
                     RenderFormat(formats[q1]);
                 }
                 else if (formats.count(q0) > 0)
                 {
                     this->self = t;
                     RenderFormat(formats[q0]);
                 }
                 else
                 {
                     t = w->opening + t + w->closing;
                     RenderText (t);
                 }
                 this->current_quote = saveq;
             }   break;
             case NAME:
                 t = what->AsName()->value;
                 RenderFormat (t, t, "name ");
                 break;
             case PREFIX: {
                 Prefix *w = what->AsPrefix();
                 Tree *l = w->left;
                 Tree *r = w->right;

                 // Don't display closures, only the value inside
                 if (l->As<Scope>())
                 {
                     output << "[closure " << (void *) l << "] ";
                     Render(r);
                     break;
                 }

                 // Create blocks for implicit parentheses
                 if (IsAmbiguousPrefix(l, false, true) ||
                     IsSubFunctionInfix(l))
                     l = ImplicitBlock(l);
                 if (priority > syntax.statement_priority)
                     if (IsAmbiguousPrefix(r, true, true) ||
                         IsSubFunctionInfix(r))
                         r = ImplicitBlock(r);
                 left = l;
                 right = r;

                 text n0 = "prefix ";
                 if (Name *lf = left->AsName())
                 {
                     text n = n0 + lf->value;
                     if (formats.count(n) > 0)
                     {
                         RenderFormat (formats[n]);
                         break;
                     }
                 }
                 if (formats.count(n0) > 0)
                 {
                     RenderFormat (formats[n0]);
                 }
                 else
                 {
                     Render (l);
                     Render (r);
                 }
             }   break;
             case POSTFIX: {
                 Postfix *w = what->AsPostfix();
                 Tree *l = w->left;
                 Tree *r = w->right;

                 // Create blocks for implicit parentheses
                 if (priority > syntax.statement_priority)
                     if (IsAmbiguousPrefix(l, true, false) ||
                         IsSubFunctionInfix(l))
                         l = ImplicitBlock(l);
                 if (IsAmbiguousPrefix(r, true, true) ||
                     IsSubFunctionInfix(r))
                     r = ImplicitBlock(r);
                 left = l;
                 right = r;

                 text n0 = "postfix ";
                 if (Name *rf = right->AsName())
                 {
                     text n = n0 + rf->value;
                     if (formats.count(n) > 0)
                         RenderFormat (formats[n]);
                     else if (formats.count(n0) > 0)
                         RenderFormat (formats[n0]);
                     else
                     {
                         Render (l);
                         Render (r);
                     }
                 }
                 else if (formats.count(n0) > 0)
                 {
                     RenderFormat (formats[n0]);
                 }
                 else
                 {
                     Render (l);
                     Render (r);
                 }
             }   break;
             case INFIX: {
                 Infix *w = what->AsInfix();
                 text in = w->name;
                 if (in == "\n")
                     in = "cr";
                 text n0 = "infix ";
                 text n = n0 + in;
                 Tree *l = w->left;
                 Tree *r = w->right;

                 // Create blocks for implicit parentheses, dealing with assoc.
                 int  p0 = InfixPriority(w);
                 int  pl = InfixPriority(l);
                 int  pr = InfixPriority(r);
                 bool la = (p0 & 1) == 0;                // Left associative
                 bool ra = (p0 & 1) == 1;                // Right associative

                 if (pl < p0 || (pl == p0 && ra) ||
                     IsAmbiguousPrefix(l, false, true))
                     l = ImplicitBlock(l);
                 if (pr < p0 || (pr == p0 && la) ||
                     IsAmbiguousPrefix(r, false, true))
                     r = ImplicitBlock(r);

                 this->priority = p0;
                 this->left = l;
                 this->right = r;
                 this->self = w->name;

                 if (formats.count(n) > 0)
                     RenderFormat(formats[n]);
                 else if (formats.count(n0) > 0)
                     RenderFormat(formats[n0]);
                 else
                 {
                     Render (l);
                     RenderFormat (w->name, w->name);
                     Render (r);
                 }
             }   break;
             case BLOCK: {
                 Block *w  = what->AsBlock();
                 text   n0 = "block ";
                 text   n  = n0 + w->opening + " " + w->closing;
                 Tree *l  = w->child;
                 this->left = l;
                 this->right = w;
                 this->self = w->opening + w->closing;
                 this->priority = syntax.InfixPriority(w->opening);
                 if (formats.count(n) > 0)
                     RenderFormat(formats[n]);
                 else if (formats.count(n0) > 0)
                     RenderFormat (formats[n0]);
                 else
                 {
                     RenderFormat (w->opening, w->opening, "opening ");
                     Render (w->child);
                     RenderFormat (w->closing, w->closing, "closing ");
                 }
             }   break;
             }

        this->self = old_self;
        this->left = old_left;
        this->right = old_right;
        this->priority = old_priority;
    }

    recursionCount--;
}


void Renderer::RenderFile(Tree *what)
// ----------------------------------------------------------------------------
//   Output the tree, including begin and end formats if any
// ----------------------------------------------------------------------------
{
    indent = 0;
    had_space = true;
    had_punctuation = false;
    need_separator = false;
    priority = 0;
    highlighted.clear();
    RenderFormat("", "begin ");
    Render(what);
    RenderFormat("", "end ");
}


std::ostream& operator<< (std::ostream &out, XL::Tree *t)
// ----------------------------------------------------------------------------
//   Just in case you want to emit a tree using normal ostream interface
// ----------------------------------------------------------------------------
{
    XL::Renderer render(out);
    render.RenderFile(t);
    return out;
}


std::ostream& operator<< (std::ostream &out, XL::Tree &t)
// ----------------------------------------------------------------------------
//   Just in case you want to emit a tree using normal ostream interface
// ----------------------------------------------------------------------------
{
    XL::Renderer render(out);
    render.RenderFile(&t);
    return out;
}


std::ostream& operator<< (std::ostream &out, XL::TreeList &list)
// ----------------------------------------------------------------------------
//   Just in case you want to emit a tree using normal ostream interface
// ----------------------------------------------------------------------------
{
    bool separator = false;
    for (XL::TreeList::iterator it = list.begin(); it != list.end(); it++)
    {
        if (separator)
            out << ",";
        else
            separator = true;
        out << *it;
    }
    return out;
}

XL_END

bool xldebug_verbose = false;

XL::Tree *xldebug(XL::Tree *tree)
// ----------------------------------------------------------------------------
//    Emit for debugging purpose
// ----------------------------------------------------------------------------
{
    if (XL::Allocator<XL::Natural>::IsAllocated(tree)   ||
        XL::Allocator<XL::Real>::IsAllocated(tree)      ||
        XL::Allocator<XL::Text>::IsAllocated(tree)      ||
        XL::Allocator<XL::Name>::IsAllocated(tree)      ||
        XL::Allocator<XL::Infix>::IsAllocated(tree)     ||
        XL::Allocator<XL::Prefix>::IsAllocated(tree)    ||
        XL::Allocator<XL::Postfix>::IsAllocated(tree)   ||
        XL::Allocator<XL::Block>::IsAllocated(tree)     ||
        XL::Allocator<XL::Scopes>::IsAllocated(tree)    ||
        XL::Allocator<XL::Rewrite>::IsAllocated(tree)   ||
        XL::Allocator<XL::Rewrites>::IsAllocated(tree))
    {
        XL::Renderer render(std::cerr);
        if (xldebug_verbose)
            render.SelectStyleSheet("debug.stylesheet");
        render.RenderFile(tree);
        std::cerr << "\n";

    }
    else if (XL::Allocator<XL::Scope>::IsAllocated(tree))
    {
        extern XL::Scope *xldebug(XL::Scope *scope);
        return xldebug((XL::Scope *) tree);
    }
    else
    {
        std::cerr << "Cowardly refusing to render unknown pointer "
                  << (void *) tree << "\n";
    }
    return tree;
}

XL::Tree *xldebug(XL::Tree_p t)         { return xldebug((XL::Tree *) t); }
XL::Tree *xldebug(XL::Natural_p t)      { return xldebug((XL::Tree *) t); }
XL::Tree *xldebug(XL::Real_p t)         { return xldebug((XL::Tree *) t); }
XL::Tree *xldebug(XL::Text_p t)         { return xldebug((XL::Tree *) t); }
XL::Tree *xldebug(XL::Name_p t)         { return xldebug((XL::Tree *) t); }
XL::Tree *xldebug(XL::Block_p t)        { return xldebug((XL::Tree *) t); }
XL::Tree *xldebug(XL::Prefix_p t)       { return xldebug((XL::Tree *) t); }
XL::Tree *xldebug(XL::Postfix_p t)      { return xldebug((XL::Tree *) t); }
XL::Tree *xldebug(XL::Infix_p t)        { return xldebug((XL::Tree *) t); }

RECORDER_TWEAK_DEFINE(recorder_dump_symbolic, 40,
                      "Size of symbolic information to show, 0=none, -1=all");
