# ******************************************************************************
# Makefile                                                            XL project
# ******************************************************************************
#
# File description:
#
#    Makefile for the XL runtime environment
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
# (C) 2003-2004,2006,2009-2015,2017,2019, Christophe de Dinechin <christophe@dinechin.org>
# (C) 2010, Jérôme Forissier <jerome@taodyne.com>
# (C) 2005, Lionel Schaffhauser <lionel@taodyne.com>
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

# List of source files to process
SOURCES     =					\
	basics.cpp				\
	flight_recorder.cpp			\
	main.cpp				\
	options.cpp				\
	errors.cpp				\
	tree.cpp				\
	hash.cpp				\
	action.cpp				\
	gc.cpp					\
	syntax.cpp				\
	scanner.cpp				\
	parser.cpp				\
	renderer.cpp				\
	context.cpp				\
	symbols.cpp				\
	runtime.cpp				\
	compiler.cpp				\
	compiler-llvm.cpp			\
	compiler-gc.cpp				\
	types.cpp				\
	args.cpp				\
	cdecls.cpp				\
	expred.cpp				\
	parms.cpp				\
	unit.cpp				\
        sha1.cpp				\
	serializer.cpp				\
        diff.cpp				\
        lcs.cpp                                 \
        traces_base.cpp                         \
	opcodes.cpp				\
	winglob.cpp

PRODUCTS=xl.exe

MIQ=../make-it-quick/
include $(MIQ)rules.mk

LLVM_CONFIG=llvm-config
LLVM_VERSION=$(shell $(LLVM_CONFIG) --version | sed -e s/[.a-z-]//g)

LLVM_FLAGS=$(shell $(LLVM_CONFIG) --cppflags | sed -e s/-DNDEBUG//g) \
	   -DLLVM_VERSION=$(LLVM_VERSION)
LLVM_LIBS=$(shell $(LLVM_CONFIG) --libs --system-libs)
LLVM_LDFLAGS=$(shell $(LLVM_CONFIG) --ldflags)

CPPFLAGS+=-DXLR $(LLVM_FLAGS) -I . -I include $(CPPFLAGS_llvm$(LLVM_VERSION))
LDFLAGS=$(LLVM_LIBS) $(LLVM_LDFLAGS)
CPPFLAGS_llvm31=-Wno-unused-local-typedefs
CPPFLAGS_llvm30=-Wno-unused-local-typedefs
CPPFLAGS_llvm342=-std=c++11 $(LDFLAGS_CPPLIB_$(BUILDENV))
CPPFLAGS_llvm350=-std=c++11 $(LDFLAGS_CPPLIB_$(BUILDENV))
CPPFLAGS_llvm352=-std=c++11 $(LDFLAGS_CPPLIB_$(BUILDENV))
CPPFLAGS_llvm360=-std=c++11 $(LDFLAGS_CPPLIB_$(BUILDENV))

# LLVM 3.9 has the good idea of defining DEBUG!!!
DEFINES_debug=      XL_DEBUG
DEFINES_opt=        XL_DEBUG OPTIMIZED
DEFINES_release=    NDEBUG OPTIMIZED RELEASE

TESTS= sha1.cpp lcs.cpp
.tests: xl_tests

xl_tests: tests/xl
	cd tests; ./alltests

tests/xl: $(MIQ_OUTEXE)
	$(PRINT_COPY) cp $(MIQ_OUTEXE) $@

$(MIQ)rules.mk:
	cd .. && git submodule update --init --recursive
