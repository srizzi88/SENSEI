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
#dens0.SetDensityFormToVolumeNormalized()
dens0.SetDensityFormToNumberOfPoints()
dens0.ComputeGradientOn()
dens0.Update()
vrange = dens0.GetOutput().GetPointData().GetArray("Gradient Magnitude").GetRange()

# Show the gradient magnitude
assign0 = svtk.svtkAssignAttribute()
assign0.SetInputConnection(dens0.GetOutputPort())
assign0.Assign("Gradient Magnitude", "SCALARS", "POINT_DATA")

map0 = svtk.svtkImageSliceMapper()
map0.BorderOn()
map0.SliceAtFocalPointOn()
map0.SliceFacesCameraOn()
map0.SetInputConnection(assign0.GetOutputPort())

slice0 = svtk.svtkImageSlice()
slice0.SetMapper(map0)
slice0.GetProperty().SetColorWindow(vrange[1]-vrange[0])
slice0.GetProperty().SetColorLevel(0.5*(vrange[0]+vrange[1]))

# Show the region labels
assign1 = svtk.svtkAssignAttribute()
assign1.SetInputConnection(dens0.GetOutputPort())
assign1.Assign("Classification", "SCALARS", "POINT_DATA")

map1 = svtk.svtkImageSliceMapper()
map1.BorderOn()
map1.SliceAtFocalPointOn()
map1.SliceFacesCameraOn()
map1.SetInputConnection(assign1.GetOutputPort())
map1.Update()

slice1 = svtk.svtkImageSlice()
slice1.SetMapper(map1)
slice1.GetProperty().SetColorWindow(1)
slice1.GetProperty().SetColorLevel(0.5)

# Show the vectors (gradient)
assign2 = svtk.svtkAssignAttribute()
assign2.SetInputConnection(dens0.GetOutputPort())
assign2.Assign("Gradient", "VECTORS", "POINT_DATA")

plane = svtk.svtkPlane()
plane.SetNormal(0,0,1)
plane.SetOrigin(0.0701652, 0.172689, 0.27271)

cut = svtk.svtkFlyingEdgesPlaneCutter()
cut.SetInputConnection(assign2.GetOutputPort())
cut.SetPlane(plane)
cut.InterpolateAttributesOn()

v = svtk.svtkHedgeHog()
v.SetInputConnection(cut.GetOutputPort())
v.SetScaleFactor(0.0001)

vMapper = svtk.svtkPolyDataMapper()
vMapper.SetInputConnection(v.GetOutputPort())
vMapper.SetScalarRange(vrange)

vectors = svtk.svtkActor()
vectors.SetMapper(vMapper)

# Create the RenderWindow, Renderer and both Actors
#
ren0 = svtk.svtkRenderer()
ren0.SetViewport(0,0,0.333,1)
ren1 = svtk.svtkRenderer()
ren1.SetViewport(0.333,0,0.667,1)
ren2 = svtk.svtkRenderer()
ren2.SetViewport(0.667,0,1,1)

renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren0)
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren0.AddActor(slice0)
ren0.SetBackground(0,0,0)
ren1.AddActor(slice1)
ren1.SetBackground(0,0,0)
ren2.AddActor(vectors)
ren2.SetBackground(0,0,0)

renWin.SetSize(450,150)

cam = ren0.GetActiveCamera()
cam.ParallelProjectionOn()
cam.SetFocalPoint(0,0,0)
cam.SetPosition(0,0,1)
ren0.ResetCamera()

ren1.SetActiveCamera(cam)
ren2.SetActiveCamera(cam)

iren.Initialize()

# render the image
#
renWin.Render()

iren.Start()
