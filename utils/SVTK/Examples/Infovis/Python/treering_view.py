#!/usr/bin/env python
from svtk import *

reader2 = svtkXMLTreeReader()
reader2.SetFileName("svtkclasses.xml")
reader2.Update()

reader3 = svtkXMLTreeReader()
reader3.SetFileName("svtklibrary.xml")
reader3.Update()

view2 = svtkTreeMapView()
view2.SetRepresentationFromInput(reader2.GetOutput())
view2.SetAreaSizeArrayName("size")
view2.SetAreaColorArrayName("vertex id")
view2.SetAreaLabelArrayName("id")
view2.SetAreaLabelVisibility(True)
view2.SetAreaHoverArrayName("id")
view2.SetShrinkPercentage(0.02)
view2.Update()

view3 = svtkTreeRingView()
view3.SetRepresentationFromInput(reader3.GetOutput())
view3.SetAreaSizeArrayName("size")
view3.SetAreaColorArrayName("vertex id")
view3.SetAreaLabelArrayName("id")
view3.SetAreaLabelVisibility(True)
view3.SetAreaHoverArrayName("id")
view3.SetShrinkPercentage(0.02)
view3.SetRootAtCenter(True)
view3.Update()

# Apply a theme to the views
theme = svtkViewTheme.CreateMellowTheme()
view2.ApplyViewTheme(theme)
view3.ApplyViewTheme(theme)
theme.FastDelete()

view2.ResetCamera()
view3.ResetCamera()
view2.Render()
view3.Render()

view2.GetInteractor().Start()
