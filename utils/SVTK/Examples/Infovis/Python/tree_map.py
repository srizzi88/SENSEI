#!/usr/bin/env python
from svtk import *
import os.path
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

dataRootDir = SVTK_DATA_ROOT + "/Data/Infovis/XML/"
if not os.path.exists(dataRootDir):
  dataRootDir = SVTK_DATA_ROOT + "/Data/Infovis/XML/"

reader1 = svtkXMLTreeReader()
reader1.SetFileName(dataRootDir + "svtklibrary.xml")
reader1.Update()

numeric = svtkStringToNumeric()
numeric.SetInputConnection(reader1.GetOutputPort())

view = svtkTreeMapView()
view.SetAreaSizeArrayName("size");
view.SetAreaColorArrayName("level");
view.SetAreaLabelArrayName("name");
view.SetAreaLabelVisibility(True);
view.SetAreaHoverArrayName("name");
view.SetLayoutStrategyToSquarify();
view.SetRepresentationFromInputConnection(numeric.GetOutputPort());

# Apply a theme to the views
theme = svtkViewTheme.CreateMellowTheme()
view.ApplyViewTheme(theme)
theme.FastDelete()

view.ResetCamera()
view.Render()

view.GetInteractor().Start()
