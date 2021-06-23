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
# create simple poly data so we can apply glyph
squad = svtk.svtkSuperquadricSource()
squadColors = svtk.svtkElevationFilter()
squadColors.SetInputConnection(squad.GetOutputPort())
squadColors.SetLowPoint(-0.25,-0.25,-0.25)
squadColors.SetHighPoint(0.25,0.25,0.25)
squadCaster = svtk.svtkCastToConcrete()
squadCaster.SetInputConnection(squadColors.GetOutputPort())
squadTransform = svtk.svtkTransform()
transformSquad = svtk.svtkTransformPolyDataFilter()
transformSquad.SetInputConnection(squadColors.GetOutputPort())
transformSquad.SetTransform(squadTransform)
transformSquad.Update()
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
    squadTransform.Identity()
    if (x == y):
        squad.ToroidalOn()
        squadTransform.Translate(xyz)
        squadTransform.RotateX(90)
        pass
    else:
        squadTransform.Translate(xyz)
        squad.ToroidalOff()
        pass
    squadTransform.Scale(scale,scale,scale)
    squad.SetPhiRoundness(expr.expr(globals(), locals(),["abs","(","x",")*","5.0"]))
    squad.SetThetaRoundness(expr.expr(globals(), locals(),["abs","(","y",")*","5.0"]))

glypher = svtk.svtkProgrammableGlyphFilter()
glypher.SetInputConnection(colors.GetOutputPort())
glypher.SetSourceConnection(transformSquad.GetOutputPort())
glypher.SetGlyphMethod(Glyph)
glypher.SetColorModeToColorBySource()
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
ren1.GetActiveCamera().Zoom(1.3)
# Get handles to some useful objects
#
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
