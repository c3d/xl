#ifndef SERIALIZER_H
#define SERIALIZER_H
// *****************************************************************************
// serializer.h                                                       XL project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2010,2014-2017,2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2010,2012, Jérôme Forissier <jerome@taodyne.com>
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

    serialNATURAL, serialREAL, serialTEXT, serialNAME,
    serialBLOCK, serialPREFIX, serialPOSTFIX, serialINFIX,
    serialINVALID,

    serialVERSION = 0x0101,
    serialMAGIC   = 0x05121968
};


typedef std::map<text, longlong>        text_map;
typedef std::map<longlong, text>        text_ids;


struct Serializer
// ----------------------------------------------------------------------------
//    Serialize a tree to a stream
// ----------------------------------------------------------------------------
{
    typedef Tree *value_type;

    Serializer(std::ostream &out);
    ~Serializer() {}

    // Serialization of the canonical nodes
    Tree *      Do(Natural *what);
    Tree *      Do(Real *what);
    Tree *      Do(Text *what);
    Tree *      Do(Name *what);
    Tree *      Do(Prefix *what);
    Tree *      Do(Postfix *what);
    Tree *      Do(Infix *what);
    Tree *      Do(Block *what);
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
