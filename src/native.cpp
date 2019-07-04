// ****************************************************************************
//  native.cpp                                                    Tao3D project
// ****************************************************************************
//
//   File Description:
//
//     Interface between XL and native code (new style)
//
//     The new style interface requires C++11 variadics
//
//
//
//
//
//
// ****************************************************************************
//   (C) 2019 Christophe de Dinechin <christophe@dinechin.org>
//   This software is licensed under the GNU General Public License v3
// ****************************************************************************
//   This file is part of Tao3D.
//
//   Tao3D is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   Foobar is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with Tao3D.  If not, see <https://www.gnu.org/licenses/>.
// ****************************************************************************

#include "native.h"

RECORDER(native, 32, "Native functions");

namespace XL
{

Native *Native::list = nullptr;


Native::~Native()
// ----------------------------------------------------------------------------
//   Remove the current native interface from the list
// ----------------------------------------------------------------------------
{
    if (list == this)
    {
        list = next;
        return;
    }

    Native *last = nullptr;
    for (Native *native = list; native; native = native->next)
    {
        if (native == this)
        {
            last->next = this->next;
            return;
        }
        last = native;
    }

    delete implementation;
}


void Native::EnterPrototypes(Compiler &compiler)
// ----------------------------------------------------------------------------
//   Enter the prototypes for all functions declared with Native objects
// ----------------------------------------------------------------------------
{
    record(native, "Entering prototypes");
    for (Native *native = list; native; native = native->next)
    {
        record(native, "Entering prototype for %s shape %t",
               native->symbol, native->Shape());
        native->Prototype(compiler, native->symbol);
    }
}

}
