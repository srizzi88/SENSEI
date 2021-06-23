#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

def PlaneSphereActors():
    ps = svtk.svtkPlaneSource()
    ps.SetXResolution(10)
    ps.SetYResolution(10)

    ss = svtk.svtkSphereSource()
    ss.SetRadius (0.3)

    group = svtk.svtkMultiBlockDataGroupFilter()
    group.AddInputConnection(ps.GetOutputPort())
    group.AddInputConnection(ss.GetOutputPort())

    ag = svtk.svtkRandomAttributeGenerator()
    ag.SetInputConnection(group.GetOutputPort())
    ag.GenerateCellScalarsOn()
    ag.AttributesConstantPerBlockOn()

    n = svtk.svtkPolyDataNormals()
    n.SetInputConnection(ag.GetOutputPort())
    n.Update ();

    actors = []
    it = n.GetOutputDataObject(0).NewIterator()
    it.InitTraversal()
    while not it.IsDoneWithTraversal():
        pm = svtk.svtkPolyDataMapper()
        pm.SetInputData(it.GetCurrentDataObject())

        a = svtk.svtkActor()
        a.SetMapper(pm)
        actors.append (a)
        it.GoToNextItem()
    return actors

# Create the RenderWindow, Renderer and interactive renderer
ren = svtk.svtkRenderer()
ren.SetBackground(0, 0, 0)
renWin = svtk.svtkRenderWindow()
renWin.SetSize(300, 300)
renWin.AddRenderer(ren)

# make sure to have the same regression image on all platforms.
renWin.SetMultiSamples(0)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Force a starting random value
raMath = svtk.svtkMath()
raMath.RandomSeed(6)

# Generate random cell attributes on a plane and a cylinder
for a in PlaneSphereActors():
    ren.AddActor(a)

# reorient the camera
camera = ren.GetActiveCamera()
camera.Azimuth(20)
camera.Elevation(20)
ren.SetActiveCamera(camera)
ren.ResetCamera()

renWin.Render()
#iren.Start()
