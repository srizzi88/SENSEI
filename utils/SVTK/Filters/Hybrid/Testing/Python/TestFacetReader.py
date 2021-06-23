#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

ren1 = svtk.svtkRenderer()
ren1.SetBackground(0,0,0)
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.SetSize(300,300)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
facet0 = svtk.svtkFacetReader()
facet0.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/clown.facet")
Mapper5 = svtk.svtkPolyDataMapper()
Mapper5.SetInputConnection(facet0.GetOutputPort())
Mapper5.UseLookupTableScalarRangeOff()
Mapper5.SetScalarVisibility(1)
Mapper5.SetScalarModeToDefault()
Actor5 = svtk.svtkLODActor()
Actor5.SetMapper(Mapper5)
Actor5.GetProperty().SetRepresentationToSurface()
Actor5.GetProperty().SetInterpolationToGouraud()
Actor5.GetProperty().SetAmbient(0.15)
Actor5.GetProperty().SetDiffuse(0.85)
Actor5.GetProperty().SetSpecular(0.1)
Actor5.GetProperty().SetSpecularPower(100)
Actor5.GetProperty().SetSpecularColor(1,1,1)
Actor5.GetProperty().SetColor(1,1,1)
Actor5.SetNumberOfCloudPoints(30000)
ren1.AddActor(Actor5)
camera = svtk.svtkCamera()
camera.SetClippingRange(3,6)
camera.SetFocalPoint(.1,.03,-.5)
camera.SetPosition(4.4,-0.5,-.5)
camera.SetViewUp(0,0,-1)
ren1.SetActiveCamera(camera)
# enable user interface interactor
#iren SetUserMethod {wm deiconify .svtkInteract}
iren.Initialize()
# prevent the tk window from showing up then start the event loop
# --- end of script --
