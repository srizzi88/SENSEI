#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()


GLYPH_TYPES = [
  'SVTK_NO_GLYPH',
  'SVTK_VERTEX_GLYPH',
  'SVTK_DASH_GLYPH',
  'SVTK_CROSS_GLYPH',
  'SVTK_THICKCROSS_GLYPH',
  'SVTK_TRIANGLE_GLYPH',
  'SVTK_SQUARE_GLYPH',
  'SVTK_CIRCLE_GLYPH',
  'SVTK_DIAMOND_GLYPH',
  'SVTK_ARROW_GLYPH',
  'SVTK_THICKARROW_GLYPH',
  'SVTK_HOOKEDARROW_GLYPH',
  'SVTK_EDGEARROW_GLYPH'
]

numRows = len(GLYPH_TYPES)
actors = []
fixedScales = [ 0.5, 1.0, 1.5, 2.0 ]

for i in range(numRows):
  for j in range(1, 5):
    points = svtk.svtkPoints()
    points.InsertNextPoint(j*3, i*3, 0)

    polydata = svtk.svtkPolyData()
    polydata.SetPoints(points)

    glyphSource = svtk.svtkGlyphSource2D()

    glyphSource.SetGlyphType(i)
    glyphSource.FilledOff()
    glyphSource.SetResolution(25)
    glyphSource.SetScale(fixedScales[j-1])

    if GLYPH_TYPES[i] == 'SVTK_TRIANGLE_GLYPH':
      glyphSource.SetRotationAngle(90)

    glyph2D = svtk.svtkGlyph2D()
    glyph2D.SetSourceConnection(glyphSource.GetOutputPort())
    glyph2D.SetInputData(polydata)
    glyph2D.Update()

    mapper = svtk.svtkPolyDataMapper()
    mapper.SetInputConnection(glyph2D.GetOutputPort())
    mapper.Update()

    actor = svtk.svtkActor()
    actor.SetMapper(mapper)

    actors.append(actor)

# Set up the renderer, render window, and interactor.
renderer = svtk.svtkRenderer()

for actor in actors:
  renderer.AddActor(actor)

renWin = svtk.svtkRenderWindow()
renWin.SetSize(400,600)
renWin.AddRenderer(renderer)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

iren.Initialize()
renWin.Render()
