#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# retrieve named colors
def GetRGBColor(colorName):
    '''
        Return the red, green and blue components for a
        color as doubles.
    '''
    rgb = [0.0, 0.0, 0.0]  # black
    svtk.svtkNamedColors().GetColorRGB(colorName, rgb)
    return rgb

# Control resolution of test (sphere resolution)
res = 9

# Create the RenderWindow, Renderer
#
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer( ren )

iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create pipeline. Two spheres: one is the target to
# be intersected against, and is placed inside a static
# cell locator. The second bounds these, it's points
# serve as starting points that shoot rays towards the
# center of the fist sphere.
#
sphere = svtk.svtkSphereSource()
sphere.SetThetaResolution(2*res)
sphere.SetPhiResolution(res)
sphere.Update()

mapper = svtk.svtkPolyDataMapper()
mapper.SetInputConnection(sphere.GetOutputPort())

actor = svtk.svtkActor()
actor.SetMapper(mapper)

# Now the locator
loc = svtk.svtkStaticCellLocator()
loc.SetDataSet(sphere.GetOutput())
loc.SetNumberOfCellsPerNode(5)
loc.BuildLocator()

locPD = svtk.svtkPolyData()
loc.GenerateRepresentation(4,locPD)
locMapper = svtk.svtkPolyDataMapper()
locMapper.SetInputData(locPD)
locActor = svtk.svtkActor()
locActor.SetMapper(locMapper)
locActor.GetProperty().SetRepresentationToWireframe()

# Now the outer sphere
sphere2 = svtk.svtkSphereSource()
sphere2.SetThetaResolution(res)
sphere2.SetPhiResolution(int(res/2))
sphere2.SetRadius(3*sphere.GetRadius())
sphere2.Update()

# Generate intersection points
center = sphere.GetCenter()
polyInts = svtk.svtkPolyData()
pts = svtk.svtkPoints()
spherePts = sphere2.GetOutput().GetPoints()
numRays = spherePts.GetNumberOfPoints()
pts.SetNumberOfPoints(numRays + 1)

polyRays = svtk.svtkPolyData()
rayPts = svtk.svtkPoints()
rayPts.SetNumberOfPoints(numRays + 1)
lines = svtk.svtkCellArray()

t = svtk.reference(0.0)
subId = svtk.reference(0)
cellId = svtk.reference(0)
xyz = [0.0,0.0,0.0]
xInt = [0.0,0.0,0.0]
pc = [0.0,0.0,0.0]

pts.SetPoint(0,center)
rayPts.SetPoint(0,center)
for i in range(0, numRays):
    spherePts.GetPoint(i,xyz)
    rayPts.SetPoint(i+1,xyz)
    cellId = svtk.reference(i);
    hit = loc.IntersectWithLine(xyz, center, 0.001, t, xInt, pc, subId, cellId)
    if ( hit == 0 ):
        print("Missed: {}".format(i))
        pts.SetPoint(i+1,center)
    else:
        pts.SetPoint(i+1,xInt)
    lines.InsertNextCell(2)
    lines.InsertCellPoint(0)
    lines.InsertCellPoint(i+1)

polyInts.SetPoints(pts)

polyRays.SetPoints(rayPts)
polyRays.SetLines(lines)

# Glyph the intersection points
glyphSphere = svtk.svtkSphereSource()
glyphSphere.SetPhiResolution(6)
glyphSphere.SetThetaResolution(12)

glypher = svtk.svtkGlyph3D()
glypher.SetInputData(polyInts)
glypher.SetSourceConnection(glyphSphere.GetOutputPort())
glypher.SetScaleFactor(0.05)

glyphMapper = svtk.svtkPolyDataMapper()
glyphMapper.SetInputConnection(glypher.GetOutputPort())

glyphActor = svtk.svtkActor()
glyphActor.SetMapper(glyphMapper)
glyphActor.GetProperty().SetColor(GetRGBColor('peacock'))

linesMapper = svtk.svtkPolyDataMapper()
linesMapper.SetInputData(polyRays)

linesActor = svtk.svtkActor()
linesActor.SetMapper(linesMapper)
linesActor.GetProperty().SetColor(GetRGBColor('tomato'))

# Render it
ren.AddActor(actor)
ren.AddActor(glyphActor)
ren.AddActor(locActor)
ren.AddActor(linesActor)

ren.GetActiveCamera().SetPosition(1,1,1)
ren.GetActiveCamera().SetFocalPoint(0,0,0)
ren.ResetCamera()

renWin.Render()
iren.Start()
