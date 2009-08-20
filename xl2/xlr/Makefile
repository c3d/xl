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
	parser.cpp				\
	options.cpp				\
	errors.cpp

PRODUCT=xlr$(EXE)

include $(BROOT)Makefile.config

CXXFLAGS+= -I/usr/X11/include -I/usr/X11/include/freetype2
LDFLAGS=-Wl,-framework -Wl,ApplicationServices \
	-Wl,-framework -Wl,OpenGL \
	-Wl,-framework -Wl,GLUT \

errors.tbl: errors.txt
	@echo Generating errors messages table
	@awk '/\#.*/ { next } /^[A-Za-z_].*/ { printf "XL_ERROR(E_%s, ", $$1; printf "\""; for (i = 2; i <= NF; i++) printf (i<NF ? "%s " : "%s"), $$i; print "\")"; }' $< > $@
	@echo "#undef XL_ERROR" >> $@
