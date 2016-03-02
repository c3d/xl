#ifndef ELFE_OPTIONS_H
#define ELFE_OPTIONS_H
// ****************************************************************************
//  Christophe de Dinechin                                       ELFE PROJECT
//  options.h
// ****************************************************************************
// 
//   File Description:
// 
//     Processing of ELFE compiler options
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

ELFE_BEGIN

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

ELFE_END

#endif /* ELFE_OPTIONS_H */
