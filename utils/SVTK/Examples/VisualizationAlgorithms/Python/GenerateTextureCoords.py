#!/usr/bin/env python

# This example shows how to generate and manipulate texture coordinates.
# A random cloud of points is generated and then triangulated with
# svtkDelaunay3D. Since these points do not have texture coordinates,
# we generate them with svtkTextureMapToCylinder.

import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Begin by generating 25 random points in the unit sphere.
sphere = svtk.svtkPointSource()
sphere.SetNumberOfPoints(25)

# Triangulate the points with svtkDelaunay3D. This generates a convex hull
# of tetrahedron.
delny = svtk.svtkDelaunay3D()
delny.SetInputConnection(sphere.GetOutputPort())
delny.SetTolerance(0.01)

# The triangulation has texture coordinates generated so we can map
# a texture onto it.
tmapper = svtk.svtkTextureMapToCylinder()
tmapper.SetInputConnection(delny.GetOutputPort())
tmapper.PreventSeamOn()

# We scale the texture coordinate to get some repeat patterns.
xform = svtk.svtkTransformTextureCoords()
xform.SetInputConnection(tmapper.GetOutputPort())
xform.SetScale(4, 4, 1)

# svtkDataSetMapper internally uses a svtkGeometryFilter to extract the
# surface from the triangulation. The output (which is svtkPolyData) is
# then passed to an internal svtkPolyDataMapper which does the
# rendering.
mapper = svtk.svtkDataSetMapper()
mapper.SetInputConnection(xform.GetOutputPort())

# A texture is loaded using an image reader. Textures are simply images.
# The texture is eventually associated with an actor.
bmpReader = svtk.svtkBMPReader()
bmpReader.SetFileName(SVTK_DATA_ROOT + "/Data/masonry.bmp")
atext = svtk.svtkTexture()
atext.SetInputConnection(bmpReader.GetOutputPort())
atext.InterpolateOn()
triangulation = svtk.svtkActor()
triangulation.SetMapper(mapper)
triangulation.SetTexture(atext)

# Create the standard rendering stuff.
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(triangulation)
ren.SetBackground(1, 1, 1)
renWin.SetSize(300, 300)

iren.Initialize()
renWin.Render()
iren.Start()
