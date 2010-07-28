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
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

#include <string>
#include "base.h"

XL_BEGIN

class Errors;

struct Traces
// ----------------------------------------------------------------------------
//   List the traces known to the compiler
// ----------------------------------------------------------------------------
{
#define OPTVAR(name, type, value)
#define OPTION(name, descr, code)
#define TRACE(name)     bool name : 1;
#include "options.tbl"

    Traces() {
#define OPTVAR(name, type, value)
#define OPTION(name, descr, code)
#define TRACE(name)     name = false;
#include "options.tbl"
    }
};


class Options
/*---------------------------------------------------------------------------*/
/*  Class holding options for the compiler                                   */
/*---------------------------------------------------------------------------*/
{
  public:
    Options(Errors &err, int argc, char **argv);
    text                Parse(int argc, char **argv, bool consumeFile);
    text                ParseNext(bool consumeFile = true);

  public:
    // Read options from the options.tbl file
#define OPTVAR(name, type, value)       type name;
#define OPTION(name, descr, code)
#define TRACE(name)
#include "options.tbl"
#undef OPTVAR
#undef OPTION
#undef TRACE

    Traces              traces;
    int                 arg;
    int                 argc;
    char **             argv;
    Errors &            errors;

    static Options *    options;
};

 XL_END

#endif /* XL_OPTIONS_H */
