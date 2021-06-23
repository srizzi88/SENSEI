#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Create the RenderWindow, Renderer, and RenderWindowInteractor
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# create pipeline
#
PIECE = 0
NUMBER_OF_PIECES = 8
reader = svtk.svtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,63,0,63,1,64)
reader.SetFilePrefix("" + str(SVTK_DATA_ROOT) + "/Data/headsq/quarter")
reader.SetDataMask(0x7fff)
reader.SetDataSpacing(1.6,1.6,1.5)
clipper = svtk.svtkImageClip()
clipper.SetInputConnection(reader.GetOutputPort())
clipper.SetOutputWholeExtent(30,36,30,36,30,36)
clipper2 = svtk.svtkImageClip()
clipper2.SetInputConnection(reader.GetOutputPort())
clipper2.SetOutputWholeExtent(30,36,30,36,30,36)
tris = svtk.svtkDataSetTriangleFilter()
tris.SetInputConnection(clipper.GetOutputPort())
tris2 = svtk.svtkDataSetTriangleFilter()
tris2.SetInputConnection(clipper2.GetOutputPort())
geom = svtk.svtkGeometryFilter()
geom.SetInputConnection(tris.GetOutputPort())
pf = svtk.svtkProgrammableFilter()
pf.SetInputConnection(tris2.GetOutputPort())
def remove_ghosts():
    input = pf.GetInput()
    output = pf.GetOutputDataObject(0)
    output.ShallowCopy(input)
    output.RemoveGhostCells()
pf.SetExecuteMethod(remove_ghosts)
edges = svtk.svtkExtractEdges()
edges.SetInputConnection(pf.GetOutputPort())
mapper1 = svtk.svtkPolyDataMapper()
mapper1.SetInputConnection(geom.GetOutputPort())
mapper1.ScalarVisibilityOn()
mapper1.SetScalarRange(0,1200)
mapper1.SetPiece(PIECE)
mapper1.SetNumberOfPieces(NUMBER_OF_PIECES)
mapper2 = svtk.svtkPolyDataMapper()
mapper2.SetInputConnection(edges.GetOutputPort())
mapper2.SetPiece(PIECE)
mapper2.SetNumberOfPieces(NUMBER_OF_PIECES)
mapper2.GetInput().RemoveGhostCells()
mapper2.SetResolveCoincidentTopologyToPolygonOffset()
mapper2.SetResolveCoincidentTopologyLineOffsetParameters(0,-7)
actor1 = svtk.svtkActor()
actor1.SetMapper(mapper1)
actor2 = svtk.svtkActor()
actor2.SetMapper(mapper2)
# add the actor to the renderer; set the size
#
ren1.AddActor(actor1)
ren1.AddActor(actor2)
renWin.SetSize(450,450)
ren1.SetBackground(1,1,1)
renWin.Render()
# render the image
#
iren.Initialize()
# prevent the tk window from showing up then start the event loop
# --- end of script --
