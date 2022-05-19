#******************************************************************************
# Makefile                          (C) 1992-2003 Christophe de Dinechin (ddd)
#                                                                  XL2 project
#******************************************************************************
#
#  File Description:
#
#    Makefile for the native version of the XL compiler
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

STAGE=stage2
BOOTSTRAP=stage1

SOURCES=compiler.xl
OTHER_SOURCES=					\
	xl.scanner.xs				\
	xl.scanner.xl				\
	xl.scanner.position.xs			\
	xl.scanner.position.xl			\
	xl.errors.xs				\
	xl.errors.xl				\
	xl.parser.tree.xs			\
	xl.parser.tree.xl			\
	xl.parser.xs				\
	xl.parser.xl				\
	xl.translator.xs			\
	xl.translator.xl			\
	xl.symbols.xs				\
	xl.symbols.xl				\
	xl.constants.xl				\
	xl.semantics.xs				\
	xl.semantics.xl				\
	xl.semantics.declarations.xs		\
	xl.semantics.declarations.xl		\
	xl.semantics.macros.xs			\
	xl.semantics.macros.xl			\
	xl.semantics.functions.xs		\
	xl.semantics.functions.xl		\
	xl.semantics.iterators.xs		\
	xl.semantics.iterators.xl		\
	xl.semantics.overload.xs		\
	xl.semantics.overload.xl		\
	xl.semantics.generics.xs		\
	xl.semantics.generics.xl		\
	xl.semantics.instructions.xs		\
	xl.semantics.instructions.xl		\
	xl.semantics.modules.xs			\
	xl.semantics.modules.xl			\
	xl.semantics.types.xs			\
	xl.semantics.types.xl			\
	xl.semantics.types.functions.xs		\
	xl.semantics.types.functions.xl		\
	xl.semantics.types.records.xs		\
	xl.semantics.types.records.xl		\
	xl.semantics.types.enumerations.xs	\
	xl.semantics.types.enumerations.xl	\
	xl.semantics.types.generics.xs		\
	xl.semantics.types.generics.xl		\
	xl.semantics.writtenforms.xs		\
	xl.semantics.writtenforms.xl		\
	xl.semantics.pragmas.xl			\
	xl.codegenerator.xs			\
	xl.codegenerator.xl			\
	xl.renderer.xs				\
	xl.renderer.xl				\
	xl.codegenerator.machine.xs		\
	xl.codegenerator.machine.xl		\
	xl.bytecode.xs				\
	xl.bytecode.xl				\
	xl.optimizer.xs				\
	xl.optimizer.xl				\
	xl.textio.xl				\
	xl_lib.h				\
	xl.syntax				\
						\
	xl.plugin.xl2c.xl			\
	xl.plugin.listing.xl			\
	xl.plugin.include.xl			\
	xl.plugin.transforms.xl			\
	xl.plugin.differentiation.xl		\
	xl.plugin.constantfold.xl

XLOPTS=-bootstrap

include ../stage.mk
