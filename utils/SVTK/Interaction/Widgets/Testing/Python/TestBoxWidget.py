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

class TestBoxWidget(svtk.test.Testing.svtkTest):

    def testBoxWidget(self):

        # Demonstrate how to use the svtkBoxWidget.
        # This script uses a 3D box widget to define a "clipping box" to clip some
        # simple geometry (a mace). Make sure that you hit the "W" key to activate the widget.

        # create a sphere source
        #
        sphere = svtk.svtkSphereSource()
        cone = svtk.svtkConeSource()
        glyph = svtk.svtkGlyph3D()
        glyph.SetInputConnection(sphere.GetOutputPort())
        glyph.SetSourceConnection(cone.GetOutputPort())
        glyph.SetVectorModeToUseNormal()
        glyph.SetScaleModeToScaleByVector()
        glyph.SetScaleFactor(0.25)
        apd = svtk.svtkAppendPolyData()
        apd.AddInputConnection(glyph.GetOutputPort())
        apd.AddInputConnection(sphere.GetOutputPort())
        maceMapper = svtk.svtkPolyDataMapper()
        maceMapper.SetInputConnection(apd.GetOutputPort())
        maceActor = svtk.svtkLODActor()
        maceActor.SetMapper(maceMapper)
        maceActor.VisibilityOn()

        planes = svtk.svtkPlanes()
        clipper = svtk.svtkClipPolyData()
        clipper.SetInputConnection(apd.GetOutputPort())
        clipper.SetClipFunction(planes)
        clipper.InsideOutOn()
        selectMapper = svtk.svtkPolyDataMapper()
        selectMapper.SetInputConnection(clipper.GetOutputPort())
        selectActor = svtk.svtkLODActor()
        selectActor.SetMapper(selectMapper)
        selectActor.GetProperty().SetColor(0, 1, 0)
        selectActor.VisibilityOff()
        selectActor.SetScale(1.01, 1.01, 1.01)

        # Create the RenderWindow, Renderer and both Actors
        #
        ren = svtk.svtkRenderer()
        renWin = svtk.svtkRenderWindow()
        renWin.AddRenderer(ren)

        iRen = svtk.svtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);
        boxWidget = svtk.svtkBoxWidget()
        boxWidget.SetInteractor(iRen)

        ren.AddActor(maceActor)
        ren.AddActor(selectActor)

        # Add the actors to the renderer, set the background and size
        #
        ren.SetBackground(0.1, 0.2, 0.4)
        renWin.SetSize(300, 300)

        def SelectPolygons(widget, event_string):
            '''
            The callback takes two parameters.
            Parameters:
              widget - the object that generates the event.
              event_string - the event name (which is a string).
            '''
            boxWidget, selectActor
            boxWidget.GetPlanes(planes)
            selectActor.VisibilityOn()

        # place the interactor initially
        boxWidget.SetInputConnection(glyph.GetOutputPort())
        boxWidget.PlaceWidget()
        boxWidget.AddObserver("EndInteractionEvent", SelectPolygons)

        # render and interact with data

        renWin.Render()

        img_file = "TestBoxWidget.png"
        svtk.test.Testing.compareImage(iRen.GetRenderWindow(), svtk.test.Testing.getAbsImagePath(img_file), threshold=25)
        svtk.test.Testing.interact()

if __name__ == "__main__":
     svtk.test.Testing.main([(TestBoxWidget, 'test')])
