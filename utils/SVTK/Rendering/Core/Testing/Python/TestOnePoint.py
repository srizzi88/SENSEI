#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
points = svtk.svtkPoints()
points.InsertPoint(0,0.0,0.0,0.0)
verts = svtk.svtkCellArray()
verts.InsertNextCell(1)
verts.InsertCellPoint(0)
polyData = svtk.svtkPolyData()
polyData.SetPoints(points)
polyData.SetVerts(verts)
mapper = svtk.svtkPolyDataMapper()
mapper.SetInputData(polyData)
actor = svtk.svtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetPointSize(8)
ren1.AddViewProp(actor)
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
