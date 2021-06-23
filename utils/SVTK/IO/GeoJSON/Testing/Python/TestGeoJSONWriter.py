from __future__ import print_function

import svtk, os, sys

ss = svtk.svtkSphereSource() #make mesh to test with

af = svtk.svtkElevationFilter() #add some attributes
af.SetInputConnection(ss.GetOutputPort())

ef = svtk.svtkExtractEdges() #make lines to test
ef.SetInputConnection(af.GetOutputPort())

gf = svtk.svtkGlyph3D() #make verts to test
pts = svtk.svtkPoints()
pts.InsertNextPoint(0,0,0)
verts = svtk.svtkCellArray()
avert = svtk.svtkVertex()
avert.GetPointIds().SetId(0, 0)
verts.InsertNextCell(avert)
onevertglyph = svtk.svtkPolyData()
onevertglyph.SetPoints(pts)
onevertglyph.SetVerts(verts)
gf.SetSourceData(onevertglyph)
gf.SetInputConnection(af.GetOutputPort())

testwrites = ["points","lines","mesh"]
failed = False
for datasetString in testwrites:
  if datasetString == "points":
    toshow=gf
  elif datasetString == "lines":
    toshow = ef
  else:
    toshow = af
  gw = svtk.svtkGeoJSONWriter()
  fname = "sphere_"+datasetString+".json"
  gw.SetInputConnection(toshow.GetOutputPort())
  gw.SetFileName(fname)
  gw.Write()
  if (os.path.exists(fname) and
     os.path.isfile(fname)):
    os.remove(fname)
  else:
    print("Failed to write " + fname + " to file")
    failed = True
  gw.WriteToOutputStringOn()
  gw.Write()
  gj = "['"+str(gw.RegisterAndGetOutputString()).replace('\n','')+"']"
  if len(gj) <= 1000:
    print("Failed to write " + fname + " to buffer")
    failed = True

if failed:
  sys.exit(1)
sys.exit(0)
