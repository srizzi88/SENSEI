#!/usr/bin/env python

import os
import svtk
from svtk.util.misc import svtkGetDataRoot
from svtk.util.misc import svtkGetTempDir

SVTK_DATA_ROOT = svtkGetDataRoot()
SVTK_TEMP_DIR = svtkGetTempDir()

file0 = SVTK_TEMP_DIR + '/ugFile0.vtu'
file1 = SVTK_TEMP_DIR + '/ugFile1.vtu'
file2 = SVTK_TEMP_DIR + '/ugFile2.vtu'

# read in some unstructured grid data
ugReader = svtk.svtkUnstructuredGridReader()
ugReader.SetFileName(SVTK_DATA_ROOT + "/Data/blow.svtk")
ugReader.SetScalarsName("thickness9")
ugReader.SetVectorsName("displacement9")

extract = svtk.svtkExtractUnstructuredGridPiece()
extract.SetInputConnection(ugReader.GetOutputPort())

# write various versions
ugWriter = svtk.svtkXMLUnstructuredGridWriter()
ugWriter.SetFileName(file0)
ugWriter.SetDataModeToAscii()
ugWriter.SetInputConnection(ugReader.GetOutputPort())
ugWriter.Write()

ugWriter.SetFileName(file1)
ugWriter.SetInputConnection(extract.GetOutputPort())
ugWriter.SetDataModeToAppended()
ugWriter.SetNumberOfPieces(2)
ugWriter.Write()

ugWriter.SetFileName(file2)
ugWriter.SetDataModeToBinary()
ugWriter.SetGhostLevel(2)
ugWriter.Write()


# read the ASCII version
reader = svtk.svtkXMLUnstructuredGridReader()
reader.SetFileName(file0)
reader.Update()

ug0 = svtk.svtkUnstructuredGrid()
ug0.DeepCopy(reader.GetOutput())
sF = svtk.svtkDataSetSurfaceFilter()
sF.SetInputData(ug0)

mapper0 = svtk.svtkPolyDataMapper()
mapper0.SetInputConnection(sF.GetOutputPort())

actor0 = svtk.svtkActor()
actor0.SetMapper(mapper0)
actor0.SetPosition(0, 40, 20)


# read appended piece 0
reader.SetFileName(file1)

sF1 = svtk.svtkDataSetSurfaceFilter()
sF1.SetInputConnection(reader.GetOutputPort())

mapper1 = svtk.svtkPolyDataMapper()
mapper1.SetInputConnection(sF1.GetOutputPort())
mapper1.SetPiece(1)
mapper1.SetNumberOfPieces(2)

actor1 = svtk.svtkActor()
actor1.SetMapper(mapper1)


# read binary piece 0 (with ghost level)
reader2 = svtk.svtkXMLUnstructuredGridReader()
reader2.SetFileName(file2)

sF2 = svtk.svtkDataSetSurfaceFilter()
sF2.SetInputConnection(reader2.GetOutputPort())

mapper2 = svtk.svtkPolyDataMapper()
mapper2.SetInputConnection(sF2.GetOutputPort())
mapper2.SetPiece(1)
mapper2.SetNumberOfPieces(2)
mapper2.SetGhostLevel(2)

actor2 = svtk.svtkActor()
actor2.SetMapper(mapper2)
actor2.SetPosition(0, 0, 30)

# Create the RenderWindow, Renderer and both Actors
#
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(actor0)
ren.AddActor(actor1)
ren.AddActor(actor2)

ren.ResetCamera()
ren.GetActiveCamera().SetPosition(180, 55, 65)
ren.GetActiveCamera().SetFocalPoint(3.5, 32, 15)
renWin.SetSize(300, 300)
renWin.Render()

#os.remove(file0)
#os.remove(file1)
#os.remove(file2)
