#!/usr/bin/env python

from svtk import *

source = svtkRandomGraphSource()

view = svtkGraphLayoutView()
view.AddRepresentationFromInputConnection(source.GetOutputPort())

view.GetRenderWindow().SetSize(600, 600)
view.ResetCamera()
view.Render()
view.GetInteractor().Start()


