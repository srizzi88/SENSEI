#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# create pipeline
#
reader = svtk.svtkDataSetReader()
reader.SetFileName(SVTK_DATA_ROOT + "/Data/RectGrid2.svtk")
reader.Update()

warper = svtk.svtkWarpVector()
warper.SetInputConnection(reader.GetOutputPort())
warper.SetScaleFactor(0.2)

extract = svtk.svtkExtractGrid()
extract.SetInputConnection(warper.GetOutputPort())
extract.SetVOI(0, 100, 0, 100, 7, 15)

mapper = svtk.svtkDataSetMapper()
mapper.SetInputConnection(extract.GetOutputPort())
mapper.SetScalarRange(0.197813, 0.710419)

actor = svtk.svtkActor()
actor.SetMapper(mapper)

# Graphics stuff
# Create the RenderWindow, Renderer and both Actors
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(actor)
ren1.SetBackground(1, 1, 1)

renWin.SetSize(400, 400)

cam1 = ren1.GetActiveCamera()
cam1.SetClippingRange(3.76213, 10.712)
cam1.SetFocalPoint(-0.0842503, -0.136905, 0.610234)
cam1.SetPosition(2.53813, 2.2678, -5.22172)
cam1.SetViewUp(-0.241047, 0.930635, 0.275343)

# render the image
#
iren.Initialize()
#iren.Start()
