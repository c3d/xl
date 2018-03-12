// ****************************************************************************
//  args.cpp                                                     XL project
// ****************************************************************************
//
//   File Description:
//
//    Check if a tree matches the form on the left of a rewrite
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
#include "compiler.h"
#include "save.h"
#include "types.h"
#include "errors.h"
#include "renderer.h"
#include "main.h"
#include "basics.h"


XL_BEGIN

bool RewriteBinding::IsDeferred()
// ----------------------------------------------------------------------------
//   Return true if we want to defer evaluation for the given value
// ----------------------------------------------------------------------------
// We defer evaluation for indent and {} blocks, sequences and functions
{
    Tree *val = value;
    if (Block *block = val->AsBlock())
    {
        // Defer evaluation of indent and {} blocks
        if (block->IsIndent() || block->IsBraces())
            return true;

        // If we have a block with a deferred child, defer
        if (Infix *infix = block->child->AsInfix())
            val = infix;
    }

    // Defer sequences and function definitions
    if (Infix *infix = val->AsInfix())
        return infix->name == ";" || infix->name == "\n" || infix->name == "is";

    return false;
}


llvm_value RewriteBinding::Closure(CompilerFunction &function)
// ----------------------------------------------------------------------------
//   Return closure for this value if we need one
// ----------------------------------------------------------------------------
{
    if (!closure && IsDeferred())
        closure = function.NamedClosure(name, value);

    return closure;
}


Tree *RewriteCalls::Check (Prefix *scope,
                           Tree *what,
                           Infix *candidate)
// ----------------------------------------------------------------------------
//   Check which candidates match, and what binding is required to match
// ----------------------------------------------------------------------------
{
    Errors errors;
    errors.Log(Error("Pattern '$1' doesn't match:", candidate->left), true);
    RewriteCandidate rc(candidate);
    types->AssignType(what);

    // Create local type inference deriving from ours
    Context_p valueContext = types->context;
    Types_p childTypes = new Types(valueContext, types);

    // Create local context in which we will do the bindings
    Context_p childContext = new Context(scope);
    childContext->CreateScope();

    // Attempt binding / unification of parameters to arguments
    Save<Types *> saveTypes(types, childTypes);
    Tree *form = candidate->left;
    Tree *defined = RewriteDefined(form);
    Tree *defType = RewriteType(form);

    BindingStrength binding = Bind(childContext, defined, what, rc);
    if (binding == FAILED)
        return NULL;

    // If argument/parameters binding worked, try to typecheck the definition
    childTypes->context = childContext;
    Tree *value = candidate->right;
    bool builtin = false;
    if (value && value != xl_self)
    {
        // Check if we have a type to match
        if (defType)
        {
            if (!childTypes->AssignType(value, defType))
                binding = FAILED;
            if (!childTypes->UnifyExpressionTypes(what, value))
                binding = FAILED;
        }

        // Check built-ins and C functions
        if (Name *name = value->AsName())
            if (name->value == "C")
                builtin = true;
        if (Prefix *prefix = value->AsPrefix())
            if (Name *pfname = prefix->left->AsName())
                if (pfname->value == "builtin" || pfname->value == "C")
                    builtin = true;

        if (!builtin)
        {
            bool childSucceeded = childTypes->TypeCheck(value);
            if (!childSucceeded)
                binding = FAILED;
        }
    }

    // If we had some errors in the process, binding fails,
    // and we report errors back up, as this may be a bad unification
    if (errors.HadErrors())
        binding = FAILED;

    if (binding != FAILED)
        if (!saveTypes.saved->Commit(childTypes))
            binding = FAILED;

    // Record the rewrite candidate if we had any success with binding
    if (binding != FAILED)
    {
        // Record the type for that specific expression
        rc.type = childTypes->Type(!builtin && value ? value : form);
        rc.types = childTypes;
        candidates.push_back(rc);
    }

    // Keep going unless we had a perfect binding
    if (binding == PERFECT)
        return what;
    return NULL;
}


RewriteCalls::BindingStrength
RewriteCalls::Bind(Context *context,
                   Tree *form,
                   Tree *value,
                   RewriteCandidate &rc)
// ----------------------------------------------------------------------------
//   Attempts to bind 'value' to the pattern form given in 'form'
// ----------------------------------------------------------------------------
{
    Tree *type = NULL;
    kind k = form->Kind();

    switch(k)
    {
    case INTEGER:
    {
        Integer *f = (Integer *) form;
        if (Integer *iv = value->AsInteger())
            return iv->value == f->value ? PERFECT : FAILED;
        type = types->Type(value);
        if (Unify(rc, type, integer_type, value, form))
        {
            rc.Condition(value, form);
            return POSSIBLE;
        }
        return FAILED;
    }
    case REAL:
    {
        Real *f = (Real *) form;
        if (Real *iv = value->AsReal())
            return iv->value == f->value ? PERFECT : FAILED;
        type = types->Type(value);
        if (Unify(rc, type, real_type, value, form))
        {
            rc.Condition(value, form);
            return POSSIBLE;
        }
        return FAILED;
    }
    case TEXT:
    {
        Text *f = (Text *) form;
        if (Text *iv = value->AsText())
            return iv->value == f->value ? PERFECT : FAILED;
        type = types->Type(value);
        if (Unify(rc, type, text_type, value, form))
        {
            rc.Condition(value, form);
            return POSSIBLE;
        }
        return FAILED;
    }

    case NAME:
    {
        Name *f = (Name *) form;
        bool needArg = true;

        // Ignore function name if that is all we have
        Tree *fname = RewriteDefined(rc.rewrite->left);
        if (fname == f)
            return POSSIBLE;

        // Check if what we have as an expression evaluates correctly
        bool old_matching = types->matching;
        types->matching = true;
        if (!value->Do(types))
            return FAILED;
        types->matching = old_matching;
        type = types->Type(value);

        // Test if the name is already bound, and if so, if trees fail to match
        if (Tree *bound = context->Bound(f, true))
        {
            if (bound != f)
            {
                Tree *boundType = types->Type(bound);
                if (!Unify(rc, type, boundType, value, form))
                    return FAILED;

                // We need to have the same value
                rc.Condition(value, form);

                // Since we are testing an existing value, don't pass arg
                needArg = false;
            }
        }

        // Check if we can unify the value and name types
        Tree *nameType = types->Type(f);
        if (!Unify(rc, type, nameType, value, form))
            return FAILED;

        // Enter the name in the context and in the bindings
        if (needArg)
        {
            context->Define(form, value);
            rc.bindings.push_back(RewriteBinding(f, value));
        }
        return POSSIBLE;
    }

    case INFIX:
    {
        Infix *fi = (Infix *) form;

        // Check type declarations
        if (fi->name == ":" || fi->name == "as")
        {
            // Check if we can bind the value from what we know
            if (Bind(context, fi->left, value, rc) == FAILED)
                return FAILED;

            // Add type binding with the given type
            type = types->Type(value);
            if (!Unify(rc, type, fi->right, value, fi->left, true))
                return FAILED;

            // Having been successful makes it a strong binding
            return rc.Unconditional() ? PERFECT : POSSIBLE;

        } // We have an infix :
        else if (fi->name == "when")
        {
            // We have a guard - first test if we can bind the left part
            if (Bind(context, fi->left, value, rc) == FAILED)
                return FAILED;

            // Check if we can evaluate the guard
            if (!fi->right->Do(types))
                return FAILED;

            // Check that the type of the guard is a boolean
            Tree *guardType = types->Type(fi->right);
            if (!Unify(rc, guardType, boolean_type, fi->right, fi->left))
                return FAILED;

            // Add the guard condition
            rc.Condition(fi->right, xl_true);

            // The guard makes the binding weak
            return POSSIBLE;
        }

        // If we match the infix name, we can bind left and right
        if (Infix *infix = value->AsInfix())
        {
            if (fi->name == infix->name)
            {
                BindingStrength left = Bind(context, fi->left, infix->left, rc);
                if (left == FAILED)
                    return FAILED;
                BindingStrength right = Bind(context,fi->right,infix->right,rc);

                // Return the weakest binding
                if (left > right)
                    left = right;
                return left;
            }
        }

        // We may have an expression that evaluates as an infix

        // Check if what we have as an expression evaluates correctly
        bool old_matching = types->matching;
        types->matching = true;
        if (!value->Do(types))
            return FAILED;
        types->matching = old_matching;

        // Then check if the type matches
        type = types->Type(value);
        if (!Unify(rc, type, infix_type, value, form))
            return FAILED;

        // If we had to evaluate, we need a runtime pattern match (weak binding)
        TreePosition pos = form->Position();
        Tree *infixLeft  = new Prefix(new Name("left", pos), value);
        BindingStrength left  = Bind(context, fi->left, infixLeft, rc);
        if (left == FAILED)
            return FAILED;
        Tree *infixRight = new Prefix(new Name("right", pos), value);
        BindingStrength right = Bind(context, fi->right, infixRight, rc);

        // Add a condition on the infix name
        Tree *infixName = new Prefix(new Name("name", pos), value);
        if (!infixName->Do(types))
            return FAILED;
        Tree *infixRequiredName = new Text(fi->name, pos);
        if (!infixRequiredName->Do(types))
            return FAILED;
        rc.Condition(infixName, infixRequiredName);

        // Return weakest binding
        if (left > right)
            left = right;
        return left;
    }

    case PREFIX:
    {
        Prefix *prefixForm = (Prefix *) form;

        // Must match a postfix with the same name
        if (Prefix *prefixValue = value->AsPrefix())
        {
            BindingStrength ok = BindBinary(context,
                                            prefixForm->left,
                                            prefixValue->left,
                                            prefixForm->right,
                                            prefixValue->right,
                                            rc);
            if (ok != FAILED)
                return ok;
        }
        return FAILED;
    }

    case POSTFIX:
    {
        Postfix *postfixForm = (Postfix *) form;

        // Must match a postfix with the same name
        // REVISIT: Variables that denote a function name...
        if (Postfix *postfixValue = value->AsPostfix())
        {
            BindingStrength ok = BindBinary(context,
                                            postfixForm->right,
                                            postfixValue->right,
                                            postfixForm->left,
                                            postfixValue->left,
                                            rc);
            if (ok != FAILED)
                return ok;
        }
        return FAILED;
    }

    case BLOCK:
    {
        // Ignore blocks, just look inside
        Block *block = (Block *) form;
        return Bind(context, block->child, value, rc);
    }
    }

    // Default is to return false
    return FAILED;
}


RewriteCalls::BindingStrength
RewriteCalls::BindBinary(Context *context,
                         Tree *form1, Tree *value1,
                         Tree *form2, Tree *value2,
                         RewriteCandidate &rc)
// ----------------------------------------------------------------------------
//    Bind a binary form (prefix or postfix)
// ----------------------------------------------------------------------------
{
    // Check if we have the same name as operand, e.g 'sin X' vs 'sin (A+B)'
    Name *formName = form1->AsName();
    if (!formName)
        return FAILED;
    Name *valueName = value1->AsName();
    if (!valueName)
        return FAILED;
    if (formName->value != valueName->value)
        return FAILED;

    return Bind(context, form2, value2, rc);

}


RECORDER(calltypes, 64, "Type information in calls");

bool RewriteCalls::Unify(RewriteCandidate &rc,
                         Tree *valueType, Tree *formType,
                         Tree *value, Tree *form,
                         bool declaration)
// ----------------------------------------------------------------------------
//   Check unification for types in a given candidate
// ----------------------------------------------------------------------------
{
    Tree *refType = types->LookupTypeName(valueType);

    record(calltypes,
           "Unify %t as %t with %t as %t", value, valueType, form, formType);

    // If we have a tree, it may have the right type, must check at runtime
    if (refType == tree_type)
    {
        Tree *vrefType = types->LookupTypeName(formType);
        kind k = valueType->Kind();
        Compiler &compiler = *MAIN->compiler;
        if (k == INTEGER || vrefType == integer_type)
            rc.KindCondition(value, INTEGER, compiler.integerTreePtrTy);
        else if (k == REAL || vrefType == real_type)
            rc.KindCondition(value, REAL, compiler.realTreePtrTy);
        else if (k == TEXT || vrefType == text_type)
            rc.KindCondition(value, TEXT, compiler.textTreePtrTy);
        else if (vrefType == name_type || vrefType == boolean_type)
            rc.KindCondition(value, NAME, compiler.nameTreePtrTy);
        else if (vrefType == block_type)
            rc.KindCondition(value, BLOCK, compiler.blockTreePtrTy);
        else if (k == INFIX || vrefType == infix_type)
            rc.KindCondition(value, INFIX, compiler.infixTreePtrTy);
        else if (vrefType == prefix_type)
            rc.KindCondition(value, PREFIX, compiler.prefixTreePtrTy);
        else if (vrefType == postfix_type)
            rc.KindCondition(value, POSTFIX, compiler.postfixTreePtrTy);
    }

    // Otherwise, do type inference
    return types->Unify(valueType, formType, value, form,
                        declaration ? Types::DECLARATION : Types::STANDARD);
}


XL_END
