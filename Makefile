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

SUBDIRS=	src xl2

MIQ=make-it-quick/
include $(MIQ)rules.mk
$(MIQ)rules.mk:
	@git submodule update --init --recursive
