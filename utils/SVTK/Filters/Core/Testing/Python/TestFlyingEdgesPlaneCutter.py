#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

useFECutter = 1
res = 100

# Create the RenderWindow, Renderer and both Actors
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create a synthetic source: sample a sphere across a volume
sphere = svtk.svtkSphere()
sphere.SetCenter( 0.0,0.0,0.0)
sphere.SetRadius(0.25)

sample = svtk.svtkSampleFunction()
sample.SetImplicitFunction(sphere)
sample.SetModelBounds(-0.5,0.5, -0.5,0.5, -0.5,0.5)
sample.SetSampleDimensions(res,res,res)
sample.Update()

# The cut plane
plane = svtk.svtkPlane()
plane.SetOrigin(0,0,0)
plane.SetNormal(1,1,1)

if useFECutter:
    cut = svtk.svtkFlyingEdgesPlaneCutter()
    cut.SetInputConnection(sample.GetOutputPort())
    cut.SetPlane(plane)
    cut.ComputeNormalsOff() #make it equivalent to svtkCutter
else:
    # Compare against previous method
    cut = svtk.svtkCutter()
    cut.SetInputConnection(sample.GetOutputPort())
    cut.SetCutFunction(plane)
    cut.SetValue(0,0.0)

# Time the execution of the filter w/out scalar tree
CG_timer = svtk.svtkExecutionTimer()
CG_timer.SetFilter(cut)
cut.Update()
CG = CG_timer.GetElapsedWallClockTime()
print ("Cut volume:", CG)

cutMapper = svtk.svtkPolyDataMapper()
cutMapper.SetInputConnection(cut.GetOutputPort())

cutActor = svtk.svtkActor()
cutActor.SetMapper(cutMapper)
cutActor.GetProperty().SetColor(1,1,1)
cutActor.GetProperty().SetOpacity(1)

outline = svtk.svtkOutlineFilter()
outline.SetInputConnection(sample.GetOutputPort())

outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)
outlineProp = outlineActor.GetProperty()

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(outlineActor)
ren1.AddActor(cutActor)
ren1.SetBackground(0,0,0)
renWin.SetSize(400,400)
ren1.ResetCamera()
iren.Initialize()

renWin.Render()
# --- end of script --
#iren.Start()
