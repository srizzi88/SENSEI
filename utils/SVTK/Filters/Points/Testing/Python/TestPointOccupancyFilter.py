#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# The resolution of the masking volume
res = 15

# Parameters for debugging
math = svtk.svtkMath()
math.RandomSeed(31415)

# create pipeline
#
# A set of points at the extreme positions
pts = svtk.svtkPoints()
pts.SetNumberOfPoints(9)
pts.SetPoint(0, 0,0,0)
pts.SetPoint(1, -1,-1,-1)
pts.SetPoint(2,  1,-1,-1)
pts.SetPoint(3, -1, 1,-1)
pts.SetPoint(4,  1, 1,-1)
pts.SetPoint(5, -1,-1, 1)
pts.SetPoint(6,  1,-1, 1)
pts.SetPoint(7, -1, 1, 1)
pts.SetPoint(8,  1, 1, 1)

ptsPD = svtk.svtkPolyData()
ptsPD.SetPoints(pts);

# Generate occupancy mask
occ = svtk.svtkPointOccupancyFilter()
occ.SetInputData(ptsPD)
occ.SetSampleDimensions(res,res+2,res+4)
occ.SetOccupiedValue(255)

# Display the result: outline plus surface
outline = svtk.svtkOutlineFilter()
outline.SetInputConnection(occ.GetOutputPort())

outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper);

surface = svtk.svtkThreshold()
surface.SetInputConnection(occ.GetOutputPort())
surface.ThresholdByUpper(1)
surface.AllScalarsOff()

surfaceMapper = svtk.svtkDataSetMapper()
surfaceMapper.SetInputConnection(surface.GetOutputPort())
surfaceMapper.ScalarVisibilityOff()

surfaceActor = svtk.svtkActor()
surfaceActor.SetMapper(surfaceMapper);

points = svtk.svtkMaskPointsFilter()
points.SetInputData(ptsPD)
points.SetMaskConnection(occ.GetOutputPort())
points.GenerateVerticesOn()

pointsMapper = svtk.svtkPolyDataMapper()
pointsMapper.SetInputConnection(points.GetOutputPort())

pointsActor = svtk.svtkActor()
pointsActor.SetMapper(pointsMapper);
pointsActor.GetProperty().SetPointSize(3)
pointsActor.GetProperty().SetColor(0,1,0)

# Create the RenderWindow, Renderer and both Actors
#
ren0 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren0)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
#ren0.AddActor(outlineActor)
ren0.AddActor(surfaceActor)
#ren0.AddActor(pointsActor)
ren0.SetBackground(0,0,0)

renWin.SetSize(300,300)

cam = ren0.GetActiveCamera()
cam.SetFocalPoint(0,0,0)
cam.SetPosition(1,1,1)
ren0.ResetCamera()

iren.Initialize()

# render the image
#
renWin.Render()

iren.Start()
