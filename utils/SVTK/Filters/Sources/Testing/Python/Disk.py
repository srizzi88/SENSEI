#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
disk = svtk.svtkDiskSource()
disk.SetInnerRadius(1.0)
disk.SetOuterRadius(2.0)
disk.SetRadialResolution(1)
disk.SetCircumferentialResolution(20)
diskMapper = svtk.svtkPolyDataMapper()
diskMapper.SetInputConnection(disk.GetOutputPort())
diskActor = svtk.svtkActor()
diskActor.SetMapper(diskMapper)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(diskActor)
ren1.SetBackground(0.1,0.2,0.4)
renWin.SetSize(200,200)
# Get handles to some useful objects
#
iren.Initialize()
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
