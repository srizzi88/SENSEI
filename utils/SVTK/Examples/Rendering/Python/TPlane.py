#!/usr/bin/env python

# This simple example shows how to do basic texture mapping.

import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Load in the texture map. A texture is any unsigned char image. If it
# is not of this type, you will have to map it through a lookup table
# or by using svtkImageShiftScale.
bmpReader = svtk.svtkBMPReader()
bmpReader.SetFileName(SVTK_DATA_ROOT + "/Data/masonry.bmp")
atext = svtk.svtkTexture()
atext.SetInputConnection(bmpReader.GetOutputPort())
atext.InterpolateOn()

# Create a plane source and actor. The svtkPlanesSource generates
# texture coordinates.
plane = svtk.svtkPlaneSource()
planeMapper = svtk.svtkPolyDataMapper()
planeMapper.SetInputConnection(plane.GetOutputPort())
planeActor = svtk.svtkActor()
planeActor.SetMapper(planeMapper)
planeActor.SetTexture(atext)

# Create the RenderWindow, Renderer and both Actors
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(planeActor)
ren.SetBackground(0.1, 0.2, 0.4)
renWin.SetSize(500, 500)

ren.ResetCamera()
cam1 = ren.GetActiveCamera()
cam1.Elevation(-30)
cam1.Roll(-20)
ren.ResetCameraClippingRange()

iren.Initialize()
renWin.Render()
iren.Start()
