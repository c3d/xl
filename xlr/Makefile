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
	runtime.cpp				\
	interpreter.cpp				\
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
        traces_base.cpp                         \
	opcodes.cpp				\
	winglob.cpp

PRODUCT=xlr$(EXE)

include $(BROOT)Makefile.config

LLVM_CONFIG=llvm-config
LLVM_VERSION=$(shell $(LLVM_CONFIG) --version | sed -e s/[.a-z-]//g)

LLVM_FLAGS=$(shell $(LLVM_CONFIG) --cppflags | sed -e s/-DNDEBUG//g) \
	   -DLLVM_VERSION=$(LLVM_VERSION)
LLVM_LIBS=$(shell $(LLVM_CONFIG) --libs)
LLVM_LDFLAGS=$(shell $(LLVM_CONFIG) --ldflags) $(LDFLAGS_llvm$(LLVM_VERSION))

CPPFLAGS+=-DXLR $(LLVM_FLAGS) -I . -I include $(CPPFLAGS_llvm$(LLVM_VERSION))
LDFLAGS=$(LLVM_LIBS) $(LLVM_LDFLAGS)
CPPFLAGS_llvm31=-Wno-unused-local-typedefs
CPPFLAGS_llvm30=-Wno-unused-local-typedefs
CPPFLAGS_llvm350=-std=c++11
CPPFLAGS_llvm360=-std=c++11
LDFLAGS_llvm350=-lz -lncurses
LDFLAGS_llvm360=-lz -lncurses

test: unit_tests xl_tests
hello: show_llvm_version
show_llvm_version:
	@echo Building wiht LLVM version $(LLVM_VERSION)

unit_tests: sha1_test 

%_test: %.cpp
	$(CXX) -DUNIT_TEST -Iinclude $(CXXFLAGS) -o $*.exe $*.cpp && ./$*.exe && rm ./$*.exe

xl_tests:
	cd tests; ./alltests

opt_tests opt-tests:
	cd tests;		\
	./alltests -r O0;	\
	./alltests -r O1;	\
	./alltests -r O2;	\
	./alltests -r O3

