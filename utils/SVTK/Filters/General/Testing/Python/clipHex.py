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

# define a Single Cube
Scalars = svtk.svtkFloatArray()
Scalars.InsertNextValue(1.0)
Scalars.InsertNextValue(1.0)
Scalars.InsertNextValue(0.0)
Scalars.InsertNextValue(0.0)
Scalars.InsertNextValue(0.0)
Scalars.InsertNextValue(0.0)
Scalars.InsertNextValue(0.0)
Scalars.InsertNextValue(0.0)

Points = svtk.svtkPoints()
Points.InsertNextPoint(0, 0, 0)
Points.InsertNextPoint(1, 0, 0)
Points.InsertNextPoint(1, 1, 0)
Points.InsertNextPoint(0, 1, 0)
Points.InsertNextPoint(0, 0, 1)
Points.InsertNextPoint(1, 0, 1)
Points.InsertNextPoint(1, 1, 1)
Points.InsertNextPoint(0, 1, 1)

Ids = svtk.svtkIdList()
Ids.InsertNextId(0)
Ids.InsertNextId(1)
Ids.InsertNextId(2)
Ids.InsertNextId(3)
Ids.InsertNextId(4)
Ids.InsertNextId(5)
Ids.InsertNextId(6)
Ids.InsertNextId(7)

Grid = svtk.svtkUnstructuredGrid()
Grid.Allocate(10, 10)
Grid.InsertNextCell(12, Ids)
Grid.SetPoints(Points)
Grid.GetPointData().SetScalars(Scalars)

# Clip the hex
clipper = svtk.svtkClipDataSet()
clipper.SetInputData(Grid)
clipper.SetValue(0.5)

# build tubes for the triangle edges
#
tetEdges = svtk.svtkExtractEdges()
tetEdges.SetInputConnection(clipper.GetOutputPort())

tetEdgeTubes = svtk.svtkTubeFilter()
tetEdgeTubes.SetInputConnection(tetEdges.GetOutputPort())
tetEdgeTubes.SetRadius(.005)
tetEdgeTubes.SetNumberOfSides(6)
tetEdgeTubes.UseDefaultNormalOn()
tetEdgeTubes.SetDefaultNormal(.577, .577, .577)

tetEdgeMapper = svtk.svtkPolyDataMapper()
tetEdgeMapper.SetInputConnection(tetEdgeTubes.GetOutputPort())
tetEdgeMapper.ScalarVisibilityOff()

tetEdgeActor = svtk.svtkActor()
tetEdgeActor.SetMapper(tetEdgeMapper)
tetEdgeActor.GetProperty().SetDiffuseColor(GetRGBColor('lamp_black'))
tetEdgeActor.GetProperty().SetSpecular(.4)
tetEdgeActor.GetProperty().SetSpecularPower(10)

# shrink the triangles so we can see each one
aShrinker = svtk.svtkShrinkFilter()
aShrinker.SetShrinkFactor(1)
aShrinker.SetInputConnection(clipper.GetOutputPort())

aMapper = svtk.svtkDataSetMapper()
aMapper.ScalarVisibilityOff()
aMapper.SetInputConnection(aShrinker.GetOutputPort())

Tets = svtk.svtkActor()
Tets.SetMapper(aMapper)
Tets.GetProperty().SetDiffuseColor(GetRGBColor('banana'))

# build a model of the cube
CubeModel = svtk.svtkCubeSource()
CubeModel.SetCenter(.5, .5, .5)

Edges = svtk.svtkExtractEdges()
Edges.SetInputConnection(CubeModel.GetOutputPort())

Tubes = svtk.svtkTubeFilter()
Tubes.SetInputConnection(Edges.GetOutputPort())
Tubes.SetRadius(.01)
Tubes.SetNumberOfSides(6)
Tubes.UseDefaultNormalOn()
Tubes.SetDefaultNormal(.577, .577, .577)

TubeMapper = svtk.svtkPolyDataMapper()
TubeMapper.SetInputConnection(Tubes.GetOutputPort())

CubeEdges = svtk.svtkActor()
CubeEdges.SetMapper(TubeMapper)
CubeEdges.GetProperty().SetDiffuseColor(GetRGBColor('khaki'))
CubeEdges.GetProperty().SetSpecular(.4)
CubeEdges.GetProperty().SetSpecularPower(10)

# build the vertices of the cube
#
Sphere = svtk.svtkSphereSource()
Sphere.SetRadius(0.04)
Sphere.SetPhiResolution(20)
Sphere.SetThetaResolution(20)

ThresholdIn = svtk.svtkThresholdPoints()
ThresholdIn.SetInputData(Grid)
ThresholdIn.ThresholdByUpper(.5)

Vertices = svtk.svtkGlyph3D()
Vertices.SetInputConnection(ThresholdIn.GetOutputPort())
Vertices.SetSourceConnection(Sphere.GetOutputPort())

SphereMapper = svtk.svtkPolyDataMapper()
SphereMapper.SetInputConnection(Vertices.GetOutputPort())
SphereMapper.ScalarVisibilityOff()

CubeVertices = svtk.svtkActor()
CubeVertices.SetMapper(SphereMapper)
CubeVertices.GetProperty().SetDiffuseColor(GetRGBColor('tomato'))

# define the text for the labels
caseLabel = svtk.svtkVectorText()
caseLabel.SetText("Case 2")

aLabelTransform = svtk.svtkTransform()
aLabelTransform.Identity()
aLabelTransform.Translate(-.2, 0, 1.25)
aLabelTransform.Scale(.05, .05, .05)

labelTransform = svtk.svtkTransformPolyDataFilter()
labelTransform.SetTransform(aLabelTransform)
labelTransform.SetInputConnection(caseLabel.GetOutputPort())

labelMapper = svtk.svtkPolyDataMapper()
labelMapper.SetInputConnection(labelTransform.GetOutputPort())

labelActor = svtk.svtkActor()
labelActor.SetMapper(labelMapper)

# define the base
baseModel = svtk.svtkCubeSource()
baseModel.SetXLength(1.5)
baseModel.SetYLength(.01)
baseModel.SetZLength(1.5)

baseMapper = svtk.svtkPolyDataMapper()
baseMapper.SetInputConnection(baseModel.GetOutputPort())

base = svtk.svtkActor()
base.SetMapper(baseMapper)

# Create the RenderWindow, Renderer and both Actors
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# position the base
base.SetPosition(.5, -.09, .5)
ren1.AddActor(tetEdgeActor)
ren1.AddActor(base)
ren1.AddActor(labelActor)
ren1.AddActor(CubeEdges)
ren1.AddActor(CubeVertices)
ren1.AddActor(Tets)
ren1.SetBackground(GetRGBColor('slate_grey'))

def case2(scalars, IN, OUT, caseLabel):
    scalars.InsertValue(0, IN)
    scalars.InsertValue(1, IN)
    scalars.InsertValue(2, OUT)
    scalars.InsertValue(3, OUT)
    scalars.InsertValue(4, OUT)
    scalars.InsertValue(5, OUT)
    scalars.InsertValue(6, OUT)
    scalars.InsertValue(7, OUT)
    if IN == 1:
        caseLabel.SetText("Case 2 - 00000011")
    else:
        caseLabel.SetText("Case 2c - 11111100")

case2(Scalars, 1, 0, caseLabel)
renWin.SetSize(400, 400)

ren1.ResetCamera()
ren1.GetActiveCamera().Dolly(1.2)
ren1.GetActiveCamera().Azimuth(30)
ren1.GetActiveCamera().Elevation(20)
ren1.ResetCameraClippingRange()

renWin.Render()

iren.Initialize()
#iren.Start()
