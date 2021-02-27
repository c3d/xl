#ifndef VERSION_H
#define VERSION_H
// *****************************************************************************
// version.h                                                          XL project
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

#include <string>

// Some variants of the GNU library define major and minor macros
#undef major
#undef minor

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
    operator    bool() const;

private:
    unsigned    major;
    unsigned    minor;
    unsigned    patch;

    friend uintptr_t _recorder_arg(const Version &v)
    {
        return (v.major << 16) | (v.minor << 8) | (v.patch << 0);
    }
};


extern const Version VERSION;
extern const Version COMPATIBLE_VERSION;
} // namespace XL

#endif // VERSION_H
