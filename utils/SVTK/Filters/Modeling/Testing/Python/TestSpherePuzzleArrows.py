#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# create a rendering window and renderer
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

renWin.SetSize(400, 400)

puzzle = svtk.svtkSpherePuzzle()

mapper = svtk.svtkPolyDataMapper()
mapper.SetInputConnection(puzzle.GetOutputPort())

actor = svtk.svtkActor()
actor.SetMapper(mapper)

arrows = svtk.svtkSpherePuzzleArrows()

mapper2 = svtk.svtkPolyDataMapper()
mapper2.SetInputConnection(arrows.GetOutputPort())

actor2 = svtk.svtkActor()
actor2.SetMapper(mapper2)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(actor)
ren1.AddActor(actor2)
ren1.SetBackground(0.1, 0.2, 0.4)
LastVal = -1

def MotionCallback (x, y):
    global LastVal
    WindowY = 400
    y = WindowY - y
    z = ren1.GetZ(x, y)
    ren1.SetDisplayPoint(x, y, z)
    ren1.DisplayToWorld()
    pt = ren1.GetWorldPoint()
    print(pt)  ###############
    x = pt[0]
    y = pt[1]
    z = pt[2]
    val = puzzle.SetPoint(x, y, z)
    if (val != LastVal):
        renWin.Render()
        LastVal = val
        pass

def ButtonCallback (x, y):
    WindowY = 400
    y = WindowY - y
    z = ren1.GetZ(x, y)
    ren1.SetDisplayPoint(x, y, z)
    ren1.DisplayToWorld()
    pt = ren1.GetWorldPoint()
#    print pt
    x = pt[0]
    y = pt[1]
    z = pt[2]
    i = 0
    while i <= 100:
        puzzle.SetPoint(x, y, z)
        puzzle.MovePoint(i)
        renWin.Render()
        i += 5


renWin.Render()

cam = ren1.GetActiveCamera()
cam.Elevation(-40)

ButtonCallback(261, 272)

arrows.SetPermutation(puzzle)

renWin.Render()

iren.Initialize()
#iren.Start()
