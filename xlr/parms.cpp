// ****************************************************************************
//  parms.cpp                                                      XLR project
// ****************************************************************************
// 
//   File Description:
// 
//    Actions collecting parameters on the left of a rewrite
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

#include "parms.h"
#include "args.h"
#include "unit.h"
#include "errors.h"

#include "llvm/DerivedTypes.h"
#include "llvm/Function.h"

XL_BEGIN


bool ParameterList::EnterName(Name *what,
                              bool untyped)
// ----------------------------------------------------------------------------
//   Enter a name in the parameter list
// ----------------------------------------------------------------------------
{
    // We only allow names here, not symbols (bug #154)
    if (what->value.length() == 0 || !isalpha(what->value[0]))
    {
        Ooops("The pattern variable $1 is not a name", what);
        return false;
    }

    // Check the LLVM type for the given form
    llvm_type type = unit->ExpressionMachineType(what);

    // Check if the name already exists in parameter list, e.g. in 'A+A'
    text name = what->value;
    Parameters::iterator it;
    for (it = parameters.begin(); it != parameters.end(); it++)
    {
        if ((*it).name->value == name)
        {
            llvm_type nameType = unit->ExpressionMachineType((*it).name);
            if (type == nameType)
                return true;

            Ooops("Conflicting machine types for $1", what);
            return false;
        }
    }

    // Check if the name already exists in context, e.g. 'false'
    if (untyped)
        if (unit->context->scope->Bound(what))
            return true;
        
    // We need to record a new parameter
    parameters.push_back(Parameter(what, type));
    return true;
}


bool ParameterList::DoInteger(Integer *)
// ----------------------------------------------------------------------------
//   Nothing to do for leaves
// ----------------------------------------------------------------------------
{
    return true;
}


bool ParameterList::DoReal(Real *)
// ----------------------------------------------------------------------------
//   Nothing to do for leaves
// ----------------------------------------------------------------------------
{
    return true;
}


bool ParameterList::DoText(Text *)
// ----------------------------------------------------------------------------
//   Nothing to do for leaves
// ----------------------------------------------------------------------------
{
    return true;
}


bool ParameterList::DoName(Name *what)
// ----------------------------------------------------------------------------
//    Identify the named parameters being defined in the shape
// ----------------------------------------------------------------------------
{
    if (!defined)
    {
        // The first name we see must match exactly, e.g. 'sin' in 'sin X'
        defined = what;
        name = what->value;
        return true;
    }
    else
    {
        // We need to record a new parameter, type is Tree * by default
        return EnterName(what, true);
    }
}


bool ParameterList::DoBlock(Block *what)
// ----------------------------------------------------------------------------
//   Parameters may be in a block, we just look inside
// ----------------------------------------------------------------------------
{
    return what->child->Do(this);
}


bool ParameterList::DoInfix(Infix *what)
// ----------------------------------------------------------------------------
//   Check if we match an infix operator
// ----------------------------------------------------------------------------
{
    // Check if we match a type, e.g. 2 vs. 'K : integer'
    if (what->name == ":")
    {
        // Check the variable name, e.g. K in example above
        if (Name *varName = what->left->AsName())
        {
            // Enter a name in the parameter list with adequate machine type
            llvm_type mtype = unit->MachineType(what->right);
            llvm_type ntype = unit->ExpressionMachineType(varName);
            if (mtype != ntype)
            {
                Ooops("Conflicting machine type for declaration $1", what);
                return false;
            }
            return EnterName(varName, false);
        }
        else
        {
            // We ar specifying the type of the expression, e.g. (X+Y):integer
            if (returned || defined)
            {
                Ooops("Cannot specify type of $1", what->left);
                return false;
            }

            // Remember the specified returned value
            returned = unit->ExpressionMachineType(what);

            // Keep going with the left-hand side
            return what->left->Do(this);
        }
    }

    // If this is the first one, this is what we define
    if (!defined)
    {
        defined = what;
        name = what->name;
    }

    // Otherwise, test left and right
    if (!what->left->Do(this))
        return false;
    if (!what->right->Do(this))
        return false;
    return true;
}


bool ParameterList::DoPrefix(Prefix *what)
// ----------------------------------------------------------------------------
//   For prefix expressions, simply test left then right
// ----------------------------------------------------------------------------
{
    // In 'if X then Y', 'then' is defined first, but we want 'if'
    Infix *defined_infix = defined->AsInfix();
    text defined_name = name;
    if (defined_infix)
        defined = NULL;

    if (!what->left->Do(this))
        return false;
    if (!what->right->Do(this))
        return false;

    if (!defined && defined_infix)
    {
        defined = defined_infix;
        name = defined_name;
    }

    return true;
}


bool ParameterList::DoPostfix(Postfix *what)
// ----------------------------------------------------------------------------
//    For postfix expressions, simply test right, then left
// ----------------------------------------------------------------------------
{
    // Note that ordering is reverse compared to prefix, so that
    // the 'defined' names is set correctly
    if (!what->right->Do(this))
        return false;
    if (!what->left->Do(this))
        return false;
    return true;
}

XL_END
