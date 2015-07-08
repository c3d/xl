// ****************************************************************************
//  traces_base.cpp                                              ELIOT project
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
// This document is released under the GNU General Public License, with the
// following clarification and exception.
//
// Linking this library statically or dynamically with other modules is making
// a combined work based on this library. Thus, the terms and conditions of the
// GNU General Public License cover the whole combination.
//
// As a special exception, the copyright holders of this library give you
// permission to link this library with independent modules to produce an
// executable, regardless of the license terms of these independent modules,
// and to copy and distribute the resulting executable under terms of your
// choice, provided that you also meet, for each linked independent module,
// the terms and conditions of the license of that module. An independent
// module is a module which is not derived from or based on this library.
// If you modify this library, you may extend this exception to your version
// of the library, but you are not obliged to do so. If you do not wish to
// do so, delete this exception statement from your version.
//
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************


#include "traces.h"
#include <iostream>

namespace ELIOT {

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
    if (enabledNames.count(name))
        groupEnableTrace(name, true);
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

// A trace may be enabled before it is declared (for instance: enabled
// through the command line, later declared by a dynamically loaded module).
// This set records the names of all enabled traces.
std::set<std::string> Traces::enabledNames;

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
    if (!enabledNames.count(name) && enable)
        enabledNames.insert(name);
    else
    if (enabledNames.count(name) && !enable)
        enabledNames.erase(name);

    bool ok = false;
    std::map<std::string, Traces *>::iterator it;
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
    return (enabledNames.count(name) != 0);
}


std::set<std::string> Traces::names()
// ----------------------------------------------------------------------------
//   Return the names of all traces in all trace groups
// ----------------------------------------------------------------------------
{
    std::set<std::string> ret;
    std::map<std::string, Traces *>::iterator it;
    for (it = groups.begin(); it != groups.end(); it++)
    {
        Traces * group = (*it).second;
        std::set<std::string> names = group->groupTraceNames();
        ret.insert(names.begin(), names.end());
    }
    return ret;
}

}
