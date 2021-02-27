// *****************************************************************************
// version.cpp                                                        XL project
// *****************************************************************************
//
// File description:
//
//     Semantic versioning for the XL library
//
//
//
//
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2019-2020, Christophe de Dinechin <christophe@dinechin.org>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License,
// or (at your option) any later version.
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

#include "version.h"
#include <stdio.h>
#include <sstream>

namespace XL
{

Version::Version(const char *input)
// ----------------------------------------------------------------------------
//   Scan the input string for version numbers
// ----------------------------------------------------------------------------
// This accept input in the form 1.0.3 or v1.0.3
// If scan fails, then the version will be 0.0.0 and operator bool() is false
    : major(0), minor(0), patch(0)
{
    if (sscanf(input, "%u.%u.%u", &major, &minor, &patch) == 0)
        sscanf(input, "v%u.%u.%u", &major, &minor, &patch);
}


bool Version::operator<(const Version &o) const
// ----------------------------------------------------------------------------
//   Less-than comparison between version numbers
// ----------------------------------------------------------------------------
{
    return (major < o.major ||
            (major == o.major && (minor < o.minor ||
                                  (minor == o.minor && patch < o.patch))));
}


bool Version::operator==(const Version &o) const
// ----------------------------------------------------------------------------
//   Equality comparison between version numbers
// ----------------------------------------------------------------------------
{
    return major == o.major && minor == o.minor && patch == o.patch;
}


bool Version::operator>(const Version &o) const
// ----------------------------------------------------------------------------
//   Greater-than comparison
// ----------------------------------------------------------------------------
{
    return o < *this;
}


bool Version::operator<=(const Version &o) const
// ----------------------------------------------------------------------------
//   Less-or-equal comparison
// ----------------------------------------------------------------------------
{
    return !(*this > o);
}


bool Version::operator>=(const Version &o) const
// ----------------------------------------------------------------------------
//   Greater-or-equal comparison
// ----------------------------------------------------------------------------
{
    return !(*this < o);
}


bool Version::operator!=(const Version &o) const
// ----------------------------------------------------------------------------
//   Less-or-equal comparison
// ----------------------------------------------------------------------------
{
    return !(*this == o);
}


bool Version::IsCompatibleWith(const Version &o) const
// ----------------------------------------------------------------------------
//    Check if two version are compatible
// ----------------------------------------------------------------------------
//    Versions 0.x.y are assumed incompatible with one another
{
    return major && o.major && *this >= o;
}


Version::operator bool() const
// ----------------------------------------------------------------------------
//    Return true for any version except 0.0.0
// ----------------------------------------------------------------------------
{
    return major || minor || patch;
}


Version::operator std::string() const
// ----------------------------------------------------------------------------
//   Generate a version string
// ----------------------------------------------------------------------------
{
    std::ostringstream os;
    os << major << '.' << minor << '.' << patch;
    return os.str();
}


const Version VERSION(0, 0, 1);
const Version COMPATIBLE_VERSION(0, 0, 1);

} // namespace XL
