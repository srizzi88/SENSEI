#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# this is a tcl version of plate vibration
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# read a svtk file
#
plate = svtk.svtkPolyDataReader()
plate.SetFileName(SVTK_DATA_ROOT + "/Data/plate.svtk")
plate.SetVectorsName("mode8")

warp = svtk.svtkWarpVector()
warp.SetInputConnection(plate.GetOutputPort())
warp.SetScaleFactor(0.5)

normals = svtk.svtkPolyDataNormals()
normals.SetInputConnection(warp.GetOutputPort())

color = svtk.svtkVectorDot()
color.SetInputConnection(normals.GetOutputPort())

lut = svtk.svtkLookupTable()
lut.SetNumberOfColors(256)
lut.Build()

i = 0
while i < 128:
    lut.SetTableValue(i, (128.0 - i) / 128.0, (128.0 - i) / 128.0,
      (128.0 - i) / 128.0, 1)
    i += 1

i = 128
while i < 256:
    lut.SetTableValue(i, (i - 128.0) / 128.0, (i - 128.0) / 128.0,
      (i - 128.0) / 128.0, 1)
    i += 1

plateMapper = svtk.svtkDataSetMapper()
plateMapper.SetInputConnection(color.GetOutputPort())
plateMapper.SetLookupTable(lut)
plateMapper.SetScalarRange(-1, 1)

plateActor = svtk.svtkActor()
plateActor.SetMapper(plateMapper)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(plateActor)
ren1.SetBackground(1, 1, 1)

renWin.SetSize(250, 250)

ren1.GetActiveCamera().SetPosition(13.3991, 14.0764, 9.97787)
ren1.GetActiveCamera().SetFocalPoint(1.50437, 0.481517, 4.52992)
ren1.GetActiveCamera().SetViewAngle(30)
ren1.GetActiveCamera().SetViewUp(-0.120861, 0.458556, -0.880408)
ren1.GetActiveCamera().SetClippingRange(12.5724, 26.8374)

# render the image
#
iren.Initialize()
#iren.Start()
