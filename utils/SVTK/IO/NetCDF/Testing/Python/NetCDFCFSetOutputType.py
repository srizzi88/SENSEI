#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# This test checks netCDF reader.  It uses the COARDS convention.
renWin = svtk.svtkRenderWindow()
renWin.SetSize(400,400)
#############################################################################
# Case 1: Image type.
# Open the file.
reader_image = svtk.svtkNetCDFCFReader()
reader_image.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/tos_O1_2001-2002.nc")
reader_image.SetOutputTypeToImage()
# Set the arrays we want to load.
reader_image.UpdateMetaData()
reader_image.SetVariableArrayStatus("tos",1)
reader_image.SphericalCoordinatesOff()
aa_image = svtk.svtkAssignAttribute()
aa_image.SetInputConnection(reader_image.GetOutputPort())
aa_image.Assign("tos","SCALARS","POINT_DATA")
thresh_image = svtk.svtkThreshold()
thresh_image.SetInputConnection(aa_image.GetOutputPort())
thresh_image.ThresholdByLower(10000)
surface_image = svtk.svtkDataSetSurfaceFilter()
surface_image.SetInputConnection(thresh_image.GetOutputPort())
mapper_image = svtk.svtkPolyDataMapper()
mapper_image.SetInputConnection(surface_image.GetOutputPort())
mapper_image.SetScalarRange(270,310)
actor_image = svtk.svtkActor()
actor_image.SetMapper(mapper_image)
ren_image = svtk.svtkRenderer()
ren_image.AddActor(actor_image)
ren_image.SetViewport(0.0,0.0,0.5,0.5)
renWin.AddRenderer(ren_image)
#############################################################################
# Case 2: Rectilinear type.
# Open the file.
reader_rect = svtk.svtkNetCDFCFReader()
reader_rect.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/tos_O1_2001-2002.nc")
reader_rect.SetOutputTypeToRectilinear()
# Set the arrays we want to load.
reader_rect.UpdateMetaData()
reader_rect.SetVariableArrayStatus("tos",1)
reader_rect.SphericalCoordinatesOff()
aa_rect = svtk.svtkAssignAttribute()
aa_rect.SetInputConnection(reader_rect.GetOutputPort())
aa_rect.Assign("tos","SCALARS","POINT_DATA")
thresh_rect = svtk.svtkThreshold()
thresh_rect.SetInputConnection(aa_rect.GetOutputPort())
thresh_rect.ThresholdByLower(10000)
surface_rect = svtk.svtkDataSetSurfaceFilter()
surface_rect.SetInputConnection(thresh_rect.GetOutputPort())
mapper_rect = svtk.svtkPolyDataMapper()
mapper_rect.SetInputConnection(surface_rect.GetOutputPort())
mapper_rect.SetScalarRange(270,310)
actor_rect = svtk.svtkActor()
actor_rect.SetMapper(mapper_rect)
ren_rect = svtk.svtkRenderer()
ren_rect.AddActor(actor_rect)
ren_rect.SetViewport(0.5,0.0,1.0,0.5)
renWin.AddRenderer(ren_rect)
#############################################################################
# Case 3: Structured type.
# Open the file.
reader_struct = svtk.svtkNetCDFCFReader()
reader_struct.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/tos_O1_2001-2002.nc")
reader_struct.SetOutputTypeToStructured()
# Set the arrays we want to load.
reader_struct.UpdateMetaData()
reader_struct.SetVariableArrayStatus("tos",1)
reader_struct.SphericalCoordinatesOff()
aa_struct = svtk.svtkAssignAttribute()
aa_struct.SetInputConnection(reader_struct.GetOutputPort())
aa_struct.Assign("tos","SCALARS","POINT_DATA")
thresh_struct = svtk.svtkThreshold()
thresh_struct.SetInputConnection(aa_struct.GetOutputPort())
thresh_struct.ThresholdByLower(10000)
surface_struct = svtk.svtkDataSetSurfaceFilter()
surface_struct.SetInputConnection(thresh_struct.GetOutputPort())
mapper_struct = svtk.svtkPolyDataMapper()
mapper_struct.SetInputConnection(surface_struct.GetOutputPort())
mapper_struct.SetScalarRange(270,310)
actor_struct = svtk.svtkActor()
actor_struct.SetMapper(mapper_struct)
ren_struct = svtk.svtkRenderer()
ren_struct.AddActor(actor_struct)
ren_struct.SetViewport(0.0,0.5,0.5,1.0)
renWin.AddRenderer(ren_struct)
#############################################################################
# Case 4: Unstructured type.
# Open the file.
reader_auto = svtk.svtkNetCDFCFReader()
reader_auto.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/tos_O1_2001-2002.nc")
reader_auto.SetOutputTypeToUnstructured()
# Set the arrays we want to load.
reader_auto.UpdateMetaData()
reader_auto.SetVariableArrayStatus("tos",1)
reader_auto.SphericalCoordinatesOff()
aa_auto = svtk.svtkAssignAttribute()
aa_auto.SetInputConnection(reader_auto.GetOutputPort())
aa_auto.Assign("tos","SCALARS","POINT_DATA")
thresh_auto = svtk.svtkThreshold()
thresh_auto.SetInputConnection(aa_auto.GetOutputPort())
thresh_auto.ThresholdByLower(10000)
surface_auto = svtk.svtkDataSetSurfaceFilter()
surface_auto.SetInputConnection(thresh_auto.GetOutputPort())
mapper_auto = svtk.svtkPolyDataMapper()
mapper_auto.SetInputConnection(surface_auto.GetOutputPort())
mapper_auto.SetScalarRange(270,310)
actor_auto = svtk.svtkActor()
actor_auto.SetMapper(mapper_auto)
ren_auto = svtk.svtkRenderer()
ren_auto.AddActor(actor_auto)
ren_auto.SetViewport(0.5,0.5,1.0,1.0)
renWin.AddRenderer(ren_auto)
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
