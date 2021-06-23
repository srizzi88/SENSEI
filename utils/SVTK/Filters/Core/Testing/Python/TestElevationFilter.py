#!/usr/bin/env python

import math
import svtk
import svtk.test.Testing
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

res = 200

# Create the RenderWindow, Renderer and both Actors
#
ren1 = svtk.svtkRenderer()
ren2 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)

# Create the data -- a plane
#
plane = svtk.svtkPlaneSource()
plane.SetXResolution(res)
plane.SetYResolution(res)
plane.SetOrigin(-10,-10,0)
plane.SetPoint1(10,-10,0)
plane.SetPoint2(-10,10,0)

tf = svtk.svtkTriangleFilter()
tf.SetInputConnection(plane.GetOutputPort())
tf.Update()

pd = svtk.svtkPolyData()
pd.CopyStructure(tf.GetOutput())
numPts = pd.GetNumberOfPoints()
oldPts = tf.GetOutput().GetPoints()
newPts = svtk.svtkPoints()
newPts.SetNumberOfPoints(numPts)
for i in range(0, numPts):
    pt = oldPts.GetPoint(i)
    r = math.sqrt(pt[0]*pt[0] + pt[1]*pt[1])
    z = 1.5*math.cos(2*r)
    newPts.SetPoint(i, pt[0], pt[1], z)
pd.SetPoints(newPts)

ele = svtk.svtkSimpleElevationFilter()
ele.SetInputData(pd)

mapper = svtk.svtkPolyDataMapper()
mapper.SetInputConnection(ele.GetOutputPort())

actor = svtk.svtkActor()
actor.SetMapper(mapper)

ele2 = svtk.svtkElevationFilter()
ele2.SetInputData(pd)
ele2.SetLowPoint(0,0,-1.5)
ele2.SetHighPoint(0,0,1.5)
ele2.SetScalarRange(-1.5,1.5)

mapper2 = svtk.svtkPolyDataMapper()
mapper2.SetInputConnection(ele2.GetOutputPort())

actor2 = svtk.svtkActor()
actor2.SetMapper(mapper2)

# Add the actors to the renderer, set the background and size
#
ren1.SetViewport(0, 0, .5, 1)
ren2.SetViewport(.5, 0, 1, 1)

ren1.AddActor(actor)
ren2.AddActor(actor2)

camera = svtk.svtkCamera()
camera.SetPosition(1,1,1)
ren1.SetActiveCamera(camera)
ren2.SetActiveCamera(camera)

ren1.SetBackground(0, 0, 0)
ren2.SetBackground(0, 0, 0)

renWin.SetSize(500, 250)

# render and interact with data

iRen = svtk.svtkRenderWindowInteractor()
iRen.SetRenderWindow(renWin);
ren1.ResetCamera()
renWin.Render()

iRen.Initialize()
#iRen.Start()
