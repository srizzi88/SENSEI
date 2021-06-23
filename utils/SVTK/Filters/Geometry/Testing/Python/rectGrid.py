#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

def GetRGBColor(colorName):
    '''
        Return the red, green and blue components for a
        color as doubles.
    '''
    rgb = [0.0, 0.0, 0.0]  # black
    svtk.svtkNamedColors().GetColorRGB(colorName, rgb)
    return rgb

SVTK_VARY_RADIUS_BY_VECTOR = 2

# create pipeline
#
reader = svtk.svtkDataSetReader()
reader.SetFileName(SVTK_DATA_ROOT + "/Data/RectGrid2.svtk")
reader.Update()

toRectilinearGrid = svtk.svtkCastToConcrete()
toRectilinearGrid.SetInputConnection(reader.GetOutputPort())
toRectilinearGrid.Update()
plane = svtk.svtkRectilinearGridGeometryFilter()
plane.SetInputData(toRectilinearGrid.GetRectilinearGridOutput())
plane.SetExtent(0, 100, 0, 100, 15, 15)
warper = svtk.svtkWarpVector()
warper.SetInputConnection(plane.GetOutputPort())
warper.SetScaleFactor(0.05)
planeMapper = svtk.svtkDataSetMapper()
planeMapper.SetInputConnection(warper.GetOutputPort())
planeMapper.SetScalarRange(0.197813, 0.710419)
planeActor = svtk.svtkActor()
planeActor.SetMapper(planeMapper)

cutPlane = svtk.svtkPlane()
cutPlane.SetOrigin(reader.GetOutput().GetCenter())
cutPlane.SetNormal(1, 0, 0)
planeCut = svtk.svtkCutter()
planeCut.SetInputData(toRectilinearGrid.GetRectilinearGridOutput())
planeCut.SetCutFunction(cutPlane)
cutMapper = svtk.svtkDataSetMapper()
cutMapper.SetInputConnection(planeCut.GetOutputPort())
cutMapper.SetScalarRange(
  reader.GetOutput().GetPointData().GetScalars().GetRange())
cutActor = svtk.svtkActor()
cutActor.SetMapper(cutMapper)

iso = svtk.svtkContourFilter()
iso.SetInputData(toRectilinearGrid.GetRectilinearGridOutput())
iso.SetValue(0, 0.7)
normals = svtk.svtkPolyDataNormals()
normals.SetInputConnection(iso.GetOutputPort())
normals.SetFeatureAngle(45)
isoMapper = svtk.svtkPolyDataMapper()
isoMapper.SetInputConnection(normals.GetOutputPort())
isoMapper.ScalarVisibilityOff()
isoActor = svtk.svtkActor()
isoActor.SetMapper(isoMapper)
isoActor.GetProperty().SetColor(GetRGBColor('bisque'))
isoActor.GetProperty().SetRepresentationToWireframe()

streamer = svtk.svtkStreamTracer()
streamer.SetInputConnection(reader.GetOutputPort())
streamer.SetStartPosition(-1.2, -0.1, 1.3)
streamer.SetMaximumPropagation(500)
streamer.SetInitialIntegrationStep(0.05)
streamer.SetIntegrationDirectionToBoth()

streamTube = svtk.svtkTubeFilter()
streamTube.SetInputConnection(streamer.GetOutputPort())
streamTube.SetRadius(0.025)
streamTube.SetNumberOfSides(6)
streamTube.SetVaryRadius(SVTK_VARY_RADIUS_BY_VECTOR)
mapStreamTube = svtk.svtkPolyDataMapper()
mapStreamTube.SetInputConnection(streamTube.GetOutputPort())
mapStreamTube.SetScalarRange(
  reader.GetOutput().GetPointData().GetScalars().GetRange())
streamTubeActor = svtk.svtkActor()
streamTubeActor.SetMapper(mapStreamTube)
streamTubeActor.GetProperty().BackfaceCullingOn()

outline = svtk.svtkOutlineFilter()
outline.SetInputData(toRectilinearGrid.GetRectilinearGridOutput())
outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(GetRGBColor('black'))

# Graphics stuff
# Create the RenderWindow, Renderer and both Actors
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(outlineActor)
ren1.AddActor(planeActor)
ren1.AddActor(cutActor)
ren1.AddActor(isoActor)
ren1.AddActor(streamTubeActor)
ren1.SetBackground(1, 1, 1)

renWin.SetSize(400, 400)

cam1 = ren1.GetActiveCamera()
cam1.SetClippingRange(3.76213, 10.712)
cam1.SetFocalPoint(-0.0842503, -0.136905, 0.610234)
cam1.SetPosition(2.53813, 2.2678, -5.22172)
cam1.SetViewUp(-0.241047, 0.930635, 0.275343)

iren.Initialize()
#iren.Start()
