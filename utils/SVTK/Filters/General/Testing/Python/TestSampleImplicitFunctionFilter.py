#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Parameters for debugging
NPts = 100000

# create pipeline
#
points = svtk.svtkPointSource()
points.SetNumberOfPoints(NPts)
points.SetRadius(5)

# create some scalars based on implicit function
# Create a cylinder
cyl = svtk.svtkCylinder()
cyl.SetCenter(0,0,0)
cyl.SetRadius(0.1)

# Generate scalars and vector
sample = svtk.svtkSampleImplicitFunctionFilter()
sample.SetInputConnection(points.GetOutputPort())
sample.SetImplicitFunction(cyl)
sample.Update()
print(sample.GetOutput().GetScalarRange())

# Draw the points
sampleMapper = svtk.svtkPointGaussianMapper()
sampleMapper.SetInputConnection(sample.GetOutputPort(0))
sampleMapper.EmissiveOff()
sampleMapper.SetScaleFactor(0.0)
sampleMapper.SetScalarRange(0,20)

sampleActor = svtk.svtkActor()
sampleActor.SetMapper(sampleMapper)

# Create an outline
outline = svtk.svtkOutlineFilter()
outline.SetInputConnection(sample.GetOutputPort())

outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)

# Create the RenderWindow, Renderer and both Actors
#
ren0 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren0)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren0.AddActor(sampleActor)
ren0.AddActor(outlineActor)
ren0.SetBackground(0.1, 0.2, 0.4)

renWin.SetSize(250,250)

cam = ren0.GetActiveCamera()
cam.SetFocalPoint(0,0,0)
cam.SetPosition(1,1,1)
ren0.ResetCamera()

iren.Initialize()

# render the image
#
renWin.Render()

#iren.Start()
