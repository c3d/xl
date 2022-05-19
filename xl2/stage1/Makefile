#******************************************************************************
# Makefile                          (C) 1992-2003 Christophe de Dinechin (ddd)
#                                                                  XL2 project
#******************************************************************************
#
#  File Description:
#
#    Stage-1 XL compiler
#
#    This compiler must be compiled with the stage-0 compiler (written in C++)
#
#
#
#
#
#
#******************************************************************************
# This program is released under the GNU General Public License.
# See http://www.gnu.org/copyleft/gpl.html for details
#******************************************************************************

SOURCES=compiler.xl
OTHER_SOURCES=					\
	xl.scanner.xs				\
	xl.scanner.xl				\
	xl.errors.xs				\
	xl.errors.xl				\
	xl.parser.tree.xs			\
	xl.parser.tree.xl			\
	xl.parser.xs				\
	xl.parser.xl				\
	xl.translator.xs			\
	xl.translator.xl			\
	xl.textio.xl				\
	xl_lib.h				\
	xl.syntax

STAGE=stage1
BOOTSTRAP=stage0

include ../stage.mk
