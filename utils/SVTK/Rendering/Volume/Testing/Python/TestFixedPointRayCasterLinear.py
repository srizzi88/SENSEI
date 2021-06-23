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

import sys
import svtk
import svtk.test.Testing
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

'''
  Prevent .pyc files from being created.
  Stops the svtk source being polluted
  by .pyc files.
'''
sys.dont_write_bytecode = True

import TestFixedPointRayCasterNearest

class TestFixedPointRayCasterLinear(svtk.test.Testing.svtkTest):

    def testFixedPointRayCasterLinear(self):

        ren = svtk.svtkRenderer()
        renWin = svtk.svtkRenderWindow()
        iRen = svtk.svtkRenderWindowInteractor()

        tFPRCN = TestFixedPointRayCasterNearest.FixedPointRayCasterNearest(ren, renWin, iRen)
        volumeProperty = tFPRCN.GetVolumeProperty()

        for j in range(0, 5):
            for i in range(0, 5):
                volumeProperty[i][j].SetInterpolationTypeToLinear()

        # render and interact with data

        renWin.Render()

        img_file = "TestFixedPointRayCasterLinear.png"
        svtk.test.Testing.compareImage(iRen.GetRenderWindow(), svtk.test.Testing.getAbsImagePath(img_file), threshold=10)
        svtk.test.Testing.interact()

if __name__ == "__main__":
     svtk.test.Testing.main([(TestFixedPointRayCasterLinear, 'test')])
