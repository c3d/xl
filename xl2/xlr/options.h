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
