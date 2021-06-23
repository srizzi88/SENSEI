#!/usr/bin/env python

# This example demonstrates the use of the contour filter, and the use of
# the svtkSampleFunction to generate a volume of data samples from an
# implicit function.

import svtk

# SVTK supports implicit functions of the form f(x,y,z)=constant. These
# functions can represent things spheres, cones, etc. Here we use a
# general form for a quadric to create an elliptical data field.
quadric = svtk.svtkQuadric()
quadric.SetCoefficients(.5, 1, .2, 0, .1, 0, 0, .2, 0, 0)

# svtkSampleFunction samples an implicit function over the x-y-z range
# specified (here it defaults to -1,1 in the x,y,z directions).
sample = svtk.svtkSampleFunction()
sample.SetSampleDimensions(30, 30, 30)
sample.SetImplicitFunction(quadric)

# Create five surfaces F(x,y,z) = constant between range specified. The
# GenerateValues() method creates n isocontour values between the range
# specified.
contours = svtk.svtkContourFilter()
contours.SetInputConnection(sample.GetOutputPort())
contours.GenerateValues(5, 0.0, 1.2)

contMapper = svtk.svtkPolyDataMapper()
contMapper.SetInputConnection(contours.GetOutputPort())
contMapper.SetScalarRange(0.0, 1.2)

contActor = svtk.svtkActor()
contActor.SetMapper(contMapper)

# We'll put a simple outline around the data.
outline = svtk.svtkOutlineFilter()
outline.SetInputConnection(sample.GetOutputPort())

outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(0,0,0)

# The usual rendering stuff.
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren.SetBackground(1, 1, 1)
ren.AddActor(contActor)
ren.AddActor(outlineActor)

iren.Initialize()
renWin.Render()
iren.Start()
