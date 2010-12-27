// ****************************************************************************
//  hash.cpp                                                       XLR project
// ****************************************************************************
//
//   File Description:
//
//    Simple SHA-based hashing algorithm for trees
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

#include "hash.h"
#include "sha1_ostream.h"
#include <sstream>

XL_BEGIN

text sha1(Tree *t)
// ----------------------------------------------------------------------------
//    Compute the SHA-1 for a tree and return it
// ----------------------------------------------------------------------------
{
    text result;
    if (t)
    {
        TreeHashAction<> hash;
        t->Do(hash);

        std::ostringstream os;
        os << t->Get< HashInfo<> > ();

        result = os.str();
    }
    return result;
}

XL_END
