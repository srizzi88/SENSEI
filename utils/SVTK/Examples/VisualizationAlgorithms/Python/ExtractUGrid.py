#!/usr/bin/env python

# This example shows how to extract portions of an unstructured grid
# using svtkExtractUnstructuredGrid. svtkConnectivityFilter is also used
# to extract connected components.
#
# The data found here represents a blow molding process. Blow molding
# requires a mold and parison (hot, viscous plastic) which is shaped
# by the mold into the final form. The data file contains several steps
# in time for the analysis.

import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Create a reader to read the unstructured grid data. We use a
# svtkDataSetReader which means the type of the output is unknown until
# the data file is read. So we follow the reader with a
# svtkCastToConcrete and cast the output to svtkUnstructuredGrid.
reader = svtk.svtkDataSetReader()
reader.SetFileName(SVTK_DATA_ROOT + "/Data/blow.svtk")
reader.SetScalarsName("thickness9")
reader.SetVectorsName("displacement9")
castToUnstructuredGrid = svtk.svtkCastToConcrete()
castToUnstructuredGrid.SetInputConnection(reader.GetOutputPort())
warp = svtk.svtkWarpVector()
warp.SetInputConnection(castToUnstructuredGrid.GetOutputPort())

# The connectivity filter extracts the first two regions. These are
# know to represent the mold.
connect = svtk.svtkConnectivityFilter()

connect.SetInputConnection(warp.GetOutputPort())
connect.SetExtractionModeToSpecifiedRegions()
connect.AddSpecifiedRegion(0)
connect.AddSpecifiedRegion(1)
moldMapper = svtk.svtkDataSetMapper()
moldMapper.SetInputConnection(reader.GetOutputPort())
moldMapper.ScalarVisibilityOff()
moldActor = svtk.svtkActor()
moldActor.SetMapper(moldMapper)
moldActor.GetProperty().SetColor(.2, .2, .2)
moldActor.GetProperty().SetRepresentationToWireframe()

# Another connectivity filter is used to extract the parison.
connect2 = svtk.svtkConnectivityFilter()
connect2.SetInputConnection(warp.GetOutputPort())
connect2.SetExtractionModeToSpecifiedRegions()
connect2.AddSpecifiedRegion(2)

# We use svtkExtractUnstructuredGrid because we are interested in
# looking at just a few cells. We use cell clipping via cell id to
# extract the portion of the grid we are interested in.
extractGrid = svtk.svtkExtractUnstructuredGrid()
extractGrid.SetInputConnection(connect2.GetOutputPort())
extractGrid.CellClippingOn()
extractGrid.SetCellMinimum(0)
extractGrid.SetCellMaximum(23)
parison = svtk.svtkGeometryFilter()
parison.SetInputConnection(extractGrid.GetOutputPort())
normals2 = svtk.svtkPolyDataNormals()
normals2.SetInputConnection(parison.GetOutputPort())
normals2.SetFeatureAngle(60)
lut = svtk.svtkLookupTable()
lut.SetHueRange(0.0, 0.66667)
parisonMapper = svtk.svtkPolyDataMapper()
parisonMapper.SetInputConnection(normals2.GetOutputPort())
parisonMapper.SetLookupTable(lut)
parisonMapper.SetScalarRange(0.12, 1.0)
parisonActor = svtk.svtkActor()
parisonActor.SetMapper(parisonMapper)

# graphics stuff
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(parisonActor)
ren.AddActor(moldActor)
ren.SetBackground(1, 1, 1)
ren.ResetCamera()
ren.GetActiveCamera().Azimuth(60)
ren.GetActiveCamera().Roll(-90)
ren.GetActiveCamera().Dolly(2)
ren.ResetCameraClippingRange()
renWin.SetSize(500, 375)

iren.Initialize()
renWin.Render()
iren.Start()
