// ****************************************************************************
//  traces_base.cpp                                                XLR project
// ****************************************************************************
//
//   File Description:
//
//     Implementation of debug trace functions
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
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************


#include "traces.h"
#include <iostream>

namespace XL {

// ============================================================================
//
//    Trace group management
//
// ============================================================================

void Traces::groupAddTrace(std::string name, bool * flagptr)
// ----------------------------------------------------------------------------
//   Associate trace flag with name so that flag can be accessed by name
// ----------------------------------------------------------------------------
{
    flags[name] = flagptr;
}


bool Traces::groupEnableTrace(std::string name, bool enable)
// ----------------------------------------------------------------------------
//   Enable or disable a trace level in this group
// ----------------------------------------------------------------------------
{
    if (!flags.count(name))
        return false;
    *(flags[name]) = enable;
    return true;
}


bool Traces::groupTraceEnabled(std::string name)
// ----------------------------------------------------------------------------
//   Check if trace level is enabled in this group
// ----------------------------------------------------------------------------
{
    if (!flags.count(name))
        return false;
    return *(flags[name]);
}


std::set<std::string> Traces::groupTraceNames()
// ----------------------------------------------------------------------------
//   Return the names of all traces in this group
// ----------------------------------------------------------------------------
{
    std::set<std::string> ret;
    std::map<std::string, bool *>::iterator it;
    for (it = flags.begin(); it != flags.end(); it++)
        ret.insert((*it).first);
    return ret;
}

// ============================================================================
//
//    Global trace management
//
// ============================================================================

std::map<std::string, Traces *> Traces::groups;


void Traces::addGroup(std::string name, Traces *inst)
// ----------------------------------------------------------------------------
//   Add a trace group to the global list of all groups
// ----------------------------------------------------------------------------
{
    if (groups.count(name))
    {
        std::cerr << "Trace group '" << name << "' already registered\n";
        return;
    }
    groups[name] = inst;
}


bool Traces::enable(std::string name, bool enable)
// ----------------------------------------------------------------------------
//   Enable or disable a trace level in any group
// ----------------------------------------------------------------------------
{
    bool ok = false;
    std::map<std::string, struct Traces *>::iterator it;
    for (it = groups.begin(); it != groups.end(); it++)
        if ((*it).second->groupEnableTrace(name, enable))
            ok = true;
    return ok;
}


bool Traces::enabled(std::string name)
// ----------------------------------------------------------------------------
//   Check if trace level is enabled in any group
// ----------------------------------------------------------------------------
{
    std::map<std::string, struct Traces *>::iterator it;
    for (it = groups.begin(); it != groups.end(); it++)
        if ((*it).second->groupTraceEnabled(name))
            return true;
    return false;
}


std::set<std::string> Traces::names()
// ----------------------------------------------------------------------------
//   Return the names of all traces in all trace groups
// ----------------------------------------------------------------------------
{
    std::set<std::string> ret;
    std::map<std::string, struct Traces *>::iterator it;
    for (it = groups.begin(); it != groups.end(); it++)
    {
        Traces * group = (*it).second;
        std::set<std::string> names = group->groupTraceNames();
        ret.insert(names.begin(), names.end());
    }
    return ret;
}

}
