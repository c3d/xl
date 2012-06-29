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

PRODUCT=xlr$(EXE)

include $(BROOT)Makefile.config

LLVM_FLAGS=`llvm-config --cppflags | sed -e s/-DNDEBUG//g`
LLVM_LIBS=`llvm-config --libs`
LLVM_LDFLAGS=`llvm-config --ldflags`

CPPFLAGS+=-DXLR $(LLVM_FLAGS) -I. -Iinclude
LDFLAGS=-L/usr/local/lib $(LLVM_LIBS) $(LLVM_LDFLAGS)

test: unit_tests xl_tests

unit_tests: sha1_test lcs_test

%_test: %.cpp
	$(CXX) -DUNIT_TEST -Iinclude $(CXXFLAGS) -o $* $*.cpp && ./$* && rm ./$*

xl_tests:
	cd tests; ./alltests
