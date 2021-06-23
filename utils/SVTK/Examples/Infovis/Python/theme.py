#!/usr/bin/env python
from svtk import *

source = svtkRandomGraphSource()

view = svtkGraphLayoutView()
view.AddRepresentationFromInputConnection(source.GetOutputPort())

theme = svtkViewTheme.CreateMellowTheme()
view.ApplyViewTheme(theme)
theme.FastDelete()

view.GetRenderWindow().SetSize(600, 600)
view.SetVertexColorArrayName("VertexDegree");
view.SetColorVertices(True);
view.ResetCamera()
view.Render()
view.GetInteractor().Start()

