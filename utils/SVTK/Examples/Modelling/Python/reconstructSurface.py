#!/usr/bin/env python

# This example shows how to construct a surface from a point cloud.
# First we generate a volume using the
# svtkSurfaceReconstructionFilter. The volume values are a distance
# field. Once this is generated, the volume is countoured at a
# distance value of 0.0.

import os
import string
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Read some points. Use a programmable filter to read them.
pointSource = svtk.svtkProgrammableSource()

def readPoints():
    output = pointSource.GetPolyDataOutput()
    points = svtk.svtkPoints()
    output.SetPoints(points)

    file = open(os.path.normpath(os.path.join(SVTK_DATA_ROOT, "Data/cactus.3337.pts")))

    line = file.readline()
    while line:
        data = line.split()
        if data and data[0] == 'p':
            x, y, z = float(data[1]), float(data[2]), float(data[3])
            points.InsertNextPoint(x, y, z)
        line = file.readline()

pointSource.SetExecuteMethod(readPoints)


# Construct the surface and create isosurface.
surf = svtk.svtkSurfaceReconstructionFilter()
surf.SetInputConnection(pointSource.GetOutputPort())

cf = svtk.svtkContourFilter()
cf.SetInputConnection(surf.GetOutputPort())
cf.SetValue(0, 0.0)

# Sometimes the contouring algorithm can create a volume whose gradient
# vector and ordering of polygon (using the right hand rule) are
# inconsistent. svtkReverseSense cures this problem.
reverse = svtk.svtkReverseSense()
reverse.SetInputConnection(cf.GetOutputPort())
reverse.ReverseCellsOn()
reverse.ReverseNormalsOn()

map = svtk.svtkPolyDataMapper()
map.SetInputConnection(reverse.GetOutputPort())
map.ScalarVisibilityOff()

surfaceActor = svtk.svtkActor()
surfaceActor.SetMapper(map)
surfaceActor.GetProperty().SetDiffuseColor(1.0000, 0.3882, 0.2784)
surfaceActor.GetProperty().SetSpecularColor(1, 1, 1)
surfaceActor.GetProperty().SetSpecular(.4)
surfaceActor.GetProperty().SetSpecularPower(50)

# Create the RenderWindow, Renderer and both Actors
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(surfaceActor)
ren.SetBackground(1, 1, 1)
renWin.SetSize(400, 400)
ren.GetActiveCamera().SetFocalPoint(0, 0, 0)
ren.GetActiveCamera().SetPosition(1, 0, 0)
ren.GetActiveCamera().SetViewUp(0, 0, 1)
ren.ResetCamera()
ren.GetActiveCamera().Azimuth(20)
ren.GetActiveCamera().Elevation(30)
ren.GetActiveCamera().Dolly(1.2)
ren.ResetCameraClippingRange()

iren.Initialize()
renWin.Render()
iren.Start()
