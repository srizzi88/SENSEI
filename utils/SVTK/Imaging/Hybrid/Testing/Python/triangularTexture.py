#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

#
# create a triangular texture
#
# Create the RenderWindow, Renderer and both Actors
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
aTriangularTexture = svtk.svtkTriangularTexture()
aTriangularTexture.SetTexturePattern(1)
aTriangularTexture.SetXSize(32)
aTriangularTexture.SetYSize(32)
points = svtk.svtkPoints()
points.InsertPoint(0,0.0,0.0,0.0)
points.InsertPoint(1,1.0,0.0,0.0)
points.InsertPoint(2,.5,1.0,0.0)
points.InsertPoint(3,1.0,0.0,0.0)
points.InsertPoint(4,0.0,0.0,0.0)
points.InsertPoint(5,.5,-1.0,.5)
tCoords = svtk.svtkFloatArray()
tCoords.SetNumberOfComponents(2)
tCoords.InsertTuple2(0,0.0,0.0)
tCoords.InsertTuple2(1,1.0,0.0)
tCoords.InsertTuple2(2,.5,.86602540378443864676)
tCoords.InsertTuple2(3,0.0,0.0)
tCoords.InsertTuple2(4,1.0,0.0)
tCoords.InsertTuple2(5,.5,.86602540378443864676)
pointData = svtk.svtkPointData()
pointData.SetTCoords(tCoords)
triangles = svtk.svtkCellArray()
triangles.InsertNextCell(3)
triangles.InsertCellPoint(0)
triangles.InsertCellPoint(1)
triangles.InsertCellPoint(2)
triangles.InsertNextCell(3)
triangles.InsertCellPoint(3)
triangles.InsertCellPoint(4)
triangles.InsertCellPoint(5)
triangle = svtk.svtkPolyData()
triangle.SetPolys(triangles)
triangle.SetPoints(points)
triangle.GetPointData().SetTCoords(tCoords)
triangleMapper = svtk.svtkPolyDataMapper()
triangleMapper.SetInputData(triangle)
aTexture = svtk.svtkTexture()
aTexture.SetInputConnection(aTriangularTexture.GetOutputPort())
triangleActor = svtk.svtkActor()
triangleActor.SetMapper(triangleMapper)
triangleActor.SetTexture(aTexture)
ren1.SetBackground(.3,.7,.2)
ren1.AddActor(triangleActor)
ren1.ResetCamera()
ren1.GetActiveCamera().Zoom(1.5)
# render the image
#
iren.Initialize()
# prevent the tk window from showing up then start the event loop
# --- end of script --
