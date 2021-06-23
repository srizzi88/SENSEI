#!/usr/bin/env python

from svtk import *
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

rdr = svtkExodusIIReader()
rdr.SetFileName(str(SVTK_DATA_ROOT) + "/Data/shared_face_polyhedra.exo")
rdr.Update()

srf = svtkDataSetSurfaceFilter()
srf.SetInputConnection(rdr.GetOutputPort())
srf.Update()

aa = svtkAssignAttribute()
aa.SetInputConnection(srf.GetOutputPort())
aa.Assign('ObjectId', svtkDataSetAttributes.SCALARS, svtkAssignAttribute.CELL_DATA)
aa.Update()

renWin = svtkRenderWindow()
ri = svtkRenderWindowInteractor()
rr = svtkRenderer()
rr.SetBackground(1, 1, 1)
ac = svtkActor()
dm = svtkCompositePolyDataMapper()
ac.SetMapper(dm)
dm.SetInputConnection(aa.GetOutputPort())
dm.SetScalarVisibility(True)
#dm.SetScalarModeToUseCellFieldData()
#dm.SelectColorArray('ObjectId')
#dm.ColorByArrayComponent('ObjectId', 0)
dm.SetScalarRange([10000, 20000])
dm.UseLookupTableScalarRangeOff()
rr.AddActor(ac)
renWin.AddRenderer(rr)
renWin.SetInteractor(ri)
rr.GetActiveCamera().SetPosition(4.45025631439989, -0.520617222824798, 1.08873910981941)
rr.GetActiveCamera().SetFocalPoint(-0.441087924633898, 1.43633923685504, -1.32653110659694)
rr.GetActiveCamera().SetViewUp(-0.424967955165741, 0.0530900205407349, 0.903650201572065)
rr.ResetCamera()
renWin.Render()
#ri.Start()
