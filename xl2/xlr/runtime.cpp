// ****************************************************************************
//  runtime.cpp                     (C) 1992-2009 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//     Runtime functions necessary to execute XL programs
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
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

#include <iostream>

#include "runtime.h"
#include "tree.h"
#include "renderer.h"
#include "context.h"
#include "options.h"
#include "opcodes.h"


XL_BEGIN

Tree *xl_identity(Tree *what)
// ----------------------------------------------------------------------------
//   Return the input tree unchanged
// ----------------------------------------------------------------------------
{
    return what;
}


Tree *xl_evaluate(Tree *what)
// ----------------------------------------------------------------------------
//   Compile the tree if necessary, then evaluate it
// ----------------------------------------------------------------------------
// This is similar to Context::Run, but we save stack space for recursion
{
    if (!what)
        return what;

    Tree *result = what;
    if (result->Kind() >= NAME)
    {
        if (!what->code)
            what = Context::context->CompileAll(what);
        
        assert(what->code);
        result = what->code(what);
    }
    return result;
}


bool xl_same_text(Tree *what, const char *ref)
// ----------------------------------------------------------------------------
//   Compile the tree if necessary, then evaluate it
// ----------------------------------------------------------------------------
{
    Text *tval = what->AsText(); assert(tval);
    return tval->value == text(ref);
}


bool xl_same_shape(Tree *left, Tree *right)
// ----------------------------------------------------------------------------
//   Check equality of two trees
// ----------------------------------------------------------------------------
{
    XL::TreeMatch compareForEquality(right);
    if (left->Do(compareForEquality))
        return true;
    return false;
}


bool xl_type_check(Tree *value, Tree *type)
// ----------------------------------------------------------------------------
//   Check if value has the type of 'type'
// ----------------------------------------------------------------------------
{
    IFTRACE(typecheck)
        std::cerr << "Type check " << value << " against " << type << '\n';
    if (!value || !type->code)
        return false;
    Tree *afterTypeCast = type->code(value);
    if (afterTypeCast != value)
        return false;
    return true;
}



// ========================================================================
// 
//    Creating entities (callbacks for compiled code)
// 
// ========================================================================

Tree *xl_new_integer(longlong value)
{
    return new Integer(value);
}

Tree *xl_new_real(double value)
{
    return new Real (value);
}

Tree *xl_new_character(kstring value)
{
    return new Text(value, "'", "'");
}

Tree *xl_new_text(kstring value)
{
    return new Text(text(value));
}

Tree *xl_new_xtext(kstring value, kstring open, kstring close)
{
    return new Text(value, open, close);
}


// ========================================================================
// 
//    Type matching
// 
// ========================================================================

Tree *xl_boolean(Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a boolean value (true/false)
// ----------------------------------------------------------------------------
{
    if (value == xl_true || value == xl_false)
        return value;
    return NULL;
}
    
    
Tree *xl_integer(Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as an integer
// ----------------------------------------------------------------------------
{
    if (Integer *it = value->AsInteger())
        return it;
    return NULL;
}
    
    
Tree *xl_real(Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a real
// ----------------------------------------------------------------------------
{
    if (Real *rt = value->AsReal())
        return rt;
    return NULL;
}
    
    
Tree *xl_text(Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a text
// ----------------------------------------------------------------------------
{
    if (Text *tt = value->AsText())
        if (tt->opening != "'")
            return tt;
    return NULL;
}
    
    
Tree *xl_character(Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a character
// ----------------------------------------------------------------------------
{
    if (Text *tt = value->AsText())
        if (tt->opening == "'")
            return tt;
    return NULL;
}
    
    
Tree *xl_tree(Tree *value)
// ----------------------------------------------------------------------------
//   Don't really check the argument
// ----------------------------------------------------------------------------
{
    return value;
}

    
Tree *xl_infix(Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as an infix
// ----------------------------------------------------------------------------
{
    if (Infix *it = value->AsInfix())
        return it;
    return NULL;
}
    
    
Tree *xl_prefix(Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a prefix
// ----------------------------------------------------------------------------
{
    if (Prefix *it = value->AsPrefix())
        return it;
    return NULL;
}
    
    
Tree *xl_postfix(Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a postfix
// ----------------------------------------------------------------------------
{
    if (Postfix *it = value->AsPostfix())
        return it;
    return NULL;
}
    
    
Tree *xl_block(Tree *value)
// ----------------------------------------------------------------------------
//   Check if argument can be evaluated as a block
// ----------------------------------------------------------------------------
{
    if (Block *it = value->AsBlock())
        return it;
    return NULL;
}

XL_END
