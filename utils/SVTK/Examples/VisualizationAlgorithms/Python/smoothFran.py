#!/usr/bin/env python

# This example shows how to use decimation to reduce a polygonal
# mesh. We also use mesh smoothing and generate surface normals to
# give a pleasing result.

import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# We start by reading some data that was originally captured from a
# Cyberware laser digitizing system.
fran = svtk.svtkPolyDataReader()
fran.SetFileName(SVTK_DATA_ROOT + "/Data/fran_cut.svtk")

# We want to preserve topology (not let any cracks form). This may
# limit the total reduction possible, which we have specified at 90%.
deci = svtk.svtkDecimatePro()
deci.SetInputConnection(fran.GetOutputPort())
deci.SetTargetReduction(0.9)
deci.PreserveTopologyOn()
smoother = svtk.svtkSmoothPolyDataFilter()
smoother.SetInputConnection(deci.GetOutputPort())
smoother.SetNumberOfIterations(50)
normals = svtk.svtkPolyDataNormals()
normals.SetInputConnection(smoother.GetOutputPort())
normals.FlipNormalsOn()
franMapper = svtk.svtkPolyDataMapper()
franMapper.SetInputConnection(normals.GetOutputPort())
franActor = svtk.svtkActor()
franActor.SetMapper(franMapper)
franActor.GetProperty().SetColor(1.0, 0.49, 0.25)

# Create the RenderWindow, Renderer and both Actors
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(franActor)
ren.SetBackground(1, 1, 1)
renWin.SetSize(250, 250)

cam1 = svtk.svtkCamera()
cam1.SetClippingRange(0.0475572, 2.37786)
cam1.SetFocalPoint(0.052665, -0.129454, -0.0573973)
cam1.SetPosition(0.327637, -0.116299, -0.256418)
cam1.SetViewUp(-0.0225386, 0.999137, 0.034901)
ren.SetActiveCamera(cam1)

iren.Initialize()
renWin.Render()
iren.Start()
