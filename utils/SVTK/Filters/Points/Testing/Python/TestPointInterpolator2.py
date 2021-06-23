#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Interpolate onto a volume

# Parameters for debugging
res = 40

# create pipeline
#
extent = [0,56, 0,32, 0,24]
pl3d = svtk.svtkMultiBlockPLOT3DReader()
pl3d.SetXYZFileName(SVTK_DATA_ROOT + "/Data/combxyz.bin")
pl3d.SetQFileName(SVTK_DATA_ROOT + "/Data/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()

output = pl3d.GetOutput().GetBlock(0)

# Create a probe volume
center = output.GetCenter()
bounds = output.GetBounds()
length = output.GetLength()

probe = svtk.svtkImageData()
probe.SetDimensions(res,res,res)
probe.SetOrigin(bounds[0],bounds[2],bounds[4])
probe.SetSpacing((bounds[1]-bounds[0])/(res-1),
                 (bounds[3]-bounds[2])/(res-1),
                 (bounds[5]-bounds[4])/(res-1))

# Reuse the locator
locator = svtk.svtkStaticPointLocator()
locator.SetDataSet(output)
locator.BuildLocator()

# Use a gaussian kernel------------------------------------------------
gaussianKernel = svtk.svtkGaussianKernel()
gaussianKernel.SetRadius(0.5)
gaussianKernel.SetSharpness(4)
print ("Radius: {0}".format(gaussianKernel.GetRadius()))

interpolator = svtk.svtkPointInterpolator()
interpolator.SetInputData(probe)
interpolator.SetSourceData(output)
interpolator.SetKernel(gaussianKernel)
interpolator.SetLocator(locator)
interpolator.SetNullPointsStrategyToClosestPoint()

# Time execution
timer = svtk.svtkTimerLog()
timer.StartTimer()
interpolator.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Interpolate Points (Volume probe): {0}".format(time))

intMapper = svtk.svtkDataSetMapper()
intMapper.SetInputConnection(interpolator.GetOutputPort())

intActor = svtk.svtkActor()
intActor.SetMapper(intMapper)

# Create an outline
outline = svtk.svtkStructuredGridOutlineFilter()
outline.SetInputData(output)

outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)

# Create the RenderWindow, Renderer and both Actors
#
ren0 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren0)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren0.AddActor(intActor)
ren0.AddActor(outlineActor)
ren0.SetBackground(0.1, 0.2, 0.4)

renWin.SetSize(250,250)

cam = ren0.GetActiveCamera()
cam.SetClippingRange(3.95297, 50)
cam.SetFocalPoint(8.88908, 0.595038, 29.3342)
cam.SetPosition(-12.3332, 31.7479, 41.2387)
cam.SetViewUp(0.060772, -0.319905, 0.945498)

iren.Initialize()

# render the image
#
renWin.Render()

iren.Start()
