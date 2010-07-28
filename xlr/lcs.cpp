#ifdef UNIT_TEST

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
