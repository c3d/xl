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

SUBDIRS=	xlr

# Disable generation of .pc file at top-level
PACKAGE_NAME=

MIQ=make-it-quick/
include $(MIQ)rules.mk

