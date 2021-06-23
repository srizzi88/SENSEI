#!/usr/bin/env python

# This example demonstrates the use of svtkLabeledDataMapper.  This
# class is used for displaying numerical data from an underlying data
# set.  In the case of this example, the underlying data are the point
# and cell ids.

import svtk

# Create a selection window.  We will display the point and cell ids
# that lie within this window.
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

# Create a sphere and its associated mapper and actor.
sphere = svtk.svtkSphereSource()
sphereMapper = svtk.svtkPolyDataMapper()
sphereMapper.SetInputConnection(sphere.GetOutputPort())
sphereActor = svtk.svtkActor()
sphereActor.SetMapper(sphereMapper)

# Generate data arrays containing point and cell ids
ids = svtk.svtkIdFilter()
ids.SetInputConnection(sphere.GetOutputPort())
ids.PointIdsOn()
ids.CellIdsOn()
ids.FieldDataOn()

# Create the renderer here because svtkSelectVisiblePoints needs it.
ren = svtk.svtkRenderer()

# Create labels for points
visPts = svtk.svtkSelectVisiblePoints()
visPts.SetInputConnection(ids.GetOutputPort())
visPts.SetRenderer(ren)
visPts.SelectionWindowOn()
visPts.SetSelection(xmin, xmin + xLength, ymin, ymin + yLength)

# Create the mapper to display the point ids.  Specify the format to
# use for the labels.  Also create the associated actor.
ldm = svtk.svtkLabeledDataMapper()
# ldm.SetLabelFormat("%g")
ldm.SetInputConnection(visPts.GetOutputPort())
ldm.SetLabelModeToLabelFieldData()
pointLabels = svtk.svtkActor2D()
pointLabels.SetMapper(ldm)

# Create labels for cells
cc = svtk.svtkCellCenters()
cc.SetInputConnection(ids.GetOutputPort())
visCells = svtk.svtkSelectVisiblePoints()
visCells.SetInputConnection(cc.GetOutputPort())
visCells.SetRenderer(ren)
visCells.SelectionWindowOn()
visCells.SetSelection(xmin, xmin + xLength, ymin, ymin + yLength)

# Create the mapper to display the cell ids.  Specify the format to
# use for the labels.  Also create the associated actor.
cellMapper = svtk.svtkLabeledDataMapper()
cellMapper.SetInputConnection(visCells.GetOutputPort())
# cellMapper.SetLabelFormat("%g")
cellMapper.SetLabelModeToLabelFieldData()
cellMapper.GetLabelTextProperty().SetColor(0, 1, 0)
cellLabels = svtk.svtkActor2D()
cellLabels.SetMapper(cellMapper)

# Create the RenderWindow and RenderWindowInteractor
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer; set the background and size;
# render
ren.AddActor(sphereActor)
ren.AddActor2D(rectActor)
ren.AddActor2D(pointLabels)
ren.AddActor2D(cellLabels)

ren.SetBackground(1, 1, 1)
renWin.SetSize(500, 500)

# Create a function to move the selection window across the data set.
def MoveWindow():
    for y in range(100, 300, 25):
        for x in range(100, 300, 25):
            PlaceWindow(x, y)


# Create a function to draw the selection window at each location it
# is moved to.
def PlaceWindow(xmin, ymin):
    global xLength, yLength, visPts, visCells, pts, renWin

    xmax = xmin + xLength
    ymax = ymin + yLength

    visPts.SetSelection(xmin, xmax, ymin, ymax)
    visCells.SetSelection(xmin, xmax, ymin, ymax)

    pts.InsertPoint(0, xmin, ymin, 0)
    pts.InsertPoint(1, xmax, ymin, 0)
    pts.InsertPoint(2, xmax, ymax, 0)
    pts.InsertPoint(3, xmin, ymax, 0)
    # Call Modified because InsertPoints does not modify svtkPoints
    # (for performance reasons)
    pts.Modified()
    renWin.Render()


# Initialize the interactor.
iren.Initialize()
renWin.Render()

# Move the selection window across the data set.
MoveWindow()

# Put the selection window in the center of the render window.
# This works because the xmin = ymin = 200, xLength = yLength = 100, and
# the render window size is 500 x 500.
PlaceWindow(xmin, ymin)

# Now start normal interaction.
iren.Start()
