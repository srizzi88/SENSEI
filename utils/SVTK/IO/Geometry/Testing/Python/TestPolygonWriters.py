#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# read data
#
input = svtk.svtkPolyDataReader()
input.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/brainImageSmooth.svtk")
#
# generate vectors
clean = svtk.svtkCleanPolyData()
clean.SetInputConnection(input.GetOutputPort())
smooth = svtk.svtkWindowedSincPolyDataFilter()
smooth.SetInputConnection(clean.GetOutputPort())
smooth.GenerateErrorVectorsOn()
smooth.GenerateErrorScalarsOn()
smooth.Update()
mapper = svtk.svtkPolyDataMapper()
mapper.SetInputConnection(smooth.GetOutputPort())
mapper.SetScalarRange(smooth.GetOutput().GetScalarRange())
brain = svtk.svtkActor()
brain.SetMapper(mapper)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(brain)
renWin.SetSize(320,240)
ren1.GetActiveCamera().SetPosition(149.653,-65.3464,96.0401)
ren1.GetActiveCamera().SetFocalPoint(146.003,22.3839,0.260541)
ren1.GetActiveCamera().SetViewAngle(30)
ren1.GetActiveCamera().SetViewUp(-0.255578,-0.717754,-0.647695)
ren1.GetActiveCamera().SetClippingRange(79.2526,194.052)
iren.Initialize()
renWin.Render()
# render the image
#
# prevent the tk window from showing up then start the event loop
#
# If the current directory is writable, then test the witers
#
if (catch.catch(globals(),"""channel = open("test.tmp", "w")""") == 0):
    channel.close()
    file.delete("-force", "test.tmp")
    #
    #
    # test the writers
    dsw = svtk.svtkDataSetWriter()
    dsw.SetInputConnection(smooth.GetOutputPort())
    dsw.SetFileName("brain.dsw")
    dsw.Write()
    file.delete("-force", "brain.dsw")
    pdw = svtk.svtkPolyDataWriter()
    pdw.SetInputConnection(smooth.GetOutputPort())
    pdw.SetFileName("brain.pdw")
    pdw.Write()
    file.delete("-force", "brain.pdw")
    if (info.command(globals(), locals(),  "svtkIVWriter") != ""):
        iv = svtk.svtkIVWriter()
        iv.SetInputConnection(smooth.GetOutputPort())
        iv.SetFileName("brain.iv")
        iv.Write()
        file.delete("-force", "brain.iv")
        pass
    #
    # the next writers only handle triangles
    triangles = svtk.svtkTriangleFilter()
    triangles.SetInputConnection(smooth.GetOutputPort())
    if (info.command(globals(), locals(),  "svtkIVWriter") != ""):
        iv2 = svtk.svtkIVWriter()
        iv2.SetInputConnection(triangles.GetOutputPort())
        iv2.SetFileName("brain2.iv")
        iv2.Write()
        file.delete("-force", "brain2.iv")
        pass
    if (info.command(globals(), locals(),  "svtkIVWriter") != ""):
        edges = svtk.svtkExtractEdges()
        edges.SetInputConnection(triangles.GetOutputPort())
        iv3 = svtk.svtkIVWriter()
        iv3.SetInputConnection(edges.GetOutputPort())
        iv3.SetFileName("brain3.iv")
        iv3.Write()
        file.delete("-force", "brain3.iv")
        pass
    byu = svtk.svtkBYUWriter()
    byu.SetGeometryFileName("brain.g")
    byu.SetScalarFileName("brain.s")
    byu.SetDisplacementFileName("brain.d")
    byu.SetInputConnection(triangles.GetOutputPort())
    byu.Write()
    file.delete("-force", "brain.g")
    file.delete("-force", "brain.s")
    file.delete("-force", "brain.d")
    mcubes = svtk.svtkMCubesWriter()
    mcubes.SetInputConnection(triangles.GetOutputPort())
    mcubes.SetFileName("brain.tri")
    mcubes.SetLimitsFileName("brain.lim")
    mcubes.Write()
    file.delete("-force", "brain.lim")
    file.delete("-force", "brain.tri")
    stl = svtk.svtkSTLWriter()
    stl.SetInputConnection(triangles.GetOutputPort())
    stl.SetFileName("brain.stl")
    stl.Write()
    file.delete("-force", "brain.stl")
    stlBinary = svtk.svtkSTLWriter()
    stlBinary.SetInputConnection(triangles.GetOutputPort())
    stlBinary.SetFileName("brainBinary.stl")
    stlBinary.SetFileType(2)
    stlBinary.Write()
    file.delete("-force", "brainBinary.stl")
    cgm = svtk.svtkCGMWriter()
    cgm.SetInputConnection(triangles.GetOutputPort())
    cgm.SetFileName("brain.cgm")
    cgm.Write()
    file.delete("-force", "brain.cgm")
    pass
# --- end of script --
