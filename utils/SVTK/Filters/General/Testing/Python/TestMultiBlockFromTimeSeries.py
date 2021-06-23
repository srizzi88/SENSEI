#!/usr/bin/python
import svtk
from svtk.util.svtkAlgorithm import SVTKPythonAlgorithmBase

class MovingSphereSource(SVTKPythonAlgorithmBase):
    def __init__(self):
        SVTKPythonAlgorithmBase.__init__(self,
            nInputPorts=0,
            nOutputPorts=1, outputType='svtkPolyData')

    def RequestInformation(self, request, inInfo, outInfo):
        info = outInfo.GetInformationObject(0)
        t = range(0, 10)
        info.Set(svtk.svtkStreamingDemandDrivenPipeline.TIME_STEPS(), t, len(t))
        info.Set(svtk.svtkStreamingDemandDrivenPipeline.TIME_RANGE(), [t[0], t[-1]], 2)
        return 1

    def RequestData(self, request, inInfo, outInfo):
        info = outInfo.GetInformationObject(0)
        output = svtk.svtkPolyData.GetData(outInfo)

        t = info.Get(svtk.svtkStreamingDemandDrivenPipeline.UPDATE_TIME_STEP())

        sphere = svtk.svtkSphereSource()
        sphere.SetCenter(t * 2, 0, 0)
        sphere.Update()

        output.ShallowCopy(sphere.GetOutput())
        return 1

source = MovingSphereSource()
source.Update()

group = svtk.svtkMultiBlockFromTimeSeriesFilter()
group.SetInputConnection(source.GetOutputPort())
group.Update()

ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

mapper = svtk.svtkCompositePolyDataMapper2()
mapper.SetInputConnection(group.GetOutputPort())
mapper.Update()

actor = svtk.svtkActor()
actor.SetMapper(mapper)
ren1.AddActor(actor)

ren1.ResetCamera()
renWin.SetSize(300, 300)

renWin.Render()

# render the image
#
iren.Initialize()
#iren.Start()
