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

enum Trace
// ----------------------------------------------------------------------------
//   List the traces known to the compiler
// ----------------------------------------------------------------------------
{
    TRACE_none = 0,
#define OPTVAR(name, type, value)
#define OPTION(name, descr, code)
#define TRACE(name)     TRACE_##name,
#include "options.tbl"
    TRACE_last
};


class Options
/*---------------------------------------------------------------------------*/
/*  Class holding options for the compiler                                   */
/*---------------------------------------------------------------------------*/
{
  public:
    Options(Errors &err);
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
    Errors &            errors;

    static Options *    options;
};

 XL_END

#endif /* XL_OPTIONS_H */
