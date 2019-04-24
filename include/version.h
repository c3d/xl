#ifndef VERSION_H
#define VERSION_H
// ****************************************************************************
//  version.h                                                     Tao3D project
// ****************************************************************************
//
//   File Description:
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

#include <string>

namespace XL
{
struct Version
// ----------------------------------------------------------------------------
//    Structured versioning
// ----------------------------------------------------------------------------
{
    Version(unsigned major = 0, unsigned minor = 0, unsigned patch = 0)
        : major(major), minor(minor), patch(patch) {}
    Version(const char *input);

    unsigned    Major() const   { return major; }
    unsigned    Minor() const   { return minor; }
    unsigned    Patch() const   { return patch; }

    bool        operator<(const Version &o) const;
    bool        operator==(const Version &o) const;
    bool        operator>(const Version &o) const;
    bool        operator<=(const Version &o) const;
    bool        operator!=(const Version &o) const;
    bool        operator>=(const Version &o) const;

    bool        IsCompatibleWith(const Version &o) const;
    operator    std::string() const;

private:
    unsigned    major;
    unsigned    minor;
    unsigned    patch;
};


extern const Version VERSION;
extern const Version COMPATIBLE_VERSION;
} // namespace XL

#endif // VERSION_H
