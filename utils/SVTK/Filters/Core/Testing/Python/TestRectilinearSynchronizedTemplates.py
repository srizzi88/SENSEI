#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# create pipeline - rectilinear grid
#
rgridReader = svtk.svtkRectilinearGridReader()
rgridReader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/RectGrid2.svtk")
rgridReader.Update()
contour = svtk.svtkRectilinearSynchronizedTemplates()
contour.SetInputConnection(rgridReader.GetOutputPort())
contour.SetValue(0,1)
contour.ComputeScalarsOff()
contour.ComputeNormalsOn()
contour.ComputeGradientsOn()
cMapper = svtk.svtkPolyDataMapper()
cMapper.SetInputConnection(contour.GetOutputPort())
cActor = svtk.svtkActor()
cActor.SetMapper(cMapper)
# Create the RenderWindow, Renderer and both Actors
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.SetSize(200,200)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
ren1.AddActor(cActor)
iren.Initialize()
# render the image
#
# prevent the tk window from showing up then start the event loop
# --- end of script --
