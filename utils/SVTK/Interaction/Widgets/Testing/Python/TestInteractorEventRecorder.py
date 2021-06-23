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

class TestInteractorEventRecorder(svtk.test.Testing.svtkTest):

    def testInteractorEventRecorder(self):

        # Demonstrate how to use the svtkInteractorEventRecorder to play back some
        # events.

        # Create a mace out of filters.
        #
        sphere = svtk.svtkSphereSource()
        cone = svtk.svtkConeSource()
        glyph = svtk.svtkGlyph3D()
        glyph.SetInputConnection(sphere.GetOutputPort())
        glyph.SetSourceConnection(cone.GetOutputPort())
        glyph.SetVectorModeToUseNormal()
        glyph.SetScaleModeToScaleByVector()
        glyph.SetScaleFactor(0.25)

        # The sphere and spikes are appended into a single polydata. This just makes things
        # simpler to manage.
        apd = svtk.svtkAppendPolyData()
        apd.AddInputConnection(glyph.GetOutputPort())
        apd.AddInputConnection(sphere.GetOutputPort())
        maceMapper = svtk.svtkPolyDataMapper()
        maceMapper.SetInputConnection(apd.GetOutputPort())
        maceActor = svtk.svtkLODActor()
        maceActor.SetMapper(maceMapper)
        maceActor.VisibilityOn()

        # This portion of the code clips the mace with the svtkPlanes implicit function.
        # The clipped region is colored green.
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
        #iren = svtk.svtkRenderWindowInteractor()
        #iren.SetRenderWindow(renWin)
        iRen.AddObserver("ExitEvent", sys.exit)

        # The SetInteractor method is how 3D widgets are associated with the render
        # window interactor. Internally, SetInteractor sets up a bunch of callbacks
        # using the Command/Observer mechanism (AddObserver()).
        ren.AddActor(maceActor)
        ren.AddActor(selectActor)

        # Add the actors to the renderer, set the background and size
        #
        ren.SetBackground(0.1, 0.2, 0.4)
        renWin.SetSize(300, 300)

        # This does the actual work: updates the svtkPlanes implicit function.
        # This in turn causes the pipeline to update.
        def SelectPolygons(widget, event_string):
            '''
            The callback takes two parameters.
            Parameters:
              widget - the object that generates the event.
              event_string - the event name (which is a string).
            '''
            boxRep.GetPlanes(planes)
            selectActor.VisibilityOn()

        # Place the interactor initially. The input to a 3D widget is used to
        # initially position and scale the widget. The EndInteractionEvent is
        # observed which invokes the SelectPolygons callback.
        boxRep = svtk.svtkBoxRepresentation()
        boxRep.SetPlaceFactor(0.75)
        boxRep.PlaceWidget(glyph.GetOutput().GetBounds())
        boxWidget = svtk.svtkBoxWidget2()
        boxWidget.SetInteractor(iRen)
        boxWidget.SetRepresentation(boxRep)
        boxWidget.AddObserver("EndInteractionEvent", SelectPolygons)
        boxWidget.SetPriority(1)

        # record events
        recorder = svtk.svtkInteractorEventRecorder()
        recorder.SetInteractor(iRen)
        recorder.SetFileName(SVTK_DATA_ROOT + "/Data/EventRecording.log")

        # render the image
        iRen.Initialize()
        renWin.Render()
        #recorder.Record()

        recorder.Play()
        recorder.Off()

        # render and interact with data

        renWin.Render()

        img_file = "TestInteractorEventRecorder.png"
        svtk.test.Testing.compareImage(iRen.GetRenderWindow(), svtk.test.Testing.getAbsImagePath(img_file), threshold=25)
        svtk.test.Testing.interact()

if __name__ == "__main__":
     svtk.test.Testing.main([(TestInteractorEventRecorder, 'test')])
