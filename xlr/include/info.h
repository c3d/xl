#ifndef INFO_H
#define INFO_H
// *****************************************************************************
// info.h                                                             XL project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2003-2004,2006,2010,2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2010-2012, Jérôme Forissier <jerome@taodyne.com>
// (C) 2004, Sébastien Brochet <sebbrochet@sourceforge.net>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can r redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// XL is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with XL, in a file named COPYING.
// If not, see <https://www.gnu.org/licenses/>.
// *****************************************************************************

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
