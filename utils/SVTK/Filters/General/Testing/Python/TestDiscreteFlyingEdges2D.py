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
discrete = svtk.svtkDiscreteFlyingEdges2D()
discrete.SetInputConnection(quantize.GetOutputPort())
discrete.SetValue(0,0)

# Create polgons
polyLoops = svtk.svtkContourLoopExtraction()
polyLoops.SetInputConnection(discrete.GetOutputPort())

# Triangle filter because concave polygons are not rendered correctly
triF = svtk.svtkTriangleFilter()
triF.SetInputConnection(polyLoops.GetOutputPort())

# Polylines are generated
mapper = svtk.svtkPolyDataMapper()
mapper.SetInputConnection(discrete.GetOutputPort())
mapper.ScalarVisibilityOff()

actor = svtk.svtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetColor(0,0,0)

# Polygons are displayed
polyMapper = svtk.svtkPolyDataMapper()
polyMapper.SetInputConnection(triF.GetOutputPort())

polyActor = svtk.svtkActor()
polyActor.SetMapper(polyMapper)

ren1.AddActor(actor)
ren1.AddActor(polyActor)

renWin.Render()
iren.Start()
