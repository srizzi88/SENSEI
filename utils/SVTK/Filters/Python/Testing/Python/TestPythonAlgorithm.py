import svtk
from svtk.util import svtkAlgorithm as vta
from svtk.test import Testing

class TestPythonAlgorithm(Testing.svtkTest):
    def testSource(self):
        class MyAlgorithm(vta.SVTKAlgorithm):
            def __init__(self):
                vta.SVTKAlgorithm.__init__(self, nInputPorts=0, outputType='svtkImageData')
                self.Wavelet = svtk.svtkRTAnalyticSource()

            def RequestInformation(self, svtkself, request, inInfo, outInfo):
                self.Wavelet.UpdateInformation()
                wOutInfo = self.Wavelet.GetOutputInformation(0)
                svtkSDDP = svtk.svtkStreamingDemandDrivenPipeline
                outInfo.GetInformationObject(0).Set(svtkSDDP.WHOLE_EXTENT(), wOutInfo.Get(svtkSDDP.WHOLE_EXTENT()), 6)
                return 1

            def RequestData(self, svtkself, request, inInfo, outInfo):
                self.Wavelet.Update()
                out = self.GetOutputData(outInfo, 0)
                out.ShallowCopy(self.Wavelet.GetOutput())
                return 1

        ex = svtk.svtkPythonAlgorithm()
        ex.SetPythonObject(MyAlgorithm())

        ex.Update()

        w = svtk.svtkRTAnalyticSource()
        w.Update()

        output = ex.GetOutputDataObject(0)
        self.assertEqual(output.GetPointData().GetScalars().GetRange(),\
            w.GetOutput().GetPointData().GetScalars().GetRange())
        svtkSDDP = svtk.svtkStreamingDemandDrivenPipeline
        self.assertEqual(ex.GetOutputInformation(0).Get(svtkSDDP.WHOLE_EXTENT()),\
            w.GetOutputInformation(0).Get(svtkSDDP.WHOLE_EXTENT()))

    def testSource2(self):
        class MyAlgorithm(vta.SVTKPythonAlgorithmBase):
            def __init__(self):
                vta.SVTKPythonAlgorithmBase.__init__(self, nInputPorts=0, outputType='svtkImageData')
                self.Wavelet = svtk.svtkRTAnalyticSource()

            def RequestInformation(self, request, inInfo, outInfo):
                self.Wavelet.UpdateInformation()
                wOutInfo = self.Wavelet.GetOutputInformation(0)
                svtkSDDP = svtk.svtkStreamingDemandDrivenPipeline
                outInfo.GetInformationObject(0).Set(
                    svtkSDDP.WHOLE_EXTENT(), wOutInfo.Get(svtkSDDP.WHOLE_EXTENT()), 6)
                return 1

            def RequestData(self, request, inInfo, outInfo):
                self.Wavelet.Update()
                out = svtk.svtkImageData.GetData(outInfo)
                out.ShallowCopy(self.Wavelet.GetOutput())
                return 1

        ex = MyAlgorithm()

        ex.Update()

        w = svtk.svtkRTAnalyticSource()
        w.Update()

        output = ex.GetOutputDataObject(0)
        self.assertEqual(output.GetPointData().GetScalars().GetRange(),\
            w.GetOutput().GetPointData().GetScalars().GetRange())
        svtkSDDP = svtk.svtkStreamingDemandDrivenPipeline
        self.assertEqual(ex.GetOutputInformation(0).Get(svtkSDDP.WHOLE_EXTENT()),\
            w.GetOutputInformation(0).Get(svtkSDDP.WHOLE_EXTENT()))

    def testFilter(self):
        class MyAlgorithm(vta.SVTKAlgorithm):
            def RequestData(self, svtkself, request, inInfo, outInfo):
                inp = self.GetInputData(inInfo, 0, 0)
                out = self.GetOutputData(outInfo, 0)
                out.ShallowCopy(inp)
                return 1

        sphere = svtk.svtkSphereSource()

        ex = svtk.svtkPythonAlgorithm()
        ex.SetPythonObject(MyAlgorithm())

        ex.SetInputConnection(sphere.GetOutputPort())

        ex.Update()

        output = ex.GetOutputDataObject(0)
        ncells = output.GetNumberOfCells()
        self.assertNotEqual(ncells, 0)
        self.assertEqual(ncells, sphere.GetOutput().GetNumberOfCells())
        self.assertEqual(output.GetBounds(), sphere.GetOutput().GetBounds())

    def testFilter2(self):
        class MyAlgorithm(vta.SVTKPythonAlgorithmBase):
            def __init__(self):
                vta.SVTKPythonAlgorithmBase.__init__(self)
            def RequestData(self, request, inInfo, outInfo):
                inp = self.GetInputData(inInfo, 0, 0)
                out = self.GetOutputData(outInfo, 0)
                out.ShallowCopy(inp)
                return 1

        sphere = svtk.svtkSphereSource()

        ex = MyAlgorithm()
        ex.SetInputConnection(sphere.GetOutputPort())

        ex.Update()

        output = ex.GetOutputDataObject(0)
        ncells = output.GetNumberOfCells()
        self.assertNotEqual(ncells, 0)
        self.assertEqual(ncells, sphere.GetOutput().GetNumberOfCells())
        self.assertEqual(output.GetBounds(), sphere.GetOutput().GetBounds())

if __name__ == "__main__":
    Testing.main([(TestPythonAlgorithm, 'test')])
