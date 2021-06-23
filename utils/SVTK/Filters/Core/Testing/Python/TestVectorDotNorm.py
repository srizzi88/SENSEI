#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# this is a tcl version of plate vibration
ren1 = svtk.svtkRenderer()
ren2 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
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

color2 = svtk.svtkVectorNorm()
color2.SetInputConnection(plate.GetOutputPort())
color2.NormalizeOn()

plateMapper2 = svtk.svtkDataSetMapper()
plateMapper2.SetInputConnection(color2.GetOutputPort())
plateMapper2.SetLookupTable(lut)
plateMapper2.SetScalarRange(0, 1)

plateActor2 = svtk.svtkActor()
plateActor2.SetMapper(plateMapper2)

# Add the actors to the renderer, set the background and size
#
ren1.SetViewport(0, 0, .5, 1)
ren2.SetViewport(.5, 0, 1, 1)

ren1.AddActor(plateActor)
ren1.SetBackground(1, 1, 1)
ren2.AddActor(plateActor2)
ren2.SetBackground(1, 1, 1)

renWin.SetSize(500, 250)

camera = svtk.svtkCamera()
camera.SetPosition(1,1,1)
ren1.SetActiveCamera(camera)
ren2.SetActiveCamera(camera)

ren1.ResetCamera()
renWin.Render()

# render the image
#
iren.Initialize()
#iren.Start()
