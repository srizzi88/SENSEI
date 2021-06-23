#!/usr/bin/env python

import random
from svtk import *

n = 10000

qinit = svtkQtInitialization()

pd = svtkPolyData()
pts = svtkPoints()
verts = svtkCellArray()
orient = svtkDoubleArray()
orient.SetName('orientation')
label = svtkStringArray()
label.SetName('label')
for i in range(n):
  pts.InsertNextPoint(random.random(), random.random(), random.random())
  verts.InsertNextCell(1)
  verts.InsertCellPoint(i)
  orient.InsertNextValue(random.random()*360.0)
  label.InsertNextValue(str(i))

pd.SetPoints(pts)
pd.SetVerts(verts)
pd.GetPointData().AddArray(label)
pd.GetPointData().AddArray(orient)

hier = svtkPointSetToLabelHierarchy()
hier.SetInputData(pd)
hier.SetOrientationArrayName('orientation')
hier.SetLabelArrayName('label')
hier.GetTextProperty().SetColor(0.0, 0.0, 0.0)

lmapper = svtkLabelPlacementMapper()
lmapper.SetInputConnection(hier.GetOutputPort())
strategy = svtkQtLabelRenderStrategy()
lmapper.SetRenderStrategy(strategy)
lmapper.SetShapeToRoundedRect()
lmapper.SetBackgroundColor(1.0, 1.0, 0.7)
lmapper.SetBackgroundOpacity(0.8)
lmapper.SetMargin(3)

lactor = svtkActor2D()
lactor.SetMapper(lmapper)

mapper = svtkPolyDataMapper()
mapper.SetInputData(pd)
actor = svtkActor()
actor.SetMapper(mapper)

ren = svtkRenderer()
ren.AddActor(lactor)
ren.AddActor(actor)
ren.ResetCamera()

win = svtkRenderWindow()
win.AddRenderer(ren)

iren = svtkRenderWindowInteractor()
iren.SetRenderWindow(win)

iren.Initialize()
iren.Start()

