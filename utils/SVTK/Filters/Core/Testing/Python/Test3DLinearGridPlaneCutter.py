
#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Control test size
res = 50
#res = 200

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

# Now create the usual cutter - without a tree
cutter = svtk.svtkCutter()
cutter.SetInputConnection(extract.GetOutputPort())
cutter.SetCutFunction(plane)

cutterMapper = svtk.svtkCompositePolyDataMapper()
cutterMapper.SetInputConnection(cutter.GetOutputPort())

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

# Now create the faster plane cutter
pcut = svtk.svtkPlaneCutter()
pcut.SetInputConnection(extract.GetOutputPort())
pcut.SetPlane(plane)
pcut.BuildTreeOff()

pCutterMapper = svtk.svtkCompositePolyDataMapper()
pCutterMapper.SetInputConnection(pcut.GetOutputPort())

pCutterActor = svtk.svtkActor()
pCutterActor.SetMapper(pCutterMapper)
pCutterActor.GetProperty().SetColor(1,1,1)

# Throw in an outline
pOutline = svtk.svtkOutlineFilter()
pOutline.SetInputConnection(sample.GetOutputPort())

pOutlineMapper = svtk.svtkPolyDataMapper()
pOutlineMapper.SetInputConnection(outline.GetOutputPort())

pOutlineActor = svtk.svtkActor()
pOutlineActor.SetMapper(pOutlineMapper)

# Specialized cutter for 3D linear grids
scut = svtk.svtk3DLinearGridPlaneCutter()
scut.SetInputConnection(extract.GetOutputPort())
scut.SetPlane(plane)
scut.ComputeNormalsOff()
scut.MergePointsOff()
scut.InterpolateAttributesOn()

sCutterMapper = svtk.svtkPolyDataMapper()
sCutterMapper.SetInputConnection(scut.GetOutputPort())

sCutterActor = svtk.svtkActor()
sCutterActor.SetMapper(sCutterMapper)
sCutterActor.GetProperty().SetColor(1,1,1)

# Outline
sOutline = svtk.svtkOutlineFilter()
sOutline.SetInputConnection(sample.GetOutputPort())

sOutlineMapper = svtk.svtkPolyDataMapper()
sOutlineMapper.SetInputConnection(sOutline.GetOutputPort())

sOutlineActor = svtk.svtkActor()
sOutlineActor.SetMapper(sOutlineMapper)

# Time the execution of the usual cutter
cutter_timer = svtk.svtkExecutionTimer()
cutter_timer.SetFilter(cutter)
cutter.Update()
CT = cutter_timer.GetElapsedWallClockTime()
print ("svtkCutter:", CT)

# Time the execution of the usual cutter
cutter_timer.SetFilter(pcut)
pcut.Update()
CT = cutter_timer.GetElapsedWallClockTime()
print ("svtkPlaneCutter:", CT)

# Time the execution of the filter w/o sphere tree
cutter_timer.SetFilter(scut)
scut.Update()
SC = cutter_timer.GetElapsedWallClockTime()
print ("svtk3DLinearGridPlaneCutter: ", SC)

# Add the actors to the renderer, set the background and size
ren0.AddActor(outlineActor)
ren0.AddActor(cutterActor)
ren1.AddActor(pOutlineActor)
ren1.AddActor(pCutterActor)
ren2.AddActor(sOutlineActor)
ren2.AddActor(sCutterActor)

ren0.SetBackground(0,0,0)
ren1.SetBackground(0,0,0)
ren2.SetBackground(0,0,0)
ren0.SetViewport(0,0,0.33,1);
ren1.SetViewport(0.33,0,0.67,1);
ren2.SetViewport(0.67,0,1,1);
renWin.SetSize(400,200)
ren0.ResetCamera()

cam = ren0.GetActiveCamera()
ren1.SetActiveCamera(cam)
ren2.SetActiveCamera(cam)
iren.Initialize()

renWin.Render()
iren.Start()
# --- end of script --
