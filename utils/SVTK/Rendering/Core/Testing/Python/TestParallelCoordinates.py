#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# This example converts data to a field and then displays it using 
# parallel coordinates,
# Create a reader and write out the field
reader = svtk.svtkUnstructuredGridReader()
reader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/blow.svtk")
reader.SetVectorsName("displacement9")
reader.SetScalarsName("thickness9")
ds2do = svtk.svtkDataSetToDataObjectFilter()
ds2do.SetInputConnection(reader.GetOutputPort())
ds2do.ModernTopologyOff() # Backwards compatibility
ds2do.Update()
actor = svtk.svtkParallelCoordinatesActor()
actor.SetInputConnection(ds2do.GetOutputPort())
actor.SetTitle("Parallel Coordinates Plot of blow.tcl")
actor.SetIndependentVariablesToColumns()
actor.GetPositionCoordinate().SetValue(0.05,0.05,0.0)
actor.GetPosition2Coordinate().SetValue(0.95,0.85,0.0)
actor.GetProperty().SetColor(1,0,0)
# Set text colors (same as actor for backward compat with test)
actor.GetTitleTextProperty().SetColor(1,0,0)
actor.GetLabelTextProperty().SetColor(1,0,0)
# Create the RenderWindow, Renderer and both Actors
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
ren1.AddActor(actor)
ren1.SetBackground(1,1,1)
renWin.SetSize(500,200)
# render the image
#
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
