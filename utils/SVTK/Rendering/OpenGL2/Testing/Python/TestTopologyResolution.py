#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# This script shows a cube with edges and faces (using a coincident
# topology offset for polygons) and with ambient, diffuse and specular
# lighting.
# Three polydata mappers are tested with (top row) and without
# (bottom row) lighting:
# 1) left renderer shows a svtkPolyDataMapper which effectively is a
#    svtkOpenGLPolyDataMapper
# 2) middle renderer shows a svtkCompositePolyDataMapper which delegates
#    to a svtkOpenGLPolyDataMapper
# 3) right renderer shows a svtkCompositePolyDataMapper2 which does the
#    opengl rendering by itself

# The cube
cube = svtk.svtkCubeSource()
rib=float(1.)
cube.SetBounds(-rib/2,rib/2,-rib/2,rib/2,-rib/2,rib/2)
cube.SetCenter(0,0,0)
cube.Update()

# Rotate the cube to show the faces being displaced
transform = svtk.svtkTransform()
transform.Identity()
transform.RotateX(45)
transform.RotateY(45)
transformer=svtk.svtkTransformPolyDataFilter()
transformer.SetInputConnection(cube.GetOutputPort())
transformer.SetTransform(transform)
transformer.Update()

# A trivial multi-block to be used as input for
# svtkCompositePolyDataMapper and svtkCompositePolyDataMapper2
mbd = svtk.svtkMultiBlockDataSet()
mbd.SetNumberOfBlocks(1)
mbd.SetBlock(0,transformer.GetOutput())
source=svtk.svtkTrivialProducer()
source.SetOutput(mbd)

# Set up the render window and the interactor.
renWin=svtk.svtkRenderWindow()
renWin.SetSize(600,400)
iren=svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

mappers=list()
actors=list()
renderers=list()

polyMapper = svtk.svtkPolyDataMapper()
polyMapper.SetInputConnection(transformer.GetOutputPort())
mappers.append(polyMapper)

mbdMapper = svtk.svtkCompositePolyDataMapper()
mbdMapper.SetInputConnection(source.GetOutputPort())
mappers.append(mbdMapper)

mbdMapper2 = svtk.svtkCompositePolyDataMapper2()
mbdMapper2.SetInputConnection(source.GetOutputPort())
mappers.append(mbdMapper2)

x=0
dx=float(1)/len(mappers)

# Define the property: lighting intensities, color,
# edge visibility, point visibilty
p1=svtk.svtkProperty()
p1.SetColor(1,0,0)
p1.LightingOff()
p1.SetAmbient(0.2)
p1.SetDiffuse(0.7)
p1.SetSpecular(0.4)
p1.SetSpecularPower(35)
p1.EdgeVisibilityOn()
p1.SetEdgeColor(1,1,1)
p1.VertexVisibilityOn()
p1.SetVertexColor(0,1,0)
p1.SetPointSize(4)

# Top row has lighting on
p2=svtk.svtkProperty()
p2.DeepCopy(p1)
p2.LightingOn()

light=svtk.svtkLight()
light.SetPosition(1,1,1)

for m in mappers:
    # Push back the polygons
    m.SetRelativeCoincidentTopologyPolygonOffsetParameters(10,2)

    # Bottom renderer shows cube without lighting
    actors.append(svtk.svtkActor())
    a1=actors[-1]
    a1.SetMapper(m)
    a1.SetProperty(p1)
    renderers.append(svtk.svtkRenderer())
    r1 = renderers[-1]
    r1.AddActor(a1)
    r1.RemoveAllLights()
    r1.AddLight(light)
    r1.SetViewport(x,0,x+dx,0.5)
    renWin.AddRenderer(r1)

    # Top renderer shows cube with lighting
    actors.append(svtk.svtkActor())
    a2=actors[-1]
    a2.SetMapper(m)
    a2.SetProperty(p2)
    renderers.append(svtk.svtkRenderer())
    r2 = renderers[-1]
    r2.AddActor(a2)
    r2.RemoveAllLights()
    r2.AddLight(light)
    r2.SetViewport(x,0.5,x+dx,1.0)
    renWin.AddRenderer(r2)
    x=x+dx

iren.Initialize()
renWin.Render()
# --- end of script ---
