// ****************************************************************************
//  serializer.cpp                                                  XLR project
// ****************************************************************************
// 
//   File Description:
// 
//     A couple of classes used to serialize and deserialize XL trees
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
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

#include "serializer.h"
#include <machine/endian.h>



XL_BEGIN

// ============================================================================
// 
//    Internal format for IEEE-754 double type
// 
// ============================================================================

union ieee754_double
// ----------------------------------------------------------------------------
//   Decompose an IEEE-754 double precision format into sub-components
// ----------------------------------------------------------------------------
{
    double value;

    struct
    {
#if	BYTE_ORDER == BIG_ENDIAN
	unsigned int negative:1;
	unsigned int exponent:11;
	unsigned int mantissa0:20;
	unsigned int mantissa1:32;
#endif  /* Big endian.  */
#if	BYTE_ORDER == LITTLE_ENDIAN
# if	FLOAT_WORD_ORDER == BIG_ENDIAN
	unsigned int mantissa0:20;
	unsigned int exponent:11;
	unsigned int negative:1;
	unsigned int mantissa1:32;
# else
	unsigned int mantissa1:32;
	unsigned int mantissa0:20;
	unsigned int exponent:11;
	unsigned int negative:1;
# endif
#endif  /* Little endian.  */
    } ieee;
};



// ============================================================================
// 
//   Class Serializer : Convert trees to serialized form
// 
// ============================================================================

Serializer::Serializer(std::ostream &out)
// ----------------------------------------------------------------------------
//   Constructor sends the magic and version number
// ----------------------------------------------------------------------------
    : out (out)
{
    WriteUnsigned(serialMAGIC);
    WriteUnsigned(serialVERSION);
}


Tree *Serializer::DoInteger(Integer *what)
// ----------------------------------------------------------------------------
//   Serialize an integer leaf
// ----------------------------------------------------------------------------
{
    WriteUnsigned(serialINTEGER);
    WriteSigned(what->value);
    return what;
}


Tree *Serializer::DoReal(Real *what)
// ----------------------------------------------------------------------------
//   Serialize a real leaf
// ----------------------------------------------------------------------------
{
    WriteUnsigned(serialREAL);
    WriteReal(what->value);
    return what;
}


Tree *Serializer::DoText(Text *what)
// ----------------------------------------------------------------------------
//   Serialize a text leaf
// ----------------------------------------------------------------------------
{
    WriteUnsigned(serialTEXT);
    WriteText(what->opening);
    WriteText(what->value);
    WriteText(what->closing);
    return what;
}


Tree *Serializer::DoName(Name *what)
// ----------------------------------------------------------------------------
//   Serialize a name/symbol leaf
// ----------------------------------------------------------------------------
{
    WriteUnsigned(serialNAME);
    WriteText(what->value);
    return what;
}


Tree *Serializer::DoPrefix(Prefix *what)
// ----------------------------------------------------------------------------
//   Serialize a prefix tree
// ----------------------------------------------------------------------------
{
    WriteUnsigned(serialPREFIX);
    WriteChild(what->left);
    WriteChild(what->right);
    return what;
}
    

Tree *Serializer::DoPostfix(Postfix *what)
// ----------------------------------------------------------------------------
//   Serialize a postfix tree
// ----------------------------------------------------------------------------
{
    WriteUnsigned(serialPOSTFIX);
    WriteChild(what->left);
    WriteChild(what->right);
    return what;
}


Tree *Serializer::DoInfix(Infix *what)
// ----------------------------------------------------------------------------
//   Serialize an infix tree
// ----------------------------------------------------------------------------
{
    WriteUnsigned(serialINFIX);
    WriteChild(what->left);
    WriteText(what->name);
    WriteChild(what->right);
    return what;
}


Tree *Serializer::DoBlock(Block *what)
// ----------------------------------------------------------------------------
//   Serialize a block tree
// ----------------------------------------------------------------------------
{
    WriteUnsigned(serialBLOCK);
    WriteText(what->opening);
    WriteChild(what->child);
    WriteText(what->closing);
    return what;
}


void Serializer::WriteSigned(longlong value)
// ----------------------------------------------------------------------------
//   Write a signed longlong value (largest native machine type)
// ----------------------------------------------------------------------------
{
    byte b;
    do
    {
        b = value & 0x7F;
        value >>= 7;
        if ((value != 0 && value != -1) || (value & 0x40) != (b & 0x40))
            b |= 0x80;
        out << b;
    } while (b & 0x80);
}


void Serializer::WriteUnsigned(ulonglong value)
// ----------------------------------------------------------------------------
//   Write an unsigned longlong value (largest native machine type)
// ----------------------------------------------------------------------------
{
    byte b;
    do
    {
        b = value & 0x7F;
        value >>= 7;
        if (value != 0)
            b |= 0x80;
        out << b;
    } while (b & 0x80);
}


void Serializer::WriteReal(double value)
// ----------------------------------------------------------------------------
//   Write a real number in machine-independent binary format
// ----------------------------------------------------------------------------
{
    ieee754_double cvt;
    cvt.value = value;
    longlong exponent = cvt.ieee.exponent;
    longlong mantissa = cvt.ieee.mantissa0;
    if (cvt.ieee.negative)
        exponent = ~exponent;
    mantissa |= longlong(cvt.ieee.mantissa1) << 32;
    WriteSigned (exponent);
    WriteSigned (mantissa);
}


void Serializer::WriteText(text value)
// ----------------------------------------------------------------------------
//   Write the length followed by data bytes
// ----------------------------------------------------------------------------
{
    WriteUnsigned(value.length());
    out.write(value.data(), value.length());
}


void Serializer::WriteChild(Tree *child)
// ----------------------------------------------------------------------------
//   Serialie a child, either NULL or actual child
// ----------------------------------------------------------------------------
{
    if (child)
        child->Do(this);
    else
        WriteUnsigned(serialNULL);
}



// ============================================================================
// 
//   Class Deserializer : Read back serialized data from a stream
// 
// ============================================================================

Deserializer::Deserializer(std::istream &in, tree_position pos)
// ----------------------------------------------------------------------------
//   Read a few bytes from the stream, check version and magic value
// ----------------------------------------------------------------------------
    : in(in), pos(pos)
{
    if (ReadUnsigned() != serialMAGIC)
        throw Error (this, serialMAGIC);
    if (ReadUnsigned() != serialVERSION)
        throw Error (this, serialVERSION);
}


Tree *Deserializer::ReadTree()
// ----------------------------------------------------------------------------
//   Read back data from input stream and build tree from it
// ----------------------------------------------------------------------------
{
    SerializationTag tag = SerializationTag(ReadUnsigned());
    text             tvalue, opening, closing;
    longlong         ivalue;
    double           rvalue;
    Tree *           left;
    Tree *           right;
    Tree *           child;


    switch(tag)
    {
    case serialNULL:
        return NULL;

    case serialINTEGER:
        ivalue = ReadSigned();
        return new Integer(ivalue, pos);
    case serialREAL:
        rvalue = ReadReal();
        return new Real(rvalue, pos);
    case serialTEXT:
        opening = ReadText();
        tvalue = ReadText();
        closing = ReadText();
        return new Text(tvalue, opening, closing, pos);
    case serialNAME:
        tvalue = ReadText();
        return new Name(tvalue, pos);

    case serialBLOCK:
        opening = ReadText();
        child = ReadTree();
        closing = ReadText();
        return new Block(child, opening, closing, pos);
    case serialINFIX:
        left = ReadTree();
        tvalue = ReadText();
        right = ReadTree();
        return new Infix(tvalue, left, right, pos);
    case serialPREFIX:
        left = ReadTree();
        right = ReadTree();
        return new Prefix(left, right, pos);
    case serialPOSTFIX:
        left = ReadTree();
        right = ReadTree();
        return new Postfix(left, right, pos);

    default:
        throw Error(this, tag);
    }
}


longlong Deserializer::ReadSigned()
// ----------------------------------------------------------------------------
//   Read values from input stream, checking that it fits local longlong
// ----------------------------------------------------------------------------
{
    byte     b;
    longlong value = 0;
    longlong shifted = 0;
    uint     shift = 0;
    do
    {
        in >> b;
        shifted = longlong(b & 0x7f) << shift;
        value |= shifted;
        if ((shifted >> shift) != (b & 0x7f))
            throw Error(this, serialINTEGER);            
        shift += 7;
    }
    while (b & 0x80);

    if (b & 0x40)
        value |= ~0UL << shift;

    return value;
}


ulonglong Deserializer::ReadUnsigned()
// ----------------------------------------------------------------------------
//   Read unsigned values from input stream, checking that it fits local ull
// ----------------------------------------------------------------------------
{
    byte      b;
    ulonglong value   = 0;
    ulonglong shifted = 0;
    uint      shift   = 0;
    do
    {
        in >> b;
        shifted = ulonglong(b & 0x7f) << shift;
        value |= shifted;
        if ((shifted >> shift) != (b & 0x7f))
            throw Error (this, serialINTEGER);
        shift += 7;
    }
    while (b & 0x80);

    return value;
}


double Deserializer::ReadReal()
// ----------------------------------------------------------------------------
//   Read a real number from the input stream
// ----------------------------------------------------------------------------
{
    ieee754_double cvt;
    longlong exponent = ReadSigned();
    longlong mantissa = ReadSigned();
    if (exponent < 0)
    {
        cvt.ieee.negative = 1;
        cvt.ieee.exponent = ~exponent;
    }
    else
    {
        cvt.ieee.negative = 0;
        cvt.ieee.exponent = exponent;
    }
    cvt.ieee.mantissa1 = mantissa >> 32;
    cvt.ieee.mantissa0 = mantissa;
    return cvt.value;
}


text Deserializer::ReadText()
// ----------------------------------------------------------------------------
//   Read a text from the input stream
// ----------------------------------------------------------------------------
{
    ulonglong length = ReadUnsigned();
    text      result;
    char *    buffer = new char[length];
    in.read(buffer, length);
    result.insert(0, buffer, length);
    delete[] buffer;
    return result;
}


XL_END
