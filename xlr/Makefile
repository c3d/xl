#******************************************************************************
# Makefile                          (C) 1992-2003 Christophe de Dinechin (ddd) 
#                                                                  XLR project 
#******************************************************************************
#
#  File Description:
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
#******************************************************************************
# This program is released under the GNU General Public License.
# See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
#******************************************************************************
#* File       : $RCSFile$
#* Revision   : $Revision$
#* Date       : $Date$
#******************************************************************************

VARIANTS=lib exe

# For pkg-config generation
PACKAGE_NAME=xl
PACKAGE_DESCRIPTION=An extensible programming language
PACKAGE_VERSION=1.70.0
PACKAGE_URL="http://github.com/c3d/xl"
PACKAGE_REQUIRES=

# Set to 'none' to disable LLVM - Not yet supported in Tao3D version
#COMPILER=none
COMPILER=llvm

# List of source files to process
SOURCES=$(SOURCES_$(VARIANT))
SOURCES_exe=xl.cpp
SOURCES_lib=					\
	basics.cpp				\
	flight_recorder.cpp			\
	sources.cpp				\
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

HEADERS=$(wildcard include/*.h include/*.tbl)

PRODUCTS=$(PRODUCTS_$(VARIANT))
PRODUCTS_exe=xl.exe
PRODUCTS_lib=xl.dll

SHR_INSTALL=$(SHR_INSTALL_$(VARIANT))
SHR_INSTALL_lib=builtins.xl xl.syntax C.syntax *.stylesheet
HDR_INSTALL=$(HDR_INSTALL_$(VARIANT))
HDR_INSTALL_lib=$(HEADER)

DEFINES=	$(DEFINES_$(COMPILER))				\
		$(DEFINES_$(VARIANT))				\
		XL_PREFIX='"$(PREFIX_SHR)$(PACKAGE_DIR)"'	\
		XL_VERSION='"$(git describe --always --tags --dirty=-dirty)"'
DEFINED_lib=	LIBXLR
DEFINES_llvm=	LLVM_VERSION=$(LLVM_VERSION)
DEFINES_none=	INTERPRETER_ONLY

INCLUDES=. include recorder

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

MIQ=make-it-quick/
include $(MIQ)rules.mk

# Name of 'llvm-config' to use
LLVM_CONFIG=llvm-config

LLVM_VERSION=$(LLVM_VERSION_$(COMPILER))
LLVM_FLAGS=$(LLVM_FLAGS_$(COMPILER))
LLVM_LIBS=$(LLVM_LIBS_$(COMPILER))
LLVM_LDFLAGS=$(LLVM_LDFLAGS_$(COMPILER))

# When LLVM is enabled, we get those
LLVM_VERSION_llvm=  $(shell $(LLVM_CONFIG) --version | sed -e s/[.a-z-]//g)
LLVM_FLAGS_llvm=    $(shell $(LLVM_CONFIG) --cppflags | sed -e s/-DNDEBUG//g)
LLVM_LIBS_llvm=     $(shell $(LLVM_CONFIG) --libs --system-libs)
LLVM_LDFLAGS_llvm=  $(shell $(LLVM_CONFIG) --ldflags)   \
                    $(LDFLAGS_llvm$(LLVM_VERSION))

# When LLVM is not enabled
LLVM_FLAGS_none=    -DINTERPRETER_ONLY

CPPFLAGS+=$(LLVM_FLAGS)  $(CPPFLAGS_llvm$(LLVM_VERSION))
LDFLAGS_VARIANT_lib=$(LLVM_LIBS) $(LLVM_LDFLAGS) $(MINGW_LIBS_$(BUILDENV)) $(RECORDER_LIBS)
LDFLAGS_VARIANT_exe=-lxl
CPPFLAGS_llvm31=-Wno-unused-local-typedefs
CPPFLAGS_llvm30=-Wno-unused-local-typedefs
LDFLAGS_llvm350=-lz -lncurses
LDFLAGS_llvm352=-lz -lncurses
LDFLAGS_llvm360=-lz -lncurses
MINGW_LIBS_mingw=-lws2_32
RECORDER_LIBS=-lpthread -lm

.tests: run-tests$(VARIANT:%=-%)
run-tests: .product
	cd tests; ./alltests
run-tests-exe:
run-tests-lib:
.hello: .show_$(COMPILER)_version
.show_llvm_version:
	@$(INFO) "[INFO]" Building with LLVM version $(LLVM_VERSION)
.show_none_version:
	@$(INFO) "[INFO]" Building in interpreter-only mode
