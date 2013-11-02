#ifndef GV_H
#define GV_H
// ****************************************************************************
//  gv.h                                                           XLR project
// ****************************************************************************
//
//   File Description:
//
//     Definition of a class to output a tree in Graphviz DOT format.
//       http://www.graphviz.org
//
//
//
//
//
//
//
// ****************************************************************************
// This document is released under the GNU General Public License, with the
// following clarification and exception.
//
// Linking this library statically or dynamically with other modules is making
// a combined work based on this library. Thus, the terms and conditions of the
// GNU General Public License cover the whole combination.
//
// As a special exception, the copyright holders of this library give you
// permission to link this library with independent modules to produce an
// executable, regardless of the license terms of these independent modules,
// and to copy and distribute the resulting executable under terms of your
// choice, provided that you also meet, for each linked independent module,
// the terms and conditions of the license of that module. An independent
// module is a module which is not derived from or based on this library.
// If you modify this library, you may extend this exception to your version
// of the library, but you are not obliged to do so. If you do not wish to
// do so, delete this exception statement from your version.
//
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Jerome Forissier <jerome@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#include "tree.h"
#include <stdio.h>


XL_BEGIN

struct GvOutput : Action
// ----------------------------------------------------------------------------
//   An action to recursively dump a tree in Graphviz DOT format
// ----------------------------------------------------------------------------
{
    GvOutput(std::ostream &out): out(out)
    {
        out << "graph {\nnode [style=filled];\n";
    }
    virtual ~GvOutput() { out << "\n};\n"; }


    struct GvNodeName
    // ------------------------------------------------------------------------
    //   Utility class to format a graph node name
    // ------------------------------------------------------------------------
    {
        GvNodeName(XL::Tree *t) : t(t) {}
        std::ostream &operator()(std::ostream &out) const
        {
            char buf[10];
            snprintf(buf, 10, "n%lx", (long)(const Tree *) t);
            out << buf;
            return out;
        };
        XL::Tree_p  t;
    };


    struct Id
    // ------------------------------------------------------------------------
    //   Utility class to format a node ID
    // ------------------------------------------------------------------------
    {
        Id(XL::Tree *t) : t(t) {}
        std::ostream &operator()(std::ostream &out) const
        {
            bool present = t->Exists<NodeIdInfo>();
            if (present)
                out << t->Get<NodeIdInfo>() << "\\n";
            return out;
        };
        XL::Tree_p  t;
    };

    Tree *DoInteger(Integer *what);
    Tree *DoReal(Real *what);
    Tree *DoText(Text *what);
    Tree *DoName(Name *what);
    Tree *DoBlock(Block *what);
    Tree *DoInfix(Infix *what);
    Tree *DoPrefix(Prefix *what);
    Tree *DoPostfix(Postfix *what);
    Tree *Do(Tree *) { return NULL; }

    std::ostream &out;
};


std::ostream &operator<<(std::ostream &out, XL::GvOutput::GvNodeName gvnn)
{
    return gvnn(out);
}

std::ostream &operator<<(std::ostream &out, XL::GvOutput::Id id)
{
    return id(out);
}

Tree *GvOutput::DoInteger(Integer *what)
{
	out << GvNodeName(what) << "\n"
	    << GvNodeName(what) << " [color=orange, label=\""
		<< Id(what) << "[Integer]\\n" << what->value << "\"]\n";
	return NULL;
}

Tree *GvOutput::DoReal(Real *what)
{
	out << GvNodeName(what) << "\n"
	    << GvNodeName(what) << " [color=lightblue, label=\""
        << Id(what) << "[Real]\\n" << what->value << "\"]\n";
	return NULL;
}

Tree *GvOutput::DoText(Text *what)
{
	out << GvNodeName(what) << "\n"
	    << GvNodeName(what) << " [color=darkorange1, label=\""
	    << Id(what) << "[Text]\\n" << what->value << "\"]\n";
	return NULL;
}

Tree *GvOutput::DoName(Name *what)
{
	out << GvNodeName(what) << "\n"
	    << GvNodeName(what) << " [color=gold1, label=\""
	    << Id(what) << "[Name]\\n" << what->value << "\"]\n";
	return NULL;
}

Tree *GvOutput::DoBlock(Block *what)
{
	out << GvNodeName(what) << "\n"
	    << GvNodeName(what) << " [color=darkolivegreen3, label=\""
	    << Id(what) << "[Block]\\n" << what->opening
	    << " " << what->closing << "\"]\n"
		<< GvNodeName(what) << " -- ";
	what->child->Do(this);
	return NULL;
}

Tree *GvOutput::DoInfix(Infix *what)
{
	std::string name = (!what->name.compare("\n")) ?  "<CR>" : what->name;
	out << GvNodeName(what) << "\n"
	    << GvNodeName(what) << " [color=darkolivegreen4, label=\""
	    << Id(what) << "[Infix]\\n" << name << "\"]\n"
		<< GvNodeName(what) << " -- ";
	what->left->Do(this);
	out << GvNodeName(what) << " -- ";
	what->right->Do(this);
	return NULL;
}

Tree *GvOutput::DoPrefix(Prefix *what)
{
	out << GvNodeName(what) << "\n"
	    << GvNodeName(what) << " [color=greenyellow, label=\""
	    << Id(what) << "[Prefix]\"]\n"
		<< GvNodeName(what) << " -- ";
	what->left->Do(this);
	out << GvNodeName(what) << " -- ";
	what->right->Do(this);
	return NULL;
}

Tree *GvOutput::DoPostfix(Postfix *what)
{
	out << GvNodeName(what) << "\n"
	    << GvNodeName(what) << " [color=aquamarine2, label=\""
	    << Id(what) << "[Postfix]\"]\n"
	    << GvNodeName(what) << " -- ";
	what->left->Do(this);
	out << GvNodeName(what) << " -- ";
	what->right->Do(this);
	return NULL;
}

XL_END

#endif // GV_H
