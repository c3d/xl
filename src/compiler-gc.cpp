// ****************************************************************************
//  compiler-gc.cpp                                              XL project
// ****************************************************************************
//
//   File Description:
//
//     Information connecting the LLVM compiler to the XL garbage collector
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
//
//  When the garbage collector gets rid of a particular tree, the classes
//  defined here will be notified and make sure that the corresponding LLVM
//  data structures are also purged.
//

#include "compiler-gc.h"

#include <recorder/recorder.h>


XL_BEGIN

// ============================================================================
//
//   Compiler info in trees
//
// ============================================================================

CompilerInfo::CompilerInfo(Tree *form, Function_p function)
// ----------------------------------------------------------------------------
//    Create a compiler info
// ----------------------------------------------------------------------------
    : form(form), function(function)
{
    record(llvm_gc, "Create Info %p for form %t function %v",
           this, form, function);
}


CompilerInfo::~CompilerInfo()
// ----------------------------------------------------------------------------
//   Notice when we lose a compiler info
// ----------------------------------------------------------------------------
{
    record(llvm_gc, "Delete Info %p for form %t function %v",
           this, form, function);
}


CompilerInfo *CompilerInfo::Info(Tree *form, bool create)
// ----------------------------------------------------------------------------
//   Find or create the compiler-related info for a given form
// ----------------------------------------------------------------------------
{
    CompilerInfo *result = form->GetInfo<CompilerInfo>();
    if (!result && create)
    {
        result = new CompilerInfo(form);
        form->SetInfo<CompilerInfo>(result);
    }
    record(llvm_gc, "Info for %t is %p%s",
           form, result, create ? " (creating)" : "");
    return result;
}


Function_p CompilerInfo::Function(Tree *form)
// ----------------------------------------------------------------------------
//   Return the function associated to the form
// ----------------------------------------------------------------------------
{
    CompilerInfo *info = Info(form);
    Function_p function = info ? info->function : nullptr;
    record(llvm_gc, "Info for %t is %p function %v", form, info, function);
    return function;
}


void CompilerInfo::Function(Tree *form, Function_p function)
// ----------------------------------------------------------------------------
//   Associate a function to the given form
// ----------------------------------------------------------------------------
{
    CompilerInfo *info = Info(form, true);
    record(llvm_gc, "Setting function %v for %t in info %p",
           function, form, info);
    info->function = function;
}


StructType_p CompilerInfo::Closure(Tree *form)
// ----------------------------------------------------------------------------
//   Return the closure type associated to the form
// ----------------------------------------------------------------------------
{
    CompilerInfo *info = Info(form);
    StructType_p closure = info ? info->closure : nullptr;
    record(llvm_gc, "Info for %t is %p closure %v", form, info, closure);
    return closure;
}


void CompilerInfo::Closure(Tree *form, StructType_p closure)
// ----------------------------------------------------------------------------
//   Associate a closure to the given form
// ----------------------------------------------------------------------------
{
    CompilerInfo *info = Info(form, true);
    record(llvm_gc, "Setting closure %v for %t in info %p",
           closure, form, info);
    info->closure = closure;
}


Type_p CompilerInfo::Returned(Tree *form)
// ----------------------------------------------------------------------------
//   Return the returned type for the form
// ----------------------------------------------------------------------------
{
    CompilerInfo *info = Info(form);
    Type_p returned = info ? info->returned : nullptr;
    record(llvm_gc, "Info for %t is %p returned %v", form, info, returned);
    return returned;
}


void CompilerInfo::Returned(Tree *form, Type_p returned)
// ----------------------------------------------------------------------------
//   Associate a returned type to the given form
// ----------------------------------------------------------------------------
{
    CompilerInfo *info = Info(form, true);
    record(llvm_gc, "Setting returned %v for %t in info %p",
           returned, form, info);
    info->returned = returned;
}


bool CompilerInfo::FreeResources(Tree *form)
// ----------------------------------------------------------------------------
//   Free the LLVM resources associated to the form, if any
// ----------------------------------------------------------------------------
//   In the first pass, we need to clear the body and machine code for all
//   functions. This is because if we have foo() calling bar() and bar()
//   calling foo(), we will get an LLVM assert deleting one while the
//   other's body still makes a reference.
{
    bool result = true;
    CompilerInfo *info = Info(form);

    if (!info)
    {
        record(llvm_gc, "FreeResources %t no info", form);
        return true;
    }

    if (Function_p f = info->function)
    {
        bool inUse = !JIT::InUse(f);
        record(compiler_gc, "FreeResources %t function %v is %s",
               form, f, inUse ? "in use" : "unused");

        if (inUse)
        {
            // Defer deletion until later
            result = false;
        }
        else
        {
            // Not in use, we can delete it directly
            JIT::EraseFromParent(f);
            info->function = nullptr;
        }
    }
    record(compiler_gc, "FreeResources %t: %s",
           form, result ? "deleted" : "preserved");
    return result;
}

XL_END
