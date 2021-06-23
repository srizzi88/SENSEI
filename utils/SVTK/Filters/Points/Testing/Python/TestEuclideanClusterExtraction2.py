#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Parameters for debugging
NPts = 100000
math = svtk.svtkMath()
math.RandomSeed(31415)

# create pipeline
#
points = svtk.svtkBoundedPointSource()
points.SetNumberOfPoints(NPts)
points.SetBounds(-3,3, -1,1, -1,1)
points.ProduceRandomScalarsOff()
points.ProduceCellOutputOff()

# create some scalars based on implicit function
# Create a cylinder
cyl = svtk.svtkCylinder()
cyl.SetCenter(-2,0,0)
cyl.SetRadius(0.02)

# Create a (thin) box implicit function
box = svtk.svtkBox()
box.SetBounds(-1,0.5, -0.5,0.5, -0.0005, 0.0005)

# Create a sphere implicit function
sphere = svtk.svtkSphere()
sphere.SetCenter(2,0,0)
sphere.SetRadius(0.8)

# Boolean (union) these together
imp = svtk.svtkImplicitBoolean()
imp.SetOperationTypeToUnion()
imp.AddFunction(cyl)
imp.AddFunction(box)
imp.AddFunction(sphere)

# Generate scalars and vector
sample = svtk.svtkSampleImplicitFunctionFilter()
sample.SetInputConnection(points.GetOutputPort())
sample.SetImplicitFunction(imp)
sample.Update()
print(sample.GetOutput().GetScalarRange())

# Now see if we can extract the three objects as separate clusters.
extr = svtk.svtkEuclideanClusterExtraction()
extr.SetInputConnection(sample.GetOutputPort())
extr.SetRadius(0.15)
#extr.ColorClustersOn()
#extr.SetExtractionModeToAllClusters()
extr.SetExtractionModeToLargestCluster()
extr.ScalarConnectivityOn()
extr.SetScalarRange(-0.64,-.3)

# Time execution
timer = svtk.svtkTimerLog()
timer.StartTimer()
extr.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Points processed: {0}".format(points.GetOutput().GetNumberOfPoints()))
print("   Time to segment objects: {0}".format(time))
print("   Number of clusters: {0}".format(extr.GetNumberOfExtractedClusters()))

# Draw the points
subMapper = svtk.svtkPointGaussianMapper()
subMapper.SetInputConnection(extr.GetOutputPort(0))
#subMapper.SetInputConnection(sample.GetOutputPort(0))
subMapper.EmissiveOff()
subMapper.SetScaleFactor(0.0)
subMapper.SetScalarRange(-0.64,2.25)

subActor = svtk.svtkActor()
subActor.SetMapper(subMapper)

# Create an outline
outline = svtk.svtkOutlineFilter()
outline.SetInputConnection(sample.GetOutputPort())

outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)

# Create the RenderWindow, Renderer and both Actors
#
ren0 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren0)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren0.AddActor(subActor)
ren0.AddActor(outlineActor)
ren0.SetBackground(0.1, 0.2, 0.4)

renWin.SetSize(250,250)

cam = ren0.GetActiveCamera()
cam.SetFocalPoint(0,0,-1)
cam.SetPosition(0,0,0)
ren0.ResetCamera()

iren.Initialize()

# render the image
#
renWin.Render()

iren.Start()
