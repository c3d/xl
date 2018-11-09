#******************************************************************************
# Makefile<xl>                                                      XL project
#******************************************************************************
#
#  File Description:
#
#     The top-level makefile for XL, the eXtensible Language
#
#
#
#
#
#
#
#******************************************************************************
# (C) 2015-2018 Christophe de Dinechin <christophe@dinechin.org>
#******************************************************************************

SUBDIRS=	src

MIQ=make-it-quick/
include $(MIQ)rules.mk
$(MIQ)rules.mk:
	$(PRINT_BUILD) git submodule update --init --recursive
