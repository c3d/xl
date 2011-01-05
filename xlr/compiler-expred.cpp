// ****************************************************************************
//  compiler-expred.cpp                                             XLR project
// ****************************************************************************
// 
//   File Description:
// 
//    Information required by the compiler for expression reduction
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

#include "compiler-expred.h"
#include "compiler-unit.h"
#include "compiler-arg.h"
#include "types.h"

#include <llvm/Support/IRBuilder.h>
#include <llvm/GlobalVariable.h>

XL_BEGIN

using namespace llvm;



// ============================================================================
// 
//    Compile an expression
// 
// ============================================================================

llvm_value CompileExpression::DoInteger(Integer *what)
// ----------------------------------------------------------------------------
//   Compile an integer constant
// ----------------------------------------------------------------------------
{
    Compiler *compiler = unit->compiler;
    return ConstantInt::get(compiler->integerTy, what->value);
}
    

llvm_value CompileExpression::DoReal(Real *what)
// ----------------------------------------------------------------------------
//   Compile a real constant
// ----------------------------------------------------------------------------
{
    Compiler *compiler = unit->compiler;
    return ConstantFP::get(compiler->realTy, what->value);
}


llvm_value CompileExpression::DoText(Text *what)
// ----------------------------------------------------------------------------
//   Compile a text constant
// ----------------------------------------------------------------------------
{
    Compiler *compiler = unit->compiler;
    GlobalVariable *global = compiler->TextConstant(what->value);
    return unit->code->CreateConstGEP2_32(global, 0U, 0U);
}


llvm_value CompileExpression::DoName(Name *what)
// ----------------------------------------------------------------------------
//   Compile a name
// ----------------------------------------------------------------------------
{
    Context_p  where;
    Rewrite_p  rewrite;
    Context   *context  = unit->context;
    Tree      *existing = context->Bound(what, Context::SCOPE_LOOKUP,
                                         &where, &rewrite);
    assert(existing || !"Type checking didn't realize a name is missing");
    if (where == context)
        return unit->Known(rewrite->from);

    // For now assume a global
    return unit->Global(what);
}


llvm_value CompileExpression::DoInfix(Infix *infix)
// ----------------------------------------------------------------------------
//   Compile infix expressions
// ----------------------------------------------------------------------------
{
    // Sequences
    if (infix->name == "\n" || infix->name == ";")
    {
        infix->left->Do(this);
        return infix->right->Do(this);
    }

    // Type casts - REVISIT: may need to do some actual conversion
    if (infix->name == ":")
    {
        return infix->left->Do(this);
    }

    // Declarations: enter a function
    if (infix->name == "->")
    {
        
    }

    // General case: expression
    return DoCall(infix);
}


llvm_value CompileExpression::DoPrefix(Prefix *what)
// ----------------------------------------------------------------------------
//   Compile prefix expressions
// ----------------------------------------------------------------------------
{
    return DoCall(what);
}


llvm_value CompileExpression::DoPostfix(Postfix *what)
// ----------------------------------------------------------------------------
//   Compile postfix expressions
// ----------------------------------------------------------------------------
{
    return DoCall(what);
}


llvm_value CompileExpression::DoBlock(Block *block)
// ----------------------------------------------------------------------------
//   Compile blocks
// ----------------------------------------------------------------------------
{
    return block->child->Do(this);
}


llvm_value CompileExpression::DoCall(Tree *call)
// ----------------------------------------------------------------------------
//   Compile expressions into calls for the right expression
// ----------------------------------------------------------------------------
{
    rcall_map &rcalls = unit->inference->rcalls;
    rcall_map::iterator found = rcalls.find(call);
    if (found != rcalls.end())
    {
        RewriteCalls *rc = (*found).second;
        RewriteCandidates &calls = rc->candidates;
        if (calls.size() == 1)
        {
            RewriteCandidate &cand = calls[0];
            Rewrite *rw = cand.rewrite;

            // Check if this is an LLVM builtin
            Tree *builtin = NULL;
            if (rw->to)
                if (Prefix *prefix = rw->to->AsPrefix())
                    if (Name *name = prefix->left->AsName())
                        if (name->value == "opcode")
                            builtin = prefix->right;

            // Evaluate parameters
            std::vector<llvm_value> args;
            std::vector<RewriteBinding> &bnds = cand.bindings;
            std::vector<RewriteBinding>::iterator b;
            for (b = bnds.begin(); b != bnds.end(); b++)
            {
                Tree *tree = (*b).value;
                llvm_value value = tree->Do(this);
                args.push_back(value);
            }

            if (builtin)
            {
                llvm_builder bld = unit->code;
                if (Prefix *prefix = builtin->AsPrefix())
                {
                    if (Name *name = prefix->left->AsName())
                    {
                        if (name->value == "data")
                        {
                            bld = unit->data;
                            builtin = prefix->right;
                        }
                    }
                }

                if (Name *name = builtin->AsName())
                {
                    Compiler *compiler = unit->compiler;
                    text op = name->value;
                    uint sz = args.size();
                    llvm_value *a = &args[0];
                    return compiler->Primitive(bld, op, sz, a);
                }                        
            }
            else
            {
                llvm_value function = unit->Compile(rw, cand.calls);
                if (function)
                {
                    llvm_value result =
                    unit->code->CreateCall(function, args.begin(), args.end());
                    return result;
                }
            }
        }
        else
        {
            assert(!"Not implemented yet");
        }
    }
    return NULL;
}



// ============================================================================
// 
//    Expression reduction
// 
// ============================================================================
//   An expression reduction typically compiles as:
//     if (cond1) if (cond2) if (cond3) invoke(T)
//   However, we may determine during compilation of if(cond2) that the
//   call is statically not valid. So we save the initial basic block,
//   and decide at the end to connect it or not. Let LLVM optimize branches
//   and dead code away...

ExpressionReduction::ExpressionReduction(CompiledUnit &u, Tree *src)
// ----------------------------------------------------------------------------
//    Snapshot current basic blocks in the compiled unit
// ----------------------------------------------------------------------------
    : unit(u), source(src), llvm(u.llvm),
      storage(NULL), computed(NULL),
      savedfailbb(NULL),
      entrybb(NULL), savedbb(NULL), successbb(NULL)
{
    // We need storage and a compute flag to skip this computation if needed
    storage = u.NeedStorage(src);
    computed = u.NeedLazy(src);

    // Save compile unit's data
    savedfailbb = u.failbb;
    u.failbb = NULL;

    // Create the end-of-expression point if we find a match or had it already
    successbb = u.BeginLazy(src);
}


ExpressionReduction::~ExpressionReduction()
// ----------------------------------------------------------------------------
//   Destructor restores the prevous sucess and fail points
// ----------------------------------------------------------------------------
{
    CompiledUnit &u = unit;

    // Mark the end of a lazy expression evaluation
    u.EndLazy(source, successbb);

    // Restore saved 'failbb' and value map
    u.failbb = savedfailbb;
}


void ExpressionReduction::NewForm ()
// ----------------------------------------------------------------------------
//    Indicate that we are testing a new form for evaluating the expression
// ----------------------------------------------------------------------------
{
    CompiledUnit &u = unit;

    // Save previous basic blocks in the compiled unit
    savedbb = u.code->GetInsertBlock();
    assert(savedbb || !"NewForm called after unconditional success");

    // Create entry / exit basic blocks for this expression
    entrybb = BasicBlock::Create(*llvm, "subexpr", u.function);
    u.failbb = NULL;

    // Set the insertion point to the new invokation code
    u.code->SetInsertPoint(entrybb);

}


void ExpressionReduction::Succeeded(void)
// ----------------------------------------------------------------------------
//   We successfully compiled a reduction for that expression
// ----------------------------------------------------------------------------
//   In that case, we connect the basic blocks to evaluate the expression
{
    CompiledUnit &u = unit;

    // Branch from current point (end of expression) to exit of evaluation
    u.code->CreateBr(successbb);

    // Branch from initial basic block position to this subcase
    u.code->SetInsertPoint(savedbb);
    u.code->CreateBr(entrybb);

    // If there were tests, we keep testing from that 'else' spot
    if (u.failbb)
    {
        u.code->SetInsertPoint(u.failbb);
    }
    else
    {
        // Create a fake basic block in case someone decides to add code
        BasicBlock *empty = BasicBlock::Create(*llvm, "empty", u.function);
        u.code->SetInsertPoint(empty);
    }
    u.failbb = NULL;
}


void ExpressionReduction::Failed()
// ----------------------------------------------------------------------------
//    We figured out statically that the current form doesn't apply
// ----------------------------------------------------------------------------
{
    CompiledUnit &u = unit;

    u.CallFormError(source);
    u.code->CreateBr(successbb);
    if (u.failbb)
    {
        IRBuilder<> failTail(u.failbb);
        u.code->SetInsertPoint(u.failbb);
        u.CallFormError(source);
        failTail.CreateBr(successbb);
        u.failbb = NULL;
    }

    u.code->SetInsertPoint(savedbb);
}


XL_END
