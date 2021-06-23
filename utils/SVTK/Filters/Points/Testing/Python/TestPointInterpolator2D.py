#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Parameters for debugging
NPts = 1000000
math = svtk.svtkMath()

# create pipeline: use terrain dataset
#
# Read the data: a height field results
demReader = svtk.svtkDEMReader()
demReader.SetFileName(SVTK_DATA_ROOT + "/Data/SainteHelens.dem")
demReader.Update()

lo = demReader.GetOutput().GetScalarRange()[0]
hi = demReader.GetOutput().GetScalarRange()[1]

geom = svtk.svtkImageDataGeometryFilter()
geom.SetInputConnection(demReader.GetOutputPort())

warp = svtk.svtkWarpScalar()
warp.SetInputConnection(geom.GetOutputPort())
warp.SetNormal(0, 0, 1)
warp.UseNormalOn()
warp.SetScaleFactor(2)
warp.Update()

bds = warp.GetOutput().GetBounds()
center = warp.GetOutput().GetCenter()

# A randomized point cloud, whose attributes are set via implicit function
points = svtk.svtkPoints()
points.SetDataTypeToFloat()
points.SetNumberOfPoints(NPts)
for i in range(0,NPts):
    points.SetPoint(i,math.Random(bds[0],bds[1]),math.Random(bds[2],bds[3]),math.Random(bds[4],bds[5]))

source = svtk.svtkPolyData()
source.SetPoints(points)

sphere = svtk.svtkSphere()
sphere.SetCenter(center[0],center[1]-7500,center[2])

attr = svtk.svtkSampleImplicitFunctionFilter()
attr.SetInputData(source)
attr.SetImplicitFunction(sphere)
attr.Update()

# Gaussian kernel-------------------------------------------------------
gaussianKernel = svtk.svtkGaussianKernel()
gaussianKernel.SetSharpness(4)
gaussianKernel.SetRadius(50)

voronoiKernel = svtk.svtkVoronoiKernel()

interpolator1 = svtk.svtkPointInterpolator2D()
interpolator1.SetInputConnection(warp.GetOutputPort())
interpolator1.SetSourceConnection(attr.GetOutputPort())
#interpolator1.SetKernel(gaussianKernel)
interpolator1.SetKernel(voronoiKernel)
interpolator1.SetNullPointsStrategyToClosestPoint()

# Time execution
timer = svtk.svtkTimerLog()
timer.StartTimer()
interpolator1.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Interpolate Terrain Points (Gaussian): {0}".format(time))

scalarRange = attr.GetOutput().GetScalarRange()

intMapper1 = svtk.svtkPolyDataMapper()
intMapper1.SetInputConnection(interpolator1.GetOutputPort())
intMapper1.SetScalarRange(scalarRange)

intActor1 = svtk.svtkActor()
intActor1.SetMapper(intMapper1)

# Create an outline
outline1 = svtk.svtkOutlineFilter()
outline1.SetInputConnection(warp.GetOutputPort())

outlineMapper1 = svtk.svtkPolyDataMapper()
outlineMapper1.SetInputConnection(outline1.GetOutputPort())

outlineActor1 = svtk.svtkActor()
outlineActor1.SetMapper(outlineMapper1)

# Create the RenderWindow, Renderer and both Actors
#
ren0 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren0)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren0.AddActor(intActor1)
ren0.AddActor(outlineActor1)
ren0.SetBackground(0.1, 0.2, 0.4)

renWin.SetSize(250, 250)

cam = ren0.GetActiveCamera()
cam.SetFocalPoint(center)

fp = cam.GetFocalPoint()
cam.SetPosition(fp[0]+.2,fp[1]+.1,fp[2]+1)
ren0.ResetCamera()

iren.Initialize()

# render the image
#
renWin.Render()

iren.Start()
