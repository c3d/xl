#ifndef TRACES_H
#define TRACES_H
// *****************************************************************************
// include/traces.h                                                   XL project
// *****************************************************************************
//
// File description:
//
//     Traces declarations. This file relies on traces.tbl to build a class
//     derived from XL::Traces (traces_base.h).
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
// (C) 2019, Christophe de Dinechin <christophe@dinechin.org>
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


#include "traces_base.h"


#define TRACE(name)
#include "traces.tbl"
#undef TRACE

#ifdef XL_TRACE_GROUP

#define _XL_MAKE_TRACE_CLASSNAME(x)    x##Traces
#define  XL_MAKE_TRACE_CLASSNAME(x)   _XL_MAKE_TRACE_CLASSNAME(x)
#define  XL_TRACE_CLASSNAME            XL_MAKE_TRACE_CLASSNAME(XL_TRACE_GROUP)

#define _XL_MAKE_TRACE_INSTNAME(x)     x##TracesInstance
#define  XL_MAKE_TRACE_INSTNAME(x)    _XL_MAKE_TRACE_INSTNAME(x)
#define  XL_TRACE_INSTNAME             XL_MAKE_TRACE_INSTNAME(XL_TRACE_GROUP)

class XL_TRACE_CLASSNAME : public XL::Traces
// ----------------------------------------------------------------------------
//   Wrap all the trace flags for the group
// ----------------------------------------------------------------------------
{
public:

    XL_TRACE_CLASSNAME() :
#define TRACE(name)     name(false),
#include "traces.tbl"
#undef TRACE
        unused(false)
    {
#define TRACE(name)        groupAddTrace(#name, &name);
#include "traces.tbl"
#undef  TRACE
#define _STRINGIFY(x) #x
#define STRINGIFY(x)  _STRINGIFY(x)
        addGroup(STRINGIFY(XL_TRACE_GROUP), this);
#undef  STRINGIFY
    }

#define TRACE(name)     bool name;
#include "traces.tbl"
#undef  TRACE
    bool unused;
};

extern XL_TRACE_CLASSNAME *XL_TRACE_INSTNAME;

#define XL_DEFINE_TRACES  XL_TRACE_CLASSNAME *XL_TRACE_INSTNAME = NULL;
#define XL_INIT_TRACES() \
    do { \
        if (!XL_TRACE_INSTNAME) \
            XL_TRACE_INSTNAME = new XL_TRACE_CLASSNAME(); \
    } while(0)

#endif // XL_TRACE_GROUP

#endif
