#******************************************************************************
# Makefile                          (C) 1992-2003 Christophe de Dinechin (ddd) 
#                                                                  XL2 project 
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

BROOT=../

# List of directories to process
SOURCES     =					\
	main.cpp				\
	options.cpp				\
	errors.cpp				\
	tree.cpp				\
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
	opcodes.cpp

PRODUCT=xlr$(EXE)

include $(BROOT)Makefile.config

LLVM_FLAGS=`llvm-config --cppflags | sed -e s/-DNDEBUG//g`
LLVM_LIBS=`llvm-config --ldflags --libs core jit native interpreter`

CPPFLAGS+=$(LLVM_FLAGS)
LDFLAGS=-L/usr/local/lib $(LLVM_LIBS)

test: sha1_test xl_tests

sha1_test: sha1.cpp
	$(CXX) -DUNIT_TEST $(CXXFLAGS) -o sha1 sha1.cpp && ./sha1 && rm ./sha1

xl_tests:
	cd tests; ./alltests
