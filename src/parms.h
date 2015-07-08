#ifndef COMPILER_PARM_H
#define COMPILER_PARM_H
// ****************************************************************************
//  parms.h                                                       ELIOT project
// ****************************************************************************
// 
//   File Description:
// 
//    Actions collecting parameters on the left of a rewrite
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

#include "compiler.h"


ELIOT_BEGIN

struct Parameter
// ----------------------------------------------------------------------------
//   Internal representation of a parameter
// ----------------------------------------------------------------------------
{
    Parameter(Name *name, llvm_type type = 0) : name(name), type(type) {}
    Name_p              name;
    llvm_type           type;
};
typedef std::vector<Parameter>      Parameters;


struct ParameterList
// ----------------------------------------------------------------------------
//   Collect parameters on the left of a rewrite
// ----------------------------------------------------------------------------
{
    typedef bool value_type;
    
public:
    ParameterList(CompiledUnit *unit)
        : unit(unit), defined(NULL), returned(NULL) {}

public:
    bool EnterName(Name *what, llvm_type declaredType = NULL);

    bool DoInteger(Integer *what);
    bool DoReal(Real *what);
    bool DoText(Text *what);
    bool DoName(Name *what);
    bool DoPrefix(Prefix *what);
    bool DoPostfix(Postfix *what);
    bool DoInfix(Infix *what);
    bool DoBlock(Block *what);

public:
    CompiledUnit *  unit;         // Current compilation unit
    Tree_p          defined;      // Tree being defined, e.g. 'sin' in 'sin X'
    text            name;         // Name being given to the LLVM function
    Parameters      parameters;   // Parameters and their order
    llvm_type       returned;     // Returned type if specified
};

ELIOT_END

#endif // COMPILER_PARM_H

