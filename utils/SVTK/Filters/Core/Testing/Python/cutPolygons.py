import svtk
from math import sqrt

plane = svtk.svtkPlane()
plane.SetOrigin(5,5,9.8)
plane.SetNormal(0,0,1)

coords = [(0,0,0),(10,0,0),(10,10,0),(0,10,0),
          (0,0,10),(10,0,10),(10,10,10),(0,10,10),
          (5,0,0),(10,5,0),(5,10,0),(0,5,0),
          (5,0,9.5),(10,5,9.3),(5,10,9.5),(0,5,9.3),
          (0,0,5),(10,0,5),(10,10,5),(0,10,5)]
data = svtk.svtkFloatArray()
points = svtk.svtkPoints()
ptIds = svtk.svtkIdList()
mesh = svtk.svtkUnstructuredGrid()
mesh.SetPoints(points)
mesh.GetPointData().SetScalars(data)

for id in range(0,20):
    x=coords[id][0]
    y=coords[id][1]
    z=coords[id][2]
    ptIds.InsertNextId(id)
    points.InsertNextPoint(x,y,z)
    data.InsertNextValue(sqrt(x*x + y*y + z*z))
mesh.InsertNextCell(svtk.SVTK_QUADRATIC_HEXAHEDRON,ptIds)

ptIds.Reset()
for id in range(0,8):
    x=coords[id][0] + 20
    y=coords[id][1] + 20
    z=coords[id][2]
    ptIds.InsertNextId(id + 20)
    points.InsertNextPoint(x,y,z)
    data.InsertNextValue(sqrt(x*x + y*y + z*z))
mesh.InsertNextCell(svtk.SVTK_HEXAHEDRON,ptIds)

print( "Mesh bounding box : ({0})".format( mesh.GetPoints().GetBounds() ) )

triCutter = svtk.svtkCutter()
triCutter.SetInputData(mesh)
triCutter.SetCutFunction(plane)
triCutter.GenerateTrianglesOn()
triCutter.Update()
print( "Triangle cutter bounding box : ({0})".format( triCutter.GetOutput().GetPoints().GetBounds() ) )

polyCutter = svtk.svtkCutter()
polyCutter.SetInputData(mesh)
polyCutter.SetCutFunction(plane)
polyCutter.GenerateTrianglesOff()
polyCutter.Update()
print( "Polygon cutter bounding box : ({0})".format( polyCutter.GetOutput().GetPoints().GetBounds() ) )

# Display input and output side-by-side
meshMapper = svtk.svtkDataSetMapper()
meshMapper.SetInputData(mesh)

meshActor=svtk.svtkActor()
meshActor.SetMapper(meshMapper)

meshRen = svtk.svtkRenderer()
meshRen.AddActor(meshActor)
meshRen.SetViewport(0.0,0.0,0.5,1.0)

triCutMapper = svtk.svtkPolyDataMapper()
triCutMapper.SetInputData(triCutter.GetOutput())

triCutActor=svtk.svtkActor()
triCutActor.SetMapper(triCutMapper)
triCutActor.GetProperty().EdgeVisibilityOn()
triCutActor.GetProperty().SetEdgeColor(1,1,1)

triCutRen = svtk.svtkRenderer()
triCutRen.AddActor(triCutActor)
triCutRen.SetViewport(0.5,0.5,1.0,1.0)

polyCutMapper = svtk.svtkPolyDataMapper()
polyCutMapper.SetInputData(polyCutter.GetOutput())

polyCutActor=svtk.svtkActor()
polyCutActor.SetMapper(polyCutMapper)
polyCutActor.GetProperty().EdgeVisibilityOn()
polyCutActor.GetProperty().SetEdgeColor(1,1,1)

polyCutRen = svtk.svtkRenderer()
polyCutRen.AddActor(polyCutActor)
polyCutRen.SetViewport(0.5,0.0,1.0,0.5)

renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(meshRen)
renWin.AddRenderer(triCutRen)
renWin.AddRenderer(polyCutRen)
renWin.SetSize(800,400)

iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

renWin.Render()
iren.Initialize()
