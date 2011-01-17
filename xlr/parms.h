#ifndef COMPILER_PARM_H
#define COMPILER_PARM_H
// ****************************************************************************
//  parms.h                                                         XLR project
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
// This document is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#include "compiler.h"


XL_BEGIN

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
    bool EnterName(Name *what, bool untyped);

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

XL_END

#endif // COMPILER_PARM_H

