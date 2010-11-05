#ifndef TRACES_H
#define TRACES_H
// ****************************************************************************
//   traces.h                                                      XLR project
//
// ****************************************************************************
//
//   File Description:
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
// ****************************************************************************
// This document is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
// ****************************************************************************


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
