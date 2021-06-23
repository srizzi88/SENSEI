#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

renWin = svtk.svtkRenderWindow()
renWin.OffScreenRenderingOn()
renWin.SetMultiSamples(0)
ren = svtk.svtkRenderer()
renWin.AddRenderer(ren)
cone = svtk.svtkConeSource()
mp = svtk.svtkPolyDataMapper()
mp.SetInputConnection(cone.GetOutputPort())
actor = svtk.svtkActor()
actor.SetMapper(mp)
ren.AddActor(actor)
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
