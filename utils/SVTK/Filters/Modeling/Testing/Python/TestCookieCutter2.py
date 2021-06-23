#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot

# This tests special cases like polylines and triangle strips

# create planes
# Create the RenderWindow, Renderer
#
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer( ren )

iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Custom polydata to test polylines
linesData = svtk.svtkPolyData()
linesPts = svtk.svtkPoints()
linesLines = svtk.svtkCellArray()
linesData.SetPoints(linesPts)
linesData.SetLines(linesLines)

linesPts.InsertPoint(0,  -1.0,-1.0, 0.0)
linesPts.InsertPoint(1,   1.0,-1.0, 0.0)
linesPts.InsertPoint(2,   1.0, 1.0, 0.0)
linesPts.InsertPoint(3,  -1.0, 1.0, 0.0)
linesPts.InsertPoint(4,  -0.2,-0.2, 0.0)
linesPts.InsertPoint(5,   0.0, 0.0, 0.0)
linesPts.InsertPoint(6,   0.2, 0.2, 0.0)

linesLines.InsertNextCell(3)
linesLines.InsertCellPoint(1)
linesLines.InsertCellPoint(5)
linesLines.InsertCellPoint(3)
linesLines.InsertNextCell(5)
linesLines.InsertCellPoint(0)
linesLines.InsertCellPoint(4)
linesLines.InsertCellPoint(5)
linesLines.InsertCellPoint(6)
linesLines.InsertCellPoint(2)

# Create a triangle strip to cookie cut
plane = svtk.svtkPlaneSource()
plane.SetXResolution(25)
plane.SetYResolution(1)
plane.SetOrigin(-1,-0.1,0)
plane.SetPoint1( 1,-0.1,0)
plane.SetPoint2(-1, 0.1,0)

tri = svtk.svtkTriangleFilter()
tri.SetInputConnection(plane.GetOutputPort())

stripper = svtk.svtkStripper()
stripper.SetInputConnection(tri.GetOutputPort())

append = svtk.svtkAppendPolyData()
append.AddInputData(linesData)
append.AddInputConnection(stripper.GetOutputPort())

# Create a loop for cookie cutting
loops = svtk.svtkPolyData()
loopPts = svtk.svtkPoints()
loopPolys = svtk.svtkCellArray()
loops.SetPoints(loopPts)
loops.SetPolys(loopPolys)

loopPts.SetNumberOfPoints(4)
loopPts.SetPoint(0, -0.35,0.0,0.0)
loopPts.SetPoint(1, 0,-0.35,0.0)
loopPts.SetPoint(2, 0.35,0.0,0.0)
loopPts.SetPoint(3, 0.0,0.35,0.0)

loopPolys.InsertNextCell(4)
loopPolys.InsertCellPoint(0)
loopPolys.InsertCellPoint(1)
loopPolys.InsertCellPoint(2)
loopPolys.InsertCellPoint(3)

cookie = svtk.svtkCookieCutter()
cookie.SetInputConnection(append.GetOutputPort())
cookie.SetLoopsData(loops)
cookie.Update()

mapper = svtk.svtkPolyDataMapper()
mapper.SetInputConnection(cookie.GetOutputPort())

actor = svtk.svtkActor()
actor.SetMapper(mapper)

loopMapper = svtk.svtkPolyDataMapper()
loopMapper.SetInputData(loops)

loopActor = svtk.svtkActor()
loopActor.SetMapper(loopMapper)
loopActor.GetProperty().SetColor(1,0,0)
loopActor.GetProperty().SetRepresentationToWireframe()

ren.AddActor(actor)
ren.AddActor(loopActor)

renWin.Render()
#iren.Start()
