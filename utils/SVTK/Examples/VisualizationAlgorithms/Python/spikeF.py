#!/usr/bin/env python

# This example demonstrates the use of glyphing. We also use a mask filter
# to select a subset of points to glyph.

import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Read a data file. This originally was a Cyberware laser digitizer scan
# of Fran J.'s face. Surface normals are generated based on local geometry
# (i.e., the polygon normals surrounding eash point are averaged). We flip
# the normals because we want them to point out from Fran's face.
fran = svtk.svtkPolyDataReader()
fran.SetFileName(SVTK_DATA_ROOT + "/Data/fran_cut.svtk")
normals = svtk.svtkPolyDataNormals()
normals.SetInputConnection(fran.GetOutputPort())
normals.FlipNormalsOn()
franMapper = svtk.svtkPolyDataMapper()
franMapper.SetInputConnection(normals.GetOutputPort())
franActor = svtk.svtkActor()
franActor.SetMapper(franMapper)
franActor.GetProperty().SetColor(1.0, 0.49, 0.25)

# We subsample the dataset because we want to glyph just a subset of
# the points. Otherwise the display is cluttered and cannot be easily
# read. The RandonModeOn and SetOnRatio combine to random select one out
# of every 10 points in the dataset.
ptMask = svtk.svtkMaskPoints()
ptMask.SetInputConnection(normals.GetOutputPort())
ptMask.SetOnRatio(10)
ptMask.RandomModeOn()

# In this case we are using a cone as a glyph. We transform the cone so
# its base is at 0,0,0. This is the point where glyph rotation occurs.
cone = svtk.svtkConeSource()
cone.SetResolution(6)
transform = svtk.svtkTransform()
transform.Translate(0.5, 0.0, 0.0)
transformF = svtk.svtkTransformPolyDataFilter()
transformF.SetInputConnection(cone.GetOutputPort())
transformF.SetTransform(transform)

# svtkGlyph3D takes two inputs: the input point set (SetInput) which can be
# any svtkDataSet; and the glyph (SetSource) which must be a svtkPolyData.
# We are interested in orienting the glyphs by the surface normals that
# we previously generated.
glyph = svtk.svtkGlyph3D()
glyph.SetInputConnection(ptMask.GetOutputPort())
glyph.SetSourceConnection(transformF.GetOutputPort())
glyph.SetVectorModeToUseNormal()
glyph.SetScaleModeToScaleByVector()
glyph.SetScaleFactor(0.004)
spikeMapper = svtk.svtkPolyDataMapper()
spikeMapper.SetInputConnection(glyph.GetOutputPort())
spikeActor = svtk.svtkActor()
spikeActor.SetMapper(spikeMapper)
spikeActor.GetProperty().SetColor(0.0, 0.79, 0.34)

# Create the RenderWindow, Renderer and both Actors
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(franActor)
ren.AddActor(spikeActor)

renWin.SetSize(500, 500)
ren.SetBackground(0.1, 0.2, 0.4)

# Set a nice camera position.
ren.ResetCamera()
cam1 = ren.GetActiveCamera()
cam1.Zoom(1.4)
cam1.Azimuth(110)

iren.Initialize()
renWin.Render()
iren.Start()
