#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Read some Fluent UCD data in ASCII form
r = svtk.svtkFLUENTReader()
r.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/room.cas")
r.EnableAllCellArrays()

g = svtk.svtkGeometryFilter()
g.SetInputConnection(r.GetOutputPort())

FluentMapper = svtk.svtkCompositePolyDataMapper2()
FluentMapper.SetInputConnection(g.GetOutputPort())
FluentMapper.SetScalarModeToUseCellFieldData()
FluentMapper.SelectColorArray("PRESSURE")
FluentMapper.SetScalarRange(-31, 44)
FluentActor = svtk.svtkActor()
FluentActor.SetMapper(FluentMapper)

ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(FluentActor)
renWin.SetSize(300,300)
renWin.Render()
ren1.ResetCamera()
ren1.GetActiveCamera().SetPosition(2, 2, 11)
ren1.GetActiveCamera().SetFocalPoint(2, 2, 0)
ren1.GetActiveCamera().SetViewUp(0, 1, 0)
ren1.GetActiveCamera().SetViewAngle(30)
ren1.ResetCameraClippingRange()

iren.Initialize()
# --- end of script --
