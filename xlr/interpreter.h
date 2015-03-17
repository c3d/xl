#ifndef INTERPRETER_H
#define INTERPRETER_H
// ****************************************************************************
//  interpreter.h                                                  XLR project
// ****************************************************************************
//
//   File Description:
//
//     A fully interpreted mode for XLR, that does not rely on LLVM at all
//
//
//
//
//
//
//
//
// ****************************************************************************
//  (C) 2015 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2015 Taodyne SAS
// ****************************************************************************

#include "tree.h"
#include "context.h"
#include "opcodes.h"


XL_BEGIN

Tree *Evaluate(Context *context, Tree *code);
Tree *EvaluateClosure(Context *context, Tree *code);
Tree *TypeCheck(Context *scope, Tree *type, Tree *value);
Tree *IsClosure(Tree *value, Context_p *context);


struct ClosureInfo : Info
// ----------------------------------------------------------------------------
//   Mark a given Prefix as a closure
// ----------------------------------------------------------------------------
{};

XL_END

#endif // INTERPRETER_H
