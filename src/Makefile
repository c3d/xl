#******************************************************************************
# Makefile                                                       XL project
#******************************************************************************
#
#  File Description:
#
#    Makefile for the Extensible Language for the Internet of Things
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
#  (C) 2000-2015 Christophe de Dinechin <christophe@taodyne.com>
#  (C) 2010-2015 Taodyne SAS
#******************************************************************************

# Define where the build files are located (needs the trailing /)
TOP=../
BUILD=$(TOP)build/

# Set to 'none' to disable LLVM
COMPILER=none
#COMPILER=llvm

# List of modules to build
MODULES=basics io math text remote time_functions temperature
MODULES_SOURCES=$(MODULES:%=%_module.cpp)
MODULES_HEADERS=$(MODULES:%=%_module.h)

# List of source files to process
SOURCES     =					\
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
	recorder/recorder.c			\
	recorder/recorder_ring.c		\
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

PRODUCTS=xl.exe

PREFIX_LIB=$(PREFIX)lib/xl/
LIB_INSTALL=builtins.xl xl.syntax C.syntax *.stylesheet

DEFINES=XL_VERSION='"$(git describe --always --tags --dirty=-dirty)"'
INCLUDES=. includes recorder

CONFIG=		struct_sigaction		\
		struct_stat			\
		sbrk				\
		posix_memalign			\
		mingw_aligned_malloc		\
		<regex.h>			\
		<sys/mman.h>

include $(BUILD)rules.mk

# Name of 'llvm-config' to use (e.g. llvm-config-2.9)
LLVM_CONFIG=llvm-config

LLVM_VERSION=$(LLVM_VERSION_$(COMPILER))
LLVM_FLAGS=$(LLVM_FLAGS_$(COMPILER))
LLVM_LIBS=$(LLVM_LIBS_$(COMPILER))
LLVM_LDFLAGS=$(LLVM_LDFLAGS_$(COMPILER))

# When LLVM is enabled, we get those
LLVM_VERSION_llvm=$(shell $(LLVM_CONFIG) --version | sed -e s/[.a-z-]//g)
LLVM_FLAGS_llvm=$(shell $(LLVM_CONFIG) --cppflags | sed -e s/-DNDEBUG//g) \
		  -DLLVM_VERSION=$(LLVM_VERSION)
LLVM_LIBS_llvm=$(shell $(LLVM_CONFIG) --libs --system-libs)
LLVM_LDFLAGS_llvm=$(shell $(LLVM_CONFIG) --ldflags) $(LDFLAGS_llvm$(LLVM_VERSION))

# When LLVM is not enabled
LLVM_FLAGS_none=-DINTERPRETER_ONLY

CPPFLAGS+=$(LLVM_FLAGS)  $(CPPFLAGS_llvm$(LLVM_VERSION))
LDFLAGS+=$(LLVM_LIBS) $(LLVM_LDFLAGS)
CPPFLAGS_llvm31=-Wno-unused-local-typedefs
CPPFLAGS_llvm30=-Wno-unused-local-typedefs
LDFLAGS_llvm350=-lz -lncurses
LDFLAGS_llvm352=-lz -lncurses
LDFLAGS_llvm360=-lz -lncurses
LDFLAGS_linux  = -lpthread -lm
LDFLAGS_cygwin = -lpthread -lm
LDFLAGS_mingw  = -lpthread -lm -lws2_32

CPPFLAGS+=	-DXL_BIN='"'$(PREFIX_BIN)'"'	\
		-DXL_LIB='"'$(PREFIX_LIB)'"'

test: run-all-xl-tests
run-all-xl-tests: $(TARGET)
	cd ../tests; ./alltests
hello: show_$(COMPILER)_version
show_llvm_version:
	@$(INFO) "[INFO]" Building with LLVM version $(LLVM_VERSION)
show_none_version:
	@$(INFO) "[INFO]" Building in interpreter-only mode

opt_tests opt-tests:
	cd ../tests;		\
	./alltests -r O0;	\
	./alltests -r O1;	\
	./alltests -r O2;	\
	./alltests -r O3


# Module-related rules
clean: modules-clean

modules-clean:
	rm -f $(MODULES_SOURCES) $(MODULES_HEADERS)

interp-%:
	$(MAKE) COMPILER=none $*
ionly-%:
	$(MAKE) COMPILER=none $*

%_module.cpp: %.tbl %_module.h Makefile
	$(PRINT_GENERATE) ./generate-module-body $* > $@

%_module.h: %.tbl Makefile
	$(PRINT_GENERATE) ./generate-module-header $* >  $@

prebuild: 				\
	$(MODULES_HEADERS) 		\
	$(MODULES_SOURCES) 		\
	recorder/recorder.h 		\
	recorder/recorder_ring.h
.PRECIOUS: $(MODULE_HEADERS) $(MODULE_SOURCES)

llvm:
	(cd .. && make llvm)

$(BUILD)rules.mk recorder/recorder.cpp recorder/recorder.h:
	cd .. && git submodule update --init --recursive
