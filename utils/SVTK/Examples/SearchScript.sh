#!/bin/bash
#
#  Author: Darren Weber
#
#  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
#  All rights reserved.
#  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
#
#    This software is distributed WITHOUT ANY WARRANTY; without even
#    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#    PURPOSE.  See the above copyright notices for more information.
#
#  Copy this script to somewhere on your computer and edit it,
#  setting the paths in the variables
#  svtkExamplePath, svtkTestingPath and svtkSourcePath
#  to wherever you have installed SVTK.

if [ $# -lt 1 ]; then
    echo "$0 'search term' ['search term' ...]"
    exit 1
fi

#
# Search the CXX and Python files
#
# You may need to set the paths for these variables.
svtkExamplePath="/opt/local/share/svtk/examples"
svtkTestingPath="/opt/local/share/svtk/testing"
svtkSourcePath="/opt/local/share/svtk"

for term in $@; do
    echo
    echo "Search term: ${term}"
    for svtkPath in "${svtkExamplePath}" "${svtkTestingPath}" "${svtkSourcePath}" ; do
        if [ ! -d ${svtkPath} ]; then
            echo "Path not found: ${svtkPath}"
        else
            echo "Searching SVTK files in: ${svtkPath}"
            cxxFiles=$(find ${svtkPath} -name "*.cxx")
            grep -l -E -e ${term} ${cxxFiles}
            pyFiles=$(find ${svtkPath} -name "*.py")
            grep -l -E -e ${term} ${pyFiles}
      fi
    done
done
