#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

reader = svtk.svtkSimplePointsReader()
reader.SetFileName(SVTK_DATA_ROOT + "/Data/points.txt")

mapper = svtk.svtkPolyDataMapper()
mapper.SetInputConnection(reader.GetOutputPort())

actor = svtk.svtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetPointSize(5)

ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren1.AddActor(actor)

renWin.SetSize(300, 300)

iren.Initialize()

renWin.Render()

#iren.Start()
