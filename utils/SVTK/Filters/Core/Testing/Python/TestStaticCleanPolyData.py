#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Control problem size and set debugging parameters
NPts = 1000
#NPts = 1000000
MaxTileClips = NPts
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
voronoi.Update()

clean = svtk.svtkStaticCleanPolyData()
#clean = svtk.svtkCleanPolyData()
clean.SetInputConnection(voronoi.GetOutputPort())
clean.ToleranceIsAbsoluteOn()
clean.SetAbsoluteTolerance(0.00001)

# Time execution
timer = svtk.svtkTimerLog()
timer.StartTimer()
clean.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Number of points processed: {0}".format(NPts))
print("   Time to clean: {0}".format(time))
print("   #In pts: {0}".format(clean.GetInput().GetNumberOfPoints()))
print("   #Out pts: {0}".format(clean.GetOutput().GetNumberOfPoints()))

mapper = svtk.svtkPolyDataMapper()
mapper.SetInputConnection(clean.GetOutputPort())
mapper.SetScalarRange(0,NPts)
print("Scalar Range: {}".format(mapper.GetScalarRange()))

actor = svtk.svtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetColor(1,0,0)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(actor)
ren1.AddActor(ptActor)

ren1.SetBackground(1,1,1)
renWin.SetSize(300,300)
renWin.Render()
cam1 = ren1.GetActiveCamera()

renWin.Render()
iren.Start()
# --- end of script --
