#!/usr/bin/env python
import os
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Now create the RenderWindow, Renderer and Interactor
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

sphereModel = svtk.svtkSphereSource()
sphereModel.SetThetaResolution(10)
sphereModel.SetPhiResolution(10)

voxelModel = svtk.svtkVoxelModeller()
voxelModel.SetInputConnection(sphereModel.GetOutputPort())
voxelModel.SetSampleDimensions(21, 21, 21)
voxelModel.SetModelBounds(-1.5, 1.5, -1.5, 1.5, -1.5, 1.5)
voxelModel.SetScalarTypeToBit()
voxelModel.SetForegroundValue(1)
voxelModel.SetBackgroundValue(0)

#
# If the current directory is writable, then test the writers
#
try:
    channel = open("voxelModel.svtk", "wb")
    channel.close()

    aWriter = svtk.svtkDataSetWriter()
    aWriter.SetFileName("voxelModel.svtk")
    aWriter.SetInputConnection(voxelModel.GetOutputPort())
    aWriter.Update()

    aReader = svtk.svtkDataSetReader()
    aReader.SetFileName("voxelModel.svtk")

    voxelSurface = svtk.svtkContourFilter()
    voxelSurface.SetInputConnection(aReader.GetOutputPort())
    voxelSurface.SetValue(0, .999)

    voxelMapper = svtk.svtkPolyDataMapper()
    voxelMapper.SetInputConnection(voxelSurface.GetOutputPort())

    voxelActor = svtk.svtkActor()
    voxelActor.SetMapper(voxelMapper)

    sphereMapper = svtk.svtkPolyDataMapper()
    sphereMapper.SetInputConnection(sphereModel.GetOutputPort())

    sphereActor = svtk.svtkActor()
    sphereActor.SetMapper(sphereMapper)

    ren1.AddActor(sphereActor)
    ren1.AddActor(voxelActor)
    ren1.SetBackground(.1, .2, .4)

    renWin.SetSize(256, 256)

    ren1.ResetCamera()
    ren1.GetActiveCamera().SetViewUp(0, -1, 0)
    ren1.GetActiveCamera().Azimuth(180)
    ren1.GetActiveCamera().Dolly(1.75)
    ren1.ResetCameraClippingRange()

    # cleanup
    #
    try:
        os.remove("voxelModel.svtk")
    except OSError:
        pass

    iren.Initialize()
    # render the image
#    iren.Start()

except IOError:
    print("Unable to test the writer/reader.")
