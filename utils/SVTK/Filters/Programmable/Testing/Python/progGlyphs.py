#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

res = 6
plane = svtk.svtkPlaneSource()
plane.SetResolution(res,res)
colors = svtk.svtkElevationFilter()
colors.SetInputConnection(plane.GetOutputPort())
colors.SetLowPoint(-0.25,-0.25,-0.25)
colors.SetHighPoint(0.25,0.25,0.25)
planeMapper = svtk.svtkPolyDataMapper()
planeMapper.SetInputConnection(colors.GetOutputPort())
planeActor = svtk.svtkActor()
planeActor.SetMapper(planeMapper)
planeActor.GetProperty().SetRepresentationToWireframe()
# procedure for generating glyphs
def Glyph (__svtk__temp0=0,__svtk__temp1=0):
    global res
    ptId = glypher.GetPointId()
    pd = glypher.GetPointData()
    xyz = glypher.GetPoint()
    x = lindex(xyz,0)
    y = lindex(xyz,1)
    length = glypher.GetInput(0).GetLength()
    scale = expr.expr(globals(), locals(),["length","/","(","2.0","*","res",")"])
    squad.SetScale(scale,scale,scale)
    squad.SetCenter(xyz)
    squad.SetPhiRoundness(expr.expr(globals(), locals(),["abs","(","x",")*","5.0"]))
    squad.SetThetaRoundness(expr.expr(globals(), locals(),["abs","(","y",")*","5.0"]))
    squad.Update()

# create simple poly data so we can apply glyph
squad = svtk.svtkSuperquadricSource()
squad.Update()
glypher = svtk.svtkProgrammableGlyphFilter()
glypher.SetInputConnection(colors.GetOutputPort())
glypher.SetSourceData(squad.GetOutput())
glypher.SetGlyphMethod(Glyph)
glyphMapper = svtk.svtkPolyDataMapper()
glyphMapper.SetInputConnection(glypher.GetOutputPort())
glyphActor = svtk.svtkActor()
glyphActor.SetMapper(glyphMapper)
# Create the rendering stuff
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
ren1.AddActor(planeActor)
ren1.AddActor(glyphActor)
ren1.SetBackground(1,1,1)
renWin.SetSize(450,450)
renWin.Render()
ren1.GetActiveCamera().Zoom(1.5)
# Get handles to some useful objects
#
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
