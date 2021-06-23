#!/usr/bin/env python
import svtk
from svtk.test import Testing
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

class TestGridSynchronizedTemplates3D(Testing.svtkTest):
    def testAll(self):
        # cut data
        pl3d = svtk.svtkMultiBlockPLOT3DReader()
        pl3d.SetXYZFileName(SVTK_DATA_ROOT + "/Data/combxyz.bin")
        pl3d.SetQFileName(SVTK_DATA_ROOT + "/Data/combq.bin")
        pl3d.SetScalarFunctionNumber(100)
        pl3d.SetVectorFunctionNumber(202)
        pl3d.Update()
        pl3d_output = pl3d.GetOutput().GetBlock(0)

        range = pl3d_output.GetPointData().GetScalars().GetRange()
        min = range[0]
        max = range[1]
        value = (min + max) / 2.0

        # cf = svtk.svtkGridSynchronizedTemplates3D()
        cf = svtk.svtkContourFilter()
        cf.SetInputData(pl3d_output)
        cf.SetValue(0, value)
        cf.GenerateTrianglesOff()
        cf.Update()
        self.failUnlessEqual(
          cf.GetOutputDataObject(0).GetNumberOfPoints(), 4674)
        self.failUnlessEqual(
          cf.GetOutputDataObject(0).GetNumberOfCells(), 4012)

        cf.GenerateTrianglesOn()
        cf.Update()
        self.failUnlessEqual(
          cf.GetOutputDataObject(0).GetNumberOfPoints(), 4674)
        self.failUnlessEqual(
          cf.GetOutputDataObject(0).GetNumberOfCells(), 8076)

        # cf ComputeNormalsOff
        cfMapper = svtk.svtkPolyDataMapper()
        cfMapper.SetInputConnection(cf.GetOutputPort())
        cfMapper.SetScalarRange(
          pl3d_output.GetPointData().GetScalars().GetRange())

        cfActor = svtk.svtkActor()
        cfActor.SetMapper(cfMapper)

        # outline
        outline = svtk.svtkStructuredGridOutlineFilter()
        outline.SetInputData(pl3d_output)

        outlineMapper = svtk.svtkPolyDataMapper()
        outlineMapper.SetInputConnection(outline.GetOutputPort())

        outlineActor = svtk.svtkActor()
        outlineActor.SetMapper(outlineMapper)
        outlineActor.GetProperty().SetColor(0, 0, 0)

         # # Graphics stuff
         # Create the RenderWindow, Renderer and both Actors
         #
        ren1 = svtk.svtkRenderer()
        renWin = svtk.svtkRenderWindow()
        renWin.SetMultiSamples(0)
        renWin.AddRenderer(ren1)
        iren = svtk.svtkRenderWindowInteractor()
        iren.SetRenderWindow(renWin)

        # Add the actors to the renderer, set the background and size
        #
        ren1.AddActor(outlineActor)
        ren1.AddActor(cfActor)
        ren1.SetBackground(1, 1, 1)

        renWin.SetSize(400, 400)

        cam1 = ren1.GetActiveCamera()
        cam1.SetClippingRange(3.95297, 50)
        cam1.SetFocalPoint(9.71821, 0.458166, 29.3999)
        cam1.SetPosition(2.7439, -37.3196, 38.7167)
        cam1.SetViewUp(-0.16123, 0.264271, 0.950876)
        iren.Initialize()

        # render the image
        #
        # loop over surfaces
        i = 0
        while i < 17:
            cf.SetValue(0, min + (i / 16.0) * (max - min))

            renWin.Render()

            cf.SetValue(0, min + (0.2) * (max - min))

            renWin.Render()

            i += 1

#        iren.Start()

if __name__ == "__main__":
    Testing.main([(TestGridSynchronizedTemplates3D, 'test')])
