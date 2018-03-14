#ifndef COMPILER_H
#define COMPILER_H
// ****************************************************************************
//  compiler.h                       (C) 1992-2009 Christophe de Dinechin (ddd)
//                                                               XL project
// ****************************************************************************
//
//   File Description:
//
//    Just-in-time compiler for the trees
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

#include "tree.h"
#include "context.h"
#include "evaluator.h"
#include "llvm-crap.h"
#include <map>
#include <set>



// ============================================================================
//
//    Forward declarations
//
// ============================================================================

RECORDER_DECLARE(compiler);

XL_BEGIN
// ============================================================================
//
//    Global structures to access the LLVM just-in-time compiler
//
// ============================================================================

struct Compiler : Evaluator, TypeAllocator::Listener
// ----------------------------------------------------------------------------
//   Just-in-time compiler data
// ----------------------------------------------------------------------------
{
    Compiler(kstring name, unsigned opts, int argc, char **argv);
    ~Compiler();

    // Interpreter interface
    Tree *              Evaluate(Scope *, Tree *source) override;
    Tree *              TypeCheck(Scope *, Tree *type, Tree *val) override;
    bool                TypeAnalysis(Scope *, Tree *tree) override;

    // Top-level entry point: analyze and compile a tree or a whole program
    eval_fn             Compile(Scope *scope, Tree *program);

    // Find the machine type corresponding to the tree type or value
    PointerType_p       TreeMachineType(Tree *tree);
    Type_p              MachineType(Tree *tree);

public:
    // Garbage collector listener interface
    void                BeginCollection() override;
    bool                CanDelete (void *obj) override;
    void                EndCollection() override;

public:
    JIT                 jit;
    IntegerType_p       booleanTy;
    IntegerType_p       integerTy;
    IntegerType_p       integer8Ty;
    IntegerType_p       integer16Ty;
    IntegerType_p       integer32Ty;
    IntegerType_p       integer64Ty;
    IntegerType_p       integer128Ty;
    IntegerType_p       unsignedTy;
    IntegerType_p       ulongTy;
    IntegerType_p       ulonglongTy;
    Type_p              realTy;
    Type_p              real32Ty;
    Type_p              real64Ty;
    IntegerType_p       characterTy;
    PointerType_p       charPtrTy;
    PointerType_p       charPtrPtrTy;
    StructType_p        textTy;
    StructType_p        infoTy;
    PointerType_p       infoPtrTy;
    StructType_p        treeTy;
    PointerType_p       treePtrTy;
    PointerType_p       treePtrPtrTy;
    StructType_p        integerTreeTy;
    PointerType_p       integerTreePtrTy;
    StructType_p        realTreeTy;
    PointerType_p       realTreePtrTy;
    StructType_p        textTreeTy;
    PointerType_p       textTreePtrTy;
    StructType_p        nameTreeTy;
    PointerType_p       nameTreePtrTy;
    StructType_p        blockTreeTy;
    PointerType_p       blockTreePtrTy;
    StructType_p        prefixTreeTy;
    PointerType_p       prefixTreePtrTy;
    StructType_p        postfixTreeTy;
    PointerType_p       postfixTreePtrTy;
    StructType_p        infixTreeTy;
    PointerType_p       infixTreePtrTy;
    StructType_p        scopeTy;
    PointerType_p       scopePtrTy;
    FunctionType_p      evalTy;
    PointerType_p       evalFnTy;
};


// ============================================================================
//
//   Useful macros
//
// ============================================================================

// Index in data structures of fields in Tree types
#define TAG_INDEX           0
#define INFO_INDEX          1
#define INTEGER_VALUE_INDEX 2
#define REAL_VALUE_INDEX    2
#define TEXT_VALUE_INDEX    2
#define TEXT_OPENING_INDEX  3
#define TEXT_CLOSING_INDEX  4
#define NAME_VALUE_INDEX    2
#define BLOCK_CHILD_INDEX   2
#define BLOCK_OPENING_INDEX 3
#define BLOCK_CLOSING_INDEX 4
#define LEFT_VALUE_INDEX    2
#define RIGHT_VALUE_INDEX   3
#define INFIX_NAME_INDEX    4

XL_END

#endif // COMPILER_H
