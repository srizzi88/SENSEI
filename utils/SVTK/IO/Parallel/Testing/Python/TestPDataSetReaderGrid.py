#!/usr/bin/env python


def DoPlot3DReaderTests(reader):
    # Ensure disable function works.
    reader.RemoveAllFunctions()
    reader.AddFunction(211) # vorticity magnitude.
    reader.Update()
    if reader.GetOutput().GetBlock(0).GetPointData().GetArray("VorticityMagnitude") is None:
        print("Failed to read `VorticityMagnitude`")
        sys.exit(1)
    if reader.GetOutput().GetBlock(0).GetPointData().GetArray("Velocity") is None:
        print("Failed to read `Velocity` to compute `VorticityMagnitude`")
        sys.exit(1)
    reader.RemoveAllFunctions()
    reader.Update()
    if reader.GetOutput().GetBlock(0).GetPointData().GetArray("VorticityMagnitude") is not None:
        print("Failed to not-read `VorticityMagnitude`")
        sys.exit(1)

    # Let's ensure intermediate results can be dropped.
    reader.PreserveIntermediateFunctionsOff()
    reader.AddFunction(211) # vorticity magnitude.
    reader.Update()
    if reader.GetOutput().GetBlock(0).GetPointData().GetArray("Velocity") is not None:
        print("PreserveIntermediateFunctionsOff is not working as expected.")
        sys.exit(1)

# Create the RenderWindow, Renderer and both Actors
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
#
# If the current directory is writable, then test the witers
#
if (catch.catch(globals(),"""channel = open("test.tmp", "w")""") == 0):
    channel.close()
    file.delete("-force", "test.tmp")
    # ====== Structured Grid ======
    # First save out a grid in parallel form.
    reader = svtk.svtkMultiBlockPLOT3DReader()
    reader.SetXYZFileName("" + str(SVTK_DATA_ROOT) + "/Data/combxyz.bin")
    reader.SetQFileName("" + str(SVTK_DATA_ROOT) + "/Data/combq.bin")
    reader.Update()

    # before we continue on with the test, let's quickly do some
    # svtkMultiBlockPLOT3DReader option tests.
    DoPlot3DReaderTests(reader)

    contr = svtk.svtkDummyController()
    extract = svtk.svtkTransmitStructuredDataPiece()
    extract.SetController(contr)
    extract.SetInputData(reader.GetOutput().GetBlock(0))
    writer = svtk.svtkPDataSetWriter()
    writer.SetFileName("comb.psvtk")
    writer.SetInputConnection(extract.GetOutputPort())
    writer.SetNumberOfPieces(4)
    writer.Write()
    pReader = svtk.svtkPDataSetReader()
    pReader.SetFileName("comb.psvtk")
    surface = svtk.svtkDataSetSurfaceFilter()
    surface.SetInputConnection(pReader.GetOutputPort())
    mapper = svtk.svtkPolyDataMapper()
    mapper.SetInputConnection(surface.GetOutputPort())
    mapper.SetNumberOfPieces(2)
    mapper.SetPiece(0)
    mapper.SetGhostLevel(1)
    mapper.Update()
    w = svtk.svtkDataSetWriter()
#    w.SetInputData(mapper.GetInput())
    w.SetInputData(surface.GetInput())
    w.SetFileName("foo.svtk")
    w.SetFileTypeToASCII()
    w.Write()
    file.delete("-force", "comb.psvtk")
    file.delete("-force", "comb.0.svtk")
    file.delete("-force", "comb.1.svtk")
    file.delete("-force", "comb.2.svtk")
    file.delete("-force", "comb.3.svtk")
    actor = svtk.svtkActor()
    actor.SetMapper(mapper)
    actor.SetPosition(-5,0,-29)
    # Add the actors to the renderer, set the background and size
    #
    ren1.AddActor(actor)
    # ====== ImageData ======
    # First save out a grid in parallel form.
    fractal = svtk.svtkImageMandelbrotSource()
    fractal.SetWholeExtent(0,9,0,9,0,9)
    fractal.SetSampleCX(0.1,0.1,0.1,0.1)
    fractal.SetMaximumNumberOfIterations(10)
    extract2 = svtk.svtkTransmitStructuredDataPiece()
    extract2.SetController(contr)
    extract2.SetInputConnection(fractal.GetOutputPort())
    writer2 = svtk.svtkPDataSetWriter()
    writer.SetFileName("fractal.psvtk")
    writer.SetInputConnection(extract2.GetOutputPort())
    writer.SetNumberOfPieces(4)
    writer.Write()
    pReader2 = svtk.svtkPDataSetReader()
    pReader2.SetFileName("fractal.psvtk")
    iso = svtk.svtkContourFilter()
    iso.SetInputConnection(pReader2.GetOutputPort())
    iso.SetValue(0,4)
    mapper2 = svtk.svtkPolyDataMapper()
    mapper2.SetInputConnection(iso.GetOutputPort())
    mapper2.SetNumberOfPieces(3)
    mapper2.SetPiece(0)
    mapper2.SetGhostLevel(0)
    mapper2.Update()
    # Strip the ghost cells requested by the contour filter
    mapper2.GetInput().RemoveGhostCells()
    file.delete("-force", "fractal.psvtk")
    file.delete("-force", "fractal.0.svtk")
    file.delete("-force", "fractal.1.svtk")
    file.delete("-force", "fractal.2.svtk")
    file.delete("-force", "fractal.3.svtk")
    actor2 = svtk.svtkActor()
    actor2.SetMapper(mapper2)
    actor2.SetScale(5,5,5)
    actor2.SetPosition(6,6,6)
    # Add the actors to the renderer, set the background and size
    #
    ren1.AddActor(actor2)
    # ====== PolyData ======
    # First save out a grid in parallel form.
    sphere = svtk.svtkSphereSource()
    sphere.SetRadius(2)
    writer3 = svtk.svtkPDataSetWriter()
    writer3.SetFileName("sphere.psvtk")
    writer3.SetInputConnection(sphere.GetOutputPort())
    writer3.SetNumberOfPieces(4)
    writer3.Write()
    pReader3 = svtk.svtkPDataSetReader()
    pReader3.SetFileName("sphere.psvtk")
    mapper3 = svtk.svtkPolyDataMapper()
    mapper3.SetInputConnection(pReader3.GetOutputPort())
    mapper3.SetNumberOfPieces(2)
    mapper3.SetPiece(0)
    mapper3.SetGhostLevel(1)
    mapper3.Update()
    file.delete("-force", "sphere.psvtk")
    file.delete("-force", "sphere.0.svtk")
    file.delete("-force", "sphere.1.svtk")
    file.delete("-force", "sphere.2.svtk")
    file.delete("-force", "sphere.3.svtk")
    actor3 = svtk.svtkActor()
    actor3.SetMapper(mapper3)
    actor3.SetPosition(6,6,6)
    # Add the actors to the renderer, set the background and size
    #
    ren1.AddActor(actor3)

    # do some extra checking to make sure we have the proper number of cells
    if surface.GetOutput().GetNumberOfCells() != 4016:
        print("surface output should have 4016 cells but has %d" % surface.GetOutput().GetNumberOfCells())
        sys.exit(1)
    if iso.GetOutput().GetNumberOfCells() != 89:
        print("iso output should have 89 cells but has %d" % iso.GetOutput().GetNumberOfCells())
        sys.exit(1)
    if pReader3.GetOutput().GetNumberOfCells() != 48:
        print("pReader3 output should have 48 cells but has %d" % pReader3.GetOutput().GetNumberOfCells())
        sys.exit(1)
    pass
ren1.SetBackground(0.1,0.2,0.4)
renWin.SetSize(300,300)
# render the image
#
cam1 = ren1.GetActiveCamera()
cam1.Azimuth(20)
cam1.Elevation(40)
ren1.ResetCamera()
cam1.Zoom(1.2)
iren.Initialize()
# prevent the tk window from showing up then start the event loop
# --- end of script --
