#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# This example demonstrates the use of the linear extrusion filter and
# the USA state outline svtk dataset. It also tests the triangulation filter.

# Create the RenderWindow, Renderer and both Actors
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create pipeline - read data
#
reader = svtk.svtkPolyDataReader()
reader.SetFileName(SVTK_DATA_ROOT + "/Data/usa.svtk")

# okay, now create some extrusion filters with actors for each US state
math = svtk.svtkMath()
i = 0
while i < 51:
    idx = str(i)
    exec("extractCell" + idx + " = svtk.svtkGeometryFilter()")
    eval("extractCell" + idx).SetInputConnection(reader.GetOutputPort())
    eval("extractCell" + idx).CellClippingOn()
    eval("extractCell" + idx).SetCellMinimum(i)
    eval("extractCell" + idx).SetCellMaximum(i)

    exec("tf" + idx + " = svtk.svtkTriangleFilter()")
    eval("tf" + idx).SetInputConnection(
      eval("extractCell" + idx).GetOutputPort())

    exec("extrude" + idx + " = svtk.svtkLinearExtrusionFilter()")
    eval("extrude" + idx).SetInputConnection(
      eval("tf" + idx).GetOutputPort())
    eval("extrude" + idx).SetExtrusionType(1)
    eval("extrude" + idx).SetVector(0, 0, 1)
    eval("extrude" + idx).CappingOn()
    eval("extrude" + idx).SetScaleFactor(math.Random(1, 10))

    exec("mapper" + idx + " = svtk.svtkPolyDataMapper()")
    eval("mapper" + idx).SetInputConnection(
      eval("extrude" + idx).GetOutputPort())

    exec("actor" + idx + " = svtk.svtkActor()")
    eval("actor" + idx).SetMapper(eval("mapper" + idx))
    eval("actor" + idx).GetProperty().SetColor(math.Random(0, 1),
      math.Random(0, 1), math.Random(0, 1))

    ren1.AddActor(eval("actor" + idx))

    i += 1

ren1.SetBackground(1, 1, 1)

renWin.SetSize(500, 250)

cam1 = ren1.GetActiveCamera()
cam1.SetClippingRange(10.2299, 511.497)
cam1.SetPosition(-119.669, -25.5502, 79.0198)
cam1.SetFocalPoint(-115.96, 41.6709, 1.99546)
cam1.SetViewUp(-0.0013035, 0.753456, 0.657497)

# render the image
#
renWin.Render()

iren.Initialize()
#iren.Start()
