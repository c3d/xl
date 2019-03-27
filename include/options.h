#ifndef XL_OPTIONS_H
#define XL_OPTIONS_H
// *****************************************************************************
// options.h                                                          XL project
// *****************************************************************************
//
// File description:
//
//     Processing of XL compiler options
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
// (C) 2003,2009-2012,2014-2017,2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2010, Jérôme Forissier <jerome@taodyne.com>
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

#include <string>
#include <vector>
#include "base.h"

XL_BEGIN

struct Errors;


struct Options
/*---------------------------------------------------------------------------*/
/*  Class holding options for the compiler                                   */
/*---------------------------------------------------------------------------*/
{
  public:
    Options(int argc, char **argv);
    text ParseFirst(bool consumeFile = true);
    text ParseNext(bool consumeFile = true);

  public:
    // Read options from the options.tbl file
#define OPTVAR(name, type, value)       type name;
#define OPTION(name, descr, code)
#include "options.tbl"
#undef OPTVAR
#undef OPTION

    uint                arg;
    kstring             argt;
    std::vector<text>   args;
    std::vector<text>   files;

    static Options *    options;
};

XL_END

#endif /* XL_OPTIONS_H */
