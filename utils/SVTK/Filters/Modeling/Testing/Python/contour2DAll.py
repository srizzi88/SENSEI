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

class contour2DAll(svtk.test.Testing.svtkTest):

    def testContour2DAll(self):

        # Create the RenderWindow, Renderer and both Actors
        #
        ren = svtk.svtkRenderer()
        renWin = svtk.svtkRenderWindow()
        renWin.SetMultiSamples(0)
        renWin.AddRenderer(ren)

        # create pipeline
        #
        slc = svtk.svtkSLCReader()
        slc.SetFileName(SVTK_DATA_ROOT + "/Data/nut.slc")
        slc.Update()

        types = ["Char", "UnsignedChar", "Short", "UnsignedShort", "Int", "UnsignedInt", "Long", "UnsignedLong", "Float", "Double"]

        i = 3

        clip = list()
        cast = list()
        iso = list()
        mapper = list()
        actor = list()

        for idx, svtkType in enumerate(types):

            clip.append(svtk.svtkImageClip())
            clip[idx].SetInputConnection(slc.GetOutputPort())
            clip[idx].SetOutputWholeExtent(-1000, 1000, -1000, 1000, i, i)

            i += 2

            cast.append(svtk.svtkImageCast())
            eval("cast[idx].SetOutputScalarTypeTo" + svtkType)
            cast[idx].SetInputConnection(clip[idx].GetOutputPort())
            cast[idx].ClampOverflowOn()

            iso.append(svtk.svtkContourFilter())
            iso[idx] = svtk.svtkContourFilter()
            iso[idx].SetInputConnection(cast[idx].GetOutputPort())
            iso[idx].GenerateValues(1, 30, 30)

            mapper.append(svtk.svtkPolyDataMapper())
            mapper[idx].SetInputConnection(iso[idx].GetOutputPort())
            mapper[idx].SetColorModeToMapScalars()

            actor.append(svtk.svtkActor())
            actor[idx].SetMapper(mapper[idx])
            ren.AddActor(actor[idx])

        outline = svtk.svtkOutlineFilter()
        outline.SetInputConnection(slc.GetOutputPort())
        outlineMapper = svtk.svtkPolyDataMapper()
        outlineMapper.SetInputConnection(outline.GetOutputPort())
        outlineActor = svtk.svtkActor()
        outlineActor.SetMapper(outlineMapper)
        outlineActor.VisibilityOff()
        #
        # Add the actors to the renderer, set the background and size
        #
        ren.AddActor(outlineActor)
        ren.ResetCamera()
        ren.GetActiveCamera().SetViewAngle(30)
        ren.GetActiveCamera().Elevation(20)
        ren.GetActiveCamera().Azimuth(20)
        ren.GetActiveCamera().Zoom(1.5)
        ren.ResetCameraClippingRange()

        ren.SetBackground(0.9, .9, .9)
        renWin.SetSize(200, 200)

        # render and interact with data

        iRen = svtk.svtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);
        renWin.Render()

        img_file = "contour2DAll.png"
        svtk.test.Testing.compareImage(iRen.GetRenderWindow(), svtk.test.Testing.getAbsImagePath(img_file), threshold=25)
        svtk.test.Testing.interact()

if __name__ == "__main__":
     svtk.test.Testing.main([(contour2DAll, 'test')])
