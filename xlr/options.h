#ifndef XL_OPTIONS_H
#define XL_OPTIONS_H
// ****************************************************************************
//   Christophe de Dinechin                                        XL2 PROJECT
//   XL COMPILER: options.h
// ****************************************************************************
// 
//   File Description:
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
// ****************************************************************************
// This document is distributed under the GNU General Public License
// See the enclosed COPYING file or http://www.gnu.org for information
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

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
    std::vector<text>   args;
    std::vector<text>   files;

    static Options *    options;
};

 XL_END

#endif /* XL_OPTIONS_H */
