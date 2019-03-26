// *****************************************************************************
// test_gc.cpp                                                        XL project
// *****************************************************************************
//
// File description:
//
//
//
//
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2010,2015-2017,2019, Christophe de Dinechin <christophe@dinechin.org>
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
#include "gc.h"
#include <iostream>

struct Test
{
    Test()      { std::cerr << "Test::Test\n"; }
    ~Test()     { std::cerr << "Test::~Test\n"; }

    void Do()   { std::cerr << "Test::Do\n"; }

    GARBAGE_COLLECT(Test);
};

typedef XL::GCPtr<Test> Test_p;

struct Derived : Test
{
    Derived(Test *g, Test *u): glop(g), glap(u) { std::cerr << "Derived::Derived\n"; }
    ~Derived()  { std::cerr << "Derived::~Derived\n"; }

    Test_p glop;
    Test_p glap;

    GARBAGE_COLLECT(Derived);
};

typedef XL::GCPtr<Derived> Derived_p;


int main()
{
    Test_p ptr = new Test;
    Derived_p ptr2 = new Derived(ptr, ptr);
    ptr2->Do();
    new Derived(0, 0);

    for (uint i = 0; i < 2030; i++)
    {
        new Test();
        if (i > 2000)
            XL::GarbageCollector::Collect();
    }
    XL::GarbageCollector::Collect(true);
    XL::GarbageCollector::Collect(true);
}
