#!/usr/bin/env python

# This example shows how to combine data from both the imaging and
# graphics pipelines. The svtkMergeFilter is used to merge the data
# from each together.

import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Read in an image and compute a luminance value. The image is
# extracted as a set of polygons (svtkImageDataGeometryFilter). We then
# will warp the plane using the scalar (luminance) values.
reader = svtk.svtkBMPReader()
reader.SetFileName(SVTK_DATA_ROOT + "/Data/masonry.bmp")
luminance = svtk.svtkImageLuminance()
luminance.SetInputConnection(reader.GetOutputPort())
geometry = svtk.svtkImageDataGeometryFilter()
geometry.SetInputConnection(luminance.GetOutputPort())
warp = svtk.svtkWarpScalar()
warp.SetInputConnection(geometry.GetOutputPort())
warp.SetScaleFactor(-0.1)

# Use svtkMergeFilter to combine the original image with the warped
# geometry.
merge = svtk.svtkMergeFilter()
merge.SetGeometryConnection(warp.GetOutputPort())
merge.SetScalarsConnection(reader.GetOutputPort())
mapper = svtk.svtkDataSetMapper()
mapper.SetInputConnection(merge.GetOutputPort())
mapper.SetScalarRange(0, 255)
actor = svtk.svtkActor()
actor.SetMapper(mapper)

# Create renderer stuff
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(actor)
ren.ResetCamera()
ren.GetActiveCamera().Azimuth(20)
ren.GetActiveCamera().Elevation(30)
ren.SetBackground(0.1, 0.2, 0.4)
ren.ResetCameraClippingRange()

renWin.SetSize(250, 250)

cam1 = ren.GetActiveCamera()
cam1.Zoom(1.4)

iren.Initialize()
renWin.Render()
iren.Start()
