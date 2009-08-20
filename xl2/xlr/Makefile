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
	scanner.cpp				\
	tree.cpp				\
	context.cpp				\
	syntax.cpp				\
	parser.cpp				\
	renderer.cpp				\
	options.cpp				\
	errors.cpp				\
	basics.cpp				\
	types.cpp

PRODUCT=xlr$(EXE)

include $(BROOT)Makefile.config

CXXFLAGS+= -I/usr/X11/include -I/usr/X11/include/freetype2
LDFLAGS=-Wl,-framework -Wl,ApplicationServices \
	-Wl,-framework -Wl,OpenGL \
	-Wl,-framework -Wl,GLUT \
