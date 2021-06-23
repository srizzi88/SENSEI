#!/usr/bin/env python
import sys
import svtk

# Quad
pts = svtk.svtkPoints()
pts.SetNumberOfPoints(6)
coords = [(0,0,0),(2,0,0),(4,0,0),(0,4,0),(2,4,0),(4,4,0)]
for i in range(0,6):
    pts.InsertPoint(i,coords[i])
quads = svtk.svtkCellArray()
cellpoints=[(0,1,4,3),(1,2,5,4)]
for i in range(0,2):
    quads.InsertNextCell(4)
    for j in range(0,4):
        quads.InsertCellPoint(cellpoints[i][j])

poly = svtk.svtkPolyData()
poly.SetPoints(pts)
poly.SetPolys(quads)

mapper = svtk.svtkPolyDataMapper()
mapper.SetInputData(poly)

actor = svtk.svtkActor()
actor.SetMapper(mapper)

ren = svtk.svtkRenderer()
ren.AddActor(actor)

renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
renWin.SetSize(200,200)

iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

renWin.Render()
iren.Initialize()

pos =         [ (1,1),    (35,40),  (80,40),  (120,40), (165,40),
                (199,160),(165,160),(120,160),(80,160), (35,160)  ]
pickedCells = [ -1,       0,        0,        1,        1,
                -1,       1,        1,        0,        0 ]
pickedPoints= [ -1,       0,        1,        1,        2,
                -1,       5,        4,        4,        3 ]

cellErrors  = 0
pointErrors = 0
cellPicker = svtk.svtkCellPicker()
for i in range(0,len(pos)):
    cellPicker.Pick(pos[i][0],pos[i][1],0,ren)
    if cellPicker.GetCellId() != pickedCells[i]:
        print("pos [{0}] : picked cell id={1} expected cell id={2}".format(pos[i],cellPicker.GetCellId(),pickedCells[i]))
        cellErrors = cellErrors + 1
    if cellPicker.GetPointId() != pickedPoints[i]:
        print("pos [{0}] : picked point id={1} expected point id={2}".format(pos[i],cellPicker.GetPointId(),pickedPoints[i]))
        pointErrors = pointErrors + 1
if pointErrors or cellErrors:
    print( "ERROR: Cell picker : {0} cell-id errors, {1} point-id errors".format(cellErrors,pointErrors) )
    sys.exit(1)
