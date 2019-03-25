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

VARIANTS=lib exe

# List of source files to process
SOURCES_VARIANT_exe=				\
	xl.cpp
SOURCES_VARIANT_lib=				\
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

HEADERS=$(HEADERS_$(VARIANT))
HEADERS_lib=$(wildcard include/*.h include/*.tbl)

PRODUCTS=$(PRODUCTS_$(VARIANT))
PRODUCTS_exe=xl.exe
PRODUCTS_lib=xl.dll

MIQ=../make-it-quick/
include $(MIQ)rules.mk

INCLUDES=. .. include

# LLVM 3.9 has the good idea of defining DEBUG!!! Switch to XL_DEBUG
DEFINES=XLR $(DEFINES_$(VARIANT))
DEFINES_debug=      XL_DEBUG
DEFINES_opt=        XL_DEBUG OPTIMIZED
DEFINES_release=    NDEBUG OPTIMIZED RELEASE

# Name of the 'llvm-config' to use (could be e.g. llvm-config-64-3.5)
LLVM_CONFIG=llvm-config
LLVM_VERSION=$(shell $(LLVM_CONFIG) --version | sed -e s/[.a-z-]//g)

LLVM_FLAGS=$(shell $(LLVM_CONFIG) --cppflags | sed -e s/-DNDEBUG//g) \
	   -DLLVM_VERSION=$(LLVM_VERSION)

# LLVM 3.4 and before have no --system-libs.
# So LLVM did not only develop source code incompatibility to an art form,
# they even made sure the configuration command-line was itself incompatible!
LLVM_SYSLIBS=$(shell $(LLVM_CONFIG) --system-libs 2> /dev/null || true)

LLVM_LIBS=$(shell $(LLVM_CONFIG) --libs) $(LLVM_SYSLIBS)
LLVM_LDFLAGS=$(shell $(LLVM_CONFIG) --ldflags)

CPPFLAGS+=$(LLVM_FLAGS) $(CPPFLAGS_llvm$(LLVM_VERSION))
LDFLAGS_VARIANT_lib=$(LLVM_LIBS) $(LLVM_LDFLAGS) -lrecorder
LDFLAGS_VARIANT_exe=-lxl -lrecorder
CPPFLAGS_llvm31=-Wno-unused-local-typedefs
CPPFLAGS_llvm30=-Wno-unused-local-typedefs
CPPFLAGS_llvm342=-std=c++11 $(LDFLAGS_CPPLIB_$(BUILDENV))
CPPFLAGS_llvm350=-std=c++11 $(LDFLAGS_CPPLIB_$(BUILDENV))
CPPFLAGS_llvm352=-std=c++11 $(LDFLAGS_CPPLIB_$(BUILDENV))
CPPFLAGS_llvm360=-std=c++11 $(LDFLAGS_CPPLIB_$(BUILDENV))

TESTS= sha1.cpp lcs.cpp
.tests: xl_tests

xl_tests: tests/xl
	cd tests; ./alltests

tests/xl: $(MIQ_OUTEXE)
	$(PRINT_COPY) cp $(MIQ_OUTEXE) $@

$(MIQ)rules.mk:
	cd .. && git submodule update --init --recursive
