#******************************************************************************
#  Makefile.config.gnu             (C) 1992-2000 Christophe de Dinechin (ddd)
#                                                              Mozart Project
#******************************************************************************
#
#  File Description:
#
#    Makefile configuration file for the Mozart project
#
#    This version is for GCC on most OS
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

#------------------------------------------------------------------------------
#  Tools
#------------------------------------------------------------------------------

CC=gcc
CXX=g++
CPP=gcc -E
LD=g++
AR=ar
RANLIB=ranlib
CC_DEPEND=$(CC) $(CPPFLAGS) $(CPPFLAGS_$*) -M -MG
CXX_DEPEND=$(CXX) $(CPPFLAGS) $(CPPFLAGS_$*) -M -MG
AS_DEPEND=$(CC) $(CPPFLAGS) $(CPPFLAGS_$*) -M -MG


#------------------------------------------------------------------------------
#  Compilation flags
#------------------------------------------------------------------------------

CFLAGS_debug=-g -Wall -fno-inline
CFLAGS_opt=-O3 -Wall

CXXFLAGS_debug=-g -Wall -fno-inline
CXXFLAGS_opt=-O3 -Wall -g

