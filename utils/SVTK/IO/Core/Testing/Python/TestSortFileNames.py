#!/usr/bin/env python
# -*- coding: utf-8 -*-

'''
=========================================================================

  Program:   Visualization Toolkit
  Module:    TestNamedColorsIntegration.py

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================
'''

# Run this test like so:
# svtkpython TestSortFileNames.py  -D $SVTK_DATA_ROOT

import re
import svtk
import svtk.test.Testing
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

class TestSortFileNames(svtk.test.Testing.svtkTest):

    def testSortFileNames(self):

        globFileNames = svtk.svtkGlobFileNames()

        # globs do not include Kleene star support for pattern repetitions thus
        # we insert a pattern for both single and double digit file extensions.
        globFileNames.AddFileNames(SVTK_DATA_ROOT + "/Data/headsq/quarter.[1-9]")
        globFileNames.AddFileNames(SVTK_DATA_ROOT + "/Data/headsq/quarter.[1-9][0-9]")
        globFileNames.AddFileNames(SVTK_DATA_ROOT + "/Data/track*.binary.svtk")

        sortFileNames = svtk.svtkSortFileNames()
        sortFileNames.SetInputFileNames(globFileNames.GetFileNames())
        sortFileNames.NumericSortOn()
        sortFileNames.SkipDirectoriesOn()
        sortFileNames.IgnoreCaseOn()
        sortFileNames.GroupingOn()

        if sortFileNames.GetNumberOfGroups() != 2:
             print("GetNumberOfGroups returned incorrect number")
             exit(1)

        fileNames1 = sortFileNames.GetNthGroup(0)
        fileNames2 = sortFileNames.GetNthGroup(1)

        numberOfFiles1 = 93
        numberOfFiles2 = 3

        n = fileNames1.GetNumberOfValues()
        if n != numberOfFiles1:
            for i in range(0, n):
                print(fileNames1.GetValue(i))
            print("GetNumberOfValues should return", numberOfFiles1, "not", n)
            exit(1)

        for i in range(0, numberOfFiles1):
            j = i + 1
            s = SVTK_DATA_ROOT + "/Data/headsq/quarter." + str(j)
            if fileNames1.GetValue(i) != s:
                print("string does not match pattern")
                print(fileNames1.GetValue(i))
                print(s)
                exit(1)

        n = fileNames2.GetNumberOfValues()
        if n != numberOfFiles2:
            for i in range(0, n):
                print(fileNames2.GetValue(i))
            print("GetNumberOfValues should return", numberOfFiles2, "not", n)
            exit(1)

        for i in range(0, numberOfFiles2):
            j = i + 1
            s = SVTK_DATA_ROOT + "/Data/track" + str(j) + ".binary.svtk"
            if fileNames2.GetValue(i) != s:
                print("string does not match pattern")
                print(fileNames2.GetValue(i))
                print(s)
                exit(1)

        svtk.test.Testing.interact()

if __name__ == "__main__":
     svtk.test.Testing.main([(TestSortFileNames, 'test')])
