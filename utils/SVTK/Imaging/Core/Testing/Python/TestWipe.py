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

class TestWipe(svtk.test.Testing.svtkTest):

    def testWipe(self):

        # Image pipeline

        renWin = svtk.svtkRenderWindow()

        image1 = svtk.svtkImageCanvasSource2D()
        image1.SetNumberOfScalarComponents(3)
        image1.SetScalarTypeToUnsignedChar()
        image1.SetExtent(0, 79, 0, 79, 0, 0)
        image1.SetDrawColor(255, 255, 0)
        image1.FillBox(0, 79, 0, 79)
        image1.Update()

        image2 = svtk.svtkImageCanvasSource2D()
        image2.SetNumberOfScalarComponents(3)
        image2.SetScalarTypeToUnsignedChar()
        image2.SetExtent(0, 79, 0, 79, 0, 0)
        image2.SetDrawColor(0, 255, 255)
        image2.FillBox(0, 79, 0, 79)
        image2.Update()

        mapper = svtk.svtkImageMapper()
        mapper.SetInputConnection(image1.GetOutputPort())
        mapper.SetColorWindow(255)
        mapper.SetColorLevel(127.5)
        actor = svtk.svtkActor2D()
        actor.SetMapper(mapper)
        imager = svtk.svtkRenderer()
        imager.AddActor2D(actor)

        renWin.AddRenderer(imager)

        wipes = ["Quad", "Horizontal", "Vertical", "LowerLeft", "LowerRight", "UpperLeft", "UpperRight"]

        wiper = dict()
        mapper = dict()
        actor = dict()
        imagers = dict()

        for wipe in wipes:
            wiper.update({wipe:svtk.svtkImageRectilinearWipe()})
            wiper[wipe].SetInput1Data(image1.GetOutput())
            wiper[wipe].SetInput2Data(image2.GetOutput())
            wiper[wipe].SetPosition(20, 20)
            eval('wiper[wipe].SetWipeTo' + wipe + '()')

            mapper.update({wipe:svtk.svtkImageMapper()})
            mapper[wipe].SetInputConnection(wiper[wipe].GetOutputPort())
            mapper[wipe].SetColorWindow(255)
            mapper[wipe].SetColorLevel(127.5)

            actor.update({wipe:svtk.svtkActor2D()})
            actor[wipe].SetMapper(mapper[wipe])

            imagers.update({wipe:svtk.svtkRenderer()})
            imagers[wipe].AddActor2D(actor[wipe])

            renWin.AddRenderer(imagers[wipe])

        imagers["Quad"].SetViewport(0, .5, .25, 1)
        imagers["Horizontal"].SetViewport(.25, .5, .5, 1)
        imagers["Vertical"].SetViewport(.5, .5, .75, 1)
        imagers["LowerLeft"].SetViewport(.75, .5, 1, 1)
        imagers["LowerRight"].SetViewport(0, 0, .25, .5)
        imagers["UpperLeft"].SetViewport(.25, 0, .5, .5)
        imagers["UpperRight"].SetViewport(.5, 0, .75, .5)
        imager.SetViewport(.75, 0, 1, .5)

        renWin.SetSize(400, 200)

        # render and interact with data

        iRen = svtk.svtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);
        renWin.Render()

        img_file = "TestWipe.png"
        svtk.test.Testing.compareImage(iRen.GetRenderWindow(), svtk.test.Testing.getAbsImagePath(img_file), threshold=25)
        svtk.test.Testing.interact()

if __name__ == "__main__":
     svtk.test.Testing.main([(TestWipe, 'test')])
