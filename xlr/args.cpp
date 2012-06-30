// ****************************************************************************
//  args.cpp                                                        XLR project
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
// This document is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#include "args.h"
#include "save.h"
#include "types.h"
#include "errors.h"
#include "unit.h"

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

    // Defere sequences and function definitions
    if (Infix *infix = val->AsInfix())
        return infix->name == ";" || infix->name == "\n" || infix->name == "->";

    return false;
}


llvm_value RewriteBinding::Closure(CompiledUnit *unit)
// ----------------------------------------------------------------------------
//   Return closure for this value if we need one
// ----------------------------------------------------------------------------
{
    if (!closure)
        if (IsDeferred())
            closure = unit->Closure(name, value);

    return closure;
}


Tree *RewriteCalls::Check (Infix *scope,
                           Tree *what,
                           Infix *candidate)
// ----------------------------------------------------------------------------
//   Check which candidates match, and what binding is required to match
// ----------------------------------------------------------------------------
{
    Errors errors;
    errors.Log(Error("$1 doesn't match because", candidate->left), true);
    RewriteCandidate rc(candidate);
    inference->AssignType(what);

    // Create local type inference deriving from ours
    Context_p childContext = new Context(scope);
    childContext->CreateScope();
    TypeInference_p childInference = new TypeInference(childContext, inference);

    // Attempt binding / unification of parameters to arguments
    XL::Save<TypeInference *> saveInference(inference, childInference);
    Tree *form = candidate->left;
    Tree *defined = RewriteDefined(form);
    Tree *defType = RewriteType(form);
    BindingStrength binding = Bind(childContext, defined, what, rc);
    if (binding == FAILED)
        return NULL;

    // If argument/parameters binding worked, try to typecheck the definition
    Tree *value = candidate->right;
    bool builtin = false;
    if (value && value != xl_self && !value->code)
    {
        // Check if we have a type to match
        if (defType)
        {
            if (!childInference->AssignType(value, defType))
                binding = FAILED;
            if (!childInference->UnifyTypesOf(what, value))
                binding = FAILED;
        }

        // Check built-ins and C functions
        if (Name *name = value->AsName())
            if (name->value == "C")
                builtin = true;
        if (Prefix *prefix = value->AsPrefix())
            if (Name *pfname = prefix->left->AsName())
                if (pfname->value == "opcode" || pfname->value == "C")
                    builtin = true;
        
        if (!builtin)
        {
            bool childSucceeded = childInference->TypeCheck(value);
            if (!childSucceeded)
                binding = FAILED;
        }
    }

    // If we had some errors in the process, binding fails,
    // and we report errors back up, as this may be a bad unification
    if (errors.HadErrors())
        binding = FAILED;

    if (binding != FAILED)
        if (!saveInference.saved->Commit(childInference))
            binding = FAILED;

    // Record the rewrite candidate if we had any success with binding
    if (binding != FAILED)
    {
        // Record the type for that specific expression
        rc.type = childInference->Type(!builtin && value ? value : form);
        rc.types = childInference;
        candidates.push_back(rc);
    }

    // Keep going unless we had a perfect binding
    if (binding == PERFECT)
        return what;
    return NULL;
}


RewriteCalls::BindingStrength RewriteCalls::Bind(Context *context,
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
        type = inference->Type(value);
        if (inference->Unify(type, integer_type, value, form))
        {
            rc.conditions.push_back(RewriteCondition(value, form));
            return POSSIBLE;
        }
        return FAILED;
    }
    case REAL:
    {
        Real *f = (Real *) form;
        if (Real *iv = value->AsReal())
            return iv->value == f->value ? PERFECT : FAILED;
        type = inference->Type(value);
        if (inference->Unify(type, real_type, value, form))
        {
            rc.conditions.push_back(RewriteCondition(value, form));
            return POSSIBLE;
        }
        return FAILED;
    }
    case TEXT:
    {
        Text *f = (Text *) form;
        if (Text *iv = value->AsText())
            return iv->value == f->value ? PERFECT : FAILED;
        type = inference->Type(value);
        if (inference->Unify(type, text_type, value, form))
        {
            rc.conditions.push_back(RewriteCondition(value, form));
            return POSSIBLE;
        }
        return FAILED;
    }

    case NAME:
    {
        Name *f = (Name *) form;
        bool needArg = true;

        // Ignore function name if that is all we have
        if (f == rc.rewrite->left)
            return POSSIBLE;

        // Check if what we have as an expression evaluates correctly
        bool old_matching = inference->matching;
        inference->matching = true;
        if (!value->Do(inference))
            return FAILED;
        inference->matching = old_matching;
        type = inference->Type(value);

        // Test if the name is already bound, and if so, if trees fail to match
        if (Tree *bound = context->Bound(f, true))
        {
            if (bound != f)
            {
                Tree *boundType = inference->Type(bound);
                if (!inference->Unify(boundType, type, form, value))
                    return FAILED;

                // We need to have the same value
                rc.conditions.push_back(RewriteCondition(value, form));

                // Since we are testing an existing value, don't pass arg
                needArg = false;
            }
        }

        // Check if we can unify the value and name types
        Tree *nameType = inference->Type(f);
        if (!inference->Unify(type, nameType, value, form))
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
            type = inference->Type(value);
            if (!inference->Unify(type, fi->right, value, fi->left,
                                  TypeInference::DECLARATION))
                return FAILED;

            // Having been successful makes it a strong binding
            return PERFECT;

        } // We have an infix :
        else if (fi->name == "when")
        {
            // We have a guard - first test if we can bind the left part
            if (Bind(context, fi->left, value, rc) == FAILED)
                return FAILED;

            // Check if we can evaluate the guard
            if (!fi->right->Do(inference))
                return FAILED;

            // Check that the type of the guard is a boolean
            Tree *guardType = inference->Type(fi->right);
            if (!inference->Unify(guardType, boolean_type, fi->right, fi->left))
                return FAILED;

            // Add the guard condition
            rc.conditions.push_back(RewriteCondition(fi->right, xl_true));

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
        bool old_matching = inference->matching;
        inference->matching = true;
        if (!value->Do(inference))
            return FAILED;
        inference->matching = old_matching;

        // Then check if the type matches
        type = inference->Type(value);
        if (!inference->Unify(type, infix_type, value, form))
            return FAILED;

        // If we had to evaluate, we need a runtime pattern match (weak binding)
        return POSSIBLE;
    }

    case PREFIX:
    {
        Prefix *prefixForm = (Prefix *) form;

        // Must match a prefix with the same name
        // REVISIT: Variables that denote a function name...
        Prefix *prefixValue = value->AsPrefix();
        if (!prefixValue)
            return FAILED;
        Name *formName = prefixForm->left->AsName();
        if (!formName)
            return FAILED;
        Name *valueName = prefixValue->left->AsName();
        if (!valueName)
            return FAILED;
        if (formName->value != valueName->value)
            return FAILED;

        return Bind(context, prefixForm->right, prefixValue->right, rc);
    }

    case POSTFIX:
    {
        Postfix *postfixForm = (Postfix *) form;

        // Must match a postfix with the same name
        // REVISIT: Variables that denote a function name...
        Postfix *postfixValue = value->AsPostfix();
        if (!postfixValue)
            return FAILED;
        Name *formName = postfixForm->right->AsName();
        if (!formName)
            return FAILED;
        Name *valueName = postfixValue->right->AsName();
        if (!valueName)
            return FAILED;
        if (formName != valueName)
            return FAILED;

        return Bind(context, postfixForm->left, postfixValue->left, rc);
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

XL_END
