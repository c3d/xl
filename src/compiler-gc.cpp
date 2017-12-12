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
#include "recorder.h"

XL_BEGIN

RECORDER(compiler_gc, 128, "Compiler garbage collection");


CompilerInfo::~CompilerInfo()
// ----------------------------------------------------------------------------
//   Notice when we lose a compiler info
// ----------------------------------------------------------------------------
{
    RECORD(compiler_gc, "Compiler info deleted: function %p global %p tree %p",
             function, global, tree);
    IFTRACE(llvm)
        std::cerr << "CompilerInfo deleted F" << (void *) function
                  << " G" << (void *) global
                  << " T" << (void *) tree
                  << "\n";
}


void CompilerGarbageCollectionListener::BeginCollection()
// ----------------------------------------------------------------------------
//   Begin the collection - Nothing to do here?
// ----------------------------------------------------------------------------
{}


bool CompilerGarbageCollectionListener::CanDelete(void *obj)
// ----------------------------------------------------------------------------
//   Tell the compiler to free the resources associated with the tree
// ----------------------------------------------------------------------------
{
    Tree *tree = (Tree *) obj;
    return compiler->FreeResources(tree);
}


void CompilerGarbageCollectionListener::EndCollection()
// ----------------------------------------------------------------------------
//   Finalize the collection - Nothing to do here?
// ----------------------------------------------------------------------------
{}

XL_END
