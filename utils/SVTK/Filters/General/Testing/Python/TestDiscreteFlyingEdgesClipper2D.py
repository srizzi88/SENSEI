#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = svtk.svtkRenderer()
ren1.SetBackground(1,1,1)
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Read the data. Note this creates a 3-component scalar.
red = svtk.svtkPNGReader()
red.SetFileName(SVTK_DATA_ROOT + "/Data/RedCircle.png")
red.Update()

# Next filter can only handle RGB *(&&*@
extract = svtk.svtkImageExtractComponents()
extract.SetInputConnection(red.GetOutputPort())
extract.SetComponents(0,1,2)

# Quantize the image into an index
quantize = svtk.svtkImageQuantizeRGBToIndex()
quantize.SetInputConnection(extract.GetOutputPort())
quantize.SetNumberOfColors(3)

# Create the pipeline
discrete = svtk.svtkDiscreteFlyingEdgesClipper2D()
discrete.SetInputConnection(quantize.GetOutputPort())
discrete.SetValue(0,1)

# Polygons are displayed
polyMapper = svtk.svtkPolyDataMapper()
polyMapper.SetInputConnection(discrete.GetOutputPort())

polyActor = svtk.svtkActor()
polyActor.SetMapper(polyMapper)

ren1.AddActor(polyActor)

renWin.Render()
iren.Start()
