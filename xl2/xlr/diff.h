#ifndef DIFF_H
#define DIFF_H
// ****************************************************************************
//  diff.h                                                          XLR project
// ****************************************************************************
//
//   File Description:
//
//     Definitions for tree diffing and patching.
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
//  (C) 2010 Jerome Forissier <jerome@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

#include "base.h"
#include "tree.h"
#include <set>

XL_BEGIN

// ============================================================================
//
//    The types being defined or used to manipulate tree diffs
//
// ============================================================================

struct NodePair;                        // Two node identifiers
typedef std::set<NodePair *> matching;  // A correspondence between tree nodes


// ============================================================================
//
//    The TreeDiff class
//
// ============================================================================


struct NodePair
// ----------------------------------------------------------------------------
//   A pair of node identifiers
// ----------------------------------------------------------------------------
{
    NodePair(node_id x, node_id y): x(x), y(y) {}
    virtual ~NodePair() {}

    node_id x, y;
};

struct TreeDiff
// ----------------------------------------------------------------------------
//   All you need to compare and patch parse trees
// ----------------------------------------------------------------------------
{
    TreeDiff (): next_node_id(0) {}
    virtual ~TreeDiff() {}

protected:

    struct SetNodeIdAction : XL::Action
    {
        SetNodeIdAction(node_id start_from): current_id(start_from) {}
        virtual Tree *Do(Tree *what);

        node_id current_id;
    };

protected:

    matching * FastMatch(Tree *t1, Tree *t2);
    void AssignNodeIds(Tree *t);

protected:

    node_id next_node_id;
};

XL_END

#endif // DIFF_H
