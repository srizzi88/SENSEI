#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# create reader and extract the velocity and temperature
reader = svtk.svtkPNGReader()
reader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/svtk.png")
quant = svtk.svtkImageQuantizeRGBToIndex()
quant.SetInputConnection(reader.GetOutputPort())
quant.SetNumberOfColors(32)
i2pd = svtk.svtkImageToPolyDataFilter()
i2pd.SetInputConnection(quant.GetOutputPort())
i2pd.SetLookupTable(quant.GetLookupTable())
i2pd.SetColorModeToLUT()
i2pd.SetOutputStyleToPolygonalize()
i2pd.SetError(0)
i2pd.DecimationOn()
i2pd.SetDecimationError(0.0)
i2pd.SetSubImageSize(25)
#Need a triangle filter because the polygons are complex and concave
tf = svtk.svtkTriangleFilter()
tf.SetInputConnection(i2pd.GetOutputPort())
mapper = svtk.svtkPolyDataMapper()
mapper.SetInputConnection(tf.GetOutputPort())
actor = svtk.svtkActor()
actor.SetMapper(mapper)
# Create graphics stuff
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
ren1.AddActor(actor)
ren1.SetBackground(1,1,1)
renWin.SetSize(300,250)
acamera = svtk.svtkCamera()
acamera.SetClippingRange(343.331,821.78)
acamera.SetPosition(-139.802,-85.6604,437.485)
acamera.SetFocalPoint(117.424,106.656,-14.6)
acamera.SetViewUp(0.430481,0.716032,0.549532)
acamera.SetViewAngle(30)
ren1.SetActiveCamera(acamera)
iren.Initialize()
# prevent the tk window from showing up then start the event loop
# --- end of script --
