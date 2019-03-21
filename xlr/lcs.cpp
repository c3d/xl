#ifdef UNIT_TEST
// *****************************************************************************
// lcs.cpp                                                            XL project
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
// (C) 2010,2019, Christophe de Dinechin <christophe@dinechin.org>
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

#include <cassert>
#include <iostream>
#include "lcs.h"

int main (int argc, char **argv)
// ----------------------------------------------------------------------------
//   Apply the test on a known input
// ----------------------------------------------------------------------------

{
    std::string s1 = "Hello, world!";
    std::string s2 = "I say Hello to the world.";
    std::string expected = "Hello world";

    std::string out;

    XL::LCS<std::string> lcs;

    lcs.Compute(s1, s2);
    lcs.Extract(s1, out);

    std::cout << "LCS: " << out << std::endl;
    if (out.compare(expected))
        std::cerr << "LCS implementation does not work!\n";
}

#endif
