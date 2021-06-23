#!/usr/bin/env python
import svtk

def SetRandomSeed(caller, eventId):
    #print "Restart random number generator"
    raMath = svtk.svtkMath()
    raMath.RandomSeed(6)


def SphereActor(lut, interpolateBeforeMapping):
    ss = svtk.svtkSphereSource()
    if interpolateBeforeMapping:
        ss.SetCenter(-1, 0, 0)

    bp = svtk.svtkBrownianPoints()
    bp.SetInputConnection(ss.GetOutputPort())
    bp.AddObserver (svtk.svtkCommand.EndEvent, SetRandomSeed)

    pm = svtk.svtkPolyDataMapper()
    pm.SetInputConnection(bp.GetOutputPort())
    pm.SetScalarModeToUsePointFieldData()
    pm.SelectColorArray("BrownianVectors")
    pm.SetLookupTable(lut)
    pm.SetInterpolateScalarsBeforeMapping(interpolateBeforeMapping)

    a = svtk.svtkActor()
    a.SetMapper(pm)
    return a

def ColorTransferFunction():
    opacityTransfer = svtk.svtkPiecewiseFunction()
    opacityTransfer.AddPoint(0,0)
    opacityTransfer.AddPoint(0.6,0)
    opacityTransfer.AddPoint(1,1)

    lut = svtk.svtkDiscretizableColorTransferFunction()
    lut.SetColorSpaceToDiverging();
    lut.AddRGBPoint(0.0, 0.23, 0.299, 0.754)
    lut.AddRGBPoint(1.0, 0.706, 0.016, 0.150);
    lut.SetVectorModeToMagnitude()
    lut.SetRange (0, 1)
    lut.SetScalarOpacityFunction(opacityTransfer)
    lut.EnableOpacityMappingOn()
    return lut


renWin = svtk.svtkRenderWindow()
renWin.SetSize(300, 300)
# enable depth peeling
renWin.AlphaBitPlanesOn()
renWin.SetMultiSamples(0)

ren = svtk.svtkRenderer()
ren.SetBackground(0, 0, 0)
# enable depth peeling
ren.UseDepthPeelingOn()
ren.SetMaximumNumberOfPeels(4)
renWin.AddRenderer(ren)

iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Force a starting random value
SetRandomSeed(0, 0)

lut = ColorTransferFunction()
ren.AddActor(SphereActor (lut, 0))
ren.AddActor(SphereActor (lut, 1))

renWin.Render()
#iren.Start()
