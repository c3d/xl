// ****************************************************************************
//  compiler-cdecls.cpp                                             Tao project
// ****************************************************************************
//
//   File Description:
//
//    Processing and transforming C declarations into normal XL
//
//    We will never get accurate results without a real C parser.
//    The objective here is only to deal with the most common cases
//    I know that this code doesn't parse "int unsigned typedef x", and
//    I couldn't care less.
//
//
//
// ****************************************************************************
// This document is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#include "compiler-cdecls.h"
#include "types.h"
#include "errors.h"

#include <sstream>

XL_BEGIN

ProcessCDeclaration::ProcessCDeclaration()
// ----------------------------------------------------------------------------
//   Constructor for the C declaration preprocessor
// ----------------------------------------------------------------------------
    : name(NULL),
      returnType(NULL),
      parameters(0)
{}


Tree *ProcessCDeclaration::Declaration(Tree *input)
// ----------------------------------------------------------------------------
//    Process a C declaration given into normal form
// ----------------------------------------------------------------------------
{
    Tree *source = input;
    TreePosition position = input->Position();

    // Strip out the return type and name
    while (Prefix *prefix = input->AsPrefix())
    {
        if (!TypeAndName(prefix->left, returnType, name))
            return NULL;
        input = prefix->right;
    }
        

    // Check if we got a return type or return value
    if (!returnType)
    {
        Ooops("Unable to find return type in $1", source);
        return NULL;
    }
    if (!name)
    {
        Ooops("Unable to find function name in $1", source);
        return NULL;
    }

    // Process parameter declaration
    if (Block *parms = input->AsBlock())
    {
        if (parms->IsParentheses())
        {
            // Process parameters
            input = Parameters(parms);
            if (!input)
                return NULL;
            
            // Assemble the final result
            Prefix *form = new Prefix(name, input, position);
            Infix *result = new Infix(":", form, returnType, position);
            return result;
        }
    }

    Ooops("Malformed C declaration $1", source);
    return NULL;
}


Tree *ProcessCDeclaration::TypeAndName(Tree *input,
                                       Tree_p &declType,
                                       Name_p &declName)
// ----------------------------------------------------------------------------
//   Incrementally build the return type 
// ----------------------------------------------------------------------------
{
    // Check case of pointers
    if (!declType)
    {
        Tree *type = Type(input);
        if (type)
        {
            declType = type;
            return type;
        }
    }

    // If we have a prefix, strip out the left part
    if (Prefix *prefix = input->AsPrefix())
    {
        // Check that we can process the left-hand side
        Tree *result = TypeAndName(prefix->left, declType, declName);
        if (!result)
            return NULL;

        // Check if the right hand side is [], then it's an array
        if (Block *block = prefix->right->AsBlock())
            if (block->IsSquare())
                if (Tree *arrayType = ArrayType(declType))
                    return declType = arrayType;

        // Try with what remains. If it is not a name, we lost
        input = prefix->right;
    }

    // Check if we just have a name, then it's a type
    if (Name *named = input->AsName())
    {
        if (Name *existing = declType->AsName())
            if (Tree *combined = BaroqueTypeMods(existing, named))
                return declType = combined;

        // Check that we don't have two names in a row
        if (declName)
        {
            Ooops("Got second name $1 after $2", named, declName);
            return NULL;
        }

        // Otherwise, that should be the name
        declName = named;
        return named;
    }
         

    Ooops("Unable to make sense of $1 as a C type or name", input);
    return NULL;
}


Tree *ProcessCDeclaration::Parameters(Block *input)
// ----------------------------------------------------------------------------
//   Process the parameters in a C declaration
// ----------------------------------------------------------------------------
{
    Tree *args = input->child;

    // Check if we have a name
    if (Name *named = args->AsName())
    {
        // First special case for empty parameter lists
        if (named->value == "")
            return input;
        if (named->value == "void")
            return new Block(input, new Name("", named->Position()));

        // Otherwise, this has to be a type name, tranform it
        Tree *type = NamedType(named);
        Name *parm = Anonymous();
        Infix *decl = new Infix(":", parm, type, named->Position());
        Block *result = new Block(input, decl);
        return result;
    }

    // Process non-empty parameter lists
    Tree *next = args;
    Tree_p result = NULL;
    Tree_p *parent = &result;

    while (next)
    {
        args = next;
        next = NULL;

        // Check comma-separated lists
        Infix *infix = args->AsInfix();
        if (infix)
        {
            if (infix->name == ",")
            {
                args = infix->left;
                next = infix->right;
            }
        }

        // Check if we have a prefix like 'int x'
        Tree_p declType = NULL;
        Name_p declName = NULL;
        Tree *rewritten = TypeAndName(args, declType, declName);
        if (!rewritten || !declType || !declName)
        {
            Ooops("Invalid declaration $1", args);
            return NULL;
        }

        // Build the replacement declaration
        Infix *decl = new Infix(":", declName, declType, args->Position());
        if (*parent)
        {
            Infix *sep = new Infix(",", *parent, decl, args->Position());
            *parent = sep;
            parent = &sep->right;
        }
        else
        {
            *parent = decl;
        }
    }

    // Return a block for the whole thing
    if (!result)
        result = new Name("", input->Position());
    return new Block (input, result);
}


Tree *ProcessCDeclaration::Type(Tree *input)
// ----------------------------------------------------------------------------
//   Check if something looks like a type
// ----------------------------------------------------------------------------
{
    if (Postfix *postfix = input->AsPostfix())
        if (Tree *ptrType = PointerType(postfix))
            return ptrType;
    if (Name *named = input->AsName())
        if (Tree *namedType = NamedType(named))
            return namedType;

    return NULL;
}


Tree *ProcessCDeclaration::PointerType(Postfix *input)
// ----------------------------------------------------------------------------
//   Create a C pointer type and return it
// ----------------------------------------------------------------------------
{
    if (Name *named = input->right->AsName())
    {
        if (named->value == "*")
        {
            Tree *pointedTo = Type(input->left);
            if (pointedTo)
                return new Infix("to",
                                 new Name("pointer", input->Position()),
                                 pointedTo,
                                 input->Position());
        }
    }

    // Other cases: not a pointer
    return NULL;
}


Tree *ProcessCDeclaration::ArrayType(Tree *pointedTo)
// ----------------------------------------------------------------------------
//   Create an array type. For argument passing, that's a pointer
// ----------------------------------------------------------------------------
{
    Tree *type = Type(pointedTo);
    if (!type)
        return NULL;
    return new Infix("to",
                     new Name("pointer", pointedTo->Position()),
                     type,
                     pointedTo->Position());
}


Name *ProcessCDeclaration::NamedType(Name *input)
// ----------------------------------------------------------------------------
//   Perform type replacements
// ----------------------------------------------------------------------------
{
    text n = input->value;
    static kstring cvt[] =
    {
        "int",          "integer32",
        "char",         "character",
        "short",        "integer16",
        "long",         "integer64",
        "float",        "real32",
        "double",       "real64",
        "unsigned",     "unsigned32",
        "int8_t",       "integer8",
        "int16_t",      "integer16",
        "int32_t",      "integer32",
        "int64_t",      "integer64",
        "uint8_t",      "unsigned8",
        "uint16_t",     "unsigned16",
        "uint32_t",     "unsigned32",
        "uint64_t",     "unsigned64"
    };

    for (uint i = 0; i < sizeof(cvt)/sizeof(cvt[0]); i += 2)
        if (n == cvt[i])
            return new Name(cvt[i+1], input->Position());

    return input;
}


Name *ProcessCDeclaration::BaroqueTypeMods(Name *first, Name *second)
// ----------------------------------------------------------------------------
//   Perform type replacements such as "short int"
// ----------------------------------------------------------------------------
{
    text a = first->value;
    text b = second->value;

    static kstring cvt[] =
    {
        "integer16",    "integer32",    "integer16", // short int
        "integer64",    "integer32",    "integer64", // long int
        "integer16",    "integer16",    "integer16", // short short
        "integer64",    "integer64",    "integer64", // long long
        "integer64",    "real64",       "real80",    // long double

        "integer16",    "unsigned32",    "unsigned16", // short unsigned
        "integer64",    "unsigned32",    "unsigned64", // long unsigned
        "unsigned32",   "unsigned32",    "unsigned32"  // unsigned unsigned
    };

    for (uint i = 0; i < sizeof(cvt)/sizeof(cvt[0]); i += 3)
        if ((a == cvt[i] && b == cvt[i+1]) ||
            (b == cvt[i] && a == cvt[i+1]))
            return new Name(cvt[i+2], first->Position());
            
    Ooops("Invalid type combination $1 and $2, even for C", first, second);
    return NULL;
}


Name *ProcessCDeclaration::Anonymous()
// ----------------------------------------------------------------------------
//   Generate an argument name
// ----------------------------------------------------------------------------
{
    std::ostringstream out;
    out << "arg" << ++parameters;
    text name = out.str();
    return new Name(name);
}

XL_END
