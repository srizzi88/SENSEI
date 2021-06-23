#!/usr/bin/env python

# This example shows how to extract a piece of a dataset using an
# implicit function. In this case the implicit function is formed by
# the boolean combination of two ellipsoids.

import svtk

# Here we create two ellipsoidal implicit functions and boolean them
# together tto form a "cross" shaped implicit function.
quadric = svtk.svtkQuadric()
quadric.SetCoefficients(.5, 1, .2, 0, .1, 0, 0, .2, 0, 0)
sample = svtk.svtkSampleFunction()
sample.SetSampleDimensions(50, 50, 50)
sample.SetImplicitFunction(quadric)
sample.ComputeNormalsOff()
trans = svtk.svtkTransform()
trans.Scale(1, .5, .333)
sphere = svtk.svtkSphere()
sphere.SetRadius(0.25)
sphere.SetTransform(trans)
trans2 = svtk.svtkTransform()
trans2.Scale(.25, .5, 1.0)
sphere2 = svtk.svtkSphere()
sphere2.SetRadius(0.25)
sphere2.SetTransform(trans2)
union = svtk.svtkImplicitBoolean()
union.AddFunction(sphere)
union.AddFunction(sphere2)
union.SetOperationType(0) #union

# Here is where it gets interesting. The implicit function is used to
# extract those cells completely inside the function. They are then
# shrunk to help show what was extracted.
extract = svtk.svtkExtractGeometry()
extract.SetInputConnection(sample.GetOutputPort())
extract.SetImplicitFunction(union)
shrink = svtk.svtkShrinkFilter()
shrink.SetInputConnection(extract.GetOutputPort())
shrink.SetShrinkFactor(0.5)
dataMapper = svtk.svtkDataSetMapper()
dataMapper.SetInputConnection(shrink.GetOutputPort())
dataActor = svtk.svtkActor()
dataActor.SetMapper(dataMapper)

# The outline gives context to the original data.
outline = svtk.svtkOutlineFilter()
outline.SetInputConnection(sample.GetOutputPort())
outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)
outlineProp = outlineActor.GetProperty()
outlineProp.SetColor(0, 0, 0)

# The usual rendering stuff is created.
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(outlineActor)
ren.AddActor(dataActor)
ren.SetBackground(1, 1, 1)
renWin.SetSize(500, 500)
ren.ResetCamera()
ren.GetActiveCamera().Zoom(1.5)

iren.Initialize()
renWin.Render()
iren.Start()
