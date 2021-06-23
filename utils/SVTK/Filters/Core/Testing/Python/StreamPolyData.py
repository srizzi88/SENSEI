#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

def GetRGBColor(colorName):
    '''
        Return the red, green and blue components for a
        color as doubles.
    '''
    rgb = [0.0, 0.0, 0.0]  # black
    svtk.svtkNamedColors().GetColorRGB(colorName, rgb)
    return rgb

NUMBER_OF_PIECES = 5

# Generate implicit model of a sphere
#

# Create renderer stuff
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create pipeline that handles ghost cells
sphere = svtk.svtkSphereSource()
sphere.SetRadius(3)
sphere.SetPhiResolution(100)
sphere.SetThetaResolution(150)

# Just playing with an alternative that is not currently used.
def NotUsed ():
    # This filter actually spoils the example because it asks
    # for the whole input.
    # The only reason it is here is because sphere complains
    # it cannot generate ghost cells.
    svtkExtractPolyDataPiece.piece()
    piece.SetInputConnection(sphere.GetOutputPort())
    # purposely put seams in here.
    piece.CreateGhostCellsOff()
    # purposely put seams in here.

    pdn = svtk.svtkPolyDataNormals()
    pdn.SetInputConnection(piece.GetOutputPort())

# Just playing with an alternative that is not currently used.
deci = svtk.svtkDecimatePro()
deci.SetInputConnection(sphere.GetOutputPort())
# this did not remove seams as I thought it would
deci.BoundaryVertexDeletionOff()
# deci.PreserveTopologyOn()

# Since quadric Clustering does not handle borders properly yet,
# the pieces will have dramatic "seams"
q = svtk.svtkQuadricClustering()
q.SetInputConnection(sphere.GetOutputPort())
q.SetNumberOfXDivisions(5)
q.SetNumberOfYDivisions(5)
q.SetNumberOfZDivisions(10)
q.UseInputPointsOn()

streamer = svtk.svtkPolyDataStreamer()
# streamer.SetInputConnection(deci.GetOutputPort())
streamer.SetInputConnection(q.GetOutputPort())
# streamer.SetInputConnection(pdn.GetOutputPort())
streamer.SetNumberOfStreamDivisions(NUMBER_OF_PIECES)

mapper = svtk.svtkPolyDataMapper()
mapper.SetInputConnection(streamer.GetOutputPort())
mapper.ScalarVisibilityOff()
mapper.SetPiece(0)
mapper.SetNumberOfPieces(2)

actor = svtk.svtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetColor(GetRGBColor('english_red'))

# Add the actors to the renderer, set the background and size
#
ren1.GetActiveCamera().SetPosition(5, 5, 10)
ren1.GetActiveCamera().SetFocalPoint(0, 0, 0)
ren1.AddActor(actor)
ren1.SetBackground(1, 1, 1)

renWin.SetSize(300, 300)

iren.Initialize()
# render the image
#iren.Start()
