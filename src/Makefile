#******************************************************************************
# Makefile                                                       ELIOT project 
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

# Set to 'none' to disable LLVM
COMPILER=none
#COMPILER=llvm

PREFIX=/usr/local/
PREFIX_BIN=$(PREFIX)bin/
PREFIX_LIB=$(PREFIX)lib/eliot/

# List of modules to build
MODULES=basics io math text remote time-functions temperature
MODULES_SOURCES=$(MODULES:%=%-module.cpp)
MODULES_HEADERS=$(MODULES:%=%-module.h)

# List of source files to process
SOURCES     =					\
	main.cpp				\
	tree.cpp				\
	action.cpp				\
	options.cpp				\
	scanner.cpp				\
	parser.cpp				\
	flight_recorder.cpp			\
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
        traces_base.cpp                         \
	winglob.cpp				\
	$(MODULES_SOURCES)			\
	$(SOURCES_$(COMPILER))

SOURCES_llvm =					\
	compiler.cpp				\
	compiler-llvm.cpp			\
	compiler-gc.cpp				\
	types.cpp				\
	args.cpp				\
	expred.cpp				\
	parms.cpp				\
	unit.cpp				\

PRODUCT=eliot$(EXE)

include $(BROOT)Makefile.config

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
LLVM_LIBS_llvm=$(shell $(LLVM_CONFIG) --libs)
LLVM_LDFLAGS_llvm=$(shell $(LLVM_CONFIG) --ldflags) $(LDFLAGS_llvm$(LLVM_VERSION))

# When LLVM is not enabled
LLVM_FLAGS_none=-DINTERPRETER_ONLY

CPPFLAGS+=$(LLVM_FLAGS) -I . -I include $(CPPFLAGS_llvm$(LLVM_VERSION))
LDFLAGS=$(LLVM_LIBS) $(LLVM_LDFLAGS)
CPPFLAGS_llvm31=-Wno-unused-local-typedefs
CPPFLAGS_llvm30=-Wno-unused-local-typedefs
CPPFLAGS_llvm350=-std=c++11
CPPFLAGS_llvm360=-std=c++11
LDFLAGS_llvm350=-lz -lncurses
LDFLAGS_llvm360=-lz -lncurses

CPPFLAGS+=	-DELIOT_BIN='"'$(PREFIX_BIN)'"'	\
		-DELIOT_LIB='"'$(PREFIX_LIB)'"'

install: all
	cp $(PRODUCT) $(PREFIX_BIN)
	mkdir -p $(PREFIX_LIB)
	cp builtins.eliot eliot.syntax C.syntax *.stylesheet $(PREFIX_LIB)

test:
	cd ../tests; ./alltests
hello: show_$(COMPILER)_version
show_llvm_version:
	@echo Building with LLVM version $(LLVM_VERSION)
show_none_version:
	@echo Building in interpreter-only mode


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


%-module.cpp: %.tbl %-module.h Makefile
	@echo Generating $@ from $<
	./generate-module-body $* > $@

%-module.h: %.tbl Makefile
	@echo Generating $@ from $<
	./generate-module-header $* >  $@
