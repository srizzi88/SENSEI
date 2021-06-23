#!/usr/bin/env python

# This example demonstrates how to combine a "geometric" implicit
# function with noise at different frequencies to produce the
# appearance of a landscape.

import svtk

plane = svtk.svtkPlane()

p1 = svtk.svtkPerlinNoise()
p1.SetFrequency(1, 1, 0)

p2 = svtk.svtkPerlinNoise()
p2.SetFrequency(3, 5, 0)
p2.SetPhase(0.5, 0.5, 0)

p3 = svtk.svtkPerlinNoise()
p3.SetFrequency(16, 16, 0)

sum = svtk.svtkImplicitSum()
sum.SetNormalizeByWeight(1)
sum.AddFunction(plane)
sum.AddFunction(p1, 0.2)
sum.AddFunction(p2, 0.1)
sum.AddFunction(p3, 0.02)

sample = svtk.svtkSampleFunction()
sample.SetImplicitFunction(sum)
sample.SetSampleDimensions(65, 65, 20)
sample.SetModelBounds(-1, 1, -1, 1, -0.5, 0.5)
sample.ComputeNormalsOff()
surface = svtk.svtkContourFilter()
surface.SetInputConnection(sample.GetOutputPort())
surface.SetValue(0, 0.0)

smooth = svtk.svtkPolyDataNormals()
smooth.SetInputConnection(surface.GetOutputPort())
smooth.SetFeatureAngle(90)

mapper = svtk.svtkPolyDataMapper()
mapper.SetInputConnection(smooth.GetOutputPort())
mapper.ScalarVisibilityOff()
actor = svtk.svtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetColor(0.4, 0.2, 0.1)

# Create the renderer etc.
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(actor)
ren.SetBackground(1, 1, 1)
renWin.SetSize(500, 500)
ren.ResetCamera()
ren.GetActiveCamera().Elevation(-45)
ren.GetActiveCamera().Azimuth(10)
ren.GetActiveCamera().Dolly(1.35)
ren.ResetCameraClippingRange()

iren.Initialize()
renWin.Render()
iren.Start()
