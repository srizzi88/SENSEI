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

class reconstructSurface(svtk.test.Testing.svtkTest):

    def testReconstructSurface(self):

        # Read some points. Use a programmable filter to read them.
        #
        pointSource = svtk.svtkProgrammableSource()

        def readPoints():

            fp = open(SVTK_DATA_ROOT + "/Data/cactus.3337.pts", "r")

            points = svtk.svtkPoints()
            while True:
                line = fp.readline().split()
                if len(line) == 0:
                    break
                if line[0] == "p":
                    points.InsertNextPoint(float(line[1]), float(line[2]), float(line[3]))
            pointSource.GetPolyDataOutput().SetPoints(points)

        pointSource.SetExecuteMethod(readPoints)

        # Construct the surface and create isosurface
        #
        surf = svtk.svtkSurfaceReconstructionFilter()
        surf.SetInputConnection(pointSource.GetOutputPort())

        cf = svtk.svtkContourFilter()
        cf.SetInputConnection(surf.GetOutputPort())
        cf.SetValue(0, 0.0)

        reverse = svtk.svtkReverseSense()
        reverse.SetInputConnection(cf.GetOutputPort())
        reverse.ReverseCellsOn()
        reverse.ReverseNormalsOn()

        map = svtk.svtkPolyDataMapper()
        map.SetInputConnection(reverse.GetOutputPort())
        map.ScalarVisibilityOff()

        surfaceActor = svtk.svtkActor()
        surfaceActor.SetMapper(map)
        surfaceActor.GetProperty().SetDiffuseColor(1.0000, 0.3882, 0.2784)
        surfaceActor.GetProperty().SetSpecularColor(1, 1, 1)
        surfaceActor.GetProperty().SetSpecular(.4)
        surfaceActor.GetProperty().SetSpecularPower(50)

        # Create the RenderWindow, Renderer and both Actors
        #
        ren = svtk.svtkRenderer()
        renWin = svtk.svtkRenderWindow()
        renWin.AddRenderer(ren)

        # Add the actors to the renderer, set the background and size
        #
        ren.AddActor(surfaceActor)
        ren.SetBackground(1, 1, 1)
        renWin.SetSize(300, 300)
        ren.GetActiveCamera().SetFocalPoint(0, 0, 0)
        ren.GetActiveCamera().SetPosition(1, 0, 0)
        ren.GetActiveCamera().SetViewUp(0, 0, 1)
        ren.ResetCamera()
        ren.GetActiveCamera().Azimuth(20)
        ren.GetActiveCamera().Elevation(30)
        ren.GetActiveCamera().Dolly(1.2)
        ren.ResetCameraClippingRange()

        # render and interact with data

        iRen = svtk.svtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);
        renWin.Render()

        img_file = "reconstructSurface.png"
        svtk.test.Testing.compareImage(iRen.GetRenderWindow(), svtk.test.Testing.getAbsImagePath(img_file), threshold=25)
        svtk.test.Testing.interact()

if __name__ == "__main__":
     svtk.test.Testing.main([(reconstructSurface, 'test')])
