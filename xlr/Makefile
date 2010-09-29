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
	main.cpp				\
	options.cpp				\
	errors.cpp				\
	tree.cpp				\
	gc.cpp					\
	syntax.cpp				\
	scanner.cpp				\
	parser.cpp				\
	renderer.cpp				\
	compiler.cpp				\
	context.cpp				\
	runtime.cpp				\
	basics.cpp				\
	types.cpp				\
        sha1.cpp				\
	serializer.cpp				\
        diff.cpp				\
        lcs.cpp                                 \
	opcodes.cpp

PRODUCT=xlr$(EXE)

include $(BROOT)Makefile.config

LLVM_FLAGS=`llvm-config --cppflags | sed -e s/-DNDEBUG//g`
LLVM_LIBS=`llvm-config --libs core jit native interpreter`
LLVM_LDFLAGS=`llvm-config --ldflags`

CPPFLAGS+=-DXLR $(LLVM_FLAGS) -I.
LDFLAGS=-L/usr/local/lib $(LLVM_LIBS) $(LLVM_LDFLAGS)

test: unit_tests xl_tests

unit_tests: sha1_test lcs_test

%_test: %.cpp
	$(CXX) -DUNIT_TEST $(CXXFLAGS) -o $* $*.cpp && ./$* && rm ./$*

xl_tests:
	cd tests; ./alltests
