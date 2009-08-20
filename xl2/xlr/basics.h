#ifndef BASICS_H
// ****************************************************************************
//  basics.h                        (C) 1992-2009 Christophe de Dinechin (ddd)
//                                                                 XL2 project
// ****************************************************************************
//
//   File Description:
//
//      Basic operations (arithmetic, etc)
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

#include "tree.h"
#include <cmath>

XL_BEGIN

// ============================================================================
//
//   Top level entry point
//
// ============================================================================

// Top-level entry point: enter all basic operations in the context
void EnterBasics(Context *context);

struct ListHandler : Native
// ----------------------------------------------------------------------------
//    Deal with the comma or similar constructive handlers
// ----------------------------------------------------------------------------
{
    ListHandler(): Native() {}
    virtual Tree *      Call(Context *context, Tree *args);
};

struct LastInListHandler : Native
// ----------------------------------------------------------------------------
//    Deal with the newline or semi-colon, where value is value of last
// ----------------------------------------------------------------------------
{
    LastInListHandler(): Native() {}
    virtual Tree *      Call(Context *context, Tree *args);
};



// ============================================================================
//
//    Special names
//
// ============================================================================

struct ReservedName : Name
// ----------------------------------------------------------------------------
//   Reserved names such as 'true', 'false', 'nil'
// ----------------------------------------------------------------------------
{
    ReservedName(text n) : Name(n) {}
    virtual Tree *      Run(Context *context)   { return this; }
};

extern ReservedName *true_name;
extern ReservedName *false_name;
extern ReservedName *nil_name;



// ============================================================================
//
//   Binary arithmetic
//
// ============================================================================

struct BinaryHandler : Native
// ----------------------------------------------------------------------------
//   Deal with all binary operators that apply to identical types
// ----------------------------------------------------------------------------
{
    BinaryHandler(): Native() {}
    virtual Tree *      Call(Context *context, Tree *args);
    virtual longlong    DoInteger(longlong left, longlong right);
    virtual double      DoReal(double left, double right);
    virtual text        DoText(text left, text right);
};


struct BinaryAdd : BinaryHandler
// ----------------------------------------------------------------------------
//   Implement binary addition
// ----------------------------------------------------------------------------
{
    longlong DoInteger(longlong l, longlong r)  { return l+r; }
    double   DoReal(double l, double r)         { return l+r; }
    text     DoText(text l, text r)             { return l+r; }
};


struct BinarySub : BinaryHandler
// ----------------------------------------------------------------------------
//   Implement binary subtraction
// ----------------------------------------------------------------------------
{
    longlong DoInteger(longlong l, longlong r)  { return l-r; }
    double   DoReal(double l, double r)         { return l-r; }
};


struct BinaryMul : BinaryHandler
// ----------------------------------------------------------------------------
//   Implement binary multiplication
// ----------------------------------------------------------------------------
{
    longlong DoInteger(longlong l, longlong r)  { return l*r; }
    double   DoReal(double l, double r)         { return l*r; }
};


struct BinaryDiv : BinaryHandler
// ----------------------------------------------------------------------------
//   Implement binary division
// ----------------------------------------------------------------------------
{
    longlong DoInteger(longlong l, longlong r)  { return l/r; }
    double   DoReal(double l, double r)         { return l/r; }
};


struct BinaryRemainder : BinaryHandler
// ----------------------------------------------------------------------------
//   Implement binary remainder
// ----------------------------------------------------------------------------
{
    longlong DoInteger(longlong l, longlong r)  { return l%r; }
    double   DoReal(double l, double r)         { return fmod(l,r); }
};


struct BinaryLeftShift : BinaryHandler
// ----------------------------------------------------------------------------
//   Implement binary left shift
// ----------------------------------------------------------------------------
{
    longlong DoInteger(longlong l, longlong r)  { return l<<r; }
};


struct BinaryRightShift : BinaryHandler
// ----------------------------------------------------------------------------
//   Implement binary left shift
// ----------------------------------------------------------------------------
{
    longlong DoInteger(longlong l, longlong r)  { return l>>r; }
};


struct BinaryAnd : BinaryHandler
// ----------------------------------------------------------------------------
//   Implement binary and
// ----------------------------------------------------------------------------
{
    longlong DoInteger(longlong l, longlong r)  { return l & r; }
};


struct BinaryOr : BinaryHandler
// ----------------------------------------------------------------------------
//   Implement binary or
// ----------------------------------------------------------------------------
{
    longlong DoInteger(longlong l, longlong r)  { return l | r; }
};


struct BinaryXor : BinaryHandler
// ----------------------------------------------------------------------------
//   Implement binary exclusive or
// ----------------------------------------------------------------------------
{
    longlong DoInteger(longlong l, longlong r)  { return l ^ r; }
};


// ============================================================================
//
//    Binary logic
//
// ============================================================================

struct BooleanHandler : Native
// ----------------------------------------------------------------------------
//   Deal with all binary operators that apply to identical types
// ----------------------------------------------------------------------------
{
    BooleanHandler(): Native() {}
    virtual Tree *      Call(Context *context, Tree *args);
    virtual bool        DoInteger(longlong left, longlong right);
    virtual bool        DoReal(double left, double right);
    virtual bool        DoText(text left, text right);
};

#define BOOL_OP(name, op)                                               \
struct Boolean##name : BooleanHandler                                   \
{                                                                       \
    bool DoInteger(longlong l, longlong r)      { return l op r; }      \
    bool DoReal(double l, double r)             { return l op r; }      \
    bool DoText(text l, text r)                 { return l op r; }      \
}

BOOL_OP(Less, <);
BOOL_OP(LessOrEqual, <=);
BOOL_OP(Equal, ==);
BOOL_OP(Different, !=);
BOOL_OP(Greater, >);
BOOL_OP(GreaterOrEqual, >=);


// ============================================================================
//
//   Assignments and declarations
//
// ============================================================================

struct Assignment : Native
// ----------------------------------------------------------------------------
//    Assign a value to a name
// ----------------------------------------------------------------------------
{
    Tree *Call(Context *context, Tree *args);
};


struct Definition : Native
// ----------------------------------------------------------------------------
//    Define a name to be some value
// ----------------------------------------------------------------------------
{
    Tree *Call(Context *context, Tree *args);
};


struct ParseTree : Native
// ----------------------------------------------------------------------------
//    Define a name to be some value
// ----------------------------------------------------------------------------
{
    Tree *Call(Context *context, Tree *args);
};


struct Evaluation : Native
// ----------------------------------------------------------------------------
//    Define a name to be some value
// ----------------------------------------------------------------------------
{
    Tree *Call(Context *context, Tree *args);
};

XL_END

#endif // BASICS_H
