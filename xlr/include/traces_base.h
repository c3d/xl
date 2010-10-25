#ifndef TRACES_BASE_H
#define TRACES_BASE_H
// ****************************************************************************
//   traces_base.h                                                 XLR project
//
// ****************************************************************************
//
//   File Description:
//
//     Debug trace management. The XLR runtime, as well as any other binary
//     linked against it (when built as a library), can use the Traces class
//     to define new trace levels.
//     A trace level has a name and may be enabled through the command line
//     (-t<trace_name>).
//
//
//
//
//
// ****************************************************************************
// This document is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
// ****************************************************************************

// Usage
//
// 1. Tracing
//
//      #include "base.h"
//      IFTRACE(trace_name)
//          do_something();
//
// 2. Defining a new trace level
//
//    Edit the traces.tbl file and append:
//
//      TRACE(new_trace_name)
//
// 3. Using traces in another binary
//
//    Suppose you want to write a program or library that depends on XLR, and
//    want to use XLR traces. You need to:
//    a. Copy traces.tbl into your project and edit group name / trace names.
//    You must have a traces.tbl file in your project even if you don't want
//    to use traces. In this case, just leave traces.tbl empty.
//    b. Somewhere in one of your source files, define the trace object for
//    your library (with XL_DEFINE_TRACES) and call the initialization macro
//    XL_INIT_TRACES(). Typically:
//
//      #include "traces.h"
//      XL_DEFINE_TRACES
//
//      int main(int argc, char *argv[])
//      {
//          XL_INIT_TRACES();
//          ...
//      }

#include <set>
#include <string>
#include <map>

namespace XL {

class Traces
// ----------------------------------------------------------------------------
//   Manage trace flags for an XLR process. Traces are organized in groups.
// ----------------------------------------------------------------------------
{
public:

    Traces() {}

    static std::set<std::string>  names();
    static bool                   enable(std::string name, bool enable = true);
    static bool                   enabled(std::string name);

protected:

    static void  addGroup(std::string name, struct Traces * inst);

protected:

    bool                   groupEnableTrace(std::string name,
                                            bool enable = true);
    bool                   groupTraceEnabled(std::string name);
    void                   groupAddTrace(std::string name, bool * flagptr);
    std::set<std::string>  groupTraceNames();

private:

    std::map<std::string, bool *>           flags;
    static std::map<std::string, Traces *>  groups;
    static std::set<std::string>            enabledNames;
};

}

#endif // TRACES_BASE_H
