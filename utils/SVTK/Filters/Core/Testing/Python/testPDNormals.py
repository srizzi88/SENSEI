# This tests that svtkPolyDataNormals handles cell data
# for strips properly. The filter splits strips to generate
# proper normals so it also needs to split cell data.
import svtk

ps = svtk.svtkPlaneSource()
ps.SetYResolution(10)

tf = svtk.svtkTriangleFilter()
tf.SetInputConnection(ps.GetOutputPort())

ts = svtk.svtkStripper()
ts.SetInputConnection(tf.GetOutputPort())

ts.Update()
strips = ts.GetOutput()

a = svtk.svtkFloatArray()
a.InsertNextTuple1(1)
a.SetName("foo")

strips.GetCellData().AddArray(a)

s = svtk.svtkSphereSource()
s.Update()

sphere = s.GetOutput()

a2 = svtk.svtkFloatArray()
a2.SetNumberOfTuples(96)
a2.FillComponent(0, 2)
a2.SetName("foo")

sphere.GetCellData().AddArray(a2)

app = svtk.svtkAppendPolyData()
app.AddInputData(strips)
app.AddInputData(sphere)

pdn = svtk.svtkPolyDataNormals()
pdn.SetInputConnection(app.GetOutputPort())
pdn.Update()

output = pdn.GetOutput()

foo = output.GetCellData().GetArray("foo")
for i in range(0, 96):
    assert(foo.GetValue(i) == 2)

for i in range(96, 116):
    assert(foo.GetValue(i) == 1)