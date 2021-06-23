#!/usr/bin/env python

import os
import svtk
from svtk.util.misc import svtkGetDataRoot
from svtk.util.misc import svtkGetTempDir

SVTK_DATA_ROOT = svtkGetDataRoot()
SVTK_TEMP_DIR = svtkGetTempDir()

file0 = SVTK_TEMP_DIR + '/idFile0.vtp'
file1 = SVTK_TEMP_DIR + '/idFile1.vtp'
file2 = SVTK_TEMP_DIR + '/idFile2.vtp'

# read in some poly data
pdReader = svtk.svtkPolyDataReader()
pdReader.SetFileName(SVTK_DATA_ROOT + "/Data/fran_cut.svtk")
pdReader.Update()

extract = svtk.svtkExtractPolyDataPiece()
extract.SetInputConnection(pdReader.GetOutputPort())

# write various versions
pdWriter = svtk.svtkXMLPolyDataWriter()
pdWriter.SetFileName(file0)
pdWriter.SetDataModeToAscii()
pdWriter.SetInputConnection(pdReader.GetOutputPort())
pdWriter.Write()

pdWriter.SetFileName(file1)
pdWriter.SetInputConnection(extract.GetOutputPort())
pdWriter.SetDataModeToAppended()
pdWriter.SetNumberOfPieces(2)
pdWriter.Write()

pdWriter.SetFileName(file2)
pdWriter.SetDataModeToBinary()
pdWriter.SetGhostLevel(3)
pdWriter.Write()


# read the ASCII version
reader = svtk.svtkXMLPolyDataReader()
reader.SetFileName(file0)
reader.Update()

pd0 = svtk.svtkPolyData()
pd0.DeepCopy(reader.GetOutput())
mapper0 = svtk.svtkPolyDataMapper()
mapper0.SetInputData(pd0)

actor0 = svtk.svtkActor()
actor0.SetMapper(mapper0)
actor0.SetPosition(0, .15, 0)


# read appended piece 0
reader.SetFileName(file1)

mapper1 = svtk.svtkPolyDataMapper()
mapper1.SetInputConnection(reader.GetOutputPort())
mapper1.SetPiece(0)
mapper1.SetNumberOfPieces(2)

actor1 = svtk.svtkActor()
actor1.SetMapper(mapper1)


# read binary piece 0 (with ghost level)
reader2 = svtk.svtkXMLPolyDataReader()
reader2.SetFileName(file2)

mapper2 = svtk.svtkPolyDataMapper()
mapper2.SetInputConnection(reader2.GetOutputPort())
mapper2.SetPiece(0)
mapper2.SetNumberOfPieces(2)

actor2 = svtk.svtkActor()
actor2.SetMapper(mapper2)
actor2.SetPosition(0, 0, 0.1)

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

ren.GetActiveCamera().SetPosition(0.514096, -0.14323, -0.441177)
ren.GetActiveCamera().SetFocalPoint(0.0528, -0.0780001, -0.0379661)
renWin.SetSize(300, 300)
renWin.Render()

os.remove(file0)
os.remove(file1)
os.remove(file2)
