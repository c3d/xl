// ****************************************************************************
//  posix.cpp                                                        XL project
// ****************************************************************************
//
//   File Description:
//
//     Expose a number of useful POSIX functions
//
//
//
//
//
//
//
//
// ****************************************************************************
//   (C) 2021 Christophe de Dinechin <christophe@dinechin.org>
//   This software is licensed under the GNU General Public License v3
// ****************************************************************************
//   This file is part of XL.
//
//   XL is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   XL is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with XL.  If not, see <https://www.gnu.org/licenses/>.
// ****************************************************************************

#include "native.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

XL_BEGIN

NATIVE(getpid);
NATIVE(putchar);

// Need to eliminate the noreturn attribute which makes instantiation fail
void exit(int rc) { ::exit(rc); }
NATIVE(exit);

XL_END
