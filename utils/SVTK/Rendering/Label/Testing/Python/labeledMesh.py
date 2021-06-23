#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# demonstrate use of point labeling and the selection window

# Create the RenderWindow, Renderer and both Actors
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create a selection window
xmin = 200
xLength = 100
xmax = xmin + xLength
ymin = 200
yLength = 100
ymax = ymin + yLength

pts = svtk.svtkPoints()
pts.InsertPoint(0, xmin, ymin, 0)
pts.InsertPoint(1, xmax, ymin, 0)
pts.InsertPoint(2, xmax, ymax, 0)
pts.InsertPoint(3, xmin, ymax, 0)

rect = svtk.svtkCellArray()
rect.InsertNextCell(5)
rect.InsertCellPoint(0)
rect.InsertCellPoint(1)
rect.InsertCellPoint(2)
rect.InsertCellPoint(3)
rect.InsertCellPoint(0)

selectRect = svtk.svtkPolyData()
selectRect.SetPoints(pts)
selectRect.SetLines(rect)

rectMapper = svtk.svtkPolyDataMapper2D()
rectMapper.SetInputData(selectRect)

rectActor = svtk.svtkActor2D()
rectActor.SetMapper(rectMapper)

# Create asphere
sphere = svtk.svtkSphereSource()

sphereMapper = svtk.svtkPolyDataMapper()
sphereMapper.SetInputConnection(sphere.GetOutputPort())

sphereActor = svtk.svtkActor()
sphereActor.SetMapper(sphereMapper)

# Generate ids for labeling
ids = svtk.svtkIdFilter()
ids.SetInputConnection(sphere.GetOutputPort())
ids.PointIdsOn()
ids.CellIdsOn()
ids.FieldDataOn()

# Create labels for points
visPts = svtk.svtkSelectVisiblePoints()
visPts.SetInputConnection(ids.GetOutputPort())
visPts.SetRenderer(ren1)
visPts.SelectionWindowOn()
visPts.SetSelection(xmin, xmin + xLength, ymin, ymin + yLength)

ldm = svtk.svtkLabeledDataMapper()
ldm.SetInputConnection(visPts.GetOutputPort())
#    ldm.SetLabelFormat.("%g")
#    ldm.SetLabelModeToLabelScalars()
#    ldm.SetLabelModeToLabelNormals()
ldm.SetLabelModeToLabelFieldData()
#    ldm.SetLabeledComponent(0)

pointLabels = svtk.svtkActor2D()
pointLabels.SetMapper(ldm)

# Create labels for cells
cc = svtk.svtkCellCenters()
cc.SetInputConnection(ids.GetOutputPort())

visCells = svtk.svtkSelectVisiblePoints()
visCells.SetInputConnection(cc.GetOutputPort())
visCells.SetRenderer(ren1)
visCells.SelectionWindowOn()
visCells.SetSelection(xmin, xmin + xLength, ymin, ymin + yLength)

cellMapper = svtk.svtkLabeledDataMapper()
cellMapper.SetInputConnection(visCells.GetOutputPort())
#    cellMapper.SetLabelFormat("%g")
#    cellMapper.SetLabelModeToLabelScalars()
#    cellMapper.SetLabelModeToLabelNormals()
cellMapper.SetLabelModeToLabelFieldData()
cellMapper.GetLabelTextProperty().SetColor(0, 1, 0)

cellLabels = svtk.svtkActor2D()
cellLabels.SetMapper(cellMapper)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(sphereActor)
ren1.AddActor2D(rectActor)
ren1.AddActor2D(pointLabels)
ren1.AddActor2D(cellLabels)
ren1.SetBackground(1, 1, 1)

renWin.SetSize(500, 500)

renWin.Render()
# render the image
#

def PlaceWindow (xmin, ymin):
    global xLength, yLength
    xmax = xmin + xLength
    ymax = ymin + yLength
    visPts.SetSelection(xmin, xmax, ymin, ymax)
    visCells.SetSelection(xmin, xmax, ymin, ymax)
    pts.InsertPoint(0, xmin, ymin, 0)
    pts.InsertPoint(1, xmax, ymin, 0)
    pts.InsertPoint(2, xmax, ymax, 0)
    pts.InsertPoint(3, xmin, ymax, 0)
    pts.Modified()
    # because insertions don't modify object - performance reasons
    renWin.Render()

def MoveWindow ():
    y = 100
    while y < 300:
        x = 100
        while x < 300:
            PlaceWindow(x, y)
            x += 25
        y += 25


MoveWindow()
PlaceWindow(xmin, ymin)

#iren.Start()
