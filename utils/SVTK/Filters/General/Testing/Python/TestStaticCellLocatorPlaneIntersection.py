#!/usr/bin/env python
import svtk
from svtk.test import Testing

# Controls size of test
res = 15

# Create an initial set of data
wavelet = svtk.svtkRTAnalyticSource()
wavelet.SetWholeExtent(-res,res,-res,res,-res,res)
wavelet.Update()

# Isocontour
contour = svtk.svtkFlyingEdges3D()
contour.SetInputConnection(wavelet.GetOutputPort())
contour.SetValue(0,100)
contour.SetValue(1,200)
contour.Update()

# Build the locator
locator = svtk.svtkStaticCellLocator()
locator.SetDataSet(contour.GetOutput())
locator.AutomaticOn()
locator.SetNumberOfCellsPerNode(20)
locator.CacheCellBoundsOn()
locator.BuildLocator()

# Now extract cells from locator and display it
origin = [0,0,0]
normal = [1,1,1]
cellIds = svtk.svtkIdList()

timer = svtk.svtkTimerLog()
timer.StartTimer()
locator.FindCellsAlongPlane(origin,normal,0.0,cellIds);
timer.StopTimer()
time = timer.GetElapsedTime()
print("Cell extraction: {0}".format(time))
print("Number cells extracted: {0}".format(cellIds.GetNumberOfIds()))

extract = svtk.svtkExtractCells()
extract.SetInputConnection(contour.GetOutputPort())
extract.SetCellList(cellIds);

mapper = svtk.svtkDataSetMapper()
#mapper.SetInputConnection(contour.GetOutputPort())
mapper.SetInputConnection(extract.GetOutputPort())

actor = svtk.svtkActor()
actor.SetMapper(mapper)

# In case we want to see the mesh and the cells that are extracted
meshMapper = svtk.svtkPolyDataMapper()
meshMapper.SetInputConnection(contour.GetOutputPort())
meshMapper.ScalarVisibilityOff()

meshActor = svtk.svtkActor()
meshActor.SetMapper(meshMapper)
meshActor.GetProperty().SetRepresentationToWireframe()
meshActor.GetProperty().SetColor(1,0,0)

# Outline around the entire dataset
outline = svtk.svtkOutlineFilter()
outline.SetInputConnection(wavelet.GetOutputPort())

outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(0,0,0)


# Create the RenderWindow, Renderer and both Actors
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(actor)
ren1.AddActor(outlineActor)
#ren1.AddActor(meshActor)

ren1.SetBackground(1,1,1)
renWin.SetSize(200,200)

iren.Initialize()
iren.Start()
