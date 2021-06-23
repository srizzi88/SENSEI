#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Create two polygon sources, one a closed polyline, one a polygon
#
polyline = svtk.svtkRegularPolygonSource()
polyline.SetCenter(1,1,1)
polyline.SetRadius(1)
polyline.SetNumberOfSides(12)
polyline.SetNormal(1,2,3)
polyline.GeneratePolylineOn()
polyline.GeneratePolygonOff()
polylineMapper = svtk.svtkPolyDataMapper()
polylineMapper.SetInputConnection(polyline.GetOutputPort())
polylineActor = svtk.svtkActor()
polylineActor.SetMapper(polylineMapper)
polylineActor.GetProperty().SetColor(0,1,0)
polylineActor.GetProperty().SetAmbient(1)
polygon = svtk.svtkRegularPolygonSource()
polygon.SetCenter(3,1,1)
polygon.SetRadius(1)
polygon.SetNumberOfSides(12)
polygon.SetNormal(1,2,3)
polygon.GeneratePolylineOff()
polygon.GeneratePolygonOn()
polygonMapper = svtk.svtkPolyDataMapper()
polygonMapper.SetInputConnection(polygon.GetOutputPort())
polygonActor = svtk.svtkActor()
polygonActor.SetMapper(polygonMapper)
polygonActor.GetProperty().SetColor(1,0,0)
polygonActor.GetProperty().SetAmbient(1)
# Create the RenderWindow, Renderer and both Actors
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# create room profile# Add the actors to the renderer, set the background and size
#
ren1.AddActor(polylineActor)
ren1.AddActor(polygonActor)
ren1.SetBackground(0,0,0)
renWin.SetSize(200,200)
renWin.Render()
iren.Initialize()
# prevent the tk window from showing up then start the event loop
# --- end of script --
