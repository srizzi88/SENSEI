#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Control problem size and set debugging parameters
NPts = 1000
#NPts = 1000000
MaxTileClips = 10000
PointsPerBucket = 2
GenerateFlower = 1
PointOfInterest = -1

# Create the RenderWindow, Renderer and both Actors
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create some points and display them
#
math = svtk.svtkMath()
math.RandomSeed(31415)
points = svtk.svtkPoints()
i = 0
while i < NPts:
    points.InsertPoint(i,math.Random(0,1),math.Random(0,1),0.0)
    i = i + 1

#points.InsertPoint(0,0,0,0.0)
#points.InsertPoint(1,0,-2,0.0)
#points.InsertPoint(2,2,0,0.0)
#points.InsertPoint(3,0,4,0.0)
#points.InsertPoint(4,-6,0,0.0)

#points.InsertPoint(0,0,0,0.0)
#points.InsertPoint(1,0,-2,0.0)
#points.InsertPoint(2,2,0,0.0)
#points.InsertPoint(3,0,2,0.0)
#points.InsertPoint(4,-2,0,0.0)

profile = svtk.svtkPolyData()
profile.SetPoints(points)

ptMapper = svtk.svtkPointGaussianMapper()
ptMapper.SetInputData(profile)
ptMapper.EmissiveOff()
ptMapper.SetScaleFactor(0.0)

ptActor = svtk.svtkActor()
ptActor.SetMapper(ptMapper)
ptActor.GetProperty().SetColor(0,0,0)
ptActor.GetProperty().SetPointSize(2)

# Tessellate them
#
voronoi = svtk.svtkVoronoi2D()
voronoi.SetInputData(profile)
voronoi.SetGenerateScalarsToNone()
voronoi.SetGenerateScalarsToThreadIds()
voronoi.SetGenerateScalarsToPointIds()
voronoi.SetPointOfInterest(PointOfInterest)
voronoi.SetMaximumNumberOfTileClips(MaxTileClips)
voronoi.GetLocator().SetNumberOfPointsPerBucket(PointsPerBucket)
voronoi.SetGenerateVoronoiFlower(GenerateFlower)

# Time execution
timer = svtk.svtkTimerLog()
timer.StartTimer()
voronoi.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Number of points processed: {0}".format(NPts))
print("   Time to generate Voronoi tessellation: {0}".format(time))
print("   Number of threads used: {0}".format(voronoi.GetNumberOfThreadsUsed()))

mapper = svtk.svtkPolyDataMapper()
mapper.SetInputConnection(voronoi.GetOutputPort())
if voronoi.GetGenerateScalars() == 1:
    mapper.SetScalarRange(0,NPts)
elif voronoi.GetGenerateScalars() == 2:
    mapper.SetScalarRange(0,voronoi.GetNumberOfThreadsUsed())
print("Scalar Range: {}".format(mapper.GetScalarRange()))

actor = svtk.svtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetColor(1,0,0)

# Debug code
sphere = svtk.svtkSphereSource()
sphere.SetRadius(0.05)
sphere.SetThetaResolution(16)
sphere.SetPhiResolution(8)
if PointOfInterest >= 0:
    sphere.SetCenter(points.GetPoint(PointOfInterest))

sphereMapper = svtk.svtkPolyDataMapper()
sphereMapper.SetInputConnection(sphere.GetOutputPort())

sphereActor = svtk.svtkActor()
sphereActor.SetMapper(sphereMapper)
sphereActor.GetProperty().SetColor(0,0,0)

# Voronoi flower
fMapper = svtk.svtkPointGaussianMapper()
fMapper.SetInputConnection(voronoi.GetOutputPort(1))
fMapper.EmissiveOff()
fMapper.SetScaleFactor(0.0)

fActor = svtk.svtkActor()
fActor.SetMapper(fMapper)
fActor.GetProperty().SetColor(0,0,1)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(actor)
ren1.AddActor(ptActor)
if PointOfInterest >= 0:
    ren1.AddActor(sphereActor)
    if GenerateFlower > 0:
         ren1.AddActor(fActor)
         ren1.RemoveActor(ptActor)

ren1.SetBackground(1,1,1)
renWin.SetSize(300,300)
renWin.Render()
cam1 = ren1.GetActiveCamera()
if PointOfInterest >= 0:
    xyz = []
    xyz = points.GetPoint(PointOfInterest)
    cam1.SetFocalPoint(xyz)
    cam1.SetPosition(xyz[0],xyz[1],xyz[2]+1)
    renWin.Render()
    ren1.ResetCameraClippingRange()
    cam1.Zoom(1.5)

# added these unused default arguments so that the prototype
# matches as required in python.
def reportPointId (a=0,b=0,__svtk__temp0=0,__svtk__temp1=0):
    print("Point Id: {}".format(picker.GetPointId()))

picker = svtk.svtkPointPicker()
picker.AddObserver("EndPickEvent",reportPointId)
picker.PickFromListOn()
picker.AddPickList(ptActor)
iren.SetPicker(picker)

renWin.Render()
iren.Start()
# --- end of script --
