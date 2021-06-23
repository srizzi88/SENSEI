#!/usr/bin/env python

import math
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Demonstrates the generation of an elliptical arc

arc=svtk.svtkEllipseArcSource()
arc.SetRatio(.25)
arc.SetResolution(40)
arc.SetStartAngle(45)
arc.SetSegmentAngle(360)
arc.CloseOff()
arc.Update()

assert arc.GetOutput().GetNumberOfPoints() == arc.GetResolution()+1

arc.CloseOn()
arc.Update()

assert arc.GetOutput().GetNumberOfPoints() == arc.GetResolution()

m=svtk.svtkPolyDataMapper()
m.SetInputData(arc.GetOutput())
a = svtk.svtkActor()
a.SetMapper(m)
a.GetProperty().SetColor(1,1,0)
a.GetProperty().EdgeVisibilityOn()
a.GetProperty().RenderLinesAsTubesOn()
a.GetProperty().SetLineWidth(3)
a.GetProperty().SetVertexColor(1,0,0)
a.GetProperty().VertexVisibilityOn()
a.GetProperty().RenderPointsAsSpheresOn()
a.GetProperty().SetPointSize(6)

r=svtk.svtkRenderer()
r.AddActor(a)
r.SetBackground(.4,.4,.4)
renWin=svtk.svtkRenderWindow()
renWin.AddRenderer(r)
renWin.Render()

# --- end of script ---
