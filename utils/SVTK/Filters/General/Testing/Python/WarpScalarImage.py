#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()


# create a rendering window
renWin = svtk.svtkRenderWindow()
renWin.SetMultiSamples(0)

renWin.SetSize(200, 200)

iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

wavelet = svtk.svtkRTAnalyticSource()
wavelet.SetWholeExtent(-100, 100, -100, 100, 0, 0)
wavelet.SetCenter(0, 0, 0)
wavelet.SetMaximum(255)
wavelet.SetStandardDeviation(.5)
wavelet.SetXFreq(60)
wavelet.SetYFreq(30)
wavelet.SetZFreq(40)
wavelet.SetXMag(10)
wavelet.SetYMag(18)
wavelet.SetZMag(5)
wavelet.SetSubsampleRate(1)

warp = svtk.svtkWarpScalar()
warp.SetInputConnection(wavelet.GetOutputPort())

mapper = svtk.svtkDataSetMapper()
mapper.SetInputConnection(warp.GetOutputPort())
mapper.SetScalarRange(75, 290)

actor = svtk.svtkActor()
actor.SetMapper(mapper)

renderer = svtk.svtkRenderer()
renderer.AddActor(actor)
renderer.ResetCamera()
renderer.GetActiveCamera().Elevation(-10)

renWin.AddRenderer(renderer)

# render the image
#
iren.Initialize()
#iren.Start()
