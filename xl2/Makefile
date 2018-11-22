#******************************************************************************
#  Makefile                        (C) 1992-2000 Christophe de Dinechin (ddd)
#                                                                 XL2 Project
#******************************************************************************
#
#  File Description:
#
#    Base makefile calling the sub-makefiles in turn
#
#
#
#
#
#
#
#
#******************************************************************************
#This document is distributed under the GNU General Public License
#See the enclosed COPYING file or http://www.gnu.org for information
#******************************************************************************
#* File       : $RCSFile$
#* Revision   : $Revision$
#* Date       : $Date$
#******************************************************************************

# List of source files to process
SOURCES=                                        \
        main.cpp                                \
        scanner.cpp                             \
        tree.cpp                                \
        context.cpp                             \
        parser.cpp                              \
        ctrans.cpp                              \
        options.cpp                             \
        errors.cpp

PRODUCTS=xl2c.exe

MIQ=../make-it-quick/
include $(MIQ)rules.mk
$(MIR)rules.mk:
	$(PRINT_BUILD) cd ..; git submodule update --init --recursive

.postbuild:	bootstrap_compiler native_compiler

bootstrap_compiler: .product
	$(PRINT_GENERATE) cd bootstrap && $(MIQ_RECURSE)

native_compiler:
	$(PRINT_GENERATE) cd $(BROOT)native && $(MIQ_RECURSE)

errors.tbl: errors.txt
	@echo Generating errors messages table
	@awk '/\#.*/ { next } /^[A-Za-z_].*/ { printf "XL_ERROR(E_%s, ", $$1; printf "\""; for (i = 2; i <= NF; i++) printf (i<NF ? "%s " : "%s"), $$i; print "\")"; }' $< > $@
	@echo "#undef XL_ERROR" >> $@
