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

#
# Demonstrate the use of clipping and capping on polyhedral data
#

# create a sphere and clip it
#
sphere = svtk.svtkSphereSource()
sphere.SetRadius(1)
sphere.SetPhiResolution(10)
sphere.SetThetaResolution(10)

plane = svtk.svtkPlane()
plane.SetOrigin(0, 0, 0)
plane.SetNormal(-1, -1, 0)

clipper = svtk.svtkClipPolyData()
clipper.SetInputConnection(sphere.GetOutputPort())
clipper.SetClipFunction(plane)
clipper.GenerateClipScalarsOn()
clipper.GenerateClippedOutputOn()
clipper.SetValue(0)

clipMapper = svtk.svtkPolyDataMapper()
clipMapper.SetInputConnection(clipper.GetOutputPort())
clipMapper.ScalarVisibilityOff()

backProp = svtk.svtkProperty()
backProp.SetDiffuseColor(GetRGBColor('tomato'))

clipActor = svtk.svtkActor()
clipActor.SetMapper(clipMapper)
clipActor.GetProperty().SetColor(GetRGBColor('peacock'))
clipActor.SetBackfaceProperty(backProp)

# now extract feature edges
boundaryEdges = svtk.svtkFeatureEdges()
boundaryEdges.SetInputConnection(clipper.GetOutputPort())
boundaryEdges.BoundaryEdgesOn()
boundaryEdges.FeatureEdgesOff()
boundaryEdges.NonManifoldEdgesOff()

boundaryClean = svtk.svtkCleanPolyData()
boundaryClean.SetInputConnection(boundaryEdges.GetOutputPort())

boundaryStrips = svtk.svtkStripper()
boundaryStrips.SetInputConnection(boundaryClean.GetOutputPort())
boundaryStrips.Update()

boundaryPoly = svtk.svtkPolyData()
boundaryPoly.SetPoints(boundaryStrips.GetOutput().GetPoints())
boundaryPoly.SetPolys(boundaryStrips.GetOutput().GetLines())

boundaryTriangles = svtk.svtkTriangleFilter()
boundaryTriangles.SetInputData(boundaryPoly)

boundaryMapper = svtk.svtkPolyDataMapper()
boundaryMapper.SetInputConnection(boundaryTriangles.GetOutputPort())

boundaryActor = svtk.svtkActor()
boundaryActor.SetMapper(boundaryMapper)
boundaryActor.GetProperty().SetColor(GetRGBColor('banana'))

# Create graphics stuff
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(clipActor)
ren1.AddActor(boundaryActor)
ren1.SetBackground(1, 1, 1)
ren1.ResetCamera()
ren1.GetActiveCamera().Azimuth(30)
ren1.GetActiveCamera().Elevation(30)
ren1.GetActiveCamera().Dolly(1.2)
ren1.ResetCameraClippingRange()

renWin.SetSize(300, 300)

iren.Initialize()
#iren.Start()
