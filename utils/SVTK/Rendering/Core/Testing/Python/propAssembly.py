#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# demonstrates the use of svtkPropAssembly
# create four parts: a top level assembly and three primitives
#
sphere = svtk.svtkSphereSource()
sphereMapper = svtk.svtkPolyDataMapper()
sphereMapper.SetInputConnection(sphere.GetOutputPort())
sphereActor = svtk.svtkActor()
sphereActor.SetMapper(sphereMapper)
sphereActor.SetOrigin(2,1,3)
sphereActor.RotateY(6)
sphereActor.SetPosition(2.25,0,0)
sphereActor.GetProperty().SetColor(1,0,1)
cube = svtk.svtkCubeSource()
cubeMapper = svtk.svtkPolyDataMapper()
cubeMapper.SetInputConnection(cube.GetOutputPort())
cubeActor = svtk.svtkActor()
cubeActor.SetMapper(cubeMapper)
cubeActor.SetPosition(0.0,.25,0)
cubeActor.GetProperty().SetColor(0,0,1)
cone = svtk.svtkConeSource()
coneMapper = svtk.svtkPolyDataMapper()
coneMapper.SetInputConnection(cone.GetOutputPort())
coneActor = svtk.svtkActor()
coneActor.SetMapper(coneMapper)
coneActor.SetPosition(0,0,.25)
coneActor.GetProperty().SetColor(0,1,0)
cylinder = svtk.svtkCylinderSource()
#top part
cylinderMapper = svtk.svtkPolyDataMapper()
cylinderMapper.SetInputConnection(cylinder.GetOutputPort())
cylinderActor = svtk.svtkActor()
cylinderActor.SetMapper(cylinderMapper)
cylinderActor.GetProperty().SetColor(1,0,0)
compositeAssembly = svtk.svtkAssembly()
compositeAssembly.AddPart(cylinderActor)
compositeAssembly.AddPart(sphereActor)
compositeAssembly.AddPart(cubeActor)
compositeAssembly.AddPart(coneActor)
compositeAssembly.SetOrigin(5,10,15)
compositeAssembly.AddPosition(5,0,0)
compositeAssembly.RotateX(15)
# Build the prop assembly out of a svtkActor and a svtkAssembly
assembly = svtk.svtkPropAssembly()
assembly.AddPart(compositeAssembly)
assembly.AddPart(coneActor)
# Create the RenderWindow, Renderer and both Actors
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddViewProp(assembly)
ren1.SetBackground(0.1,0.2,0.4)
renWin.SetSize(300,300)
# Get handles to some useful objects
#
iren.Initialize()
renWin.Render()
# should create the same image as assembly.tcl
# prevent the tk window from showing up then start the event loop
# --- end of script --
