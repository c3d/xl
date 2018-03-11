#ifndef COMPILER_GC_H
#define COMPILER_GC_H
// ****************************************************************************
//  compiler-gc.h                                                 XL project
// ****************************************************************************
//
//   File Description:
//
//     Information connecting the XL/LLVM compiler to the garbage collector
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
#include <set>

XL_BEGIN

typedef std::vector<Tree *> captured_set;

struct CompilerInfo : Info
// ----------------------------------------------------------------------------
//   Information about compiler-related data structures
// ----------------------------------------------------------------------------
//   These informations are always attached to the [Form] tree
//   in a [Form is Implementation] definition.
{
    CompilerInfo(Tree *form, Function_p function = nullptr);
    ~CompilerInfo();

private:
    Tree *                      form;
    Function_p                  function;
    StructType_p                closure; // Null if no capture
    Type_p                      returned;
    captured_set                captured;

public:
    static CompilerInfo *       Info(Tree *form, bool create = false);
    static Function_p           Function(Tree *form);
    static void                 Function(Tree *form, Function_p function);
    static StructType_p         Closure(Tree *form);
    static void                 Closure(Tree *form, StructType_p closure);
    static Type_p               Returned(Tree *form);
    static void                 Returned(Tree *form, Type_p closure);
    static captured_set *       Captured(Tree *form);

    static bool                 FreeResources(Tree *);
};

XL_END

RECORDER_DECLARE(compiler_gc);

#endif // COMPILER_GC_H
