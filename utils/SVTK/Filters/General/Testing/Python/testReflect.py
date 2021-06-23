#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

cone = svtk.svtkConeSource()
reflect = svtk.svtkReflectionFilter()
reflect.SetInputConnection(cone.GetOutputPort())
reflect.SetPlaneToXMax()
reflect2 = svtk.svtkReflectionFilter()
reflect2.SetInputConnection(reflect.GetOutputPort())
reflect2.SetPlaneToYMax()
reflect3 = svtk.svtkReflectionFilter()
reflect3.SetInputConnection(reflect2.GetOutputPort())
reflect3.SetPlaneToZMax()
mapper = svtk.svtkDataSetMapper()
mapper.SetInputConnection(reflect3.GetOutputPort())
actor = svtk.svtkActor()
actor.SetMapper(mapper)
ren1 = svtk.svtkRenderer()
ren1.AddActor(actor)
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.SetSize(200,200)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
iren.Initialize()
renWin.Render()
# --- end of script --
