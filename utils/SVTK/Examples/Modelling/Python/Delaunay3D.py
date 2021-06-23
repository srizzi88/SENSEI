#!/usr/bin/env python

# This example shows how to use Delaunay3D with alpha shapes.

import svtk

# The points to be triangulated are generated randomly in the unit
# cube located at the origin. The points are then associated with a
# svtkPolyData.
math = svtk.svtkMath()
points = svtk.svtkPoints()
for i in range(0, 25):
    points.InsertPoint(i, math.Random(0, 1), math.Random(0, 1),
                       math.Random(0, 1))

profile = svtk.svtkPolyData()
profile.SetPoints(points)

# Delaunay3D is used to triangulate the points. The Tolerance is the
# distance that nearly coincident points are merged
# together. (Delaunay does better if points are well spaced.) The
# alpha value is the radius of circumcircles, circumspheres. Any mesh
# entity whose circumcircle is smaller than this value is output.
delny = svtk.svtkDelaunay3D()
delny.SetInputData(profile)
delny.SetTolerance(0.01)
delny.SetAlpha(0.2)
delny.BoundingTriangulationOff()

# Shrink the result to help see it better.
shrink = svtk.svtkShrinkFilter()
shrink.SetInputConnection(delny.GetOutputPort())
shrink.SetShrinkFactor(0.9)

map = svtk.svtkDataSetMapper()
map.SetInputConnection(shrink.GetOutputPort())

triangulation = svtk.svtkActor()
triangulation.SetMapper(map)
triangulation.GetProperty().SetColor(1, 0, 0)

# Create graphics stuff
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(triangulation)
ren.SetBackground(1, 1, 1)
renWin.SetSize(250, 250)
renWin.Render()

cam1 = ren.GetActiveCamera()
cam1.Zoom(1.5)

iren.Initialize()
renWin.Render()
iren.Start()
