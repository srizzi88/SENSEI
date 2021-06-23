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

import svtk
import svtk.test.Testing
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

class TestThreshold(svtk.test.Testing.svtkTest):

    def testThreshold(self):

        # This script is for testing the 3D threshold filter.

        # Image pipeline

        renWin = svtk.svtkRenderWindow()
        renWin.SetSize(192, 256)

        reader = svtk.svtkImageReader()
        reader.ReleaseDataFlagOff()
        reader.SetDataByteOrderToLittleEndian()
        reader.SetDataExtent(0, 63, 0, 63, 1, 93)
        reader.SetDataSpacing(3.2, 3.2, 1.5)
        reader.SetFilePrefix(SVTK_DATA_ROOT + "/Data/headsq/quarter")
        reader.SetDataMask(0x7fff)

        outputtype = ["SignedChar", "UnsignedChar", "Long", "UnsignedLong", "Int", "UnsignedInt",
                        "Short", "UnsignedShort", "Double", "Float", "Double", "Float"]
        replacein = ["ReplaceInOn", "ReplaceInOff"]
        replaceout = ["ReplaceOutOn", "ReplaceOutOff"]
        thresholds = ["ThresholdByLower(800)", "ThresholdByUpper(1200)", "ThresholdBetween(800, 1200)"]

        thresh = list()
        map = list()
        act = list()
        ren = list()

        k = 0
        for rin in replacein:
            for rout in replaceout:
                for t in thresholds:

                    thresh.append(svtk.svtkImageThreshold())
                    thresh[k].SetInValue(2000)
                    thresh[k].SetOutValue(0)
                    eval('thresh[k].' + rin + '()')
                    eval('thresh[k].' + rout + '()')
                    thresh[k].SetInputConnection(reader.GetOutputPort())
                    eval('thresh[k].' + t)
                    eval('thresh[k].SetOutputScalarTypeTo' + outputtype[k] + '()')

                    map.append(svtk.svtkImageMapper())
                    map[k].SetInputConnection(thresh[k].GetOutputPort())
                    if k < 3:
                        map[k].SetColorWindow(255)
                        map[k].SetColorLevel(127.5)
                    else:
                        map[k].SetColorWindow(2000)
                        map[k].SetColorLevel(1000)

                    act.append(svtk.svtkActor2D())
                    act[k].SetMapper(map[k])

                    ren.append(svtk.svtkRenderer())
                    ren[k].AddActor2D(act[k])

                    renWin.AddRenderer(ren[k])

                    k += 1

        ren[0].SetViewport(0, 0, .33333, .25)
        ren[1].SetViewport(.33333, 0, .66667, .25)
        ren[2].SetViewport(.66667, 0, 1, .25)
        ren[3].SetViewport(0, .25, .33333, .5)
        ren[4].SetViewport(.33333, .25, .66667, .5)
        ren[5].SetViewport(.66667, .25, 1, .5)
        ren[6].SetViewport(0, .5, .33333, .75)
        ren[7].SetViewport(.33333, .5, .66667, .75)
        ren[8].SetViewport(.66667, .5, 1, .75)
        ren[9].SetViewport(0, .75, .33333, 1)
        ren[10].SetViewport(.33333, .75, .66667, 1)
        ren[11].SetViewport(.66667, .75, 1, 1)

        # render and interact with data

        iRen = svtk.svtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);
        renWin.Render()

        img_file = "TestThreshold.png"
        svtk.test.Testing.compareImage(iRen.GetRenderWindow(), svtk.test.Testing.getAbsImagePath(img_file), threshold=25)
        svtk.test.Testing.interact()

if __name__ == "__main__":
     svtk.test.Testing.main([(TestThreshold, 'test')])
