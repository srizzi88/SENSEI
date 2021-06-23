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

class TestTextWidget(svtk.test.Testing.svtkTest):

    def testTextWidget(self):

        # Create fake data
        #
        ss = svtk.svtkSphereSource()
        mapper = svtk.svtkPolyDataMapper()
        mapper.SetInputConnection(ss.GetOutputPort())
        actor = svtk.svtkActor()
        actor.SetMapper(mapper)

        # Create the RenderWindow, Renderer and both Actors
        #
        ren = svtk.svtkRenderer()
        renWin = svtk.svtkRenderWindow()
        renWin.AddRenderer(ren)
        iRen = svtk.svtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin)

        ren.AddActor(actor)
        ren.SetBackground(0.1, 0.2, 0.4)
        renWin.SetSize(300, 300)

        iRen.Initialize()
        renWin.Render()

        widget = svtk.svtkTextWidget()
        widget.SetInteractor(iRen)
        widget.On()
        widget.GetTextActor().SetInput("This is a test")
        widget.GetTextActor().GetTextProperty().SetColor(0, 1, 0)
        widget.GetRepresentation().GetPositionCoordinate().SetValue(.15, .15)
        widget.GetRepresentation().GetPosition2Coordinate().SetValue(.7, .2)


        # Add the actors to the renderer, set the background and size
        #
        ren.AddActor(actor)
        ren.SetBackground(.1, .2, .4)

        iRen.Initialize()
        renWin.Render()

        # render and interact with data

        renWin.Render()


        img_file = "TestTextWidget.png"
        svtk.test.Testing.compareImage(iRen.GetRenderWindow(), svtk.test.Testing.getAbsImagePath(img_file), threshold=25)
        svtk.test.Testing.interact()

if __name__ == "__main__":
     svtk.test.Testing.main([(TestTextWidget, 'test')])
