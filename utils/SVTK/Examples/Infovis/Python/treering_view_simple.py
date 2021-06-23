#!/usr/bin/env python
from svtk import *

reader1 = svtkXMLTreeReader()
reader1.SetFileName("treetest.xml")
reader1.Update()

view = svtkTreeRingView()
view.SetRepresentationFromInput(reader1.GetOutput())
view.SetAreaSizeArrayName("size")
view.SetAreaColorArrayName("level")
view.SetAreaLabelArrayName("name")
view.SetAreaLabelVisibility(True)
view.SetAreaHoverArrayName("name")
view.SetShrinkPercentage(0.05)
view.Update()

# Apply a theme to the views
theme = svtkViewTheme.CreateMellowTheme()
view.ApplyViewTheme(theme)
theme.FastDelete()

view.ResetCamera()
view.Render()

view.GetInteractor().Start()
