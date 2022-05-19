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
// This software is licensed under the GNU General Public License v3+
// (C) 2003,2015,2019, Christophe de Dinechin <christophe@dinechin.org>
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
#include "base.h"


enum XlTrace
// ----------------------------------------------------------------------------
//   List the traces known to the compiler
// ----------------------------------------------------------------------------
{
    MZ_TRACE_none = 0,
#define OPTVAR(name, type, value)
#define OPTION(name, descr, code)
#define TRACE(name)     MZ_TRACE_##name,
#include "options.tbl"
    MZ_TRACE_last
};


class XLOptions
/*---------------------------------------------------------------------------*/
/*  Class holding options for the compiler                                   */
/*---------------------------------------------------------------------------*/
{
  public:
    XLOptions();
    text                Parse(int argc, char **argv);
    text                ParseNext();

  public:
    // Read options from the options.tbl file
#define OPTVAR(name, type, value)       type name;
#define OPTION(name, descr, code)
#define TRACE(name)
#include "options.tbl"
#undef OPTVAR
#undef OPTION
#undef TRACE

    int                 arg;
    int                 argc;
    char **             argv;
};




extern XLOptions gOptions;
/*---------------------------------------------------------------------------*/
/*  The options referred to in the whole compiler                            */
/*---------------------------------------------------------------------------*/


#endif /* XL_OPTIONS_H */
