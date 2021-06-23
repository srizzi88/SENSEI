#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Control test size
numPts = 500

# Create the RenderWindow, Renderer and both Actors
#
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create an enclosing surface
ss = svtk.svtkSphereSource()
ss.SetPhiResolution(25)
ss.SetThetaResolution(38)
ss.SetCenter(4.5, 5.5, 5.0)
ss.SetRadius(2.5)

mapper = svtk.svtkPolyDataMapper()
mapper.SetInputConnection(ss.GetOutputPort())

actor = svtk.svtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetRepresentationToWireframe()

# Generate some random points and scalars
points = svtk.svtkPoints()
points.SetNumberOfPoints(numPts);

da = points.GetData()
pool = svtk.svtkRandomPool()
pool.PopulateDataArray(da, 0, 2.25, 7)
pool.PopulateDataArray(da, 1, 1, 10)
pool.PopulateDataArray(da, 2, 0.5, 10.5)

scalars = svtk.svtkFloatArray()
scalars.SetNumberOfTuples(numPts)
scalars.SetName("Random Scalars");
pool.PopulateDataArray(scalars, 100,200)

profile = svtk.svtkPolyData()
profile.SetPoints(points)
profile.GetPointData().SetScalars(scalars);

extract = svtk.svtkExtractEnclosedPoints()
extract.SetInputData(profile)
extract.SetSurfaceConnection(ss.GetOutputPort())

# Time execution
timer = svtk.svtkTimerLog()
timer.StartTimer()
extract.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Time to extract points: {0}".format(time))

glyph = svtk.svtkSphereSource()
glypher = svtk.svtkGlyph3D()
glypher.SetInputConnection(extract.GetOutputPort())
#glypher.SetInputConnection(extract.GetOutputPort(1))
glypher.SetSourceConnection(glyph.GetOutputPort())
glypher.SetScaleModeToDataScalingOff()
glypher.SetScaleFactor(0.25)

pointsMapper = svtk.svtkPolyDataMapper()
pointsMapper.SetInputConnection(glypher.GetOutputPort())
pointsMapper.SetScalarRange(100,200)

pointsActor = svtk.svtkActor()
pointsActor.SetMapper(pointsMapper)

# Add actors
ren.AddActor(actor)
ren.AddActor(pointsActor)

# Standard testing code.
renWin.SetSize(300,300)
renWin.Render()

# render the image
#
renWin.Render()
iren.Start()
