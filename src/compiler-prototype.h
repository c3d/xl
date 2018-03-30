#ifndef COMPILER_PROTOTYPE_H
#define COMPILER_PROTOTYPE_H
// ****************************************************************************
//  compiler-prototype.h                             XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     A function prototype generated in the compiler unit
//
//     Prototypes are generated for references to external functions
//     They are also used as a base class for CompilerFunction
//
//
//
//
//
// ****************************************************************************
//  (C) 2018 Christophe de Dinechin <christophe@dinechin.org>
//   This software is licensed under the GNU General Public License v3
//   See LICENSE file for details.
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
//  (C) 1992-2018 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#include "compiler.h"
#include "compiler-unit.h"
#include "compiler-args.h"
#include "types.h"


XL_BEGIN

class CompilerPrototype
// ----------------------------------------------------------------------------
//    A function generated in a compile unit
// ----------------------------------------------------------------------------
{
protected:
    CompilerUnit &      unit;       // The unit we compile from
    Tree_p              form;       // Interface for this function
    Types_p             types;      // Type system for this function
    Function_p          function;   // The LLVM function we are building

    friend class CompilerExpression;

public:
    // Constructors for the top-level functions
    CompilerPrototype(CompilerUnit &unit,
                      Tree *form,
                      Types *types,
                      FunctionType_p ftype,
                      text name);
    CompilerPrototype(CompilerPrototype &caller, RewriteCandidate *rc);
    ~CompilerPrototype();

public:
    Function_p          Function();
    virtual bool        IsInterfaceOnly();
    Scope *             FunctionScope();
    Context *           FunctionContext();
};

XL_END

RECORDER_DECLARE(compiler_prototype);

#endif // COMPILER_FUNCTION_H
