#!/usr/bin/env python

import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

pioreader = svtk.svtkPIOReader()
pioreader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/PIO/simple.pio")
pioreader.SetCurrentTimeStep(1)
pioreader.Update()

grid = pioreader.GetOutput()
block = grid.GetBlock(0)
geometryFilter = svtk.svtkHyperTreeGridGeometry()
geometryFilter.SetInputData(block)
geometryFilter.Update()

# ---------------------------------------------------------------------
# Rendering
# ---------------------------------------------------------------------

ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

lut = svtk.svtkLookupTable()
lut.SetHueRange(0.66, 0)
lut.SetSaturationRange(1.0, 0.25);
lut.SetTableRange(48.5, 50)
lut.Build()

mapper = svtk.svtkDataSetMapper()
mapper.SetLookupTable(lut)
mapper.SetColorModeToMapScalars()
mapper.SetScalarModeToUseCellFieldData()
mapper.SelectColorArray('cell_energy')

mapper.SetInputConnection(geometryFilter.GetOutputPort())

mapper.UseLookupTableScalarRangeOn()
mapper.SetScalarRange(48.5, 50)

actor = svtk.svtkActor()
actor.SetMapper(mapper)

ren.AddActor(actor)
renWin.SetSize(300,300)
ren.SetBackground(0.5,0.5,0.5)
ren.ResetCamera()
ren.GetActiveCamera().Roll(45);
ren.GetActiveCamera().Azimuth(45);
renWin.Render()

# prevent the tk window from showing up then start the event loop
pioreader.SetDefaultExecutivePrototype(None)
