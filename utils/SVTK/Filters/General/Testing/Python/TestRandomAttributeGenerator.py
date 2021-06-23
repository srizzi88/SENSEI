#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Create the RenderWindow, Renderer and interactive renderer
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)

# make sure to have the same regression image on all platforms.
renWin.SetMultiSamples(0)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Force a starting random value
raMath = svtk.svtkMath()
raMath.RandomSeed(6)

# Generate random attributes on a plane
#
ps = svtk.svtkPlaneSource()
ps.SetXResolution(10)
ps.SetYResolution(10)

ag = svtk.svtkRandomAttributeGenerator()
ag.SetInputConnection(ps.GetOutputPort())
ag.GenerateAllDataOn()

ss = svtk.svtkSphereSource()
ss.SetPhiResolution(16)
ss.SetThetaResolution(32)

tg = svtk.svtkTensorGlyph()
tg.SetInputConnection(ag.GetOutputPort())
tg.SetSourceConnection(ss.GetOutputPort())
tg.SetInputArrayToProcess(1,0,0,0,"RandomPointArray")

tg.SetScaleFactor(0.1)
tg.SetMaxScaleFactor(10)
tg.ClampScalingOn()

n = svtk.svtkPolyDataNormals()
n.SetInputConnection(tg.GetOutputPort())

pdm = svtk.svtkPolyDataMapper()
pdm.SetInputConnection(n.GetOutputPort())

a = svtk.svtkActor()
a.SetMapper(pdm)

pm = svtk.svtkPolyDataMapper()
pm.SetInputConnection(ps.GetOutputPort())

pa = svtk.svtkActor()
pa.SetMapper(pm)

ren1.AddActor(a)
ren1.AddActor(pa)
ren1.SetBackground(0, 0, 0)

renWin.SetSize(300, 300)

renWin.Render()

#iren.Start()
