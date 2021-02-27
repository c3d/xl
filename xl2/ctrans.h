#ifndef CTRANS_H
#define CTRANS_H
// *****************************************************************************
// ctrans.h                                                           XL project
// *****************************************************************************
//
// File description:
//
//     XL to C translator
//
//     This is a very bare-bones translator, designed only to allow
//     low-cost bootstrap of the XL compiler
//
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2003,2015,2019-2020, Christophe de Dinechin <christophe@dinechin.org>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License,
// or (at your option) any later version.
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
#include "context.h"

extern void XLInitCTrans();
extern void XL2C(XLTree *tree);

#endif // CTRANS_H
