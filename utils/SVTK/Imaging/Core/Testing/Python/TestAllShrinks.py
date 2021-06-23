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

class TestAllShrinks(svtk.test.Testing.svtkTest):

    def testAllShrinks(self):


        prefix = SVTK_DATA_ROOT + "/Data/headsq/quarter"


        renWin = svtk.svtkRenderWindow()

        # Image pipeline
        reader = svtk.svtkImageReader()
        reader.SetDataExtent(0, 63, 0, 63, 1, 93)
        reader.SetFilePrefix(prefix)
        reader.SetDataByteOrderToLittleEndian()
        reader.SetDataMask(0x7fff)

        factor = 4
        magFactor = 8

        ops = ["Minimum", "Maximum", "Mean", "Median", "NoOp"]

        shrink = dict()
        mag = dict()
        mapper = dict()
        actor = dict()
        imager = dict()

        for operator in ops:
            shrink.update({operator:svtk.svtkImageShrink3D()})
            shrink[operator].SetMean(0)
            if operator != "NoOp":
             eval('shrink[operator].' + operator + 'On()')
            shrink[operator].SetShrinkFactors(factor, factor, factor)
            shrink[operator].SetInputConnection(reader.GetOutputPort())
            mag.update({operator:svtk.svtkImageMagnify()})
            mag[operator].SetMagnificationFactors(magFactor, magFactor, magFactor)
            mag[operator].InterpolateOff()
            mag[operator].SetInputConnection(shrink[operator].GetOutputPort())
            mapper.update({operator:svtk.svtkImageMapper()})
            mapper[operator].SetInputConnection(mag[operator].GetOutputPort())
            mapper[operator].SetColorWindow(2000)
            mapper[operator].SetColorLevel(1000)
            mapper[operator].SetZSlice(45)
            actor.update({operator:svtk.svtkActor2D()})
            actor[operator].SetMapper(mapper[operator])
            imager.update({operator:svtk.svtkRenderer()})
            imager[operator].AddActor2D(actor[operator])
            renWin.AddRenderer(imager[operator])

        shrink["Minimum"].Update
        shrink["Maximum"].Update
        shrink["Mean"].Update
        shrink["Median"].Update

        imager["Minimum"].SetViewport(0, 0, .5, .33)
        imager["Maximum"].SetViewport(0, .33, .5, .667)
        imager["Mean"].SetViewport(.5, 0, 1, .33)
        imager["Median"].SetViewport(.5, .33, 1, .667)
        imager["NoOp"].SetViewport(0, .667, 1, 1)

        renWin.SetSize(256, 384)

        # render and interact with data

        iRen = svtk.svtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);
        renWin.Render()

        img_file = "TestAllShrinks.png"
        svtk.test.Testing.compareImage(iRen.GetRenderWindow(), svtk.test.Testing.getAbsImagePath(img_file), threshold=25)
        svtk.test.Testing.interact()

if __name__ == "__main__":
     svtk.test.Testing.main([(TestAllShrinks, 'test')])
