#******************************************************************************
#  sanitiser.py                      (C) 1992-2004 Christophe de Dinechin (ddd)
#                                                                   XL2 project
#  Python version
#******************************************************************************
#
#  File Description:
#
#    Script to remove trailing whitespace in files
#    Comment lines beginning with "//" are not modified
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
#* Credits    : Sebastien <sebbrochet@users.sourceforge.net> (original version)
#******************************************************************************

import os
from string import rstrip

ExtensionList = ["xs", ".xl", ".py"]

def sanitise(FullFilename):
    file_object = open(FullFilename)
    list_of_all_the_lines = file_object.read().splitlines()

    list_of_all_the_lines_after_sanitise = []

    for line in list_of_all_the_lines:
        if line[:2] <> '//':
            new_line = rstrip(line)
        else:
            new_line = line

        #if new_line != line:
        #   print line + ' => ' + new_line

        list_of_all_the_lines_after_sanitise.append(new_line)

    file_object.close()

    if ''.join(list_of_all_the_lines) <> ''.join(list_of_all_the_lines_after_sanitise):
    	file_object = open(FullFilename, 'w')
        file_object.write('\n'.join(list_of_all_the_lines_after_sanitise) + '\n')
        print "%s sanitising was needed...done !" % FullFilename

for root, dir, files in os.walk('.'):
    for filename in files:
        if filename[-3:] in ExtensionList:
            FullFilename = os.path.join(root, filename)

            sanitise(FullFilename)
