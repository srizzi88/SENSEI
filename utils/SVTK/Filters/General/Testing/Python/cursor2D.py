#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()


# create four cursors configured differently
cursor = svtk.svtkCursor2D()
cursor.SetModelBounds(15, 45, 15, 45, 0, 0)
cursor.SetFocalPoint(30, 30, 0)
cursor.AllOff()
cursor.AxesOn()
cursor.OutlineOn()
cursorMapper = svtk.svtkPolyDataMapper2D()
cursorMapper.SetInputConnection(cursor.GetOutputPort())
cursorActor = svtk.svtkActor2D()
cursorActor.SetMapper(cursorMapper)
cursorActor.GetProperty().SetColor(1, 0, 0)

cursor2 = svtk.svtkCursor2D()
cursor2.SetModelBounds(75, 105, 15, 45, 0, 0)
cursor2.SetFocalPoint(90, 30, 0)
cursor2.AllOff()
cursor2.AxesOn()
cursor2.OutlineOn()
cursor2.PointOn()
cursor2Mapper = svtk.svtkPolyDataMapper2D()
cursor2Mapper.SetInputConnection(cursor2.GetOutputPort())
cursor2Actor = svtk.svtkActor2D()
cursor2Actor.SetMapper(cursor2Mapper)
cursor2Actor.GetProperty().SetColor(0, 1, 0)

cursor3 = svtk.svtkCursor2D()
cursor3.SetModelBounds(15, 45, 75, 105, 0, 0)
cursor3.SetFocalPoint(30, 90, 0)
cursor3.AllOff()
cursor3.AxesOn()
cursor3.OutlineOff()
cursor3.PointOn()
cursor3.SetRadius(3)
cursor3Mapper = svtk.svtkPolyDataMapper2D()
cursor3Mapper.SetInputConnection(cursor3.GetOutputPort())
cursor3Actor = svtk.svtkActor2D()
cursor3Actor.SetMapper(cursor3Mapper)
cursor3Actor.GetProperty().SetColor(0, 1, 0)

cursor4 = svtk.svtkCursor2D()
cursor4.SetModelBounds(75, 105, 75, 105, 0, 0)
cursor4.SetFocalPoint(90, 90, 0)
cursor4.AllOff()
cursor4.AxesOn()
cursor4.SetRadius(0.0)
cursor4Mapper = svtk.svtkPolyDataMapper2D()
cursor4Mapper.SetInputConnection(cursor4.GetOutputPort())
cursor4Actor = svtk.svtkActor2D()
cursor4Actor.SetMapper(cursor4Mapper)
cursor4Actor.GetProperty().SetColor(1, 0, 0)

# rendering support
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# read data
ren1.AddActor(cursorActor)
ren1.AddActor(cursor2Actor)
ren1.AddActor(cursor3Actor)
ren1.AddActor(cursor4Actor)
ren1.SetBackground(0, 0, 0)

renWin.SetSize(150, 150)
renWin.SetMultiSamples(0)
renWin.Render()

iren.Initialize()
#iren.Start()
