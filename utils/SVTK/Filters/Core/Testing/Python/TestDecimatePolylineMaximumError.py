#!/usr/bin/env python

# Decimate polyline with maximum error
import svtk
import svtk.test.Testing
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Create rendering stuff
#
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
iRen = svtk.svtkRenderWindowInteractor()
iRen.SetRenderWindow(renWin);

# Plane source is used to create a polyline around boundary and then
# decimated.
ps = svtk.svtkPlaneSource()
ps.SetResolution(50,100)

fe = svtk.svtkFeatureEdges()
fe.SetInputConnection(ps.GetOutputPort())
fe.BoundaryEdgesOn()
fe.FeatureEdgesOff()
fe.NonManifoldEdgesOff()

s = svtk.svtkStripper()
s.SetInputConnection(fe.GetOutputPort())

deci = svtk.svtkDecimatePolylineFilter()
deci.SetInputConnection(s.GetOutputPort())
deci.SetMaximumError(0.00001)
deci.SetTargetReduction(0.99)
deci.Update()

mapper = svtk.svtkPolyDataMapper()
mapper.SetInputConnection(deci.GetOutputPort())

actor = svtk.svtkActor()
actor.SetMapper(mapper)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(actor)

cam = ren.GetActiveCamera()
cam.SetPosition(0,0,1)
cam.SetFocalPoint(0,0,0)
ren.ResetCamera()

# render and interact with data
renWin.SetSize(300, 300)
renWin.Render()
iRen.Start()
