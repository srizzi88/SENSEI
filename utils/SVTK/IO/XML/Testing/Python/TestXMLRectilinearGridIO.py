#!/usr/bin/env python

import os
import svtk
from svtk.util.misc import svtkGetDataRoot
from svtk.util.misc import svtkGetTempDir

SVTK_DATA_ROOT = svtkGetDataRoot()

SVTK_TEMP_DIR = svtkGetTempDir()
file0 = SVTK_TEMP_DIR + '/rgFile0.vtr'
file1 = SVTK_TEMP_DIR + '/rgFile1.vtr'
file2 = SVTK_TEMP_DIR + '/rgFile2.vtr'

# read in some grid data
gridReader = svtk.svtkRectilinearGridReader()
gridReader.SetFileName(SVTK_DATA_ROOT + "/Data/RectGrid2.svtk")
gridReader.Update()

# extract to reduce extents of grid
extract = svtk.svtkExtractRectilinearGrid()
extract.SetInputConnection(gridReader.GetOutputPort())
extract.SetVOI(0, 23, 0, 32, 0, 10)
extract.Update()

# write just a piece (extracted piece) as well as the whole thing
rgWriter = svtk.svtkXMLRectilinearGridWriter()
rgWriter.SetFileName(file0)
rgWriter.SetInputConnection(extract.GetOutputPort())
rgWriter.SetDataModeToAscii()
rgWriter.Write()

rgWriter.SetFileName(file1)
rgWriter.SetInputConnection(gridReader.GetOutputPort())
rgWriter.SetDataModeToAppended()
rgWriter.SetNumberOfPieces(2)
rgWriter.Write()

rgWriter.SetFileName(file2)
rgWriter.SetDataModeToBinary()
rgWriter.SetWriteExtent(3, 46, 6, 32, 1, 5)
rgWriter.SetCompressor(None)
if rgWriter.GetByteOrder():
    rgWriter.SetByteOrder(0)
else:
    rgWriter.SetByteOrder(1)
rgWriter.Write()

# read the extracted grid
reader = svtk.svtkXMLRectilinearGridReader()
reader.SetFileName(file0)
reader.WholeSlicesOff()
reader.Update()

rg0 = svtk.svtkRectilinearGrid()
rg0.DeepCopy(reader.GetOutput())
mapper0 = svtk.svtkDataSetMapper()
mapper0.SetInputData(rg0)

actor0 = svtk.svtkActor()
actor0.SetMapper(mapper0)

# read the whole grid
reader.SetFileName(file1)
reader.WholeSlicesOn()
reader.Update()

rg1 = svtk.svtkRectilinearGrid()
rg1.DeepCopy(reader.GetOutput())
mapper1 = svtk.svtkDataSetMapper()
mapper1.SetInputData(rg1)

actor1 = svtk.svtkActor()
actor1.SetMapper(mapper1)
actor1.SetPosition(-1.5, 3, 0)

# read the partially written grid
reader.SetFileName(file2)
reader.Update()

mapper2 = svtk.svtkDataSetMapper()
mapper2.SetInputConnection(reader.GetOutputPort())

actor2 = svtk.svtkActor()
actor2.SetMapper(mapper2)
actor2.SetPosition(1.5, 3, 0)

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

renWin.SetSize(300, 300)
renWin.Render()

os.remove(file0)
os.remove(file1)
os.remove(file2)
