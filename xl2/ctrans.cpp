// ****************************************************************************
//  ctrans.cpp                      (C) 1992-2003 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//     XL to C translator
// 
//     This is a very bare-bones translator, designed only to allow
//     low-cost bootstrap of the XL compiler.
// 
//     We care here only about the subset of XL that has the
//     same semantics than C, or semantics that "look and feel" like C
// 
// 
// ****************************************************************************
// This document is confidential.
// Do not redistribute without written permission
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

#include "ctrans.h"
#include "tree.h"
#include "parser.h"
#include <stdio.h>
#include <ctype.h>
#include <map>
#include <iostream>


// ============================================================================
// 
//    Typedefs and globals
// 
// ============================================================================

struct XL2CTranslation;
typedef void (*infix_fn) (XLInfix *);
typedef void (*prefix_fn) (XLPrefix *);
typedef std::map<text, text>            name_map;
typedef std::map<text, infix_fn>        infix_map;
typedef std::map<text, prefix_fn>       prefix_map;
typedef std::map<text, int>             imports_map;

name_map        XLNames;
name_map        XLModules;
prefix_map      XLUnaryOps;
infix_map       XLBinaryOps;
imports_map     XLImports;
int in_parameter_declaration = 0;
int in_procedure = 0;
int in_namespace = 0;
text in_struct = "";
text in_typedef = "";
text in_enum = "";


#define NAME(x,y)
#define SYMBOL(x,y)
#define INFIX(x)        extern void Infix_##x(XLInfix *);
#define PREFIX(x)       extern void Prefix_##x(XLPrefix *);
#define BINARY(x,y)     extern void Infix_##y(XLInfix *);
#define UNARY(x,y)      extern void Prefix_##y(XLPrefix *);
#include "ctrans.tbl"

#define out     (std::cout)



// ============================================================================
// 
//   Initialization
// 
// ============================================================================

void XLInitCTrans()
// ----------------------------------------------------------------------------
//    Initialize the translator
// ----------------------------------------------------------------------------
{
#define NAME(x,y)       XLNames[#x] = #y;
#define SYMBOL(x,y)     XLNames[x] = y;
#define INFIX(x)        XLBinaryOps[#x] = Infix_##x;
#define PREFIX(x)       XLUnaryOps[#x] = Prefix_##x;
#define BINARY(x,y)     XLBinaryOps[x] = Infix_##y;
#define UNARY(x,y)      XLUnaryOps[x] = Prefix_##y;
#include "ctrans.tbl"

    out << "#include \"xl_lib.h\"\n";
}



// ============================================================================
// 
//   Translation engine
// 
// ============================================================================

text XLNormalize (text name)
// ----------------------------------------------------------------------------
//   Return the normalized version of the text
// ----------------------------------------------------------------------------
{
    int i, max = name.length();
    text result = "";
    for (i = 0; i < max; i++)
        if (name[i] != '_')
            result += tolower(name[i]);
    return result;
}


text XLModuleName(XLTree *tree)
// ----------------------------------------------------------------------------
//   Change a tree into a module name
// ----------------------------------------------------------------------------
{
    XLName *name = dynamic_cast<XLName *> (tree);
    if (name)
    {
        text modname = XLNormalize(name->value);
        if (!XLModules.count(modname))
            XLModules[modname] = modname;
        return modname;
    }
    XLInfix *infix = dynamic_cast<XLInfix *> (tree);
    if (infix && infix->name == text("."))
    {
        text result = XLModuleName(infix->left);
        result += ".";
        result += XLModuleName(infix->right);
        return result;
    }
    return "???";
}


struct XL2CTranslation : XLAction
// ----------------------------------------------------------------------------
//   The default action indicates there is more to do...
// ----------------------------------------------------------------------------
{
    XL2CTranslation() {}

    // Override these...
    bool Integer(XLInteger *input);
    bool Real(XLReal *input);
    bool String(XLString *input);
    bool Name(XLName *input);
    bool Block(XLBlock *input);
    bool Prefix(XLPrefix *input);
    bool Infix(XLInfix *input);
    bool Builtin(XLBuiltin *input);
};


bool XL2CTranslation::Integer(XLInteger *input)
// ----------------------------------------------------------------------------
//    Translation of an integer...
// ----------------------------------------------------------------------------
{
    out << input->value;
}


bool XL2CTranslation::Real(XLReal *input)
// ----------------------------------------------------------------------------
//   Translation of a real number
// ----------------------------------------------------------------------------
{
    out << input->value;
}


bool XL2CTranslation::String(XLString *input)
// ----------------------------------------------------------------------------
//    Translation of a string
// ----------------------------------------------------------------------------
{
    out << input->quote << input->value << input->quote;
}


bool XL2CTranslation::Name(XLName *input)
// ----------------------------------------------------------------------------
//    Translation of a name
// ----------------------------------------------------------------------------
{
    text name = XLNormalize(input->value);
    if (XLNames.count(name))
        out << XLNames[name];
    else
        out << name;
}


bool XL2CTranslation::Block(XLBlock *input)
// ----------------------------------------------------------------------------
//    Translate a block
// ----------------------------------------------------------------------------
{
    switch(input->opening)
    {
    case '\t':
        out << "{\n";
        XL2C(input->child);
        if (input->child->Kind() == xlNAME)
            out << "()";
        if (in_enum.length())
            out << "}\n";
        else
            out << ";\n}\n";
        break;
    default:
        out << input->opening;
        XL2C(input->child);
        out << input->closing;
        break;
    }        
}


bool XL2CTranslation::Prefix(XLPrefix *input)
// ----------------------------------------------------------------------------
//    Translate a prefix function or operator
// ----------------------------------------------------------------------------
{
    XLName *name = dynamic_cast<XLName *> (input->left);
    text nname;
    if (name && XLUnaryOps.count(nname = XLNormalize(name->value)))
    {
        prefix_fn unary = XLUnaryOps[nname];
        unary(input);
    }
    else
    {
        XL2C(input->left);
        int has_paren = input->right->Kind() == xlBLOCK;
        if (!has_paren)
            out << "(";
        XL2C(input->right);
        if (!has_paren)
            out << ")";
    }
}


bool XL2CTranslation::Infix(XLInfix *input)
// ----------------------------------------------------------------------------
//    Translate infix operators or functions
// ----------------------------------------------------------------------------
{
    text nname = XLNormalize(input->name);
    if (XLBinaryOps.count(nname))
    {
        infix_fn binary = XLBinaryOps[nname];
        binary(input);
    }
    else
    {
        out << '(';
        XL2C(input->left);
        if (XLNames.count(nname))
            out << ' ' << XLNames[nname] << ' ';
        else
            out << ' ' << nname << ' ';
        XL2C(input->right);
        out << ')';
    }
}


bool XL2CTranslation::Builtin(XLBuiltin *input)
// ----------------------------------------------------------------------------
//    This should not happen
// ----------------------------------------------------------------------------
{
    out << "*** ERROR\n";
}


void XL2C(XLTree *tree)
// ----------------------------------------------------------------------------
//   Take an XL tree as input, and emit the corresponding C code
// ----------------------------------------------------------------------------
{
    XL2CTranslation translator;
    XLDo<XL2CTranslation> run(translator);
    run (tree);
}



// ****************************************************************************
// 
//   Implementation of the various callbacks
// 
// ****************************************************************************

#define PREFIX(x)       void Prefix_##x(XLPrefix *tree)
#define INFIX(x)        void Infix_##x(XLInfix *tree)


// ============================================================================
// 
//   Declarations
// 
// ============================================================================

template<class T> struct ScopeSaver
// ----------------------------------------------------------------------------
//   Temporarily save the value of a variable
// ----------------------------------------------------------------------------
{
    ScopeSaver(T&what, T newVal) : saved(what), value(what) { what = newVal; }
    ~ScopeSaver() { saved = value; }
    T & saved;
    T value;
};
typedef ScopeSaver<int>  IntSaver;
typedef ScopeSaver<text> TextSaver;


PREFIX(import)
// ----------------------------------------------------------------------------
//   Process an 'import' file
// ----------------------------------------------------------------------------
{
    text imported;
    XLInfix *infix = dynamic_cast<XLInfix *> (tree->right);
    if (infix && infix->name == text("="))
    {
        text alias_name = XLModuleName(infix->left);
        text cplusified = "";
        imported = XLModuleName(infix->right);
        int i, max = imported.length();
        for (i = 0; i < max; i++)
        {
            char c = imported[i];
            if (c == '.')
                cplusified += "::";
            else
                cplusified += c;
        }
        XLModules[alias_name] = cplusified;
    }
    else
    {
        imported = XLModuleName(tree->right);
    }

    text interface = imported + ".xs";
    text body = imported + ".xl";

    FILE *iface_file = fopen(interface.c_str(), "r");
    if (iface_file)
    {
        fclose(iface_file);

        if (~XLImports[imported] & 1)
        {
            out << "\n/* " << interface << "*/\n";
            XLParser iface_parser (interface.c_str(), &gContext);
            XLTree *iface_tree = iface_parser.Parse();
            XL2C(iface_tree);
            out << ";\n";
            XLImports[imported] |= 1;
        }
    }

    FILE *body_file = fopen(body.c_str(), "r");
    if (body_file)
    {
        fclose(body_file);
        if (~XLImports[imported] & 2)
        {
            out << "\n/* " << body << "*/\n";
            XLParser body_parser (body.c_str(), &gContext);
            XLTree *body_tree = body_parser.Parse();
            XL2C(body_tree);
            out << ";\n";
            XLImports[imported] |= 2;
        }
    }

    if (!iface_file && !body_file)
        out << "??? NO FILE FOR '" << imported << "'\n";
}


void XLModule2Namespace(XLTree *tree)
// ----------------------------------------------------------------------------
//   Convert an XL module name to a sequence of namespaces
// ----------------------------------------------------------------------------
{
    XLName *name = dynamic_cast<XLName *> (tree);
    if (name)
    {
        text modname = XLNormalize(name->value);
        out << "namespace " << modname << "\n";
        if (!XLModules.count(modname))
            XLModules[modname] = modname;
    }
    else
    {
        XLInfix *infix = dynamic_cast<XLInfix *> (tree);
        if (infix && infix->name == text("."))
        {
            XLModule2Namespace(infix->left);
            out << " {\n";
            in_namespace++;
            XLModule2Namespace(infix->right);
        }
    }
}


PREFIX(module)
// ----------------------------------------------------------------------------
//   If we find a module, insert a "namespace" declaration
// ----------------------------------------------------------------------------
{
    XLInfix *is_tree = dynamic_cast<XLInfix *> (tree->right);
    if (is_tree &&
        (is_tree->name == text("is") || is_tree->name == text("with")))
    {
        XLModule2Namespace(is_tree->left);
        XL2C(is_tree->right);        
    }
    else
    {
        XLPrefix *declare = dynamic_cast<XLPrefix *> (tree->right);
        if (declare)
        {
            out << "\n#warning Deprecated use of module\n";
            XLModule2Namespace(declare->left);
            XL2C(declare->right);
        }
        else
        {
            XLModule2Namespace(tree->right);
            out << "{}";
        }
    }
    while (in_namespace)
    {
        out << "}\n";
        in_namespace--;
    }
}


int XLNamespaceScope(XLTree *tree)
// ----------------------------------------------------------------------------
//   Emit a tree that may correspond to a namespace reference
// ----------------------------------------------------------------------------
{
    XLName *name = dynamic_cast<XLName *> (tree);
    if (name)
    {
        text modname = XLNormalize(name->value);
        if (XLModules.count(modname))
        {
            out << XLModules[modname];
            return 1;
        }
        else
        {
            out << "XLDeref(" << modname << ")";
            return 0;
        }
    }

    XLInfix *infix = dynamic_cast<XLInfix *> (tree);
    if (infix && infix->name == text("."))
    {
        if (XLNamespaceScope(infix->left))
            out << "::";
        else
            out << ".";
        return XLNamespaceScope(infix->right);
    }
    out << "XLDeref(";
    XL2C(tree);
    out << ")";
    return 0;
}

INFIX(scope)
// ----------------------------------------------------------------------------
//   Deal with X.Y, check if X is a namespace name
// ----------------------------------------------------------------------------
{
    if (XLNamespaceScope(tree->left))
        out << "::";
    else
        out << ".";
    XL2C(tree->right);    
}


INFIX(declaration)
// ----------------------------------------------------------------------------
//   Process declarations
// ----------------------------------------------------------------------------
{
    XLInfix *init = dynamic_cast<XLInfix *> (tree->right);
    if (init && init->name == ":=")
    {
        XL2C(init->left);       // Type
        out << ' ';
        XL2C(tree->left);       // Entity name
        out << " = ";
        XL2C(init->right);      // Initializer
    }
    else if (init && init->name == "?=")
    {
        XL2C(init->left);       // Type
        out << ' ';
        XL2C(tree->left);       // Entity name
        out << " = dynamic_cast < ";
        XL2C(init->left);
        out << " > (";
        XL2C(init->right);      // Initializer
        out << ")";
    }
    else
    {
        XL2C(tree->right);        // Type name
        out << ' ';
        XL2C(tree->left);         // Entity name
        if (in_procedure && !in_parameter_declaration)
        {
            out << " = XLDefaultInit< ";
            XL2C(tree->right);
            out << " > :: value()";
        }
    }
}


INFIX(sequence)
// ----------------------------------------------------------------------------
//   Process a sequence of declarations
// ----------------------------------------------------------------------------
{
    XL2C(tree->left);
    if (tree->left->Kind() == xlNAME)
        out << "()";
    if (in_parameter_declaration)
        out << ", ";
    else
        out << ";\n";
    XL2C(tree->right);
    if (tree->right->Kind() == xlNAME)
        out << "()";
}


INFIX(list)
// ----------------------------------------------------------------------------
//   Process a list of values
// ----------------------------------------------------------------------------
{
    XL2C(tree->left);
    out << ", ";
    XL2C(tree->right);
}


INFIX(range)
// ----------------------------------------------------------------------------
//    We represent a range using an std::pair
// ----------------------------------------------------------------------------
{
    out << "XLMakeRange(";
    XL2C(tree->left);
    out << ", ";
    XL2C(tree->right);
    out << ")";
}


INFIX(is)
// ----------------------------------------------------------------------------
//   Process the 'is' keyword
// ----------------------------------------------------------------------------
{
    XL2C(tree->left);
    if (in_procedure && tree->left->Kind() == xlNAME)
        out << "(void)";
    IntSaver in_parm(in_parameter_declaration, 0);
    XL2C(tree->right);
}


INFIX(return)
// ----------------------------------------------------------------------------
//   The infix return is part of function declaration
// ----------------------------------------------------------------------------
{
    XL2C(tree->right);
    out << ' ';
    XL2C(tree->left);
    if (in_procedure && tree->left->Kind() == xlNAME)
        out << "(void)";
}


PREFIX(function)
// ----------------------------------------------------------------------------
//   The prefix function shows its argument
// ----------------------------------------------------------------------------
{
    IntSaver in_parm(in_parameter_declaration, true);
    IntSaver in_proc(in_procedure, true);
    XLInfix *is_part = dynamic_cast<XLInfix *> (tree->right);
    if (is_part && is_part->name == text("is"))
    {
        XLInfix *return_part = dynamic_cast<XLInfix *> (is_part->left);
        if (return_part && return_part->name == text("return"))
        {
            XL2C(return_part);
            out << "\n{\n";
            IntSaver in_parm(in_parameter_declaration, 0);
            XL2C(return_part->right);
            out << " result = XLDefaultInit < ";
            XL2C(return_part->right);
            out << ">::value ();\n";
            XL2C(is_part->right);
            out << "return result;\n";
            out << "}\n";
            return;
        }
    }
    XL2C(tree->right);
}


PREFIX(procedure)
// ----------------------------------------------------------------------------
//   The prefix function shows its argument
// ----------------------------------------------------------------------------
{
    IntSaver in_parm(in_parameter_declaration, true);
    IntSaver in_proc(in_procedure, true);
    out << "void ";
    XL2C(tree->right);
}


PREFIX(type)
// ----------------------------------------------------------------------------
//   Typedefs
// ----------------------------------------------------------------------------
{
    XLInfix *right = dynamic_cast<XLInfix *> (tree->right);
    if (right && right->name == text("is"))
    {
        XLName *name = dynamic_cast<XLName *> (right->left);
        if (name)
        {
            XLInfix *with = dynamic_cast<XLInfix *> (right->right);
            if (with && with->name == text("with"))
            {
                out << "struct ";
                XL2C(right->left);
                name = dynamic_cast<XLName *> (with->left);
                if (!name || name->value != text("record"))
                {
                    out << " : ";
                    XL2C(with->left);
                }
                XL2C(with->right);
            }
            else
            {
                TextSaver type_name(in_typedef, XLNormalize(name->value));
                out << "typedef ";
                XL2C(right->right);
                out << " ";
                XL2C(right->left);
            }
        }
        else
        {
            out << "typedef ";
            XL2C(right->right);
            out << " ";
            XL2C(right->left);
        }
    }
    else if (tree->right->Kind() == xlNAME)
    {
        out << "struct ";
        XL2C(tree->right);
        out << ";\n";
    }
    else
    {
        out << "*** Bad typedef ";
        XL2C(tree->right);    
    }
}


PREFIX(record)
// ----------------------------------------------------------------------------
//   Record types are turned into structs
// ----------------------------------------------------------------------------
{
    out << "#warning Deprecated use of record\n";
    out << "struct " << in_typedef << ' ';
    TextSaver str(in_struct, in_typedef);
    XL2C(tree->right);
}


PREFIX(enumeration)
// ----------------------------------------------------------------------------
//   Enumeration types are turned into enums
// ----------------------------------------------------------------------------
{
    out << "enum " << in_typedef << ' ';
    TextSaver str(in_struct, in_typedef);
    TextSaver en(in_enum, in_typedef);
    XL2C(tree->right);
}



// ============================================================================
// 
//   Statements
// 
// ============================================================================

INFIX(then)
// ----------------------------------------------------------------------------
//   The 'then' is the top-level of an 'if'
// ----------------------------------------------------------------------------
{
    XL2C(tree->left); // The 'if'
    XL2C(tree->right); // The block    
}


INFIX(else)
// ----------------------------------------------------------------------------
//   The 'else' is the top-level of an 'if'
// ----------------------------------------------------------------------------
{
    XL2C(tree->left); // The 'if - then'
    out << "else\n";
    XL2C(tree->right); // The block    
}


PREFIX(loop)
// ----------------------------------------------------------------------------
//    A forever loop
// ----------------------------------------------------------------------------
{
    out << "for(;;)";
    XL2C(tree->right);
}


INFIX(loop)
// ----------------------------------------------------------------------------
//   A while loop
// ----------------------------------------------------------------------------
{
    XL2C(tree->left);
    XL2C(tree->right);
}


PREFIX(for)
// ----------------------------------------------------------------------------
//   Try to mimic the semantics of the XL for loop in a very ad-hoc way
// ----------------------------------------------------------------------------
{
    static int forloop = 0;
    int loop = forloop++;
    out << "XLIterator *XLiter" << loop << " = ";
    XL2C(tree->right);
    out << ";\n";
    out << "for (XLiter" << loop << "->first(); "
        << "XLiter" << loop << "->more() || "
        << "XLDeleteIterator(XLiter" << loop << "); "
        << "XLiter" << loop << "->next())";
}


PREFIX(exit)
// ----------------------------------------------------------------------------
//   Exiting from a loop
// ----------------------------------------------------------------------------
{
    XL2C(tree->right);
    out << " break";
}


INFIX(iterator)
// ----------------------------------------------------------------------------
//    Use for "A in B" to create an iterator
// ----------------------------------------------------------------------------
{
    out << "XLMakeIterator(";
    XL2C(tree->left);
    out << ", ";
    XL2C(tree->right);
    out << ")";
}


PREFIX(in)
// ----------------------------------------------------------------------------
//   We turn in parameters into ... nothing
// ----------------------------------------------------------------------------
{
    XL2C(tree->right);
}


PREFIX(out)
// ----------------------------------------------------------------------------
//   We turn out parameters into references
// ----------------------------------------------------------------------------
{
    out << "&";
    XL2C(tree->right);
}


PREFIX(map)
// ----------------------------------------------------------------------------
//   Change map to use the template type
// ----------------------------------------------------------------------------
{
    XLBlock *block = dynamic_cast<XLBlock *> (tree->right);
    if (block && block->opening == '[' && block->closing == ']')
    {
        out << "std::map < ";
        XL2C(block->child);
        out << " > ";
    }
    else
    {
        out << "*** Unknown map ";
        out << " ::map < ";
        XL2C(tree->right);
        out << " > ";
    }
}

INFIX(of)
// ----------------------------------------------------------------------------
//   We turn A of B into A<B>, expecting a template type
// ----------------------------------------------------------------------------
{
    XL2C(tree->left);
    out << "< ";
    XL2C(tree->right);
    out << " >";
}


INFIX(to)
// ----------------------------------------------------------------------------
//   We turn pointer to X into X &, so that we can use X.Y
// ----------------------------------------------------------------------------
{
    XLName *access = dynamic_cast<XLName *> (tree->left);
    if (access && access->value == text("access"))
    {
        XL2C(tree->right);
        out << "*";
    }
    else
    {
        out << "??? unexpected 'to'";
    }
}


PREFIX(new)
// ----------------------------------------------------------------------------
//   The new operator is transformed into a reference
// ----------------------------------------------------------------------------
{
    out << "(new ";
    XL2C(tree->right);
    out << ")";
}



// ============================================================================
// 
//   Built-in functions
// 
// ============================================================================

text default_stream = "";

void DoWrite(XLTree *arg)
// ----------------------------------------------------------------------------
//   Write an argument
// ----------------------------------------------------------------------------
{
    if (XLInfix *infix = dynamic_cast<XLInfix *> (arg))
    {
        if (infix->name == text(","))
        {
            DoWrite(infix->left);
            out << ";\n";
            DoWrite(infix->right);
            return;
        }
    }
    if (XLName *name = dynamic_cast<XLName *> (arg))
    {
        if (!default_stream.length())
        {
            default_stream = XLNormalize(name->value);
            return;
        }
    }

    out << "write(";
    if (!default_stream.length())
        default_stream = "&std::cout";
    out << default_stream << ", ";
    XL2C(arg);
    out << ")";
}


PREFIX(write)
// ----------------------------------------------------------------------------
//   Implement the built-in "write" function
// ----------------------------------------------------------------------------
{
    default_stream = "";
    if (in_procedure && !in_parameter_declaration)
    {
        DoWrite(tree->right);
    }
    else
    {
        XL2C(tree->left);
        out << ' ';
        XL2C(tree->right);
    }
}


PREFIX(writeln)
// ----------------------------------------------------------------------------
//   Same thing with the built-in writeln function
// ----------------------------------------------------------------------------
{
    default_stream = "";
    if (in_procedure && !in_parameter_declaration)
    {
        DoWrite(tree->right);
        out << ";\nwrite(";
        if (default_stream.length())
            out << default_stream << ", ";
        else
            out << "&std::cout, ";
        out << "\"\\n\");";
    }
    else
    {
        XL2C(tree->left);
        out << ' ';
        XL2C(tree->right);
    }
}


void DoRead(XLTree *arg)
// ----------------------------------------------------------------------------
//   Read an argument
// ----------------------------------------------------------------------------
{
    if (XLInfix *infix = dynamic_cast<XLInfix *> (arg))
    {
        if (infix->name == text(","))
        {
            DoRead(infix->left);
            out << ";\n";
            DoRead(infix->right);
            return;
        }
    }
    if (XLName *name = dynamic_cast<XLName *> (arg))
    {
        if (!default_stream.length())
        {
            default_stream = XLNormalize(name->value);
            return;
        }
    }

    out << "read(";
    if (!default_stream.length())
        default_stream = "&std::cin, ";
    out << default_stream << ", ";
    XL2C(arg);
    out << ")";
}


PREFIX(read)
// ----------------------------------------------------------------------------
//   Implement the built-in "read" function
// ----------------------------------------------------------------------------
{
    default_stream = "";
    if (in_procedure && !in_parameter_declaration)
    {
        DoRead(tree->right);
    }
    else
    {
        XL2C(tree->left);
        out << ' ';
        XL2C(tree->right);
    }
}


PREFIX(readln)
// ----------------------------------------------------------------------------
//   Same thing with the built-in readln function
// ----------------------------------------------------------------------------
{
    default_stream = "";
    if (in_procedure && !in_parameter_declaration)
    {
        DoRead(tree->right);
    }
    else
    {
        XL2C(tree->left);
        out << ' ';
        XL2C(tree->right);
    }
}



// ============================================================================
// 
//   The 'translate' extension
// 
// ============================================================================

typedef std::map< text, int > args_map;


void TranslateForm(XLTree *form, args_map &args, int nesting = 3)
// ----------------------------------------------------------------------------
//   Emit the reference tree
// ----------------------------------------------------------------------------
{
    for (int i = 0; i < nesting; i++)
        out << ' ';

    switch (form->Kind())
    {
    case xlINTEGER:
        out << "xl::parser::tree::newinteger("
            << ((XLInteger *) form)->value << ")";
        break;
    case xlREAL:
        out << "xl::parser::tree::newreal("
            << ((XLReal *) form)->value << ")";
        break;
    case xlNAME:
        out << "xl::parser::tree::newname(text(\""
            << ((XLName *) form)->value << "\"))";
        break;
    case xlSTRING:
        out << "xl::parser::tree::newwildcard(text(\""
            << ((XLString *) form)->value << "\"))";
        args[((XLString *) form)->value] = 1;
        break;

    case xlBLOCK:
        out << "xl::parser::tree::newblock(\n";
        TranslateForm(((XLBlock *) form)->child, args, nesting+2);
        out << ", ";
        if (((XLBlock *) form)->opening == '\t')
        {
            out << "'\\t', '\\n'";
        }
        else
        {
            out << "'" << ((XLBlock *) form)->opening << "'";
            out << ", ";
            out << "'" << ((XLBlock *) form)->closing << "'";
        }
        out << ")";
        break;
    case xlPREFIX:
        out << "xl::parser::tree::newprefix(\n";
        TranslateForm(((XLPrefix *) form)->left, args, nesting+2);
        out << ",\n";
        TranslateForm(((XLPrefix *) form)->right, args, nesting+2);
        out << ")";
        break;
    case xlINFIX:
        out << "xl::parser::tree::newinfix(";
        if (((XLInfix *) form)->name == text("\n"))
            out << "text(\"\\n\"),\n";
        else
            out << "text(\"" << ((XLInfix *) form)->name << "\"),\n";
        TranslateForm(((XLInfix *) form)->left, args, nesting+2);
        out << ",\n";
        TranslateForm(((XLInfix *) form)->right, args, nesting+2);
        out << ")";
        break;
    }
}


void TranslateClauses(XLTree *clauses, XLTree *to_translate)
// ----------------------------------------------------------------------------
//   Translate clauses one after the other
// ----------------------------------------------------------------------------
{
    XLInfix *infix = dynamic_cast<XLInfix *> (clauses);
    if (infix)
    {
        if (infix->name == text("\n"))
        {
            TranslateClauses(infix->left, to_translate);
            TranslateClauses(infix->right, to_translate);
            return;
        }

        if (infix->name == text("then"))
        {
            XLPrefix *when = dynamic_cast<XLPrefix *> (infix->left);
            if (when)
            {
                XLName *when_word = dynamic_cast<XLName *> (when->left);
                if (when_word && when_word->value == text("when"))
                {
                    static int idx = 0;
                    XLBlock *form = dynamic_cast<XLBlock *> (when->right);
                    args_map args;

                    idx++;
                    out << "{\n";
                    out << "static xl::parser::tree::tree ref" << idx
                        << " =\n";
                    TranslateForm(form->child, args);
                    out << ";\n";
                    out << "xl::translator::treemap args" << idx << ";\n";
                    out << "if (xl::translator::matches(";
                    XL2C(to_translate);
                    out << ", ref" << idx << ", args" << idx << ")) {\n";

                    args_map::iterator it;
                    for (it = args.begin(); it != args.end(); it++)
                    {
                        out << "xl::parser::tree::tree "
                            << XLNormalize(it->first)
                            << " = args" << idx
                            << "[text(\"" << it->first << "\")];\n";
                    }
                    XL2C(infix->right);
                    out << ";\n(";
                    XL2C(to_translate);
                    out << " = 0);";
                    out << "\nbreak;\n}\n}\n";

                    return;
                }
            }
            
        }

        if (infix->name == text("else"))
        {
            TranslateClauses(infix->left, to_translate);
            out << "if (";
            XL2C(to_translate);
            out << ") {\n";
            XL2C(infix->right);
            out << "}\n";
            return;                
        }
    }

    out << "*** Ungrokable 'translate' clause\n";
}


PREFIX(translate)
// ----------------------------------------------------------------------------
//    Translate 'translate' statements
// ----------------------------------------------------------------------------
{
    XLPrefix *right = dynamic_cast<XLPrefix *> (tree->right);
    if (right)
    {
        XLTree *to_translate = right->left;
        XLBlock *block = dynamic_cast<XLBlock *> (right->right);
        if (block)
        {
            out << "do {";
            TranslateClauses(block->child, to_translate);
            out << "\n} while (0);\n";
            return;
        }
    }
    out << "*** Ungrokable 'translate' statement\n";
}
