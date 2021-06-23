#!/usr/bin/env python

import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
#
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# cut data
pl3d = svtk.svtkMultiBlockPLOT3DReader()
pl3d.SetXYZFileName(SVTK_DATA_ROOT + "/Data/combxyz.bin")
pl3d.SetQFileName(SVTK_DATA_ROOT + "/Data/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()
output = pl3d.GetOutput().GetBlock(0)

plane = svtk.svtkPlane()
plane.SetOrigin(output.GetCenter())
plane.SetNormal(-0.287, 0, 0.9579)

planeCut = svtk.svtkCutter()
planeCut.SetInputData(output)
planeCut.SetCutFunction(plane)

probe = svtk.svtkProbeFilter()
probe.SetInputConnection(planeCut.GetOutputPort())
probe.SetSourceData(output)

cutMapper = svtk.svtkDataSetMapper()
cutMapper.SetInputConnection(probe.GetOutputPort())
cutMapper.SetScalarRange(output.GetPointData().GetScalars().GetRange())

cutActor = svtk.svtkActor()
cutActor.SetMapper(cutMapper)

#extract plane
compPlane = svtk.svtkStructuredGridGeometryFilter()
compPlane.SetInputData(output)
compPlane.SetExtent(0, 100, 0, 100, 9, 9)

planeMapper = svtk.svtkPolyDataMapper()
planeMapper.SetInputConnection(compPlane.GetOutputPort())
planeMapper.ScalarVisibilityOff()

planeActor = svtk.svtkActor()
planeActor.SetMapper(planeMapper)
planeActor.GetProperty().SetRepresentationToWireframe()
planeActor.GetProperty().SetColor(0, 0, 0)

#outline
outline = svtk.svtkStructuredGridOutlineFilter()
outline.SetInputData(output)

outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)

outlineProp = outlineActor.GetProperty()
outlineProp.SetColor(0, 0, 0)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(outlineActor)
ren.AddActor(planeActor)
ren.AddActor(cutActor)
ren.SetBackground(1, 1, 1)
renWin.SetSize(400, 300)

cam1 = ren.GetActiveCamera()
cam1.SetClippingRange(11.1034, 59.5328)
cam1.SetFocalPoint(9.71821, 0.458166, 29.3999)
cam1.SetPosition(-2.95748, -26.7271, 44.5309)
cam1.SetViewUp(0.0184785, 0.479657, 0.877262)

iren.Initialize()
renWin.Render()
iren.Start()
