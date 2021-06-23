#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Debugging parameters
sze = 300

reader = svtk.svtkExodusIIReader()
reader.SetFileName(SVTK_DATA_ROOT + "/Data/disk_out_ref.ex2")
reader.UpdateInformation()
reader.SetPointResultArrayStatus("CH4", 1)
reader.Update()

tree = svtk.svtkSpanSpace()

contour = svtk.svtkSMPContourGrid()
#contour = svtk.svtkContourGrid()
contour.SetInputConnection(reader.GetOutputPort())
contour.SetInputArrayToProcess(0, 0, 0, svtk.svtkAssignAttribute.POINT_DATA, "CH4")
contour.SetValue(0, 0.000718448)
contour.MergePiecesOff()
contour.UseScalarTreeOn()
contour.SetScalarTree(tree)
contour.Update()

mapper = svtk.svtkCompositePolyDataMapper2()
mapper.SetInputConnection(contour.GetOutputPort())
mapper.ScalarVisibilityOff()

actor = svtk.svtkActor()
actor.SetMapper(mapper)

# Create graphics stuff
#
ren1 = svtk.svtkRenderer()
ren1.AddActor(actor)
ren1.ResetCamera()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.SetSize(sze, sze)

iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

renWin.Render()
iren.Start()
