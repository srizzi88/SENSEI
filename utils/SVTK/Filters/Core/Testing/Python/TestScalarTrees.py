#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Create the RenderWindow, Renderer
#
ren0 = svtk.svtkRenderer()
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren0)
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

reader = svtk.svtkExodusIIReader()
reader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/disk_out_ref.ex2")
reader.SetPointResultArrayStatus("Temp",1);
reader.Update ();
input = reader.GetOutput().GetBlock(0).GetBlock(0);

# Create scalar trees to test
sTree = svtk.svtkSimpleScalarTree()
sTree.SetBranchingFactor(3)

spanTree = svtk.svtkSpanSpace()
spanTree.SetResolution(100);

# First create the usual contour grid (non-threaded)
cf = svtk.svtkContourFilter()
cf.SetInputData(input)
cf.SetUseScalarTree(1)
cf.SetValue(0,350)
cf.SetInputArrayToProcess(0, 0, 0, 0, "Temp");
cf.GenerateTrianglesOff()

contourMapper = svtk.svtkPolyDataMapper()
contourMapper.SetInputConnection(cf.GetOutputPort())
contourMapper.ScalarVisibilityOff()

contourActor = svtk.svtkActor()
contourActor.SetMapper(contourMapper)
contourActor.GetProperty().SetColor(1,1,1)

outline = svtk.svtkOutlineFilter()
outline.SetInputData(input)

outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)

# Now create the threaded version. Note that as of the
# writing of this test, svtkSMPContourGrid does not merge
# pieces properly hence MergePieces is disabled. Also
# svtkSMPContourGrid only outputs triangles.
cfT = svtk.svtkSMPContourGrid()
cfT.SetInputData(input)
cfT.UseScalarTreeOn()
cfT.SetValue(0,350)
cfT.SetInputArrayToProcess(0, 0, 0, 0, "Temp");
cfT.GenerateTrianglesOff()
cfT.MergePiecesOff()

# Note: Since MergePieces is off, we have to use a
# svtkCompositePolyDataMapper.
contourMapperT = svtk.svtkCompositePolyDataMapper()
contourMapperT.SetInputConnection(cfT.GetOutputPort())
contourMapperT.ScalarVisibilityOff()

contourActorT = svtk.svtkActor()
contourActorT.SetMapper(contourMapperT)
contourActorT.GetProperty().SetColor(1,1,1)

outlineT = svtk.svtkOutlineFilter()
outlineT.SetInputData(input)

outlineMapperT = svtk.svtkPolyDataMapper()
outlineMapperT.SetInputConnection(outlineT.GetOutputPort())

outlineActorT = svtk.svtkActor()
outlineActorT.SetMapper(outlineMapperT)

# Time the execution of the filter w/out scalar tree
CG_timer = svtk.svtkExecutionTimer()
CG_timer.SetFilter(cf)
cf.UseScalarTreeOff()
cf.Update()
CG = CG_timer.GetElapsedWallClockTime()
print ("Contour Grid (no Scalar Tree):", CG)

# Time the execution of the filter w/ simple scalar tree
CG_timer0 = svtk.svtkExecutionTimer()
CG_timer0.SetFilter(cf)
cf.UseScalarTreeOn()
cf.SetScalarTree(sTree)
cf.Update()
CG0 = CG_timer0.GetElapsedWallClockTime()
print ("Contour Grid (build & execute Simple Scalar Tree):", CG0)

CG_timer1 = svtk.svtkExecutionTimer()
CG_timer1.SetFilter(cf)
cf.Modified()
cf.Update()
CG1 = CG_timer1.GetElapsedWallClockTime()
print ("Contour Grid (Simple Scalar Tree):", CG1)

# Time the execution of the filter
CG2_timer0 = svtk.svtkExecutionTimer()
CG2_timer0.SetFilter(cf)
cf.SetScalarTree(spanTree)
cf.Update()
CG2 = CG2_timer0.GetElapsedWallClockTime()
print ("Contour Grid (build & execute Span Tree):", CG2)

CG2_timer1 = svtk.svtkExecutionTimer()
CG2_timer1.SetFilter(cf)
cf.Modified()
cf.Update()
CG3 = CG2_timer1.GetElapsedWallClockTime()
print ("Contour Grid (Span Tree):", CG3)

# Okay now execute the threaded algorithms
# Time the execution of the filter w/out scalar tree
CG_timer.SetFilter(cfT)
cfT.UseScalarTreeOff()
cfT.Update()
CG = CG_timer.GetElapsedWallClockTime()
print ("SMPContour Grid (no Scalar Tree):", CG)

# Time the execution of the filter w/ simple scalar tree
CG_timer0.SetFilter(cfT)
cfT.UseScalarTreeOn()
cfT.SetScalarTree(sTree)
cfT.Update()
CG0 = CG_timer0.GetElapsedWallClockTime()
print ("SMPContour Grid (build & execute Simple Scalar Tree):", CG0)

CG_timer1 = svtk.svtkExecutionTimer()
CG_timer1.SetFilter(cfT)
cfT.Modified()
cf.Update()
CG1 = CG_timer1.GetElapsedWallClockTime()
print ("SMPContour Grid (Simple Scalar Tree):", CG1)

# Time the execution of the filter
CG2_timer0 = svtk.svtkExecutionTimer()
CG2_timer0.SetFilter(cfT)
cfT.SetScalarTree(spanTree)
cfT.Update()
CG2 = CG2_timer0.GetElapsedWallClockTime()
print ("SMPContour Grid (build & execute Span Tree):", CG2)

CG2_timer1 = svtk.svtkExecutionTimer()
CG2_timer1.SetFilter(cfT)
cfT.Modified()
cfT.Update()
CG3 = CG2_timer1.GetElapsedWallClockTime()
print ("SMPContour Grid (Span Tree):", CG3)

# Add the actors to the renderer, set the background and size
#
ren0.AddActor(outlineActor)
ren0.AddActor(contourActor)
ren1.AddActor(outlineActorT)
ren1.AddActor(contourActorT)

ren0.SetBackground(0,0,0)
ren1.SetBackground(0,0,0)
ren0.SetViewport(0,0,0.5,1);
ren1.SetViewport(0.5,0,1,1);
renWin.SetSize(600,300)
ren0.ResetCamera()
ren1.ResetCamera()
iren.Initialize()

renWin.Render()
#iren.Start()
# --- end of script --
