#!/usr/bin/env python

# Perform pseudo volume rendering in a structured grid by compositing
# translucent cut planes. This same trick can be used for unstructured
# grids. Note that for better results, more planes can be created. Also,
# if your data is svtkImageData, there are much faster methods for volume
# rendering.

import svtk
from svtk.util.colors import *
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Create pipeline. Read structured grid data.
pl3d = svtk.svtkMultiBlockPLOT3DReader()
pl3d.SetXYZFileName(SVTK_DATA_ROOT + "/Data/combxyz.bin")
pl3d.SetQFileName(SVTK_DATA_ROOT + "/Data/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()

pl3d_output = pl3d.GetOutput().GetBlock(0)

# A convenience, use this filter to limit data for experimentation.
extract = svtk.svtkExtractGrid()
extract.SetVOI(1, 55, -1000, 1000, -1000, 1000)
extract.SetInputData(pl3d_output)

# The (implicit) plane is used to do the cutting
plane = svtk.svtkPlane()
plane.SetOrigin(0, 4, 2)
plane.SetNormal(0, 1, 0)

# The cutter is set up to process each contour value over all cells
# (SetSortByToSortByCell). This results in an ordered output of polygons
# which is key to the compositing.
cutter = svtk.svtkCutter()
cutter.SetInputConnection(extract.GetOutputPort())
cutter.SetCutFunction(plane)
cutter.GenerateCutScalarsOff()
cutter.SetSortByToSortByCell()

clut = svtk.svtkLookupTable()
clut.SetHueRange(0, .67)
clut.Build()

cutterMapper = svtk.svtkPolyDataMapper()
cutterMapper.SetInputConnection(cutter.GetOutputPort())
cutterMapper.SetScalarRange(.18, .7)
cutterMapper.SetLookupTable(clut)

cut = svtk.svtkActor()
cut.SetMapper(cutterMapper)

# Add in some surface geometry for interest.
iso = svtk.svtkContourFilter()
iso.SetInputData(pl3d_output)
iso.SetValue(0, .22)
normals = svtk.svtkPolyDataNormals()
normals.SetInputConnection(iso.GetOutputPort())
normals.SetFeatureAngle(45)
isoMapper = svtk.svtkPolyDataMapper()
isoMapper.SetInputConnection(normals.GetOutputPort())
isoMapper.ScalarVisibilityOff()
isoActor = svtk.svtkActor()
isoActor.SetMapper(isoMapper)
isoActor.GetProperty().SetDiffuseColor(tomato)
isoActor.GetProperty().SetSpecularColor(white)
isoActor.GetProperty().SetDiffuse(.8)
isoActor.GetProperty().SetSpecular(.5)
isoActor.GetProperty().SetSpecularPower(30)

outline = svtk.svtkStructuredGridOutlineFilter()
outline.SetInputData(pl3d_output)
outlineTubes = svtk.svtkTubeFilter()
outlineTubes.SetInputConnection(outline.GetOutputPort())
outlineTubes.SetRadius(.1)

outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outlineTubes.GetOutputPort())
outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)

# Create the RenderWindow, Renderer and Interactor
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(outlineActor)
outlineActor.GetProperty().SetColor(banana)
ren.AddActor(isoActor)
isoActor.VisibilityOn()
ren.AddActor(cut)
opacity = .1
cut.GetProperty().SetOpacity(1)
ren.SetBackground(1, 1, 1)
renWin.SetSize(640, 480)

cam1 = ren.GetActiveCamera()
cam1.SetClippingRange(3.95297, 50)
cam1.SetFocalPoint(9.71821, 0.458166, 29.3999)
cam1.SetPosition(2.7439, -37.3196, 38.7167)
cam1.ComputeViewPlaneNormal()
cam1.SetViewUp(-0.16123, 0.264271, 0.950876)

# Cut: generates n cut planes normal to camera's view plane
def Cut(n):
    global cam1, opacity
    plane.SetNormal(cam1.GetViewPlaneNormal())
    plane.SetOrigin(cam1.GetFocalPoint())
    cutter.GenerateValues(n, -5, 5)
    clut.SetAlphaRange(opacity, opacity)
    renWin.Render()


# Generate 10 cut planes
Cut(20)

iren.Initialize()
renWin.Render()
iren.Start()
