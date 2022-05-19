#******************************************************************************
# Makefile<stage0>                                                   XL project
#******************************************************************************
#
#  File Description:
#
#    Makefile for the stage-0 XL compiler
#
#    This translator is written in C++, can translate the stage-1 compiler
#
#
#
#
#
#
#******************************************************************************
#  (C) 2022 Christophe de Dinechin <christophe@dinechin.org>
#  This software is licensed under the GNU General Public License v3
#******************************************************************************
#  This file is part of XL.
#
#  XL is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  XL is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with XL.  If not, see <https://www.gnu.org/licenses/>.
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

PRODUCTS=xl-stage0-check.exe

MIQ=../../make-it-quick/
include $(MIQ)rules.mk
$(MIQ)rules.mk:
	$(PRINT_BUILD) cd ..; git submodule update --init --recursive

errors.tbl: errors.txt
	@echo Generating errors messages table
	@awk '/\#.*/ { next } /^[A-Za-z_].*/ { printf "XL_ERROR(E_%s, ", $$1; printf "\""; for (i = 2; i <= NF; i++) printf (i<NF ? "%s " : "%s"), $$i; print "\")"; }' $< > $@
	@echo "#undef XL_ERROR" >> $@
