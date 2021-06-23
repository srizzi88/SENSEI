# Tests paraview/paraview#18391

from svtkmodules.util.misc import svtkGetDataRoot, svtkGetTempDir
from svtk import svtkXMLGenericDataObjectReader, svtkDoubleArray, svtkDataSetWriter, svtkPDataSetReader

from os.path import join

reader = svtkXMLGenericDataObjectReader()
reader.SetFileName(join(svtkGetDataRoot(), "Data/multicomb_0.vts"))
reader.Update()


a1 = svtkDoubleArray()
a1.SetName("field-1")
a1.SetNumberOfTuples(1)
a1.SetValue(0, 100.0)
a1.GetRange()

a2 = svtkDoubleArray()
a2.SetName("field-2")
a2.SetNumberOfTuples(2)
a2.SetValue(0, 1.0)
a2.SetValue(1, 2.0)
a2.GetRange()

dobj = reader.GetOutputDataObject(0)
dobj.GetFieldData().AddArray(a1)
dobj.GetFieldData().AddArray(a2)

writer = svtkDataSetWriter()
writer.SetFileName(join(svtkGetTempDir(), "TestPDataSetReaderWriterWithFieldData.svtk"))
writer.SetInputDataObject(dobj)
writer.Write()

reader = svtkPDataSetReader()
reader.SetFileName(join(svtkGetTempDir(), "TestPDataSetReaderWriterWithFieldData.svtk"))
reader.Update()

dobj2 = reader.GetOutputDataObject(0)
assert (dobj.GetNumberOfPoints() == dobj2.GetNumberOfPoints() and
        dobj.GetFieldData().GetNumberOfArrays() == dobj2.GetFieldData().GetNumberOfArrays())
