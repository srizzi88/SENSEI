#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# This test checks netCDF CF reader for reading 2D bounds information.
renWin = svtk.svtkRenderWindow()
renWin.SetSize(400,200)
#############################################################################
# Case 1: Spherical coordinates off.
# Open the file.
reader_cartesian = svtk.svtkNetCDFCFReader()
reader_cartesian.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/sampleCurveGrid4.nc")
# Set the arrays we want to load.
reader_cartesian.UpdateMetaData()
reader_cartesian.SetVariableArrayStatus("sample",1)
reader_cartesian.SphericalCoordinatesOff()
# Extract a region that is non-overlapping.
voi_cartesian = svtk.svtkExtractGrid()
voi_cartesian.SetInputConnection(reader_cartesian.GetOutputPort())
voi_cartesian.SetVOI(20,47,0,31,0,0)
# Assign the field to scalars.
aa_cartesian = svtk.svtkAssignAttribute()
aa_cartesian.SetInputConnection(voi_cartesian.GetOutputPort())
aa_cartesian.Assign("sample","SCALARS","CELL_DATA")
# Extract a surface that we can render.
surface_cartesian = svtk.svtkDataSetSurfaceFilter()
surface_cartesian.SetInputConnection(aa_cartesian.GetOutputPort())
mapper_cartesian = svtk.svtkPolyDataMapper()
mapper_cartesian.SetInputConnection(surface_cartesian.GetOutputPort())
mapper_cartesian.SetScalarRange(0,1535)
actor_cartesian = svtk.svtkActor()
actor_cartesian.SetMapper(mapper_cartesian)
ren_cartesian = svtk.svtkRenderer()
ren_cartesian.AddActor(actor_cartesian)
ren_cartesian.SetViewport(0.0,0.0,0.5,1.0)
renWin.AddRenderer(ren_cartesian)
#############################################################################
# Case 2: Spherical coordinates on.
# Open the file.
reader_spherical = svtk.svtkNetCDFCFReader()
reader_spherical.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/sampleCurveGrid4.nc")
# Set the arrays we want to load.
reader_spherical.UpdateMetaData()
reader_spherical.SetVariableArrayStatus("sample",1)
reader_spherical.SphericalCoordinatesOn()
# Assign the field to scalars.
aa_spherical = svtk.svtkAssignAttribute()
aa_spherical.SetInputConnection(reader_spherical.GetOutputPort())
aa_spherical.Assign("sample","SCALARS","CELL_DATA")
# Extract a surface that we can render.
surface_spherical = svtk.svtkDataSetSurfaceFilter()
surface_spherical.SetInputConnection(aa_spherical.GetOutputPort())
mapper_spherical = svtk.svtkPolyDataMapper()
mapper_spherical.SetInputConnection(surface_spherical.GetOutputPort())
mapper_spherical.SetScalarRange(0,1535)
actor_spherical = svtk.svtkActor()
actor_spherical.SetMapper(mapper_spherical)
ren_spherical = svtk.svtkRenderer()
ren_spherical.AddActor(actor_spherical)
ren_spherical.SetViewport(0.5,0.0,1.0,1.0)
renWin.AddRenderer(ren_spherical)
# --- end of script --
