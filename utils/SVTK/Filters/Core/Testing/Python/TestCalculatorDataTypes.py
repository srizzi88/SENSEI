import sys
import svtk
from svtk.test import Testing

from svtk.util.misc import svtkGetDataRoot

SVTK_DATA_ROOT = svtkGetDataRoot()

try:
    import numpy
except ImportError:
    print("WARNING: This test requires Numeric Python: http://numpy.sf.net")
    from svtk.test import Testing
    Testing.skip()

from svtk.util import numpy_support as np_s

def makePolyData(attributeType):
    ss = svtk.svtkSphereSource()
    ss.Update()
    s = ss.GetOutput()

    pd = s.GetAttributes(attributeType)

    num = 0
    if attributeType == svtk.svtkDataObject.POINT:
        num = s.GetNumberOfPoints()
    else:
        num = s.GetNumberOfCells()

    a = [ x for x in range(num)]
    aa = numpy.array(a)

    b = [ x**2 for x in a]
    bb = numpy.array(b)

    array1 = np_s.numpy_to_svtk(aa, deep=True)
    array1.SetName("ID")

    array2 = np_s.numpy_to_svtk(bb, deep=True)
    array2.SetName("Square")

    pd.AddArray(array1)
    pd.AddArray(array2)

    return s


def makeGraph(attributeType):
    reader = svtk.svtkPDBReader()
    reader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/3GQP.pdb")
    reader.Update()

    molecule = reader.GetOutputDataObject(1)

    pd = molecule.GetAttributes(attributeType)

    num = 0
    if attributeType == svtk.svtkDataObject.VERTEX:
        num = molecule.GetNumberOfVertices()
    else:
        num = molecule.GetNumberOfEdges()

    a = [ x for x in range(num)]
    aa = numpy.array(a)

    b = [ x**2 for x in a]
    bb = numpy.array(b)

    array1 = np_s.numpy_to_svtk(aa, deep=True)
    array1.SetName("ID")

    array2 = np_s.numpy_to_svtk(bb, deep=True)
    array2.SetName("Square")

    pd.AddArray(array1)
    pd.AddArray(array2)

    return molecule

def makeTable():
    table = svtk.svtkTable()

    num = 100

    a = [ x for x in range(num)]
    aa = numpy.array(a)

    b = [ x**2 for x in a]
    bb = numpy.array(b)

    array1 = np_s.numpy_to_svtk(aa, deep=True)
    array1.SetName("ID")

    array2 = np_s.numpy_to_svtk(bb, deep=True)
    array2.SetName("Square")

    table.AddColumn(array1)
    table.AddColumn(array2)

    return table

def testCalculator(dataset, attributeType, attributeTypeForCalc=None):
    calc = svtk.svtkArrayCalculator()
    calc.SetInputData(dataset)
    if attributeTypeForCalc is not None:
        calc.SetAttributeType(attributeTypeForCalc)
    calc.AddScalarArrayName("ID")
    calc.AddScalarArrayName("Square")
    calc.SetFunction("ID + Square / 24")
    calc.SetResultArrayName("Result")
    calc.Update()

    fd = calc.GetOutput().GetAttributes(attributeType)
    if not fd.HasArray("Result"):
        print("ERROR: calculator did not produce result array when processing datatype %d" % (attributeType,))


if __name__ == '__main__':
    testCalculator(makePolyData(svtk.svtkDataObject.POINT), svtk.svtkDataObject.POINT)
    testCalculator(makePolyData(svtk.svtkDataObject.CELL), svtk.svtkDataObject.CELL, svtk.svtkDataObject.CELL)
    testCalculator(makeGraph(svtk.svtkDataObject.VERTEX), svtk.svtkDataObject.VERTEX)
    testCalculator(makeGraph(svtk.svtkDataObject.EDGE), svtk.svtkDataObject.EDGE, svtk.svtkDataObject.EDGE)
    testCalculator(makeTable(), svtk.svtkDataObject.ROW)
