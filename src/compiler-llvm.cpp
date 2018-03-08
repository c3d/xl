// ****************************************************************************
//  compiler-llvm.cpp                                            XL project
// ****************************************************************************
//
//   File Description:
//
//    The interface between the compiler and LLVM
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
#include "compiler-llvm.h"
#include "compiler-unit.h"

#include <llvm/IR/Type.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/DerivedTypes.h>



XL_BEGIN

// ============================================================================
//
//   Define all the LLVM wrappers
//
// ============================================================================

#define UNARY(Name)                                                     \
static Value_p xl_llvm_##Name(CompilerUnit &u, Value_p *args)           \
{                                                                       \
    return u.code.Name(args[0]);                                        \
}

#define BINARY(Name)                                                    \
static Value_p xl_llvm_##Name(CompilerUnit &u, Value_p *args)           \
{                                                                       \
    return u.code.Name(args[0], args[1]);                               \
}

#define CAST(Name)                                                      \
static Value_p xl_llvm_##Name(CompilerUnit &u, Value_p *args)           \
{                                                                       \
    return u.code.Name(args[0], (Type_p) args[1]);                      \
}


#define SPECIAL(Name, Arity, Code)                                      \
static Value_p xl_llvm_##Name(CompilerUnit &unit, Value_p *args)        \
{                                                                       \
    Compiler &compiler = unit.compiler; (void) compiler;                \
    JITBlock &code = unit.code; (void) code;                            \
    JITBlock &data = unit.data; (void) data;                            \
    Code                                                                \
}

#define ALIAS(from, arity, to)
#define EXTERNAL(Name, ...)

#include "compiler-llvm.tbl"


CompilerPrimitive CompilerPrimitives[] =
// ---------------- ------------------------------------------------------------
//   A table initialized with the various LLVM entry points
// ----------------------------------------------------------------------------
{
#define UNARY(Name)                     { #Name, xl_llvm_##Name, 1 },
#define BINARY(Name)                    { #Name, xl_llvm_##Name, 2 },
#define CAST(Name)                      { #Name, xl_llvm_##Name, 2 },
#define SPECIAL(Name, Arity, Code)      { #Name, xl_llvm_##Name, Arity },
#define ALIAS(From, Arity, To)          { #From, xl_llvm_##To, Arity },
#define EXTERNAL(Name, ...)

#include "compiler-llvm.tbl"

    // Terminator
    { NULL, NULL, 0 }
};

XL_END
