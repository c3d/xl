#ifndef INFO_H
#define INFO_H
// ****************************************************************************
//  info.h                                                          Tao project
// ****************************************************************************
//
//   File Description:
//
//    Information that can be attached to trees
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

struct Info
// ----------------------------------------------------------------------------
//   Information associated with a tree
// ----------------------------------------------------------------------------
{
                        Info(): next(NULL) {}
                        Info(const Info &) : next(NULL) {}
    virtual             ~Info() {}
    virtual void        Delete() { delete this; }
    virtual Info *      Copy() { return next ? next->Copy() : NULL; }
    Info *next;
};

XL_END

#endif // INFO_H
