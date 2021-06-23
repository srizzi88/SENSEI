#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Control debugging parameters
res = 25

# Create the RenderWindow, Renderer
#
ren0 = svtk.svtkRenderer()
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren0)
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create a synthetic source: sample a sphere across a volume
sphere = svtk.svtkSphere()
sphere.SetCenter(0.0,0.0,0.0)
sphere.SetRadius(0.25)

sample = svtk.svtkSampleFunction()
sample.SetImplicitFunction(sphere)
sample.SetModelBounds(-0.5,0.5, -0.5,0.5, -0.5,0.5)
sample.SetSampleDimensions(res,res,res)

# Handy dandy filter converts image data to structured grid
convert = svtk.svtkImageDataToPointSet()
convert.SetInputConnection(sample.GetOutputPort())

# Create a sphere tree and see what it look like
# (structured sphere tree)
stf = svtk.svtkSphereTreeFilter()
stf.SetInputConnection(convert.GetOutputPort())
stf.SetLevel(0);

sph = svtk.svtkSphereSource()
sph.SetPhiResolution(8)
sph.SetThetaResolution(16)
sph.SetRadius(1)

stfGlyphs = svtk.svtkGlyph3D()
stfGlyphs.SetInputConnection(stf.GetOutputPort())
stfGlyphs.SetSourceConnection(sph.GetOutputPort())

stfMapper = svtk.svtkPolyDataMapper()
stfMapper.SetInputConnection(stfGlyphs.GetOutputPort())
stfMapper.ScalarVisibilityOff()

stfActor = svtk.svtkActor()
stfActor.SetMapper(stfMapper)
stfActor.GetProperty().SetColor(1,1,1)

# Throw in an outline
outline = svtk.svtkOutlineFilter()
outline.SetInputConnection(sample.GetOutputPort())

outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)

# Convert the image data to unstructured grid
extractionSphere = svtk.svtkSphere()
extractionSphere.SetRadius(100)
extractionSphere.SetCenter(0,0,0)

extract = svtk.svtkExtractGeometry()
extract.SetImplicitFunction(extractionSphere)
extract.SetInputConnection(sample.GetOutputPort())
extract.Update()

# This time around create a sphere tree, assign it to the filter, and see
# what it look like (unstructured sphere tree)
ust = svtk.svtkSphereTree()
ust.BuildHierarchyOn()
ust.Build(extract.GetOutput())
print (ust)

ustf = svtk.svtkSphereTreeFilter()
ustf.SetSphereTree(ust)
ustf.SetLevel(0);

ustfGlyphs = svtk.svtkGlyph3D()
ustfGlyphs.SetInputConnection(ustf.GetOutputPort())
ustfGlyphs.SetSourceConnection(sph.GetOutputPort())

ustfMapper = svtk.svtkPolyDataMapper()
ustfMapper.SetInputConnection(ustfGlyphs.GetOutputPort())
ustfMapper.ScalarVisibilityOff()

ustfActor = svtk.svtkActor()
ustfActor.SetMapper(ustfMapper)
ustfActor.GetProperty().SetColor(1,1,1)

# Throw in an outline
uOutline = svtk.svtkOutlineFilter()
uOutline.SetInputConnection(sample.GetOutputPort())

uOutlineMapper = svtk.svtkPolyDataMapper()
uOutlineMapper.SetInputConnection(uOutline.GetOutputPort())

uOutlineActor = svtk.svtkActor()
uOutlineActor.SetMapper(uOutlineMapper)


# Add the actors to the renderer, set the background and size
#
ren0.AddActor(stfActor)
ren0.AddActor(outlineActor)
ren1.AddActor(ustfActor)
ren1.AddActor(uOutlineActor)

ren0.SetBackground(0,0,0)
ren1.SetBackground(0,0,0)
ren0.SetViewport(0,0,0.5,1);
ren1.SetViewport(0.5,0,1,1);
renWin.SetSize(600,300)
ren0.ResetCamera()
ren1.ResetCamera()
iren.Initialize()

renWin.Render()
#iren.Start()
# --- end of script --
