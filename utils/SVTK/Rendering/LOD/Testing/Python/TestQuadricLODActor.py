#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

def GetRGBColor(colorName):
    '''
        Return the red, green and blue components for a
        color as doubles.
    '''
    rgb = [0.0, 0.0, 0.0]  # black
    svtk.svtkNamedColors().GetColorRGB(colorName, rgb)
    return rgb

# Test the quadric decimation LOD actor
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# pipeline stuff
#
sphere = svtk.svtkSphereSource()
sphere.SetPhiResolution(150)
sphere.SetThetaResolution(150)

plane = svtk.svtkPlaneSource()
plane.SetXResolution(150)
plane.SetYResolution(150)

mapper = svtk.svtkPolyDataMapper()
mapper.SetInputConnection(sphere.GetOutputPort())
mapper.SetInputConnection(plane.GetOutputPort())

actor = svtk.svtkQuadricLODActor()
actor.SetMapper(mapper)
actor.DeferLODConstructionOff()
actor.GetProperty().SetRepresentationToWireframe()
actor.GetProperty().SetDiffuseColor(GetRGBColor('tomato'))
actor.GetProperty().SetDiffuse(.8)
actor.GetProperty().SetSpecular(.4)
actor.GetProperty().SetSpecularPower(30)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(actor)
ren1.SetBackground(1, 1, 1)

renWin.SetSize(300, 300)

iren.Initialize()
#iren.Start()
