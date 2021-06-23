#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

res = 50

# Create the RenderWindow, Renderers and both Actors
ren0 = svtk.svtkRenderer()
ren1 = svtk.svtkRenderer()
ren2 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.SetMultiSamples(0)
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

# Now create the usual cutter
cutter = svtk.svtkCutter()
cutter.SetInputConnection(extract.GetOutputPort())
cutter.SetCutFunction(plane)

cutterMapper = svtk.svtkPolyDataMapper()
cutterMapper.SetInputConnection(cutter.GetOutputPort())
cutterMapper.ScalarVisibilityOff()

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

# Accelerated cutter
cut = svtk.svtkPlaneCutter()
cut.SetInputConnection(extract.GetOutputPort())
cut.SetPlane(plane)
cut.ComputeNormalsOff()

sCutterMapper = svtk.svtkCompositePolyDataMapper()
sCutterMapper.SetInputConnection(cut.GetOutputPort())
sCutterMapper.ScalarVisibilityOff()

sCutterActor = svtk.svtkActor()
sCutterActor.SetMapper(sCutterMapper)
sCutterActor.GetProperty().SetColor(1,1,1)

# Accelerated cutter without tree
ncut = svtk.svtkPlaneCutter()
ncut.SetInputConnection(extract.GetOutputPort())
ncut.SetPlane(plane)
ncut.ComputeNormalsOff()
ncut.BuildTreeOff()

snCutterMapper = svtk.svtkCompositePolyDataMapper()
snCutterMapper.SetInputConnection(ncut.GetOutputPort())
snCutterMapper.ScalarVisibilityOff()

snCutterActor = svtk.svtkActor()
snCutterActor.SetMapper(snCutterMapper)
snCutterActor.GetProperty().SetColor(1,1,1)

outlineT = svtk.svtkOutlineFilter()
outlineT.SetInputConnection(sample.GetOutputPort())

outlineMapperT = svtk.svtkPolyDataMapper()
outlineMapperT.SetInputConnection(outlineT.GetOutputPort())

outlineActorT = svtk.svtkActor()
outlineActorT.SetMapper(outlineMapperT)

# Time the execution of the usual cutter
cutter_timer = svtk.svtkExecutionTimer()
cutter_timer.SetFilter(cutter)
cutter.Update()
CT = cutter_timer.GetElapsedWallClockTime()
print ("svtkCutter:", CT)

# Time the execution of the filter w/ sphere tree
sCutter_timer = svtk.svtkExecutionTimer()
sCutter_timer.SetFilter(cut)
cut.Update()
ST = sCutter_timer.GetElapsedWallClockTime()
print ("Build sphere tree + execute once:", ST)

# Time subsequent cuts
sCutter_timer.SetFilter(cut)
plane.Modified()
cut.Update()
SC = sCutter_timer.GetElapsedWallClockTime()
print ("svtkPlaneCutter:", SC)

# Add the actors to the renderer, set the background and size
ren0.AddActor(outlineActor)
ren0.AddActor(cutterActor)
ren1.AddActor(outlineActorT)
ren1.AddActor(sCutterActor)
ren2.AddActor(outlineActorT)
ren2.AddActor(snCutterActor)

ren0.SetBackground(0,0,0)
ren1.SetBackground(0,0,0)
ren2.SetBackground(0,0,0)
ren0.SetViewport(0,0,0.33,1);
ren1.SetViewport(0.33,0,0.66,1);
ren2.SetViewport(0.66,0,1,1);
renWin.SetSize(900,300)
ren0.ResetCamera()
ren1.ResetCamera()
ren2.ResetCamera()
iren.Initialize()

renWin.Render()
iren.Start()
# --- end of script --
