// *****************************************************************************
// compiler-parms.cpp                                                 XL project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2003-2004,2006,2010-2012,2014-2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2012, Jérôme Forissier <jerome@taodyne.com>
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

#include "compiler-args.h"
#include "compiler-unit.h"
#include "compiler-function.h"
#include "errors.h"
#include "llvm-crap.h"

XL_BEGIN

bool ParameterList::EnterName(Name *what, Type_p declaredType)
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
    Type_p type = function.ValueMachineType(what);

    // Check if the name already exists in parameter list, e.g. in 'A+A'
    text name = what->value;
    for (Parameter &p : parameters)
    {
        if (p.name->value == name)
        {
            Type_p nameType = function.ValueMachineType(p.name);
            if (type == nameType)
                return true;

            Ooops("Conflicting machine types for $1", what);
            return false;
        }
    }

    // Check if the name already exists in context, e.g. 'false'
    if (!declaredType)
        if (Context *context = function.FunctionContext())
            if (Context *parent = context->Parent())
                if (parent->Bound(what))
                    return true;

    // If there is a declared parameter type, use it
    if (declaredType)
        type = declaredType;

    // We need to record a new parameter
    parameters.push_back(Parameter(what, type));
    return true;
}


bool ParameterList::Do(Integer *)
// ----------------------------------------------------------------------------
//   Nothing to do for leaves
// ----------------------------------------------------------------------------
{
    return true;
}


bool ParameterList::Do(Real *)
// ----------------------------------------------------------------------------
//   Nothing to do for leaves
// ----------------------------------------------------------------------------
{
    return true;
}


bool ParameterList::Do(Text *)
// ----------------------------------------------------------------------------
//   Nothing to do for leaves
// ----------------------------------------------------------------------------
{
    return true;
}


bool ParameterList::Do(Name *what)
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
    // We need to record a new parameter, type is Tree * by default
    return EnterName(what);
}


bool ParameterList::Do(Block *what)
// ----------------------------------------------------------------------------
//   Parameters may be in a block, we just look inside
// ----------------------------------------------------------------------------
{
    return what->child->Do(this);
}


bool ParameterList::Do(Infix *what)
// ----------------------------------------------------------------------------
//   Check if we match an infix operator
// ----------------------------------------------------------------------------
{
    // Check if we match a type, e.g. 2 vs. 'K : integer'
    if (what->name == ":" || what->name == "as")
    {
        // Check the variable name, e.g. K in example above
        if (Name *varName = what->left->AsName())
        {
            // Enter a name in the parameter list with adequate machine type
            Type_p mtype = function.ValueMachineType(varName);
            Type_p dtype = function.BoxedType(what->right);
            if (dtype != mtype)
            {
                Ooops("Inconsistent machine types between $1 and $2",
                      what->left, what->right);
                return false;
            }
            return EnterName(varName, mtype);
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
            returned = function.ValueMachineType(what);

            // Keep going with the left-hand side
            return what->left->Do(this);
        }
    }

    // If this is the first one, this is what we define
    if (!defined)
    {
        if (what->name == "as")
            return what->left->Do(this);

        defined = what;
        name = "infix[" + what->name + "]";
    }

    // Otherwise, test left and right
    if (!what->left->Do(this))
        return false;
    if (!what->right->Do(this))
        return false;
    return true;
}


bool ParameterList::Do(Prefix *what)
// ----------------------------------------------------------------------------
//   For prefix expressions, simply test left then right
// ----------------------------------------------------------------------------
{
    // In 'if X then Y', 'then' is defined first, but we want 'if'
    Infix *defined_infix = defined->AsInfix();
    text defined_name = name;
    if (defined_infix)
        defined = nullptr;

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


bool ParameterList::Do(Postfix *what)
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
