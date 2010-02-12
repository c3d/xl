// ****************************************************************************
//  opcodes.cpp                     (C) 1992-2009 Christophe de Dinechin (ddd)
//                                                                 XL2 project
// ****************************************************************************
//
//   File Description:
//
//    Opcodes are native trees generated as part of compilation/optimization
//    to speed up execution. They represent a step in the evaluation of
//    the code.
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
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

#include "opcodes.h"
#include "basics.h"
#include <typeinfo>

XL_BEGIN

// ============================================================================
// 
//    Helper functions for native code
// 
// ============================================================================

longlong xl_integer_arg(Tree *value)
// ----------------------------------------------------------------------------
//    Return an integer value 
// ----------------------------------------------------------------------------
{
    if (Integer *ival = value->AsInteger())
        return ival->value;
    Ooops("Value '$1' is not an integer", value);
    return 0;
}


double xl_real_arg(Tree *value)
// ----------------------------------------------------------------------------
//    Return a real value 
// ----------------------------------------------------------------------------
{
    if (Real *rval = value->AsReal())
        return rval->value;
    Ooops("Value '$1' is not a real", value);
    return 0.0;
}


text xl_text_arg(Tree *value)
// ----------------------------------------------------------------------------
//    Return a text value 
// ----------------------------------------------------------------------------
{
    if (Text *tval = value->AsText())
        if (tval->opening != "'")
            return tval->value;
    Ooops("Value '$1' is not a text", value);
    return "";
}


int xl_character_arg(Tree *value)
// ----------------------------------------------------------------------------
//    Return a character value 
// ----------------------------------------------------------------------------
{
    if (Text *tval = value->AsText())
        if (tval->opening == "'" && tval->value.length() == 1)
            return tval->value[0];
    Ooops("Value '$1' is not a character", value);
    return 0;
}


bool xl_boolean_arg(Tree *value)
// ----------------------------------------------------------------------------
//    Return a boolean truth value 
// ----------------------------------------------------------------------------
{
    if (value == xl_true)
        return true;
    else if (value == xl_false)
        return false;
    Ooops("Value '$1' is not a boolean value", value);
    return false;
}


Tree *ParametersTree(tree_list parameters)
// ----------------------------------------------------------------------------
//   Create a comma-separated parameter list
// ----------------------------------------------------------------------------
{
    ulong i, max = parameters.size();
    Tree *result = NULL;
    for (i = 0; i < max; i++)
    {
        Tree *parm = parameters[i];
        if (result)
            result = new Infix(",", result, parm);
        else
            result = parm;
    }
    return result;
}


XL_END

