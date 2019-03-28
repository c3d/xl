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

# For pkg-config generation
PACKAGE_NAME=xl
PACKAGE_VERSION=0.01
PACKAGE_DESCRIPTION=An extensible programming language
PACKAGE_URL="http://github.com/c3d/xl"
PACKAGE_REQUIRES=

VARIANTS=lib exe

# Set to 'none' to disable LLVM
#COMPILER=none
COMPILER=llvm

# List of modules to build
MODULES=basics io math text remote time_functions temperature
MODULES_SOURCES=$(MODULES:%=%_module.cpp)
MODULES_HEADERS=$(MODULES:%=%_module.h)

# List of source files to process
SOURCES_VARIANT_exe=				\
	xl.cpp
SOURCES_VARIANT_lib     =			\
	$(SOURCES_$(COMPILER))			\
	main.cpp				\
	tree.cpp				\
	action.cpp				\
	options.cpp				\
	scanner.cpp				\
	parser.cpp				\
	errors.cpp				\
	gc.cpp					\
	syntax.cpp				\
	renderer.cpp				\
	context.cpp				\
	runtime.cpp				\
	bytecode.cpp				\
	interpreter.cpp				\
	opcodes.cpp				\
	cdecls.cpp				\
	serializer.cpp				\
	utf8_fileutils.cpp			\
	winglob.cpp				\
	$(MODULES_SOURCES)

SOURCES_llvm =					\
	llvm-crap.cpp				\
	compiler.cpp				\
	compiler-unit.cpp			\
	compiler-prototype.cpp			\
	compiler-function.cpp			\
	compiler-expr.cpp			\
	compiler-args.cpp			\
	types.cpp				\

HEADERS=$(HEADERS_$(VARIANT))
HEADERS_lib=$(wildcard ../include/*.h *.tbl)

PRODUCTS=$(PRODUCTS_$(VARIANT))
PRODUCTS_exe=xl.exe
PRODUCTS_lib=xl.dll

LIB_INSTALL=builtins.xl xl.syntax C.syntax $(wildcard *.stylesheet)

DEFINES=	$(DEFINES_$(COMPILER))		\
		XL_VERSION='"$(git describe --always --tags --dirty=-dirty)"'
DEFINES_llvm=	LLVM_VERSION=$(LLVM_VERSION)
DEFINES_none=	INTERPRETER_ONLY

INCLUDES=. .. ../include

CONFIG= sigaction                       \
        struct_stat                     \
        sbrk                            \
        posix_memalign                  \
        mingw_aligned_malloc            \
        <regex.h>                       \
        <sys/mman.h>                    \
        <sys/socket.h>                  \
        libregex                        \
        drand48                         \
        glob                            \
        uchar ushort uint ulong longlong

MIQ=../make-it-quick/
include $(MIQ)rules.mk

# Name of 'llvm-config' to use - Note that XL now requires llvm 6.0 or above
LLVM_CONFIG=llvm-config

LLVM_VERSION=$(LLVM_VERSION_$(COMPILER))
LLVM_FLAGS=$(LLVM_FLAGS_$(COMPILER))
LLVM_LIBS=$(LLVM_LIBS_$(COMPILER))
LLVM_LDFLAGS=$(LLVM_LDFLAGS_$(COMPILER))

# When LLVM is enabled, we get those
LLVM_VERSION_llvm=  $(shell $(LLVM_CONFIG) --version | sed -e s/[.a-z-]//g)
LLVM_FLAGS_llvm=    $(shell $(LLVM_CONFIG) --cppflags | sed -e s/-DNDEBUG//g)
LLVM_LIBS_llvm=     $(shell $(LLVM_CONFIG) --libs) $(LLVM_SYSLIBS)
# LLVM 3.4 and before have no --system-libs.
# So LLVM did not only develop source code incompatibility to an art form,
# they even made sure the configuration command-line was itself incompatible!
LLVM_SYSLIBS=$(shell $(LLVM_CONFIG) --system-libs 2> /dev/null || true)
LLVM_LDFLAGS_llvm=  $(shell $(LLVM_CONFIG) --ldflags)

# When LLVM is not enabled
LLVM_FLAGS_none=-DINTERPRETER_ONLY

CPPFLAGS+=$(LLVM_FLAGS)  $(CPPFLAGS_llvm$(LLVM_VERSION))
LDFLAGS=$(LDFLAGS_$(VARIANT))
LDFLAGS_lib=$(LLVM_LIBS) $(LLVM_LDFLAGS) $(MINGW_LIBS_$(BUILDENV)) -lrecorder
LDFLAGS_exe=-lxl -lrecorder

# Compiler options to disable annoying version-specific warnings
CPPFLAGS_llvm31=-Wno-unused-local-typedefs
CPPFLAGS_llvm30=-Wno-unused-local-typedefs
CPPFLAGS_llvm342=-std=c++11 $(LDFLAGS_CPPLIB_$(BUILDENV))
CPPFLAGS_llvm350=-std=c++11 $(LDFLAGS_CPPLIB_$(BUILDENV))
CPPFLAGS_llvm352=-std=c++11 $(LDFLAGS_CPPLIB_$(BUILDENV))
CPPFLAGS_llvm360=-std=c++11 $(LDFLAGS_CPPLIB_$(BUILDENV))

# Additional dependencies
MINGW_LIBS_mingw=-lws2_32

CPPFLAGS+=	-DXL_BIN='"'$(PREFIX_BIN)'"'	\
		-DXL_LIB='"'$(PREFIX_LIB)'"'

.tests: run-all-xl-tests
run-all-xl-tests: .product
	cd ../tests; $(TEST_ENV) ./alltests
.hello: .show_$(COMPILER)_version
.show_llvm_version:
	@$(INFO) "[INFO]" Building with LLVM version $(LLVM_VERSION)
.show_none_version:
	@$(INFO) "[INFO]" Building in interpreter-only mode

# Module-related rules
ifndef RECURSE
.clean: modules-clean

modules-clean:
	rm -f $(MODULES_SOURCES) $(MODULES_HEADERS)

interp-%:
	$(MAKE) COMPILER=none $*
ionly-%:
	$(MAKE) COMPILER=none $*
endif

# Module generation
%_module.cpp: ../include/%.tbl %_module.h Makefile
	$(PRINT_GENERATE) ./generate-module-body $* > $@

%_module.h: ../include/%.tbl Makefile
	$(PRINT_GENERATE) ./generate-module-header ../include/$* >  $@

.PRECIOUS: $(MODULES_HEADERS) $(MODULES_SOURCES)

$(MIQ)rules.mk:
	$(PRINT_COMMAND) cd .. && git submodule update --init --recursive
$(MIQ_DEPENDENCIES): 	$(MODULES_HEADERS) 	\
			../recorder/recorder.h ../recorder/recorder_ring.h
