#!/usr/bin/env python
import svtk
from svtk.test import Testing
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()
import sys

class TestClip(Testing.svtkTest):
    def testImage2DScalar(self):
        planes = ['XY', 'XZ', 'YZ']
        expectedNCells = [38, 46, 42]
        expectedNClippedCells = [104, 104, 106]
        for plane, nCells, nClippedCells in zip(planes,expectedNCells,expectedNClippedCells):
            r = svtk.svtkRTAnalyticSource()
            r.SetXFreq(600);
            r.SetYFreq(400);
            r.SetZFreq(900);
            if plane == 'XY':
                r.SetWholeExtent(-5, 5, -5, 5, 0, 0)
            elif plane == 'XZ':
                r.SetWholeExtent(-5, 5, 0, 0, -5, 5)
            else:
                r.SetWholeExtent(0, 0, -5, 5, -5, 5)
            r.Update()

            c = svtk.svtkTableBasedClipDataSet()
            c.SetInputConnection(r.GetOutputPort())
            c.SetUseValueAsOffset(0)
            c.SetValue(150)
            c.SetInsideOut(1)
            c.SetGenerateClippedOutput(1)

            c.Update()

            self.assertEqual(c.GetOutput().GetNumberOfCells(), nCells)
            self.assertEqual(c.GetClippedOutput().GetNumberOfCells(), nClippedCells)

    def testImage(self):
        r = svtk.svtkRTAnalyticSource()
        r.SetWholeExtent(-5, 5, -5, 5, -5, 5)
        r.Update()

        s = svtk.svtkSphere()
        s.SetRadius(2)
        s.SetCenter(0,0,0)

        c = svtk.svtkTableBasedClipDataSet()
        c.SetInputConnection(r.GetOutputPort())
        c.SetClipFunction(s)
        c.SetInsideOut(1)

        c.Update()

        self.assertEqual(c.GetOutput().GetNumberOfCells(), 64)

    def testRectilinear(self):
        rt = svtk.svtkRTAnalyticSource()
        rt.SetWholeExtent(-5, 5, -5, 5, -5, 5)
        rt.Update()
        i = rt.GetOutput()

        r = svtk.svtkRectilinearGrid()
        dims = i.GetDimensions()
        r.SetDimensions(dims)
        exts = i.GetExtent()
        orgs = i.GetOrigin()

        xs = svtk.svtkFloatArray()
        xs.SetNumberOfTuples(dims[0])
        for d in range(dims[0]):
            xs.SetTuple1(d, orgs[0] + exts[0] + d)
        r.SetXCoordinates(xs)

        ys = svtk.svtkFloatArray()
        ys.SetNumberOfTuples(dims[1])
        for d in range(dims[1]):
            ys.SetTuple1(d, orgs[1] + exts[2] + d)
        r.SetYCoordinates(ys)

        zs = svtk.svtkFloatArray()
        zs.SetNumberOfTuples(dims[2])
        for d in range(dims[2]):
            zs.SetTuple1(d, orgs[2] + exts[4] + d)
        r.SetZCoordinates(zs)

        s = svtk.svtkSphere()
        s.SetRadius(2)
        s.SetCenter(0,0,0)

        c = svtk.svtkTableBasedClipDataSet()
        c.SetInputData(r)
        c.SetClipFunction(s)
        c.SetInsideOut(1)

        c.Update()

        self.assertEqual(c.GetOutput().GetNumberOfCells(), 64)

    def testStructured2D(self):
        planes = ['XY', 'XZ', 'YZ']
        expectedNCells = [42, 34, 68]
        for plane, nCells in zip(planes,expectedNCells):
            rt = svtk.svtkRTAnalyticSource()
            if plane == 'XY':
                rt.SetWholeExtent(-5, 5, -5, 5, 0, 0)
            elif plane == 'XZ':
                rt.SetWholeExtent(-5, 5, 0, 0, -5, 5)
            else:
                rt.SetWholeExtent(0, 0, -5, 5, -5, 5)
            rt.Update()
            i = rt.GetOutput()

            st = svtk.svtkStructuredGrid()
            st.SetDimensions(i.GetDimensions())

            nps = i.GetNumberOfPoints()
            ps = svtk.svtkPoints()
            ps.SetNumberOfPoints(nps)
            for idx in range(nps):
                ps.SetPoint(idx, i.GetPoint(idx))

            st.SetPoints(ps)

            cyl = svtk.svtkCylinder()
            cyl.SetRadius(2)
            cyl.SetCenter(0,0,0)
            transform = svtk.svtkTransform()
            transform.RotateWXYZ(45,20,1,10)
            cyl.SetTransform(transform)

            c = svtk.svtkTableBasedClipDataSet()
            c.SetInputData(st)
            c.SetClipFunction(cyl)
            c.SetInsideOut(1)

            c.Update()

            self.assertEqual(c.GetOutput().GetNumberOfCells(), nCells)

    def testStructured(self):
        rt = svtk.svtkRTAnalyticSource()
        rt.SetWholeExtent(-5, 5, -5, 5, -5, 5)
        rt.Update()
        i = rt.GetOutput()

        st = svtk.svtkStructuredGrid()
        st.SetDimensions(i.GetDimensions())

        nps = i.GetNumberOfPoints()
        ps = svtk.svtkPoints()
        ps.SetNumberOfPoints(nps)
        for idx in range(nps):
            ps.SetPoint(idx, i.GetPoint(idx))

        st.SetPoints(ps)

        s = svtk.svtkSphere()
        s.SetRadius(2)
        s.SetCenter(0,0,0)

        c = svtk.svtkTableBasedClipDataSet()
        c.SetInputData(st)
        c.SetClipFunction(s)
        c.SetInsideOut(1)

        c.Update()

        self.assertEqual(c.GetOutput().GetNumberOfCells(), 64)

    def testUnstructured(self):
        rt = svtk.svtkRTAnalyticSource()
        rt.SetWholeExtent(-5, 5, -5, 5, -5, 5)

        t = svtk.svtkThreshold()
        t.SetInputConnection(rt.GetOutputPort())
        t.ThresholdByUpper(-10)

        s = svtk.svtkSphere()
        s.SetRadius(2)
        s.SetCenter(0,0,0)

        c = svtk.svtkTableBasedClipDataSet()
        c.SetInputConnection(t.GetOutputPort())
        c.SetClipFunction(s)
        c.SetInsideOut(1)

        c.Update()

        self.assertEqual(c.GetOutput().GetNumberOfCells(), 64)

        eg = svtk.svtkEnSightGoldReader()
        eg.SetCaseFileName(SVTK_DATA_ROOT + "/Data/EnSight/elements.case")
        eg.Update()

        pl = svtk.svtkPlane()
        pl.SetOrigin(3.5, 3.5, 0.5)
        pl.SetNormal(0, 0, 1)

        c.SetInputConnection(eg.GetOutputPort())
        c.SetClipFunction(pl)
        c.SetInsideOut(1)

        c.Update()
        data = c.GetOutputDataObject(0).GetBlock(0)
        self.assertEqual(data.GetNumberOfCells(), 75)

        rw = svtk.svtkRenderWindow()
        ren = svtk.svtkRenderer()
        rw.AddRenderer(ren)
        mapper = svtk.svtkDataSetMapper()
        mapper.SetInputData(data)
        actor = svtk.svtkActor()
        actor.SetMapper(mapper)
        ren.AddActor(actor)
        ac = ren.GetActiveCamera()
        ac.SetPosition(-7.9, 9.7, 14.6)
        ac.SetFocalPoint(3.5, 3.5, 0.5)
        ac.SetViewUp(0.08, 0.93, -0.34)
        rw.Render()
        ren.ResetCameraClippingRange()

        rtTester = svtk.svtkTesting()
        for arg in sys.argv[1:]:
            rtTester.AddArgument(arg)
        rtTester.AddArgument("-V")
        rtTester.AddArgument("tableBasedClip.png")
        rtTester.SetRenderWindow(rw)
        rw.Render()
        rtResult = rtTester.RegressionTest(10)



if __name__ == "__main__":
    Testing.main([(TestClip, 'test')])
