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
# Eight points forming a cube (the regular svtkCubeSource
# produces duplicate points)
cube = svtk.svtkPlatonicSolidSource()
cube.SetSolidTypeToCube()

sphere = svtk.svtkSphereSource()
sphere.SetRadius(0.05)

glyphs = svtk.svtkGlyph3D()
glyphs.SetInputConnection(cube.GetOutputPort())
glyphs.SetSourceConnection(sphere.GetOutputPort())
glyphs.ScalingOff()
glyphs.OrientOff()

mapper = svtk.svtkPolyDataMapper()
mapper.SetInputConnection(glyphs.GetOutputPort())

actor = svtk.svtkActor()
actor.SetMapper(mapper)

# Now densify the point cloud and reprocess. Use a
# neighborhood type of N closest points
denser1 = svtk.svtkDensifyPointCloudFilter()
denser1.SetInputConnection(cube.GetOutputPort())
denser1.SetNeighborhoodTypeToNClosest()
denser1.SetNumberOfClosestPoints(3)
denser1.SetTargetDistance(1.0)
denser1.SetMaximumNumberOfIterations(3)

glyphs1 = svtk.svtkGlyph3D()
glyphs1.SetInputConnection(denser1.GetOutputPort())
glyphs1.SetSourceConnection(sphere.GetOutputPort())
glyphs1.ScalingOff()
glyphs1.OrientOff()

mapper1 = svtk.svtkPolyDataMapper()
mapper1.SetInputConnection(glyphs1.GetOutputPort())

actor1 = svtk.svtkActor()
actor1.SetMapper(mapper1)

# Now densify the point cloud and reprocess. Use a
# neighborhood type of RADIUS
denser2 = svtk.svtkDensifyPointCloudFilter()
denser2.SetInputConnection(cube.GetOutputPort())
denser2.SetNeighborhoodTypeToRadius()
denser2.SetRadius(1.8)
denser2.SetTargetDistance(1.0)
denser2.SetMaximumNumberOfIterations(10)
denser2.SetMaximumNumberOfPoints(50)
denser2.Update()
print(denser2)

glyphs2 = svtk.svtkGlyph3D()
glyphs2.SetInputConnection(denser2.GetOutputPort())
glyphs2.SetSourceConnection(sphere.GetOutputPort())
glyphs2.ScalingOff()
glyphs2.OrientOff()

mapper2 = svtk.svtkPolyDataMapper()
mapper2.SetInputConnection(glyphs2.GetOutputPort())

actor2 = svtk.svtkActor()
actor2.SetMapper(mapper2)

# Create the RenderWindow, Renderer and both Actors
#
ren0 = svtk.svtkRenderer()
ren0.SetViewport(0,0,0.333,1.0)
ren1 = svtk.svtkRenderer()
ren1.SetViewport(0.333,0,.6667,1.0)
ren2 = svtk.svtkRenderer()
ren2.SetViewport(0.6667,0,1,1.0)

renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren0)
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren0.AddActor(actor)
ren0.SetBackground(0,0,0)
ren1.AddActor(actor1)
ren1.SetBackground(0,0,0)
ren2.AddActor(actor2)
ren2.SetBackground(0,0,0)

renWin.SetSize(900,300)

cam = ren0.GetActiveCamera()
cam.SetFocalPoint(0,0,0)
cam.SetPosition(1,1,1)
ren0.ResetCamera()

ren1.SetActiveCamera(cam)
ren2.SetActiveCamera(cam)

iren.Initialize()

# render the image
#
renWin.Render()

iren.Start()
