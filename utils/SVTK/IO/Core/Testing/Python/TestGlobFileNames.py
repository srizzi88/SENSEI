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
# svtkpython TestGlobFileNames.py  -D $SVTK_DATA_ROOT

import re
import svtk
import svtk.test.Testing
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

class TestGlobFileNames(svtk.test.Testing.svtkTest):

    def testGlobFileNames(self):

        globFileNames = svtk.svtkGlobFileNames()
        globFileNames.SetDirectory(SVTK_DATA_ROOT + "/Data/")
        # globs do not include Kleene star support for pattern repetitions thus
        # we insert a pattern for both single and double digit file extensions.
        globFileNames.AddFileNames("headsq/quarter.[1-9]")
        globFileNames.AddFileNames("headsq/quarter.[1-9][0-9]")

        fileNames = globFileNames.GetFileNames()
        n = globFileNames.GetNumberOfFileNames()

        if n != 93:
            for i in range(0, n):
                print("File:", i, " ", fileNames.GetValue(i))
            print("GetNumberOfValues should return 93, returned", n)

            print("Listing of ", SVTK_DATA_ROOT, "/Data/headsq")
            directory = svtk.svtkDirectory()
            directory.Open(SVTK_DATA_ROOT + "/Data/headsq")
            m = directory.GetNumberOfFiles()
            for j in range(0, m):
                print(directory.GetFile(j))
            exit(1)

        for i in range(0, n):
            filename = fileNames.GetValue(i)
            if filename != globFileNames.GetNthFileName(i):
                print("mismatched filename for pattern quarter.*:", filename)
                exit(1)
            m = re.search("[\w|\W]*quarter.*", filename)
            if m == None:
                print("string does not match pattern quarter.*:", filename)


        # check that we can re-use the Glob object
        globFileNames.Reset()
        globFileNames.SetDirectory(SVTK_DATA_ROOT + "/Data/")
        globFileNames.AddFileNames(SVTK_DATA_ROOT + "/Data/financial.*")
        fileNames = globFileNames.GetFileNames()

        n = fileNames.GetNumberOfValues()
        for i in range(0, n):
            filename = fileNames.GetValue(i)

            if filename != globFileNames.GetNthFileName(i):
                print("mismatched filename for pattern financial.*: ", filename)
                exit(1)

            m = re.search("[\w|\W]*financial.*", filename)
            if m == None:
                print("string does not match pattern financial.*:", filename)
                exit(1)

        svtk.test.Testing.interact()

if __name__ == "__main__":
     svtk.test.Testing.main([(TestGlobFileNames, 'test')])
