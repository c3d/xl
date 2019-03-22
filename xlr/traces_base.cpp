// *****************************************************************************
// traces_base.cpp                                                    XL project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2012,2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2010,2012, Jérôme Forissier <jerome@taodyne.com>
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
