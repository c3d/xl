#ifndef EVALUATOR_H
#define EVALUATOR_H
// ****************************************************************************
//  evaluator.h                                     XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//    An interface for all the kinds of evaluators of the XL language
//
//
//
//
//
//
//
//
// ****************************************************************************
//  (C) 2017 Christophe de Dinechin <christophe@dinechin.org>
//   This software is licensed under the GNU General Public License v3
//   See LICENSE file for details.
// ****************************************************************************

#include "tree.h"

XL_BEGIN

struct Evaluator
// ----------------------------------------------------------------------------
//   Base class for all ways to evaluate an XL tree
// ----------------------------------------------------------------------------
{
    virtual ~Evaluator() {}

    virtual Tree *      Evaluate(Scope *, Tree *source) = 0;
    virtual Tree *      TypeCheck(Scope *, Tree *type, Tree *value) = 0;
    virtual bool        TypeAnalysis(Scope *, Tree *tree) = 0;
};

XL_END

#endif // EVALUATOR_H
