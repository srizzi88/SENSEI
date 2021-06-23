#!/usr/bin/env python

# This example demonstrates the conversion of point data to cell data.
# The conversion is necessary because we want to threshold data based
# on cell scalar values.

import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Read some data with point data attributes. The data is from a
# plastic blow molding process (e.g., to make plastic bottles) and
# consists of two logical components: a mold and a parison. The
# parison is the hot plastic that is being molded, and the mold is
# clamped around the parison to form its shape.
reader = svtk.svtkUnstructuredGridReader()
reader.SetFileName(SVTK_DATA_ROOT + "/Data/blow.svtk")
reader.SetScalarsName("thickness9")
reader.SetVectorsName("displacement9")

# Convert the point data to cell data. The point data is passed
# through the filter so it can be warped. The svtkThresholdFilter then
# thresholds based on cell scalar values and extracts a portion of the
# parison whose cell scalar values lie between 0.25 and 0.75.
p2c = svtk.svtkPointDataToCellData()
p2c.SetInputConnection(reader.GetOutputPort())
p2c.PassPointDataOn()
warp = svtk.svtkWarpVector()
warp.SetInputConnection(p2c.GetOutputPort())
thresh = svtk.svtkThreshold()
thresh.SetInputConnection(warp.GetOutputPort())
thresh.ThresholdBetween(0.25, 0.75)
thresh.SetInputArrayToProcess(1, 0, 0, 0, "thickness9")
#thresh.SetAttributeModeToUseCellData()

# This is used to extract the mold from the parison.
connect = svtk.svtkConnectivityFilter()
connect.SetInputConnection(thresh.GetOutputPort())
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

# The threshold filter has been used to extract the parison.
connect2 = svtk.svtkConnectivityFilter()
connect2.SetInputConnection(thresh.GetOutputPort())
parison = svtk.svtkGeometryFilter()
parison.SetInputConnection(connect2.GetOutputPort())
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

# We generate some contour lines on the parison.
cf = svtk.svtkContourFilter()
cf.SetInputConnection(connect2.GetOutputPort())
cf.SetValue(0, .5)
contourMapper = svtk.svtkPolyDataMapper()
contourMapper.SetInputConnection(cf.GetOutputPort())
contours = svtk.svtkActor()
contours.SetMapper(contourMapper)

# Create graphics stuff
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(moldActor)
ren.AddActor(parisonActor)
ren.AddActor(contours)

ren.ResetCamera()
ren.GetActiveCamera().Azimuth(60)
ren.GetActiveCamera().Roll(-90)
ren.GetActiveCamera().Dolly(2)
ren.ResetCameraClippingRange()

ren.SetBackground(1, 1, 1)
renWin.SetSize(750, 400)

iren.Initialize()
renWin.Render()
iren.Start()
