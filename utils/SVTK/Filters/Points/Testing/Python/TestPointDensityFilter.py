#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# The resolution of the density function volume
res = 100

# Parameters for debugging
NPts = 1000000
math = svtk.svtkMath()
math.RandomSeed(31415)

# create pipeline
#
points = svtk.svtkBoundedPointSource()
points.SetNumberOfPoints(NPts)
points.ProduceRandomScalarsOn()
points.ProduceCellOutputOff()
points.Update()

# Create a sphere implicit function
sphere = svtk.svtkSphere()
sphere.SetCenter(0.0,0.1,0.2)
sphere.SetRadius(0.75)

# Extract points within sphere
extract = svtk.svtkFitImplicitFunction()
extract.SetInputConnection(points.GetOutputPort())
extract.SetImplicitFunction(sphere)
extract.SetThreshold(0.005)
extract.GenerateVerticesOn()

# Clip out some of the points with a plane; requires vertices
plane = svtk.svtkPlane()
plane.SetOrigin(sphere.GetCenter())
plane.SetNormal(1,1,1)

clipper = svtk.svtkClipPolyData()
clipper.SetInputConnection(extract.GetOutputPort())
clipper.SetClipFunction(plane);

# Generate density field from points
# Use fixed radius
dens0 = svtk.svtkPointDensityFilter()
dens0.SetInputConnection(clipper.GetOutputPort())
dens0.SetSampleDimensions(res,res,res)
dens0.SetDensityEstimateToFixedRadius()
dens0.SetRadius(0.05)
#dens0.SetDensityEstimateToRelativeRadius()
dens0.SetRelativeRadius(2.5)
dens0.SetDensityFormToVolumeNormalized()

# Time execution
timer = svtk.svtkTimerLog()
timer.StartTimer()
dens0.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Time to compute density field: {0}".format(time))
vrange = dens0.GetOutput().GetScalarRange()
print(dens0)

map0 = svtk.svtkImageSliceMapper()
map0.BorderOn()
map0.SliceAtFocalPointOn()
map0.SliceFacesCameraOn()
map0.SetInputConnection(dens0.GetOutputPort())

slice0 = svtk.svtkImageSlice()
slice0.SetMapper(map0)
slice0.GetProperty().SetColorWindow(vrange[1]-vrange[0])
slice0.GetProperty().SetColorLevel(0.5*(vrange[0]+vrange[1]))

# Generate density field from points
# Use relative radius
dens1 = svtk.svtkPointDensityFilter()
dens1.SetInputConnection(clipper.GetOutputPort())
dens1.SetSampleDimensions(res,res,res)
#dens1.SetDensityEstimateToFixedRadius()
dens1.SetRadius(0.05)
dens1.SetDensityEstimateToRelativeRadius()
dens1.SetRelativeRadius(2.5)
dens1.SetDensityFormToNumberOfPoints()

# Time execution
timer = svtk.svtkTimerLog()
timer.StartTimer()
dens1.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Time to compute density field: {0}".format(time))
vrange = dens1.GetOutput().GetScalarRange()

map1 = svtk.svtkImageSliceMapper()
map1.BorderOn()
map1.SliceAtFocalPointOn()
map1.SliceFacesCameraOn()
map1.SetInputConnection(dens1.GetOutputPort())

slice1 = svtk.svtkImageSlice()
slice1.SetMapper(map1)
slice1.GetProperty().SetColorWindow(vrange[1]-vrange[0])
slice1.GetProperty().SetColorLevel(0.5*(vrange[0]+vrange[1]))

# Generate density field from points
# Use fixed radius and weighted point density and volume normalized density
# First need to generate some scalar attributes (weights)
weights = svtk.svtkRandomAttributeGenerator()
weights.SetInputConnection(clipper.GetOutputPort())
weights.SetMinimumComponentValue(0.25)
weights.SetMaximumComponentValue(1.75)
weights.GenerateAllDataOff()
weights.GeneratePointScalarsOn()

dens2 = svtk.svtkPointDensityFilter()
dens2.SetInputConnection(weights.GetOutputPort())
dens2.SetSampleDimensions(res,res,res)
dens2.SetDensityEstimateToFixedRadius()
dens2.SetRadius(0.05)
#dens2.SetDensityEstimateToRelativeRadius()
dens2.SetRelativeRadius(2.5)
dens2.SetDensityFormToVolumeNormalized()
dens2.ScalarWeightingOn()

# Time execution
timer = svtk.svtkTimerLog()
timer.StartTimer()
dens2.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Time to compute density field: {0}".format(time))
vrange = dens2.GetOutput().GetScalarRange()

map2 = svtk.svtkImageSliceMapper()
map2.BorderOn()
map2.SliceAtFocalPointOn()
map2.SliceFacesCameraOn()
map2.SetInputConnection(dens2.GetOutputPort())

slice2 = svtk.svtkImageSlice()
slice2.SetMapper(map2)
slice2.GetProperty().SetColorWindow(vrange[1]-vrange[0])
slice2.GetProperty().SetColorLevel(0.5*(vrange[0]+vrange[1]))

# Generate density field from points
# Use relative radius and weighted point density and npts density
dens3 = svtk.svtkPointDensityFilter()
dens3.SetInputConnection(weights.GetOutputPort())
dens3.SetSampleDimensions(res,res,res)
#dens3.SetDensityEstimateToFixedRadius()
dens3.SetRadius(0.05)
dens3.SetDensityEstimateToRelativeRadius()
dens3.SetRelativeRadius(2.5)
dens3.SetDensityFormToNumberOfPoints()
dens3.ScalarWeightingOn()

# Time execution
timer = svtk.svtkTimerLog()
timer.StartTimer()
dens3.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Time to compute density field: {0}".format(time))
vrange = dens3.GetOutput().GetScalarRange()

map3 = svtk.svtkImageSliceMapper()
map3.BorderOn()
map3.SliceAtFocalPointOn()
map3.SliceFacesCameraOn()
map3.SetInputConnection(dens3.GetOutputPort())

slice3 = svtk.svtkImageSlice()
slice3.SetMapper(map3)
slice3.GetProperty().SetColorWindow(vrange[1]-vrange[0])
slice3.GetProperty().SetColorLevel(0.5*(vrange[0]+vrange[1]))

# Create the RenderWindow, Renderer and both Actors
#
ren0 = svtk.svtkRenderer()
ren0.SetViewport(0,0,0.5,0.5)
ren1 = svtk.svtkRenderer()
ren1.SetViewport(0.5,0,1,0.5)
ren2 = svtk.svtkRenderer()
ren2.SetViewport(0,0.5,0.5,1)
ren3 = svtk.svtkRenderer()
ren3.SetViewport(0.5,0.5,1,1)

renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren0)
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
renWin.AddRenderer(ren3)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren0.AddActor(slice0)
ren0.SetBackground(0,0,0)
ren1.AddActor(slice1)
ren1.SetBackground(0,0,0)
ren2.AddActor(slice2)
ren2.SetBackground(0,0,0)
ren3.AddActor(slice3)
ren3.SetBackground(0,0,0)

renWin.SetSize(300,300)

cam = ren0.GetActiveCamera()
cam.ParallelProjectionOn()
cam.SetFocalPoint(0,0,0)
cam.SetPosition(0,0,1)
ren0.ResetCamera()

ren1.SetActiveCamera(cam)
ren2.SetActiveCamera(cam)
ren3.SetActiveCamera(cam)

iren.Initialize()

# render the image
#
renWin.Render()

iren.Start()
