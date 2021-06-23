#!/usr/bin/env python
############################################################
from svtk import *
############################################################

# Create sources
line1 = svtkLineSource()
line1.SetPoint1(1, 0, 0)
line1.SetPoint2(-1, 0, 0)
line1.SetResolution(32)

points = svtkPoints()
points.InsertNextPoint(1, 0, 0)
points.InsertNextPoint(-0.5, 1, 0)
points.InsertNextPoint(0, 1, 2)
points.InsertNextPoint(2, 1, -1)
points.InsertNextPoint(-1, 0, 0)
line2 = svtkLineSource()
line2.SetPoints(points)
line2.SetResolution(16)

# Create mappers
mapper1 = svtkPolyDataMapper()
mapper1.SetInputConnection(line1.GetOutputPort())

mapper2 = svtkPolyDataMapper()
mapper2.SetInputConnection(line2.GetOutputPort())

# Create actors
actor1 = svtkActor()
actor1.SetMapper(mapper1)
actor1.GetProperty().SetColor(1., 0., 0.)
actor2 = svtkActor()
actor2.SetMapper(mapper2)
actor2.GetProperty().SetColor(0., 0., 1.)
actor2.GetProperty().SetLineWidth(2.5)

# Create renderer
renderer = svtkRenderer()
renderer.AddViewProp(actor1)
renderer.AddViewProp(actor2)
renderer.SetBackground(.3, .4, .5)

# Create render window
window = svtkRenderWindow()
window.AddRenderer(renderer)
window.SetSize(500, 500)

# Create interactor
interactor = svtkRenderWindowInteractor()
interactor.SetRenderWindow(window)

# Start interaction
window.Render()
interactor.Start()
