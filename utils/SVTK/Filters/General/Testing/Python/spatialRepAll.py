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

class spatialRepAll(svtk.test.Testing.svtkTest):

    def testspatialRepAll(self):

        ren = svtk.svtkRenderer()
        renWin = svtk.svtkRenderWindow()
        renWin.SetMultiSamples(0)
        renWin.AddRenderer(ren)

        asource = svtk.svtkSTLReader()
        asource.SetFileName(SVTK_DATA_ROOT + "/Data/42400-IDGH.stl")
        dataMapper = svtk.svtkPolyDataMapper()
        dataMapper.SetInputConnection(asource.GetOutputPort())
        model = svtk.svtkActor()
        model.SetMapper(dataMapper)
        model.GetProperty().SetColor(1, 0, 0)
        model.VisibilityOn()

        locators = ["svtkPointLocator", "svtkCellLocator", "svtkOBBTree"]

        locator = list()
        boxes = list()
        boxMapper = list()
        boxActor = list()

        for idx, svtkLocatorType in enumerate(locators):
            eval('locator.append(svtk.' + svtkLocatorType + '())')
            locator[idx].AutomaticOff()
            locator[idx].SetMaxLevel(3)

            boxes.append(svtk.svtkSpatialRepresentationFilter())
            boxes[idx].SetInputConnection(asource.GetOutputPort())
            boxes[idx].SetSpatialRepresentation(locator[idx])
            boxes[idx].SetGenerateLeaves(1)
            boxes[idx].Update()

            output = boxes[idx].GetOutput().GetBlock(boxes[idx].GetMaximumLevel() + 1)

            boxMapper.append(svtk.svtkPolyDataMapper())
            boxMapper[idx].SetInputData(output)

            boxActor.append(svtk.svtkActor())
            boxActor[idx].SetMapper(boxMapper[idx])
            boxActor[idx].AddPosition((idx + 1) * 15, 0, 0)

            ren.AddActor(boxActor[idx])


        ren.AddActor(model)
        ren.SetBackground(0.1, 0.2, 0.4)
        renWin.SetSize(400, 160)

        # render the image
        camera = svtk.svtkCamera()
        camera.SetPosition(148.579, 136.352, 214.961)
        camera.SetFocalPoint(151.889, 86.3178, 223.333)
        camera.SetViewAngle(30)
        camera.SetViewUp(0, 0, -1)
        camera.SetClippingRange(1, 100)
        ren.SetActiveCamera(camera)

        # render and interact with data

        iRen = svtk.svtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);
        renWin.Render()

        img_file = "spatialRepAll.png"
        svtk.test.Testing.compareImage(iRen.GetRenderWindow(), svtk.test.Testing.getAbsImagePath(img_file), threshold=25)
        svtk.test.Testing.interact()

if __name__ == "__main__":
     svtk.test.Testing.main([(spatialRepAll, 'test')])
