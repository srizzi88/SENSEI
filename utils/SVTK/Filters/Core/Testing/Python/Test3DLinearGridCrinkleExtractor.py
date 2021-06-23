#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Control test size
res = 50
#res = 300

# Create the RenderWindow, Renderers and both Actors
ren0 = svtk.svtkRenderer()
ren1 = svtk.svtkRenderer()
ren2 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren0)
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create a synthetic source: sample a sphere across a volume
sphere = svtk.svtkSphere()
sphere.SetCenter( 0.0,0.0,0.0)
sphere.SetRadius(0.25)

sample = svtk.svtkSampleFunction()
sample.SetImplicitFunction(sphere)
sample.SetModelBounds(-0.5,0.5, -0.5,0.5, -0.5,0.5)
sample.SetSampleDimensions(res,res,res)
sample.ComputeNormalsOff()
sample.Update()

# Adds random attributes
random = svtk.svtkRandomAttributeGenerator()
random.SetGenerateCellScalars(True)
random.SetInputConnection(sample.GetOutputPort())

# Convert the image data to unstructured grid
extractionSphere = svtk.svtkSphere()
extractionSphere.SetRadius(100)
extractionSphere.SetCenter(0,0,0)
extract = svtk.svtkExtractGeometry()
extract.SetImplicitFunction(extractionSphere)
extract.SetInputConnection(random.GetOutputPort())
extract.Update()

# The cut plane
plane = svtk.svtkPlane()
plane.SetOrigin(0,0,0)
plane.SetNormal(1,1,1)

# Now create the usual extractor
cutter = svtk.svtkExtractGeometry()
cutter.SetInputConnection(extract.GetOutputPort())
cutter.SetImplicitFunction(plane)
cutter.ExtractBoundaryCellsOn()
cutter.ExtractOnlyBoundaryCellsOn()

cutterSurface = svtk.svtkGeometryFilter()
cutterSurface.SetInputConnection(cutter.GetOutputPort())
cutterSurface.MergingOff()

cutterMapper = svtk.svtkPolyDataMapper()
cutterMapper.SetInputConnection(cutterSurface.GetOutputPort())

cutterActor = svtk.svtkActor()
cutterActor.SetMapper(cutterMapper)
cutterActor.GetProperty().SetColor(1,1,1)

# Throw in an outline
outline = svtk.svtkOutlineFilter()
outline.SetInputConnection(sample.GetOutputPort())

outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)

# Now create the faster crinkle cutter - cell data
pcut = svtk.svtk3DLinearGridCrinkleExtractor()
pcut.SetInputConnection(extract.GetOutputPort())
pcut.SetImplicitFunction(plane)
pcut.CopyPointDataOff()
pcut.CopyCellDataOn()

pCutterSurface = svtk.svtkGeometryFilter()
pCutterSurface.SetInputConnection(pcut.GetOutputPort())
pCutterSurface.MergingOff()

pCutterMapper = svtk.svtkPolyDataMapper()
pCutterMapper.SetInputConnection(pCutterSurface.GetOutputPort())

pCutterActor = svtk.svtkActor()
pCutterActor.SetMapper(pCutterMapper)
pCutterActor.GetProperty().SetColor(1,1,1)

# Throw in an outline
pOutline = svtk.svtkOutlineFilter()
pOutline.SetInputConnection(sample.GetOutputPort())

pOutlineMapper = svtk.svtkPolyDataMapper()
pOutlineMapper.SetInputConnection(pOutline.GetOutputPort())

pOutlineActor = svtk.svtkActor()
pOutlineActor.SetMapper(pOutlineMapper)

# Now create the crinkle cutter - remove unused points + point data
pcut2 = svtk.svtk3DLinearGridCrinkleExtractor()
pcut2.SetInputConnection(extract.GetOutputPort())
pcut2.SetImplicitFunction(plane)
pcut2.RemoveUnusedPointsOn()
pcut2.CopyPointDataOn()
pcut2.CopyCellDataOn()

pCutterSurface2 = svtk.svtkGeometryFilter()
pCutterSurface2.SetInputConnection(pcut2.GetOutputPort())
pCutterSurface2.MergingOff()

pCutterMapper2 = svtk.svtkPolyDataMapper()
pCutterMapper2.SetInputConnection(pCutterSurface2.GetOutputPort())

pCutterActor2 = svtk.svtkActor()
pCutterActor2.SetMapper(pCutterMapper2)
pCutterActor2.GetProperty().SetColor(1,1,1)

# Throw in an outline
pOutline2 = svtk.svtkOutlineFilter()
pOutline2.SetInputConnection(sample.GetOutputPort())

pOutlineMapper2 = svtk.svtkPolyDataMapper()
pOutlineMapper2.SetInputConnection(pOutline2.GetOutputPort())

pOutlineActor2 = svtk.svtkActor()
pOutlineActor2.SetMapper(pOutlineMapper2)

# Time the execution of the usual crinkle cutter
cutter_timer = svtk.svtkExecutionTimer()
cutter_timer.SetFilter(cutter)
cutter.Update()
CT = cutter_timer.GetElapsedWallClockTime()
print ("Standard Crinkle (svtkExtractGeometry):", CT)

# Time the execution of the faster crinkle cutter
cutter_timer.SetFilter(pcut)
pcut.Update()
CT = cutter_timer.GetElapsedWallClockTime()
print ("svtk3DLinearGridCrinkleExtractor:", CT)
print("\tNumber of threads used: {0}".format(pcut.GetNumberOfThreadsUsed()))

# Time the execution of the faster crinkle cutter
cutter_timer.SetFilter(pcut2)
pcut2.Update()
CT = cutter_timer.GetElapsedWallClockTime()
print ("svtk3DLinearGridCrinkleExtractor (unused points removed):", CT)
print("\tNumber of threads used: {0}".format(pcut.GetNumberOfThreadsUsed()))

# Add the actors to the renderer, set the background and size
ren0.AddActor(outlineActor)
ren0.AddActor(cutterActor)
ren1.AddActor(pOutlineActor)
ren1.AddActor(pCutterActor)
ren2.AddActor(pOutlineActor2)
ren2.AddActor(pCutterActor2)

ren0.SetBackground(0,0,0)
ren1.SetBackground(0,0,0)
ren2.SetBackground(0,0,0)

ren0.SetViewport(0,0,0.333,1);
ren1.SetViewport(0.333,0,0.667,1);
ren2.SetViewport(0.667,0,1,1);
renWin.SetSize(600,200)
ren0.ResetCamera()

cam = ren0.GetActiveCamera()
ren1.SetActiveCamera(cam)
ren2.SetActiveCamera(cam)
iren.Initialize()

renWin.Render()
iren.Start()
# --- end of script --
