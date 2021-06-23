#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Derived from Cursor3D.  This script increases the coverage of the
# svtkImageInplaceFilter super class.
# global values
CURSOR_X = 20
CURSOR_Y = 20
CURSOR_Z = 20
IMAGE_MAG_X = 2
IMAGE_MAG_Y = 2
IMAGE_MAG_Z = 1

# pipeline stuff
reader = svtk.svtkSLCReader()
reader.SetFileName(SVTK_DATA_ROOT + "/Data/nut.slc")

# make the image a little bigger
magnify1 = svtk.svtkImageMagnify()
magnify1.SetInputConnection(reader.GetOutputPort())
magnify1.SetMagnificationFactors(IMAGE_MAG_X, IMAGE_MAG_Y, IMAGE_MAG_Z)
magnify1.ReleaseDataFlagOn()

magnify2 = svtk.svtkImageMagnify()
magnify2.SetInputConnection(reader.GetOutputPort())
magnify2.SetMagnificationFactors(IMAGE_MAG_X, IMAGE_MAG_Y, IMAGE_MAG_Z)
magnify2.ReleaseDataFlagOn()

# a filter that does in place processing (magnify ReleaseDataFlagOn)
cursor = svtk.svtkImageCursor3D()
cursor.SetInputConnection(magnify1.GetOutputPort())
cursor.SetCursorPosition(CURSOR_X * IMAGE_MAG_X,
                          CURSOR_Y * IMAGE_MAG_Y,
                          CURSOR_Z * IMAGE_MAG_Z)
cursor.SetCursorValue(255)
cursor.SetCursorRadius(50 * IMAGE_MAG_X)

# stream to increase coverage of in place filter.
# put the two together in one image
imageAppend = svtk.svtkImageAppend()
imageAppend.SetAppendAxis(0)
imageAppend.AddInputConnection(magnify2.GetOutputPort())
imageAppend.AddInputConnection(cursor.GetOutputPort())

viewer = svtk.svtkImageViewer()
viewer.SetInputConnection(imageAppend.GetOutputPort())
viewer.SetZSlice(CURSOR_Z * IMAGE_MAG_Z)
viewer.SetColorWindow(200)
viewer.SetColorLevel(80)
# viewer DebugOn
viewer.Render()
viewer.SetPosition(50, 50)
# make interface
viewer.Render()
