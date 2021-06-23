#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Interpolate onto a volume

# Parameters for debugging
NPts = 1000000
binNum = 16
math = svtk.svtkMath()
math.RandomSeed(31415)

# create pipeline
#
points = svtk.svtkBoundedPointSource()
points.SetNumberOfPoints(NPts)
points.ProduceRandomScalarsOn()
points.ProduceCellOutputOff()
points.Update()

# Bin the points
hBin = svtk.svtkHierarchicalBinningFilter()
hBin.SetInputConnection(points.GetOutputPort())
#hBin.AutomaticOn()
hBin.AutomaticOff()
hBin.SetDivisions(2,2,2)
hBin.SetBounds(points.GetOutput().GetBounds())

# Time execution
timer = svtk.svtkTimerLog()
timer.StartTimer()
hBin.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Points processed: {0}".format(NPts))
print("   Time to bin: {0}".format(time))
#print(hBin)
#print(hBin.GetOutput())

# write stuff out
w = svtk.svtkXMLPolyDataWriter()
w.SetFileName("binPoints.vtp")
w.SetInputConnection(hBin.GetOutputPort())
#w.SetDataModeToAscii()
#w.Write()

# Output a selected bin of points
extBin = svtk.svtkExtractHierarchicalBins()
extBin.SetInputConnection(hBin.GetOutputPort())
extBin.SetBinningFilter(hBin)
extBin.SetLevel(1000)
extBin.Update() #check clamping on level number
extBin.SetBin(1000000000) # check clamping of bin number
#extBin.SetLevel(0)
extBin.SetLevel(-1)
extBin.SetBin(binNum)
extBin.Update()

subMapper = svtk.svtkPointGaussianMapper()
subMapper.SetInputConnection(extBin.GetOutputPort())
subMapper.EmissiveOff()
subMapper.SetScaleFactor(0.0)

subActor = svtk.svtkActor()
subActor.SetMapper(subMapper)

# Create an outline
outline = svtk.svtkOutlineFilter()
outline.SetInputConnection(points.GetOutputPort())

outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)

# Create another outline
bds = [0,0,0,0,0,0]
hBin.GetBinBounds(binNum,bds)
binOutline = svtk.svtkOutlineSource()
binOutline.SetBounds(bds)

binOutlineMapper = svtk.svtkPolyDataMapper()
binOutlineMapper.SetInputConnection(binOutline.GetOutputPort())

binOutlineActor = svtk.svtkActor()
binOutlineActor.SetMapper(binOutlineMapper)

# Create the RenderWindow, Renderer and both Actors
#
ren0 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren0)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren0.AddActor(subActor)
ren0.AddActor(outlineActor)
ren0.AddActor(binOutlineActor)
ren0.SetBackground(0.1, 0.2, 0.4)

renWin.SetSize(250,250)

cam = ren0.GetActiveCamera()
cam.SetFocalPoint(1,1,1)
cam.SetPosition(0,0,0)
ren0.ResetCamera()

iren.Initialize()

# render the image
#
renWin.Render()

iren.Start()
