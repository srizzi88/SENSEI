#!/usr/bin/env python

# This example demonstrates the use of svtkDepthSortPolyData. This is a
# poor man's algorithm to sort polygons for proper transparent
# blending.  It sorts polygons based on a single point (i.e.,
# centroid) so the sorting may not work for overlapping or
# intersection polygons.

import svtk

# Create a bunch of spheres that overlap and cannot be easily arranged
# so that the blending works without sorting. They are appended into a
# single svtkPolyData because the filter only sorts within a single
# svtkPolyData input.
sphere = svtk.svtkSphereSource()
sphere.SetThetaResolution(80)
sphere.SetPhiResolution(40)
sphere.SetRadius(1)
sphere.SetCenter(0, 0, 0)
sphere2 = svtk.svtkSphereSource()
sphere2.SetThetaResolution(80)
sphere2.SetPhiResolution(40)
sphere2.SetRadius(0.5)
sphere2.SetCenter(1, 0, 0)
sphere3 = svtk.svtkSphereSource()
sphere3.SetThetaResolution(80)
sphere3.SetPhiResolution(40)
sphere3.SetRadius(0.5)
sphere3.SetCenter(-1, 0, 0)
sphere4 = svtk.svtkSphereSource()
sphere4.SetThetaResolution(80)
sphere4.SetPhiResolution(40)
sphere4.SetRadius(0.5)
sphere4.SetCenter(0, 1, 0)
sphere5 = svtk.svtkSphereSource()
sphere5.SetThetaResolution(80)
sphere5.SetPhiResolution(40)
sphere5.SetRadius(0.5)
sphere5.SetCenter(0, -1, 0)
appendData = svtk.svtkAppendPolyData()
appendData.AddInputConnection(sphere.GetOutputPort())
appendData.AddInputConnection(sphere2.GetOutputPort())
appendData.AddInputConnection(sphere3.GetOutputPort())
appendData.AddInputConnection(sphere4.GetOutputPort())
appendData.AddInputConnection(sphere5.GetOutputPort())

# The dephSort object is set up to generate scalars representing
# the sort depth.  A camera is assigned for the sorting. The camera
# define the sort vector (position and focal point).
camera = svtk.svtkCamera()
depthSort = svtk.svtkDepthSortPolyData()
depthSort.SetInputConnection(appendData.GetOutputPort())
depthSort.SetDirectionToBackToFront()
depthSort.SetVector(1, 1, 1)
depthSort.SetCamera(camera)
depthSort.SortScalarsOn()
depthSort.Update()

mapper = svtk.svtkPolyDataMapper()
mapper.SetInputConnection(depthSort.GetOutputPort())
mapper.SetScalarRange(0, depthSort.GetOutput().GetNumberOfCells())
actor = svtk.svtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetOpacity(0.5)
actor.GetProperty().SetColor(1, 0, 0)
actor.RotateX(-72)

# If an Prop3D is supplied, then its transformation matrix is taken
# into account during sorting.
depthSort.SetProp3D(actor)

# Create the RenderWindow, Renderer and both Actors
ren = svtk.svtkRenderer()
ren.SetActiveCamera(camera)
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(actor)
ren.SetBackground(1, 1, 1)
renWin.SetSize(300, 200)

ren.ResetCamera()
ren.GetActiveCamera().Zoom(2.2)

iren.Initialize()
renWin.Render()
iren.Start()
