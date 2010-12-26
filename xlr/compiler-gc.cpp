// ****************************************************************************
//  compiler-gc.cpp                                                 XLR project
// ****************************************************************************
// 
//   File Description:
// 
//     Information connecting the LLVM compiler to the XLR garbage collector
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
//
//  When the garbage collector gets rid of a particular tree, the classes
//  defined here will be notified and make sure that the corresponding LLVM
//  data structures are also purged.
//

#include "compiler-gc.h"

XL_BEGIN

CompilerInfo::~CompilerInfo()
// ----------------------------------------------------------------------------
//   Notice when we lose a compiler info
// ----------------------------------------------------------------------------
{
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
