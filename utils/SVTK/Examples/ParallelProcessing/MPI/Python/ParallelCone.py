#!/usr/bin/env python
from __future__ import print_function
from svtk import *
import sys
import os
import time

myProcId = 0
numProcs = 1

compManager = svtkCompositeRenderManager()
if compManager.GetController():
    myProcId = compManager.GetController().GetLocalProcessId()
    numProcs = compManager.GetController().GetNumberOfProcesses()

try:
    v = svtkMesaRenderer()
    if myProcId > 0:
        _graphics_fact=svtkGraphicsFactory()
        _graphics_fact.SetUseMesaClasses(1)
        del _graphics_fact
    del v
except Exception as bar:
    print("No mesa", bar)



#print("I am process: %d / %d" % (myProcId, numProcs))

# create a rendering window and renderer
ren = svtkRenderer()
renWin = svtkRenderWindow()
renWin.AddRenderer(ren)
renWin.SetSize(300,300)

#if myProcId:
#    renWin.OffScreenRenderingOn()


# create an actor and give it cone geometry
cone = svtkConeSource()
cone.SetResolution(8)
coneMapper = svtkPolyDataMapper()
coneMapper.SetInputConnection(cone.GetOutputPort())
coneActor = svtkActor()
coneActor.SetMapper(coneMapper)

# assign our actor to the renderer
ren.AddActor(coneActor)

renWin.SetWindowName("I am node %d" % myProcId)

if numProcs > 1:
    compManager.SetRenderWindow(renWin)
    compManager.InitializePieces()

#print("Pid of process %d is %d" % (myProcId, os.getpid()))

def ExitMaster(a, b):
    #print("ExitMaster; I am %d / %d" % ( myProcId, numProcs ))
    if numProcs > 1 and myProcId == 0:
        #print("Trigger exit RMI on all satellite nodes")
        for a in range(1, numProcs):
            #print("Trigger exit in satellite node %d" % a)
            contr = compManager.GetController()
            contr.TriggerRMI(a, contr.GetBreakRMITag())

if myProcId == 0:
    iren = svtkRenderWindowInteractor()
    iren.SetRenderWindow(renWin)
    iren.AddObserver("ExitEvent", ExitMaster)
    iren.Initialize()
    iren.Start()
    #renWin.Render()
    #renWin.Render()
    #renWin.Render()
else:
    compManager.InitializeRMIs()
    compManager.GetController().ProcessRMIs()
    compManager.GetController().Finalize()
    #print "**********************************"
    #print "Done on the slave node"
    #print "**********************************"
    sys.exit()

ExitMaster(0, 0)

#time.sleep(5)

