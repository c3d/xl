// *****************************************************************************
// compiler-gc.cpp                                                    XL project
// *****************************************************************************
//
// File description:
//
//     Information connecting the LLVM compiler to the XLR garbage collector
//
//
//
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2010-2011,2017,2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2012, Jérôme Forissier <jerome@taodyne.com>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can r redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// XL is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with XL, in a file named COPYING.
// If not, see <https://www.gnu.org/licenses/>.
// *****************************************************************************
//
//  When the garbage collector gets rid of a particular tree, the classes
//  defined here will be notified and make sure that the corresponding LLVM
//  data structures are also purged.
//

#include "compiler-gc.h"
#include <recorder/recorder.h>

XL_BEGIN

RECORDER(compiler_info, 32, "Garbage-collected compiler information");

CompilerInfo::~CompilerInfo()
// ----------------------------------------------------------------------------
//   Notice when we lose a compiler info
// ----------------------------------------------------------------------------
{
    record(compiler_info, "Compiler info for %t deleted, function %v",
           tree, function);
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
