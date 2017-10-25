#ifndef SERIALIZER_H
#define SERIALIZER_H
// ****************************************************************************
//  serializer.h                                                  XL project
// ****************************************************************************
//
//   File Description:
//
//     A couple of classes used to serialize and read-back XL trees
//
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
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#include "base.h"
#include "tree.h"
#include "action.h"
#include <iostream>


XL_BEGIN

enum SerializationTag
// ----------------------------------------------------------------------------
//   Kind used for serialization (numerically independent from Tree kind)
// ----------------------------------------------------------------------------
{
    serialNULL,                 // NULL tree

    serialINTEGER, serialREAL, serialTEXT, serialNAME,
    serialBLOCK, serialPREFIX, serialPOSTFIX, serialINFIX,
    serialINVALID,

    serialVERSION = 0x0101,
    serialMAGIC   = 0x05121968
};


typedef std::map<text, longlong>        text_map;
typedef std::map<longlong, text>        text_ids;


struct Serializer : Action
// ----------------------------------------------------------------------------
//    Serialize a tree to a stream
// ----------------------------------------------------------------------------
{
    Serializer(std::ostream &out);
    ~Serializer() {}

    // Serialization of the canonical nodes
    Tree *      DoInteger(Integer *what);
    Tree *      DoReal(Real *what);
    Tree *      DoText(Text *what);
    Tree *      DoName(Name *what);
    Tree *      DoPrefix(Prefix *what);
    Tree *      DoPostfix(Postfix *what);
    Tree *      DoInfix(Infix *what);
    Tree *      DoBlock(Block *what);
    Tree *      DoChild(Tree *child);
    Tree *      Do(Tree *what);

    bool        IsValid()       { return out.good(); }

    static void Write(std::ostream &out, Tree *tree)
    {
        Serializer s(out);
        tree->Do(s);
    }
    
public:
    // Writing data (low level)
    void        WriteSigned(longlong);
    void        WriteUnsigned(ulonglong);
    void        WriteReal(double);
    void        WriteText(text);
    void        WriteChild(Tree *child);

protected:
    std::ostream &      out;
    text_map            texts;
};


struct Deserializer
// ----------------------------------------------------------------------------
//   Reconstruct a tree from its serialized form
// ----------------------------------------------------------------------------
{
    Deserializer(std::istream &in, TreePosition pos = Tree::NOWHERE);
    ~Deserializer();

    // Deserialize a tree from the input and return it, or return NULL
    Tree *      ReadTree();
    bool        IsValid()       { return in.good(); }

    static Tree *Read(std::istream &in)
    {
        Deserializer d(in);
        return d.ReadTree();
    }

public:
    // Reading low-level data
    longlong    ReadSigned();
    ulonglong   ReadUnsigned();
    double      ReadReal();
    text        ReadText();

protected:
    std::istream &      in;
    TreePosition        pos;
    text_ids            texts;
};

XL_END

#endif // SERIALIZE_H
