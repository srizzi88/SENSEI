#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# create pipeline
#
# create sphere to color
sphere = svtk.svtkSphereSource()
sphere.SetThetaResolution(20)
sphere.SetPhiResolution(40)
def colorCells (__svtk__temp0=0,__svtk__temp1=0):
    randomColorGenerator = svtk.svtkMath()
    input = randomColors.GetInput()
    output = randomColors.GetOutput()
    numCells = input.GetNumberOfCells()
    colors = svtk.svtkFloatArray()
    colors.SetNumberOfTuples(numCells)
    i = 0
    while i < numCells:
        colors.SetValue(i,randomColorGenerator.Random(0,1))
        i = i + 1

    output.GetCellData().CopyScalarsOff()
    output.GetCellData().PassData(input.GetCellData())
    output.GetCellData().SetScalars(colors)
    del colors
    #reference counting - it's ok
    del randomColorGenerator

# Compute random scalars (colors) for each cell
randomColors = svtk.svtkProgrammableAttributeDataFilter()
randomColors.SetInputConnection(sphere.GetOutputPort())
randomColors.SetExecuteMethod(colorCells)
# mapper and actor
mapper = svtk.svtkPolyDataMapper()
mapper.SetInputConnection(randomColors.GetOutputPort())
mapper.SetScalarRange(randomColors.GetPolyDataOutput().GetScalarRange())
sphereActor = svtk.svtkActor()
sphereActor.SetMapper(mapper)
# Create a scalar bar
scalarBar = svtk.svtkScalarBarActor()
scalarBar.SetLookupTable(mapper.GetLookupTable())
scalarBar.SetTitle("Temperature")
scalarBar.GetPositionCoordinate().SetCoordinateSystemToNormalizedViewport()
scalarBar.GetPositionCoordinate().SetValue(0.1,0.01)
scalarBar.SetOrientationToHorizontal()
scalarBar.SetWidth(0.8)
scalarBar.SetHeight(0.17)
# Test the Get/Set Position 
scalarBar.SetPosition(scalarBar.GetPosition())
# Create graphics stuff
# Create the RenderWindow, Renderer and both Actors
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
ren1.AddActor(sphereActor)
ren1.AddActor2D(scalarBar)
renWin.SetSize(350,350)
# render the image
#
ren1.ResetCamera()
ren1.GetActiveCamera().Zoom(1.5)
renWin.Render()
scalarBar.SetNumberOfLabels(8)
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
