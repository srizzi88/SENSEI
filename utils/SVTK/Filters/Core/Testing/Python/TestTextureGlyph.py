#!/usr/bin/env python

# $ svtkpython TestTextureGlyph.py --help
# provides more details on other options.

import os
import os.path
import svtk
from svtk.test import Testing
from svtk.util.misc import svtkGetDataRoot

SVTK_DATA_ROOT = svtkGetDataRoot()

class TestTextureGlyph(Testing.svtkTest):
    def testGlyphs(self):
        """Test if texturing of the glyphs works correctly."""
        # The Glyph
        cs = svtk.svtkCubeSource()
        cs.SetXLength(2.0); cs.SetYLength(1.0); cs.SetZLength(0.5)

        # Create input point data.
        pts = svtk.svtkPoints()
        pts.InsertPoint(0, (1,1,1))
        pts.InsertPoint(1, (0,0,0))
        pts.InsertPoint(2, (-1,-1,-1))
        polys = svtk.svtkCellArray()
        polys.InsertNextCell(1)
        polys.InsertCellPoint(0)
        polys.InsertNextCell(1)
        polys.InsertCellPoint(1)
        polys.InsertNextCell(1)
        polys.InsertCellPoint(2)
        pd = svtk.svtkPolyData()
        pd.SetPoints(pts)
        pd.SetPolys(polys)

        # Orient the glyphs as per vectors.
        vec = svtk.svtkFloatArray()
        vec.SetNumberOfComponents(3)
        vec.InsertTuple3(0, 1, 0, 0)
        vec.InsertTuple3(1, 0, 1, 0)
        vec.InsertTuple3(2, 0, 0, 1)
        pd.GetPointData().SetVectors(vec)

        # The glyph filter.
        g = svtk.svtkGlyph3D()
        g.SetScaleModeToDataScalingOff()
        g.SetVectorModeToUseVector()
        g.SetInputData(pd)
        g.SetSourceConnection(cs.GetOutputPort())

        m = svtk.svtkPolyDataMapper()
        m.SetInputConnection(g.GetOutputPort())
        a = svtk.svtkActor()
        a.SetMapper(m)

        # The texture.
        img_file = os.path.join(SVTK_DATA_ROOT, "Data", "masonry.bmp")
        img_r = svtk.svtkBMPReader()
        img_r.SetFileName(img_file)
        t = svtk.svtkTexture()
        t.SetInputConnection(img_r.GetOutputPort())
        t.InterpolateOn()
        a.SetTexture(t)

        # Renderer, RenderWindow etc.
        ren = svtk.svtkRenderer()
        ren.SetBackground(0.5, 0.5, 0.5)
        ren.AddActor(a)

        ren.ResetCamera();
        cam = ren.GetActiveCamera()
        cam.Azimuth(-90)
        cam.Zoom(1.4)

        renWin = svtk.svtkRenderWindow()
        renWin.AddRenderer(ren)
        rwi = svtk.svtkRenderWindowInteractor()
        rwi.SetRenderWindow(renWin)
        rwi.Initialize()
        rwi.Render()

if __name__ == "__main__":
    Testing.main([(TestTextureGlyph, 'test')])
