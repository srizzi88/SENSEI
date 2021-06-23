from __future__ import print_function

import svtk
from svtk.util.misc import svtkGetTempDir
SVTK_TEMP_DIR = svtkGetTempDir()

contr = svtk.svtkMultiProcessController.GetGlobalController()
if not contr:
    nranks = 1
    rank = 0
else:
    nranks = contr.GetNumberOfProcesses()
    rank = contr.GetLocalProcessId()

pf = svtk.svtkProgrammableFilter()

def execute():
    info = pf.GetOutputInformation(0)
    et = svtk.svtkExtentTranslator()

    if pf.GetInput().IsA("svtkDataSet"):
        et.SetWholeExtent(info.Get(svtk.svtkStreamingDemandDrivenPipeline.WHOLE_EXTENT()))
        et.SetPiece(info.Get(svtk.svtkStreamingDemandDrivenPipeline.UPDATE_PIECE_NUMBER()))
        et.SetNumberOfPieces(info.Get(svtk.svtkStreamingDemandDrivenPipeline.UPDATE_NUMBER_OF_PIECES()))
        et.PieceToExtent()

    output = pf.GetOutput()
    input = pf.GetInput()
    output.ShallowCopy(input)
    if pf.GetInput().IsA("svtkDataSet"):
        output.Crop(et.GetExtent())

pf.SetExecuteMethod(execute)

def GetSource(dataType):
    s = svtk.svtkRTAnalyticSource()

    if dataType == 'ImageData':
        return s

    elif dataType == 'UnstructuredGrid':
        dst = svtk.svtkDataSetTriangleFilter()
        dst.SetInputConnection(s.GetOutputPort())
        return dst

    elif dataType == 'RectilinearGrid':
        s.Update()

        input = s.GetOutput()

        rg = svtk.svtkRectilinearGrid()
        rg.SetExtent(input.GetExtent())
        dims = input.GetDimensions()
        spacing = input.GetSpacing()

        x = svtk.svtkFloatArray()
        x.SetNumberOfTuples(dims[0])
        for i in range(dims[0]):
            x.SetValue(i, spacing[0]*i)

        y = svtk.svtkFloatArray()
        y.SetNumberOfTuples(dims[1])
        for i in range(dims[1]):
            y.SetValue(i, spacing[1]*i)

        z = svtk.svtkFloatArray()
        z.SetNumberOfTuples(dims[2])
        for i in range(dims[2]):
            z.SetValue(i, spacing[2]*i)

        rg.SetXCoordinates(x)
        rg.SetYCoordinates(y)
        rg.SetZCoordinates(z)

        rg.GetPointData().ShallowCopy(input.GetPointData())

        pf.SetInputData(rg)
        return pf

    elif dataType == 'StructuredGrid':
        s.Update()

        input = s.GetOutput()

        sg = svtk.svtkStructuredGrid()
        sg.SetExtent(input.GetExtent())
        pts = svtk.svtkPoints()
        sg.SetPoints(pts)
        npts = input.GetNumberOfPoints()
        for i in range(npts):
            pts.InsertNextPoint(input.GetPoint(i))
        sg.GetPointData().ShallowCopy(input.GetPointData())

        pf.SetInputData(sg)
        return pf

    elif dataType == 'Table':
        s.Update()
        input = s.GetOutput()

        table = svtk.svtkTable()
        RTData = input.GetPointData().GetArray(0)
        nbTuples = RTData.GetNumberOfTuples()

        array = svtk.svtkFloatArray()
        array.SetName("RTData")
        array.SetNumberOfTuples(nbTuples)

        for i in range(0, nbTuples):
            array.SetTuple1(i, float(RTData.GetTuple1(i)))

        table.AddColumn(array)

        pf.SetInputData(table)
        return pf

def TestDataType(dataType, reader, writer, ext, numTris, useSubdir=False):
    s = GetSource(dataType)

    filename = SVTK_TEMP_DIR + "/%s.p%s" % (dataType, ext)

    writer.SetInputConnection(s.GetOutputPort())
    npieces = 16
    writer.SetNumberOfPieces(npieces)
    pperrank = npieces // nranks
    start = pperrank * rank
    end = start + pperrank - 1
    writer.SetStartPiece(start)
    writer.SetEndPiece(end)
    writer.SetFileName(filename)
    if useSubdir:
        writer.SetUseSubdirectory(True)
    #writer.SetDataModeToAscii()
    writer.Write()

    if contr:
        contr.Barrier()

    reader.SetFileName(filename)

    ntris = 0

    if dataType != "Table":
        cf = svtk.svtkContourFilter()
        cf.SetValue(0, 130)
        cf.SetComputeNormals(0)
        cf.SetComputeGradients(0)
        cf.SetInputConnection(reader.GetOutputPort())
        cf.UpdateInformation()
        cf.GetOutputInformation(0).Set(svtk.svtkStreamingDemandDrivenPipeline.UPDATE_NUMBER_OF_PIECES(), nranks)
        cf.GetOutputInformation(0).Set(svtk.svtkStreamingDemandDrivenPipeline.UPDATE_PIECE_NUMBER(), rank)
        cf.Update()

        ntris = cf.GetOutput().GetNumberOfCells()
    else:
        reader.UpdateInformation()
        reader.GetOutputInformation(0).Set(svtk.svtkStreamingDemandDrivenPipeline.UPDATE_NUMBER_OF_PIECES(), nranks)
        reader.GetOutputInformation(0).Set(svtk.svtkStreamingDemandDrivenPipeline.UPDATE_PIECE_NUMBER(), rank)
        reader.Update()
        ntris = reader.GetOutput().GetNumberOfRows()

    da = svtk.svtkIntArray()
    da.InsertNextValue(ntris)

    da2 = svtk.svtkIntArray()
    da2.SetNumberOfTuples(1)
    contr.AllReduce(da, da2, svtk.svtkCommunicator.SUM_OP)

    if rank == 0:
        print(da2.GetValue(0))
        import os
        os.remove(filename)
        for i in range(npieces):
            if not useSubdir:
                os.remove(SVTK_TEMP_DIR + "/%s_%d.%s" % (dataType, i, ext))
            else:
                os.remove(SVTK_TEMP_DIR + "/%s/%s_%d.%s" %(dataType, dataType, i, ext))

    assert da2.GetValue(0) == numTris

TestDataType('ImageData', svtk.svtkXMLPImageDataReader(), svtk.svtkXMLPImageDataWriter(), 'vti', 4924)
TestDataType('RectilinearGrid', svtk.svtkXMLPRectilinearGridReader(), svtk.svtkXMLPRectilinearGridWriter(), 'vtr', 4924)
TestDataType('StructuredGrid', svtk.svtkXMLPStructuredGridReader(), svtk.svtkXMLPStructuredGridWriter(), 'vts', 4924)
TestDataType('UnstructuredGrid', svtk.svtkXMLPUnstructuredGridReader(), svtk.svtkXMLPUnstructuredGridWriter(), 'vtu', 11856)
TestDataType('Table', svtk.svtkXMLPTableReader(), svtk.svtkXMLPTableWriter(), 'vtt', 18522)

# Test writers with UseSubdirectory on
TestDataType('ImageData', svtk.svtkXMLPImageDataReader(), svtk.svtkXMLPImageDataWriter(), 'vti', 4924, useSubdir=True)
TestDataType('RectilinearGrid', svtk.svtkXMLPRectilinearGridReader(), svtk.svtkXMLPRectilinearGridWriter(), 'vtr', 4924, useSubdir=True)
TestDataType('StructuredGrid', svtk.svtkXMLPStructuredGridReader(), svtk.svtkXMLPStructuredGridWriter(), 'vts', 4924, useSubdir=True)
TestDataType('UnstructuredGrid', svtk.svtkXMLPUnstructuredGridReader(), svtk.svtkXMLPUnstructuredGridWriter(), 'vtu', 11856, useSubdir=True)
TestDataType('Table', svtk.svtkXMLPTableReader(), svtk.svtkXMLPTableWriter(), 'vtt', 18522, useSubdir=True)
