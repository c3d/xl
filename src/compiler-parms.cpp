// ****************************************************************************
//  parms.cpp                                                    XL project
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
    // We need to record a new parameter, type is Tree * by default
    return EnterName(what);
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


bool ParameterList::DoPrefix(Prefix *what)
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
