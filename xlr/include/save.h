#ifndef SAVE_H
#define SAVE_H
// ****************************************************************************
//  save.h                                                          Tao project
// ****************************************************************************
//
//   File Description:
//
//     A local helper class that saves and restores a variable
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

#include "base.h"

XL_BEGIN

template <class T>
struct Save
// ----------------------------------------------------------------------------
//    Save a variable locally
// ----------------------------------------------------------------------------
{
    Save (T &source, T value): reference(source), saved(source)
    {
        reference = value;
    }
    Save(const Save &o): reference(o.reference), saved(o.saved) {}
    Save (T &source): reference(source), saved(source)
    {
    }
    ~Save()
    {
        reference = saved;
    }
    operator T()        { return saved; }

    T&  reference;
    T   saved;
};

XL_END

#endif // SAVE_H
