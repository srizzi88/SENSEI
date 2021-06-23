#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# This test checks netCDF reader.  It uses the CF convention.
# Open the file.
reader = svtk.svtkNetCDFCFReader()
reader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/tos_O1_2001-2002.nc")
# Set the arrays we want to load.
reader.UpdateMetaData()
reader.SetVariableArrayStatus("tos",1)
reader.SetSphericalCoordinates(1)
aa = svtk.svtkAssignAttribute()
aa.SetInputConnection(reader.GetOutputPort())
aa.Assign("tos","SCALARS","CELL_DATA")
thresh = svtk.svtkThreshold()
thresh.SetInputConnection(aa.GetOutputPort())
thresh.ThresholdByLower(10000)
surface = svtk.svtkDataSetSurfaceFilter()
surface.SetInputConnection(thresh.GetOutputPort())
mapper = svtk.svtkPolyDataMapper()
mapper.SetInputConnection(surface.GetOutputPort())
mapper.SetScalarRange(270,310)
actor = svtk.svtkActor()
actor.SetMapper(mapper)
ren = svtk.svtkRenderer()
ren.AddActor(actor)
renWin = svtk.svtkRenderWindow()
renWin.SetSize(200,200)
renWin.AddRenderer(ren)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
renWin.Render()
# # Setup a lookup table.
# svtkLookupTable lut
# lut SetTableRange 270 310
# lut SetHueRange 0.66 0.0
# lut SetRampToLinear
# # Make pretty colors
# svtkImageMapToColors map
# map SetInputConnection [asinine GetOutputPort]
# map SetLookupTable lut
# map SetOutputFormatToRGB
# # svtkImageViewer viewer
# # viewer SetInputConnection [map GetOutputPort]
# # viewer SetColorWindow 256
# # viewer SetColorLevel 127.5
# # viewer Render
# svtkImageViewer2 viewer
# viewer SetInputConnection [map GetOutputPort]
# viewer Render
# --- end of script --
