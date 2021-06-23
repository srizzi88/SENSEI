#!/usr/bin/env python
import svtk
from svtk.test import Testing

# create a test dataset
#
math = svtk.svtkMath()

# Controls size of test
res = 15

# Create an initial set of points and associated dataset
mandel = svtk.svtkImageMandelbrotSource()
mandel.SetWholeExtent(-res,res,-res,res,-res,res)
mandel.Update()

sphere = svtk.svtkSphere()
sphere.SetCenter(mandel.GetOutput().GetCenter())
sphere.SetRadius(mandel.GetOutput().GetLength()/4)

# Clip data to spit out unstructured tets
clipper = svtk.svtkClipDataSet()
clipper.SetInputConnection(mandel.GetOutputPort())
clipper.SetClipFunction(sphere)
clipper.InsideOutOn()
clipper.Update()

output = clipper.GetOutput()
numCells = output.GetNumberOfCells()
bounds = output.GetBounds()
#print bounds

# Support subsequent method calls
genCell = svtk.svtkGenericCell()
t = svtk.reference(0.0)
x = [0,0,0]
pc = [0,0,0]
subId = svtk.reference(0)
cellId = svtk.reference(0)

# Build the locator
locator = svtk.svtkStaticCellLocator()
#locator = svtk.svtkCellLocator()
locator.SetDataSet(output)
locator.AutomaticOn()
locator.SetNumberOfCellsPerNode(20)
locator.CacheCellBoundsOn()
locator.BuildLocator()

# Now visualize the locator
locatorPD = svtk.svtkPolyData()
locator.GenerateRepresentation(0,locatorPD)

locatorMapper = svtk.svtkPolyDataMapper()
locatorMapper.SetInputData(locatorPD)

locatorActor = svtk.svtkActor()
locatorActor.SetMapper(locatorMapper)

# Outline around the entire dataset
outline = svtk.svtkOutlineFilter()
outline.SetInputConnection(mandel.GetOutputPort())

outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)

# Intersect the clipped output with a ray
ray = svtk.svtkPolyData()
rayPts = svtk.svtkPoints()
rayPts.InsertPoint(0, -7.5,-5,-5)
rayPts.InsertPoint(1,  2.5, 2, 2.5)
rayLine = svtk.svtkCellArray()
rayLine.InsertNextCell(2)
rayLine.InsertCellPoint(0)
rayLine.InsertCellPoint(1)
ray.SetPoints(rayPts)
ray.SetLines(rayLine)

rayMapper = svtk.svtkPolyDataMapper()
rayMapper.SetInputData(ray)

rayActor = svtk.svtkActor()
rayActor.SetMapper(rayMapper)
rayActor.GetProperty().SetColor(0,1,0)

# Produce intersection point
hit = locator.IntersectWithLine(rayPts.GetPoint(0), rayPts.GetPoint(1), 0.001,
                                t, x, pc, subId, cellId, genCell)
#print ("Hit: {0}".format(hit))
#print ("CellId: {0}".format(cellId))
assert cellId == 209

# Create the RenderWindow, Renderer and both Actors
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(outlineActor)
ren1.AddActor(locatorActor)
ren1.AddActor(rayActor)
ren1.SetBackground(0,0,0)
renWin.SetSize(200,200)

iren.Initialize()
iren.Start()
