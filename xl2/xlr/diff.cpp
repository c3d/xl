// ****************************************************************************
//  diff.cpp                                                        XLR project
// ****************************************************************************
//
//   File Description:
//
//     Implementation of the tree diffing and patching algorithms.
//     Based on paper:
//         "Change Detection in Hierarchically Structured Information"
//         (Stanford University, 1996)
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

#include "diff.h"

XL_BEGIN


Tree *TreeDiff::SetNodeIdAction::Do(Tree *what)
{
    what->Set<NodeIdInfo>(current_id++);

    return what;
}

void TreeDiff::AssignNodeIds(Tree *t)
// ----------------------------------------------------------------------------
//    Assign a unique node_id identifier to each node of a tree
// ----------------------------------------------------------------------------
{
    // Nodes are numbered in breadth-first order, starting from the current
    // value of next_node_id (which is updated on exit)

    TreeDiff::SetNodeIdAction action(next_node_id);

    next_node_id = action.current_id;
}

matching *TreeDiff::FastMatch(Tree *t1, Tree *t2)
// ----------------------------------------------------------------------------
//    Compute a "good" matching between trees t1 and t2
// ----------------------------------------------------------------------------
{
    matching * M = new matching;

    return M;
}

XL_END
