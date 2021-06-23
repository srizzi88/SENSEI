#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# create pipeline
#
v16 = svtk.svtkVolume16Reader()
v16.SetDataDimensions(64,64)
v16.GetOutput().SetOrigin(0.0,0.0,0.0)
v16.SetDataByteOrderToLittleEndian()
v16.SetFilePrefix("" + str(SVTK_DATA_ROOT) + "/Data/headsq/quarter")
v16.SetImageRange(45,45)
v16.SetDataSpacing(3.2,3.2,1.5)
v16.Update()
# do the pixel clipping
clip = svtk.svtkClipDataSet()
clip.SetInputConnection(v16.GetOutputPort())
clip.SetValue(1000)
clipMapper = svtk.svtkDataSetMapper()
clipMapper.SetInputConnection(clip.GetOutputPort())
clipMapper.ScalarVisibilityOff()
clipActor = svtk.svtkActor()
clipActor.SetMapper(clipMapper)
# put an outline around the data
outline = svtk.svtkOutlineFilter()
outline.SetInputConnection(v16.GetOutputPort())
outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.VisibilityOff()
# Create the RenderWindow, Renderer and both Actors
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(outlineActor)
ren1.AddActor(clipActor)
ren1.SetBackground(0,0,0)
renWin.SetSize(200,200)
iren.Initialize()
# render the image
#
# prevent the tk window from showing up then start the event loop
# --- end of script --
