#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

cylinder = svtk.svtkCylinderSource()
cylinder.SetHeight(1)
cylinder.SetRadius(4)
cylinder.SetResolution(100)
cylinder.CappingOff()

foo = svtk.svtkTransform()
foo.RotateX(20)
foo.RotateY(10)
foo.RotateZ(27)
foo.Scale(1, .7, .3)

transPD = svtk.svtkTransformPolyDataFilter()
transPD.SetInputConnection(cylinder.GetOutputPort())
transPD.SetTransform(foo)

dataMapper = svtk.svtkPolyDataMapper()
dataMapper.SetInputConnection(transPD.GetOutputPort())

model = svtk.svtkActor()
model.SetMapper(dataMapper)
model.GetProperty().SetColor(1, 0, 0)

obb = svtk.svtkOBBTree()
obb.SetMaxLevel(10)
obb.SetNumberOfCellsPerNode(5)
obb.AutomaticOff()

boxes = svtk.svtkSpatialRepresentationFilter()
boxes.SetInputConnection(transPD.GetOutputPort())
boxes.SetSpatialRepresentation(obb)
boxes.SetGenerateLeaves(1)
boxes.Update()

output = boxes.GetOutput().GetBlock(boxes.GetMaximumLevel() + 1)
boxEdges = svtk.svtkExtractEdges()
boxEdges.SetInputData(output)

boxMapper = svtk.svtkPolyDataMapper()
boxMapper.SetInputConnection(boxEdges.GetOutputPort())
boxMapper.SetResolveCoincidentTopology(1)

boxActor = svtk.svtkActor()
boxActor.SetMapper(boxMapper)
boxActor.GetProperty().SetAmbient(1)
boxActor.GetProperty().SetDiffuse(0)
boxActor.GetProperty().SetRepresentationToWireframe()

ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(model)
ren1.AddActor(boxActor)
ren1.SetBackground(0.1, 0.2, 0.4)

renWin.SetSize(300, 300)

ren1.ResetCamera()
ren1.GetActiveCamera().Zoom(1.5)

# render the image
#
iren.Initialize()

#iren.Start()
