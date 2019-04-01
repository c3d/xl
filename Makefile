# ******************************************************************************
# Makefile                                                            XL project
# ******************************************************************************
#
# File description:
#
#     The top-level makefile for XLR, an extensible programming language
#
#
#
#
#
#
#
#
# ******************************************************************************
# This software is licensed under the GNU General Public License v3
# (C) 2017,2019, Christophe de Dinechin <christophe@dinechin.org>
# ******************************************************************************
# This file is part of XL
#
# XL is free software: you can r redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# XL is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with XL, in a file named COPYING.
# If not, see <https://www.gnu.org/licenses/>.
# ******************************************************************************

SUBDIRS=	recorder src

# Disable generation of .pc file at top-level
PACKAGE_NAME=

MIQ=make-it-quick/
include $(MIQ)rules.mk
$(MIQ)rules.mk:
	$(PRINT_BUILD) git submodule update --init --recursive

# Do not run tests in the recorder
RECURSE_FLAGS_recorder=RUN_TESTS=
