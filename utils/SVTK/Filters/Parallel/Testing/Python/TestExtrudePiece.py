#!/usr/bin/env python


disk = svtk.svtkDiskSource()
disk.SetRadialResolution(2)
disk.SetCircumferentialResolution(9)

clean = svtk.svtkCleanPolyData()
clean.SetInputConnection(disk.GetOutputPort())
clean.SetTolerance(0.01)

piece = svtk.svtkExtractPolyDataPiece()
piece.SetInputConnection(clean.GetOutputPort())

extrude = svtk.svtkPLinearExtrusionFilter()
extrude.SetInputConnection(piece.GetOutputPort())
extrude.PieceInvariantOn()
# Create the RenderWindow, Renderer and both Actors
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

mapper = svtk.svtkPolyDataMapper()
mapper.SetInputConnection(extrude.GetOutputPort())
mapper.SetNumberOfPieces(2)
mapper.SetPiece(1)
mapper.Update()
mapper.GetInput().RemoveGhostCells()

bf = svtk.svtkProperty()
bf.SetColor(1,0,0)
actor = svtk.svtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetColor(1,1,0.8)
actor.SetBackfaceProperty(bf)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(actor)
ren1.SetBackground(0.1,0.2,0.4)
renWin.SetSize(300,300)
# render the image
#
cam1 = ren1.GetActiveCamera()
cam1.Azimuth(20)
cam1.Elevation(40)
ren1.ResetCamera()
cam1.Zoom(1.2)
iren.Initialize()
# prevent the tk window from showing up then start the event loop
# --- end of script --
