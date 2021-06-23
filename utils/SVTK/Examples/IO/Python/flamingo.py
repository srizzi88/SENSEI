#!/usr/bin/env python

# This example demonstrates the use of svtk3DSImporter.
# svtk3DSImporter is used to load 3D Studio files.  Unlike writers,
# importers can load scenes (data as well as lights, cameras, actors
# etc.). Importers will either generate an instance of svtkRenderWindow
# and/or svtkRenderer or will use the ones you specify.

import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()


# Create the importer and read a file
importer = svtk.svtk3DSImporter()
importer.ComputeNormalsOn()
importer.SetFileName(SVTK_DATA_ROOT + "/Data/iflamigm.3ds")
importer.Read()

# Here we let the importer create a renderer and a render window for
# us. We could have also create and assigned those ourselves like so:
# renWin = svtk.svtkRenderWindow()
# importer.SetRenderWindow(renWin)

# Assign an interactor.
# We have to ask the importer for it's render window.
renWin = importer.GetRenderWindow()
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Set the render window's size
renWin.SetSize(300, 300)

# Set some properties on the renderer.
# We have to ask the importer for it's renderer.
ren = importer.GetRenderer()
ren.SetBackground(0.1, 0.2, 0.4)

# Position the camera:
# change view up to +z
camera = ren.GetActiveCamera()
camera.SetPosition(0, 1, 0)
camera.SetFocalPoint(0, 0, 0)
camera.SetViewUp(0, 0, 1)
# let the renderer compute good position and focal point
ren.ResetCamera()
camera.Dolly(1.4)
ren.ResetCameraClippingRange()

iren.Initialize()
renWin.Render()
iren.Start()
