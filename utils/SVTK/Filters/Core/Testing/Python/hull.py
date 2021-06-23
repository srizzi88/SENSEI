#!/usr/bin/env python
import math
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Generate random planes to form a convex polyhedron.
# Create a polyhedral representation of the planes.

# create some points lying between 1<=r<5 (r is radius)
# the points also have normals pointing away from the origin.
#
mathObj = svtk.svtkMath()

points = svtk.svtkPoints()
normals = svtk.svtkFloatArray()
normals.SetNumberOfComponents(3)
i = 0
while i < 100:
    radius = 1.0
    theta = mathObj.Random(0, 360)
    phi = mathObj.Random(0, 180)
    x = radius * math.sin(phi) * math.cos(theta)
    y = radius * math.sin(phi) * math.sin(theta)
    z = radius * math.cos(phi)
    points.InsertPoint(i, x, y, z)
    normals.InsertTuple3(i, x, y, z)
    i += 1

planes = svtk.svtkPlanes()
planes.SetPoints(points)
planes.SetNormals(normals)

# ss = svtk.svtkSphereSource()

hull = svtk.svtkHull()
hull.SetPlanes(planes)

pd = svtk.svtkPolyData()
hull.GenerateHull(pd, -20, 20, -20, 20, -20, 20)

# triangulate them
#
mapHull = svtk.svtkPolyDataMapper()
mapHull.SetInputData(pd)

hullActor = svtk.svtkActor()
hullActor.SetMapper(mapHull)

# Create graphics objects
# Create the rendering window, renderer, and interactive renderer
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren1.AddActor(hullActor)

renWin.SetSize(250, 250)

# render the image
#
ren1.ResetCamera()

ren1.GetActiveCamera().Zoom(1.5)

iren.Initialize()
#iren.Start()
