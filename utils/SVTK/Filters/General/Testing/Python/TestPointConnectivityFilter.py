#!/usr/bin/env python
import math
import svtk
import svtk.test.Testing
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

npts = 1000

# Create a pipeline with variable point connectivity
math = svtk.svtkMath()
points = svtk.svtkPoints()
i = 0
while i < npts:
    points.InsertPoint(i,math.Random(0,1),math.Random(0,1),0.0)
    i = i + 1

profile = svtk.svtkPolyData()
profile.SetPoints(points)
# triangulate them
#
del1 = svtk.svtkDelaunay2D()
del1.SetInputData(profile)
del1.BoundingTriangulationOff()
del1.SetTolerance(0.001)
del1.SetAlpha(0.0)

conn = svtk.svtkPointConnectivityFilter()
conn.SetInputConnection(del1.GetOutputPort())
conn.Update()

mapper = svtk.svtkPolyDataMapper()
mapper.SetInputConnection(conn.GetOutputPort())
mapper.SetScalarModeToUsePointFieldData()
mapper.SelectColorArray("Point Connectivity Count")
mapper.SetScalarRange(conn.GetOutput().GetPointData().GetArray("Point Connectivity Count").GetRange())
print ("Point connectivity range: ", mapper.GetScalarRange())

actor = svtk.svtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetColor(1,1,1)

# Create the RenderWindow, Renderer and both Actors
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)

ren1.AddActor(actor)
camera = svtk.svtkCamera()
camera.SetFocalPoint(0,0,0)
camera.SetPosition(0,0,1)
ren1.SetActiveCamera(camera)

ren1.SetBackground(0, 0, 0)

renWin.SetSize(250,250)

# render and interact with data

iRen = svtk.svtkRenderWindowInteractor()
iRen.SetRenderWindow(renWin);
ren1.ResetCamera()
renWin.Render()

iRen.Initialize()
#iRen.Start()
