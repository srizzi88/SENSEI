#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Generate texture coordinates on a "random" sphere.
# create some random points in a sphere
#
sphere = svtk.svtkPointSource()
sphere.SetNumberOfPoints(25)
# triangulate the points
#
del1 = svtk.svtkDelaunay3D()
del1.SetInputConnection(sphere.GetOutputPort())
del1.SetTolerance(0.01)
# texture map the sphere (using cylindrical coordinate system)
#
tmapper = svtk.svtkTextureMapToCylinder()
tmapper.SetInputConnection(del1.GetOutputPort())
tmapper.PreventSeamOn()
xform = svtk.svtkTransformTextureCoords()
xform.SetInputConnection(tmapper.GetOutputPort())
xform.SetScale(4,4,1)
mapper = svtk.svtkDataSetMapper()
mapper.SetInputConnection(xform.GetOutputPort())
# load in the texture map and assign to actor
#
bmpReader = svtk.svtkBMPReader()
bmpReader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/masonry.bmp")
atext = svtk.svtkTexture()
atext.SetInputConnection(bmpReader.GetOutputPort())
atext.InterpolateOn()
triangulation = svtk.svtkActor()
triangulation.SetMapper(mapper)
triangulation.SetTexture(atext)
# Create rendering stuff
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(triangulation)
ren1.SetBackground(1,1,1)
renWin.SetSize(300,300)
renWin.Render()
# render the image
#
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
