#!/usr/bin/env python

# This example demonstrates the use of svtkSTLReader to load data into
# SVTK from a file.  This example also uses svtkLODActor which changes
# its graphical representation of the data to maintain interactive
# performance.

import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Create the reader and read a data file.  Connect the mapper and
# actor.
sr = svtk.svtkSTLReader()
sr.SetFileName(SVTK_DATA_ROOT + "/Data/42400-IDGH.stl")

stlMapper = svtk.svtkPolyDataMapper()
stlMapper.SetInputConnection(sr.GetOutputPort())

stlActor = svtk.svtkLODActor()
stlActor.SetMapper(stlMapper)

# Create the Renderer, RenderWindow, and RenderWindowInteractor
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the render; set the background and size
ren.AddActor(stlActor)
ren.SetBackground(0.1, 0.2, 0.4)
renWin.SetSize(500, 500)

# Zoom in closer
ren.ResetCamera()
cam1 = ren.GetActiveCamera()
cam1.Zoom(1.4)

iren.Initialize()
renWin.Render()
iren.Start()
