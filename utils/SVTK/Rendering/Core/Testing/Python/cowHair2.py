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

# This differs from cowHair because it checks the "MergingOff" feature
# of svtkCleanPolyData....it should give the same result.
# Create the RenderWindow, Renderer and both Actors
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# read data
#
wavefront = svtk.svtkOBJReader()
wavefront.SetFileName(SVTK_DATA_ROOT + "/Data/Viewpoint/cow.obj")
wavefront.Update()

cone = svtk.svtkConeSource()
cone.SetResolution(6)
cone.SetRadius(.1)

transform = svtk.svtkTransform()
transform.Translate(0.5, 0.0, 0.0)
transformF = svtk.svtkTransformPolyDataFilter()
transformF.SetInputConnection(cone.GetOutputPort())
transformF.SetTransform(transform)

# we just clean the normals for efficiency (keep down number of cones)
clean = svtk.svtkCleanPolyData()
clean.SetInputConnection(wavefront.GetOutputPort())
clean.PointMergingOff()

glyph = svtk.svtkHedgeHog()
glyph.SetInputConnection(clean.GetOutputPort())
glyph.SetVectorModeToUseNormal()
glyph.SetScaleFactor(0.4)

hairMapper = svtk.svtkPolyDataMapper()
hairMapper.SetInputConnection(glyph.GetOutputPort())

hair = svtk.svtkActor()
hair.SetMapper(hairMapper)

cowMapper = svtk.svtkPolyDataMapper()
cowMapper.SetInputConnection(wavefront.GetOutputPort())
cow = svtk.svtkActor()
cow.SetMapper(cowMapper)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(cow)
ren1.AddActor(hair)
ren1.ResetCamera()
ren1.GetActiveCamera().Dolly(2)
ren1.GetActiveCamera().Azimuth(30)
ren1.GetActiveCamera().Elevation(30)
ren1.ResetCameraClippingRange()

# hair.GetProperty().SetDiffuseColor(saddle_brown)
# hair.GetProperty().SetAmbientColor(thistle)
# hair.GetProperty().SetAmbient(.3)
#
# cow.GetProperty().SetDiffuseColor(beige)
hair.GetProperty().SetDiffuseColor(GetRGBColor('saddle_brown'))
hair.GetProperty().SetAmbientColor(GetRGBColor('thistle'))
hair.GetProperty().SetAmbient(.3)

# Beige in svtk.svtkNamedColors() is compliant with
# http://en.wikipedia.org/wiki/Web_colors and is a different color.
# so we revert to the one used in colors.py.
# cow.GetProperty().SetDiffuseColor(GetRGBColor('beige'))
cow.GetProperty().SetDiffuseColor(163 / 255.0, 148 / 255.0, 128 / 255.0)

renWin.SetSize(320, 240)

ren1.SetBackground(.1, .2, .4)
iren.Initialize()
renWin.Render()

# render the image
#
#iren.Start()
