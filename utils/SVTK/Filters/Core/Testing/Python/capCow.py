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
# Demonstrate the use of clipping and capping on polyhedral data. Also shows how to
# use triangle filter to triangulate loops.
#

# create pipeline
#
# Read the polygonal data and generate vertex normals
cow = svtk.svtkBYUReader()
cow.SetGeometryFileName(SVTK_DATA_ROOT + "/Data/Viewpoint/cow.g")
cowNormals = svtk.svtkPolyDataNormals()
cowNormals.SetInputConnection(cow.GetOutputPort())

# Define a clip plane to clip the cow in half
plane = svtk.svtkPlane()
plane.SetOrigin(0.25, 0, 0)
plane.SetNormal(-1, -1, 0)

clipper = svtk.svtkClipPolyData()
clipper.SetInputConnection(cowNormals.GetOutputPort())
clipper.SetClipFunction(plane)
clipper.GenerateClipScalarsOn()
clipper.GenerateClippedOutputOn()
clipper.SetValue(0.5)

clipMapper = svtk.svtkPolyDataMapper()
clipMapper.SetInputConnection(clipper.GetOutputPort())
clipMapper.ScalarVisibilityOff()

backProp = svtk.svtkProperty()
backProp.SetDiffuseColor(GetRGBColor('tomato'))

clipActor = svtk.svtkActor()
clipActor.SetMapper(clipMapper)
clipActor.GetProperty().SetColor(GetRGBColor('peacock'))
clipActor.SetBackfaceProperty(backProp)

# Create polygons outlining clipped areas and triangulate them to generate cut surface
cutEdges = svtk.svtkCutter()
# Generate cut lines
cutEdges.SetInputConnection(cowNormals.GetOutputPort())
cutEdges.SetCutFunction(plane)
cutEdges.GenerateCutScalarsOn()
cutEdges.SetValue(0, 0.5)
cutStrips = svtk.svtkStripper()

# Forms loops (closed polylines) from cutter
cutStrips.SetInputConnection(cutEdges.GetOutputPort())
cutStrips.Update()

cutPoly = svtk.svtkPolyData()
# This trick defines polygons as polyline loop
cutPoly.SetPoints(cutStrips.GetOutput().GetPoints())
cutPoly.SetPolys(cutStrips.GetOutput().GetLines())

cutTriangles = svtk.svtkTriangleFilter()
# Triangulates the polygons to create cut surface
cutTriangles.SetInputData(cutPoly)
cutMapper = svtk.svtkPolyDataMapper()
cutMapper.SetInputData(cutPoly)
cutMapper.SetInputConnection(cutTriangles.GetOutputPort())

cutActor = svtk.svtkActor()
cutActor.SetMapper(cutMapper)
cutActor.GetProperty().SetColor(GetRGBColor('peacock'))

# Create the rest of the cow in wireframe
restMapper = svtk.svtkPolyDataMapper()
restMapper.SetInputData(clipper.GetClippedOutput())
restMapper.ScalarVisibilityOff()

restActor = svtk.svtkActor()
restActor.SetMapper(restMapper)
restActor.GetProperty().SetRepresentationToWireframe()

# Create graphics stuff
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren1.AddActor(clipActor)
ren1.AddActor(cutActor)
ren1.AddActor(restActor)
ren1.SetBackground(1, 1, 1)
ren1.ResetCamera()
ren1.GetActiveCamera().Azimuth(30)
ren1.GetActiveCamera().Elevation(30)
ren1.GetActiveCamera().Dolly(1.5)
ren1.ResetCameraClippingRange()

renWin.SetSize(300, 300)

iren.Initialize()

# render the image

# Lets you move the cut plane back and forth
def Cut (v):
    clipper.SetValue(v)
    cutEdges.SetValue(0, v)
    cutStrips.Update()
    cutPoly.SetPoints(cutStrips.GetOutput().GetPoints())
    cutPoly.SetPolys(cutStrips.GetOutput().GetLines())
    cutMapper.Update()
    renWin.Render()

# iren.Start()
