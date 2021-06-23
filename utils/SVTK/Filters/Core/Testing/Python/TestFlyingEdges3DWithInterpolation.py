#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create a synthetic source
sphere = svtk.svtkSphere()
sphere.SetCenter( 0.0,0.0,0.0)
sphere.SetRadius(0.25)

# Iso-surface to create geometry. Demonstrate the ability to
# interpolate data attributes.
sample = svtk.svtkSampleFunction()
sample.SetImplicitFunction(sphere)
sample.SetModelBounds(-0.5,0.5, -0.5,0.5, -0.5,0.5)
sample.SetSampleDimensions(100,100,100)

# Now create some new attributes
cyl = svtk.svtkCylinder()
cyl.SetRadius(0.1)
cyl.SetAxis(1,1,1)

attr = svtk.svtkSampleImplicitFunctionFilter()
attr.SetInputConnection(sample.GetOutputPort())
attr.SetImplicitFunction(cyl)
attr.ComputeGradientsOn()
attr.Update()

iso = svtk.svtkFlyingEdges3D()
iso.SetInputConnection(attr.GetOutputPort())
iso.SetInputArrayToProcess(0, 0, 0, svtk.svtkDataObject.FIELD_ASSOCIATION_POINTS, "scalars")
iso.SetValue(0,0.25)
iso.ComputeNormalsOn()
iso.ComputeGradientsOn()
iso.ComputeScalarsOn()
iso.InterpolateAttributesOn()

# Time execution
timer = svtk.svtkTimerLog()
timer.StartTimer()
iso.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Flying edges with attributes: {0}".format(time))

isoMapper = svtk.svtkPolyDataMapper()
isoMapper.SetInputConnection(iso.GetOutputPort())
isoMapper.ScalarVisibilityOn()
isoMapper.SetScalarModeToUsePointFieldData()
isoMapper.SelectColorArray("Implicit scalars")
isoMapper.SetScalarRange(0,.3)

isoActor = svtk.svtkActor()
isoActor.SetMapper(isoMapper)
isoActor.GetProperty().SetColor(1,1,1)
isoActor.GetProperty().SetOpacity(1)

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
ren1.AddActor(isoActor)
ren1.SetBackground(0,0,0)
renWin.SetSize(300,300)
ren1.ResetCamera()
iren.Initialize()

renWin.Render()
# --- end of script --
