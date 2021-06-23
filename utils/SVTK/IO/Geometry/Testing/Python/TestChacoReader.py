#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# read in a Chaco file
chReader = svtk.svtkChacoReader()
chReader.SetBaseName(SVTK_DATA_ROOT + "/Data/vwgt")
chReader.SetGenerateGlobalElementIdArray(1)
chReader.SetGenerateGlobalNodeIdArray(1)
chReader.SetGenerateEdgeWeightArrays(1)
chReader.SetGenerateVertexWeightArrays(1)

geom = svtk.svtkGeometryFilter()
geom.SetInputConnection(chReader.GetOutputPort())

mapper = svtk.svtkPolyDataMapper()
mapper.SetInputConnection(geom.GetOutputPort())
mapper.SetColorModeToMapScalars()
mapper.SetScalarModeToUsePointFieldData()
mapper.SelectColorArray("VertexWeight1")
mapper.SetScalarRange(1, 5)

actor0 = svtk.svtkActor()
actor0.SetMapper(mapper)

# Create the RenderWindow, Renderer and interactor
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actor to the renderer, set the background and size
#
ren1.AddActor(actor0)
ren1.SetBackground(0, 0, 0)

renWin.SetSize(300, 300)
renWin.SetMultiSamples(0)

iren.Initialize()

renWin.Render()

#iren.Start()
