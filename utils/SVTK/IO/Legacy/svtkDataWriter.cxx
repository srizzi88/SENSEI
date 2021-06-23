/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDataWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkDataWriter.h"

#include "svtkBitArray.h"
#include "svtkByteSwap.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCharArray.h"
#include "svtkDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkErrorCode.h"
#include "svtkExecutive.h"
#include "svtkFieldData.h"
#include "svtkFloatArray.h"
#include "svtkGraph.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationDoubleKey.h"
#include "svtkInformationDoubleVectorKey.h"
#include "svtkInformationIdTypeKey.h"
#include "svtkInformationIntegerKey.h"
#include "svtkInformationIntegerVectorKey.h"
#include "svtkInformationIterator.h"
#include "svtkInformationKeyLookup.h"
#include "svtkInformationStringKey.h"
#include "svtkInformationStringVectorKey.h"
#include "svtkInformationUnsignedLongKey.h"
#include "svtkIntArray.h"
#include "svtkLegacyReaderVersion.h"
#include "svtkLongArray.h"
#include "svtkLookupTable.h"
#include "svtkMath.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#ifdef SVTK_USE_SCALED_SOA_ARRAYS
#include "svtkScaledSOADataArrayTemplate.h"
#endif
#include "svtkSOADataArrayTemplate.h"
#include "svtkShortArray.h"
#include "svtkSignedCharArray.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkTypeInt64Array.h"
#include "svtkTypeTraits.h"
#include "svtkTypeUInt64Array.h"
#include "svtkUnicodeStringArray.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnsignedIntArray.h"
#include "svtkUnsignedLongArray.h"
#include "svtkUnsignedShortArray.h"
#include "svtkVariantArray.h"
#include "svtksys/FStream.hxx"

#include <cstdio>
#include <sstream>

svtkStandardNewMacro(svtkDataWriter);

// this undef is required on the hp. svtkMutexLock ends up including
// /usr/include/dce/cma_ux.h which has the gall to #define write as cma_write

#ifdef write
#undef write
#endif

// Created object with default header, ASCII format, and default names for
// scalars, vectors, tensors, normals, and texture coordinates.
svtkDataWriter::svtkDataWriter()
{
  this->FileName = nullptr;

  this->Header = new char[257];
  strcpy(this->Header, "svtk output");
  this->FileType = SVTK_ASCII;

  this->ScalarsName = nullptr;
  this->VectorsName = nullptr;
  this->TensorsName = nullptr;
  this->NormalsName = nullptr;
  this->TCoordsName = nullptr;
  this->GlobalIdsName = nullptr;
  this->PedigreeIdsName = nullptr;
  this->EdgeFlagsName = nullptr;

  this->LookupTableName = new char[13];
  strcpy(this->LookupTableName, "lookup_table");

  this->FieldDataName = new char[10];
  strcpy(this->FieldDataName, "FieldData");

  this->WriteToOutputString = 0;
  this->OutputString = nullptr;
  this->OutputStringLength = 0;
  this->WriteArrayMetaData = true;
}

svtkDataWriter::~svtkDataWriter()
{
  delete[] this->FileName;
  delete[] this->Header;
  delete[] this->ScalarsName;
  delete[] this->VectorsName;
  delete[] this->TensorsName;
  delete[] this->NormalsName;
  delete[] this->TCoordsName;
  delete[] this->GlobalIdsName;
  delete[] this->PedigreeIdsName;
  delete[] this->EdgeFlagsName;
  delete[] this->LookupTableName;
  delete[] this->FieldDataName;

  delete[] this->OutputString;
  this->OutputString = nullptr;
  this->OutputStringLength = 0;
}

// Open a svtk data file. Returns nullptr if error.
ostream* svtkDataWriter::OpenSVTKFile()
{
  // Save current locale settings and set standard one to
  // avoid locale issues - for instance with the decimal separator.
  this->CurrentLocale = std::locale::global(std::locale::classic());

  ostream* fptr;

  if ((!this->WriteToOutputString) && (!this->FileName))
  {
    svtkErrorMacro(<< "No FileName specified! Can't write!");
    this->SetErrorCode(svtkErrorCode::NoFileNameError);
    return nullptr;
  }

  svtkDebugMacro(<< "Opening svtk file for writing...");

  if (this->WriteToOutputString)
  {
    // Get rid of any old output string.
    delete[] this->OutputString;
    this->OutputString = nullptr;
    this->OutputStringLength = 0;

    // Allocate the new output string. (Note: this will only work with binary).
    if (!this->GetInputExecutive(0, 0))
    {
      svtkErrorMacro(<< "No input! Can't write!");
      return nullptr;
    }
    this->GetInputExecutive(0, 0)->Update();
    /// OutputString will be allocated on CloseSVTKFile().
    /// this->OutputStringAllocatedLength =
    ///   static_cast<int> (500+ 1024 * input->GetActualMemorySize());
    /// this->OutputString = new char[this->OutputStringAllocatedLength];

    fptr = new std::ostringstream;
  }
  else
  {
    if (this->FileType == SVTK_ASCII)
    {
      fptr = new svtksys::ofstream(this->FileName, ios::out);
    }
    else
    {
#ifdef _WIN32
      fptr = new svtksys::ofstream(this->FileName, ios::out | ios::binary);
#else
      fptr = new svtksys::ofstream(this->FileName, ios::out);
#endif
    }
  }

  if (fptr->fail())
  {
    svtkErrorMacro(<< "Unable to open file: " << this->FileName);
    this->SetErrorCode(svtkErrorCode::CannotOpenFileError);
    delete fptr;
    return nullptr;
  }

  return fptr;
}

// Write the header of a svtk data file. Returns 0 if error.
int svtkDataWriter::WriteHeader(ostream* fp)
{
  svtkDebugMacro(<< "Writing header...");

  *fp << "# svtk DataFile Version " << svtkLegacyReaderMajorVersion << "."
      << svtkLegacyReaderMinorVersion << "\n";
  *fp << this->Header << "\n";

  if (this->FileType == SVTK_ASCII)
  {
    *fp << "ASCII\n";
  }
  else
  {
    *fp << "BINARY\n";
  }

  fp->flush();
  if (fp->fail())
  {
    this->SetErrorCode(svtkErrorCode::OutOfDiskSpaceError);
    return 0;
  }

  return 1;
}

// Write the cell data (e.g., scalars, vectors, ...) of a svtk dataset.
// Returns 0 if error.
int svtkDataWriter::WriteCellData(ostream* fp, svtkDataSet* ds)
{
  svtkIdType numCells;
  svtkDataArray* scalars;
  svtkDataArray* vectors;
  svtkDataArray* normals;
  svtkDataArray* tcoords;
  svtkDataArray* tensors;
  svtkDataArray* globalIds;
  svtkAbstractArray* pedigreeIds;
  svtkFieldData* field;
  svtkCellData* cd = ds->GetCellData();

  svtkDebugMacro(<< "Writing cell data...");

  numCells = ds->GetNumberOfCells();
  if (numCells <= 0)
  {
    svtkDebugMacro(<< "No cell data to write!");
    return 1;
  }

  scalars = cd->GetScalars();
  if (scalars && scalars->GetNumberOfTuples() <= 0)
    scalars = nullptr;

  vectors = cd->GetVectors();
  if (vectors && vectors->GetNumberOfTuples() <= 0)
    vectors = nullptr;

  normals = cd->GetNormals();
  if (normals && normals->GetNumberOfTuples() <= 0)
    normals = nullptr;

  tcoords = cd->GetTCoords();
  if (tcoords && tcoords->GetNumberOfTuples() <= 0)
    tcoords = nullptr;

  tensors = cd->GetTensors();
  if (tensors && tensors->GetNumberOfTuples() <= 0)
    tensors = nullptr;

  globalIds = cd->GetGlobalIds();
  if (globalIds && globalIds->GetNumberOfTuples() <= 0)
    globalIds = nullptr;

  pedigreeIds = cd->GetPedigreeIds();
  if (pedigreeIds && pedigreeIds->GetNumberOfTuples() <= 0)
    pedigreeIds = nullptr;

  field = cd;
  if (field && field->GetNumberOfTuples() <= 0)
    field = nullptr;

  if (!(scalars || vectors || normals || tcoords || tensors || globalIds || pedigreeIds || field))
  {
    svtkDebugMacro(<< "No cell data to write!");
    return 1;
  }

  *fp << "CELL_DATA " << numCells << "\n";
  //
  // Write scalar data
  //
  if (scalars)
  {
    if (!this->WriteScalarData(fp, scalars, numCells))
    {
      return 0;
    }
  }
  //
  // Write vector data
  //
  if (vectors)
  {
    if (!this->WriteVectorData(fp, vectors, numCells))
    {
      return 0;
    }
  }
  //
  // Write normals
  //
  if (normals)
  {
    if (!this->WriteNormalData(fp, normals, numCells))
    {
      return 0;
    }
  }
  //
  // Write texture coords
  //
  if (tcoords)
  {
    if (!this->WriteTCoordData(fp, tcoords, numCells))
    {
      return 0;
    }
  }
  //
  // Write tensors
  //
  if (tensors)
  {
    if (!this->WriteTensorData(fp, tensors, numCells))
    {
      return 0;
    }
  }
  //
  // Write global ids
  //
  if (globalIds)
  {
    if (!this->WriteGlobalIdData(fp, globalIds, numCells))
    {
      return 0;
    }
  }
  //
  // Write pedigree ids
  //
  if (pedigreeIds)
  {
    if (!this->WritePedigreeIdData(fp, pedigreeIds, numCells))
    {
      return 0;
    }
  }
  //
  // Write field
  //
  if (field)
  {
    if (!this->WriteFieldData(fp, field))
    {
      return 0;
    }
  }

  return 1;
}

// Write the point data (e.g., scalars, vectors, ...) of a svtk dataset.
// Returns 0 if error.
int svtkDataWriter::WritePointData(ostream* fp, svtkDataSet* ds)
{
  svtkIdType numPts;
  svtkDataArray* scalars;
  svtkDataArray* vectors;
  svtkDataArray* normals;
  svtkDataArray* tcoords;
  svtkDataArray* tensors;
  svtkDataArray* globalIds;
  svtkAbstractArray* pedigreeIds;
  svtkDataArray* edgeFlags;
  svtkFieldData* field;
  svtkPointData* pd = ds->GetPointData();

  svtkDebugMacro(<< "Writing point data...");

  numPts = ds->GetNumberOfPoints();
  if (numPts <= 0)
  {
    svtkDebugMacro(<< "No point data to write!");
    return 1;
  }

  scalars = pd->GetScalars();
  if (scalars && scalars->GetNumberOfTuples() <= 0)
    scalars = nullptr;

  vectors = pd->GetVectors();
  if (vectors && vectors->GetNumberOfTuples() <= 0)
    vectors = nullptr;

  normals = pd->GetNormals();
  if (normals && normals->GetNumberOfTuples() <= 0)
    normals = nullptr;

  tcoords = pd->GetTCoords();
  if (tcoords && tcoords->GetNumberOfTuples() <= 0)
    tcoords = nullptr;

  tensors = pd->GetTensors();
  if (tensors && tensors->GetNumberOfTuples() <= 0)
    tensors = nullptr;

  globalIds = pd->GetGlobalIds();
  if (globalIds && globalIds->GetNumberOfTuples() <= 0)
    globalIds = nullptr;

  pedigreeIds = pd->GetPedigreeIds();
  if (pedigreeIds && pedigreeIds->GetNumberOfTuples() <= 0)
    pedigreeIds = nullptr;

  edgeFlags = pd->GetAttribute(svtkDataSetAttributes::EDGEFLAG);
  if (edgeFlags && edgeFlags->GetNumberOfTuples() <= 0)
    edgeFlags = nullptr;

  field = pd;
  if (field && field->GetNumberOfTuples() <= 0)
    field = nullptr;

  if (!(scalars || vectors || normals || tcoords || tensors || globalIds || pedigreeIds ||
        edgeFlags || field))
  {
    svtkDebugMacro(<< "No point data to write!");
    return 1;
  }

  *fp << "POINT_DATA " << numPts << "\n";
  //
  // Write scalar data
  //
  if (scalars)
  {
    if (!this->WriteScalarData(fp, scalars, numPts))
    {
      return 0;
    }
  }
  //
  // Write vector data
  //
  if (vectors)
  {
    if (!this->WriteVectorData(fp, vectors, numPts))
    {
      return 0;
    }
  }
  //
  // Write normals
  //
  if (normals)
  {
    if (!this->WriteNormalData(fp, normals, numPts))
    {
      return 0;
    }
  }
  //
  // Write texture coords
  //
  if (tcoords)
  {
    if (!this->WriteTCoordData(fp, tcoords, numPts))
    {
      return 0;
    }
  }
  //
  // Write tensors
  //
  if (tensors)
  {
    if (!this->WriteTensorData(fp, tensors, numPts))
    {
      return 0;
    }
  }
  //
  // Write global ids
  //
  if (globalIds)
  {
    if (!this->WriteGlobalIdData(fp, globalIds, numPts))
    {
      return 0;
    }
  }
  //
  // Write pedigree ids
  //
  if (pedigreeIds)
  {
    if (!this->WritePedigreeIdData(fp, pedigreeIds, numPts))
    {
      return 0;
    }
  }
  //
  // Write edge flags
  //
  if (edgeFlags)
  {
    if (!this->WriteEdgeFlagsData(fp, edgeFlags, numPts))
    {
      return 0;
    }
  }
  //
  // Write field
  //
  if (field)
  {
    if (!this->WriteFieldData(fp, field))
    {
      return 0;
    }
  }

  return 1;
}

// Write the vertex data (e.g., scalars, vectors, ...) of a svtk graph.
// Returns 0 if error.
int svtkDataWriter::WriteVertexData(ostream* fp, svtkGraph* ds)
{
  svtkIdType numVertices;
  svtkDataArray* scalars;
  svtkDataArray* vectors;
  svtkDataArray* normals;
  svtkDataArray* tcoords;
  svtkDataArray* tensors;
  svtkDataArray* globalIds;
  svtkAbstractArray* pedigreeIds;
  svtkFieldData* field;
  svtkDataSetAttributes* cd = ds->GetVertexData();

  svtkDebugMacro(<< "Writing vertex data...");

  numVertices = ds->GetNumberOfVertices();
  if (numVertices <= 0)
  {
    svtkDebugMacro(<< "No vertex data to write!");
    return 1;
  }

  scalars = cd->GetScalars();
  if (scalars && scalars->GetNumberOfTuples() <= 0)
    scalars = nullptr;

  vectors = cd->GetVectors();
  if (vectors && vectors->GetNumberOfTuples() <= 0)
    vectors = nullptr;

  normals = cd->GetNormals();
  if (normals && normals->GetNumberOfTuples() <= 0)
    normals = nullptr;

  tcoords = cd->GetTCoords();
  if (tcoords && tcoords->GetNumberOfTuples() <= 0)
    tcoords = nullptr;

  tensors = cd->GetTensors();
  if (tensors && tensors->GetNumberOfTuples() <= 0)
    tensors = nullptr;

  globalIds = cd->GetGlobalIds();
  if (globalIds && globalIds->GetNumberOfTuples() <= 0)
    globalIds = nullptr;

  pedigreeIds = cd->GetPedigreeIds();
  if (pedigreeIds && pedigreeIds->GetNumberOfTuples() <= 0)
    pedigreeIds = nullptr;

  field = cd;
  if (field && field->GetNumberOfTuples() <= 0)
    field = nullptr;

  if (!(scalars || vectors || normals || tcoords || tensors || globalIds || pedigreeIds || field))
  {
    svtkDebugMacro(<< "No cell data to write!");
    return 1;
  }

  *fp << "VERTEX_DATA " << numVertices << "\n";
  //
  // Write scalar data
  //
  if (scalars)
  {
    if (!this->WriteScalarData(fp, scalars, numVertices))
    {
      return 0;
    }
  }
  //
  // Write vector data
  //
  if (vectors)
  {
    if (!this->WriteVectorData(fp, vectors, numVertices))
    {
      return 0;
    }
  }
  //
  // Write normals
  //
  if (normals)
  {
    if (!this->WriteNormalData(fp, normals, numVertices))
    {
      return 0;
    }
  }
  //
  // Write texture coords
  //
  if (tcoords)
  {
    if (!this->WriteTCoordData(fp, tcoords, numVertices))
    {
      return 0;
    }
  }
  //
  // Write tensors
  //
  if (tensors)
  {
    if (!this->WriteTensorData(fp, tensors, numVertices))
    {
      return 0;
    }
  }
  //
  // Write global ids
  //
  if (globalIds)
  {
    if (!this->WriteGlobalIdData(fp, globalIds, numVertices))
    {
      return 0;
    }
  }
  //
  // Write pedigree ids
  //
  if (pedigreeIds)
  {
    if (!this->WritePedigreeIdData(fp, pedigreeIds, numVertices))
    {
      return 0;
    }
  }
  //
  // Write field
  //
  if (field)
  {
    if (!this->WriteFieldData(fp, field))
    {
      return 0;
    }
  }

  return 1;
}

// Write the edge data (e.g., scalars, vectors, ...) of a svtk graph.
// Returns 0 if error.
int svtkDataWriter::WriteEdgeData(ostream* fp, svtkGraph* g)
{
  svtkIdType numEdges;
  svtkDataArray* scalars;
  svtkDataArray* vectors;
  svtkDataArray* normals;
  svtkDataArray* tcoords;
  svtkDataArray* tensors;
  svtkDataArray* globalIds;
  svtkAbstractArray* pedigreeIds;
  svtkFieldData* field;
  svtkDataSetAttributes* cd = g->GetEdgeData();

  svtkDebugMacro(<< "Writing edge data...");

  numEdges = g->GetNumberOfEdges();
  if (numEdges <= 0)
  {
    svtkDebugMacro(<< "No edge data to write!");
    return 1;
  }

  scalars = cd->GetScalars();
  if (scalars && scalars->GetNumberOfTuples() <= 0)
    scalars = nullptr;

  vectors = cd->GetVectors();
  if (vectors && vectors->GetNumberOfTuples() <= 0)
    vectors = nullptr;

  normals = cd->GetNormals();
  if (normals && normals->GetNumberOfTuples() <= 0)
    normals = nullptr;

  tcoords = cd->GetTCoords();
  if (tcoords && tcoords->GetNumberOfTuples() <= 0)
    tcoords = nullptr;

  tensors = cd->GetTensors();
  if (tensors && tensors->GetNumberOfTuples() <= 0)
    tensors = nullptr;

  globalIds = cd->GetGlobalIds();
  if (globalIds && globalIds->GetNumberOfTuples() <= 0)
    globalIds = nullptr;

  pedigreeIds = cd->GetPedigreeIds();
  if (pedigreeIds && pedigreeIds->GetNumberOfTuples() <= 0)
    pedigreeIds = nullptr;

  field = cd;
  if (field && field->GetNumberOfTuples() <= 0)
    field = nullptr;

  if (!(scalars || vectors || normals || tcoords || tensors || globalIds || pedigreeIds || field))
  {
    svtkDebugMacro(<< "No edge data to write!");
    return 1;
  }

  *fp << "EDGE_DATA " << numEdges << "\n";
  //
  // Write scalar data
  //
  if (scalars)
  {
    if (!this->WriteScalarData(fp, scalars, numEdges))
    {
      return 0;
    }
  }
  //
  // Write vector data
  //
  if (vectors)
  {
    if (!this->WriteVectorData(fp, vectors, numEdges))
    {
      return 0;
    }
  }
  //
  // Write normals
  //
  if (normals)
  {
    if (!this->WriteNormalData(fp, normals, numEdges))
    {
      return 0;
    }
  }
  //
  // Write texture coords
  //
  if (tcoords)
  {
    if (!this->WriteTCoordData(fp, tcoords, numEdges))
    {
      return 0;
    }
  }
  //
  // Write tensors
  //
  if (tensors)
  {
    if (!this->WriteTensorData(fp, tensors, numEdges))
    {
      return 0;
    }
  }
  //
  // Write global ids
  //
  if (globalIds)
  {
    if (!this->WriteGlobalIdData(fp, globalIds, numEdges))
    {
      return 0;
    }
  }
  //
  // Write pedigree ids
  //
  if (pedigreeIds)
  {
    if (!this->WritePedigreeIdData(fp, pedigreeIds, numEdges))
    {
      return 0;
    }
  }
  //
  // Write field
  //
  if (field)
  {
    if (!this->WriteFieldData(fp, field))
    {
      return 0;
    }
  }

  return 1;
}

// Write the row data (e.g., scalars, vectors, ...) of a svtk table.
// Returns 0 if error.
int svtkDataWriter::WriteRowData(ostream* fp, svtkTable* t)
{
  svtkIdType numRows;
  svtkDataArray* scalars;
  svtkDataArray* vectors;
  svtkDataArray* normals;
  svtkDataArray* tcoords;
  svtkDataArray* tensors;
  svtkDataArray* globalIds;
  svtkAbstractArray* pedigreeIds;
  svtkFieldData* field;
  svtkDataSetAttributes* cd = t->GetRowData();

  numRows = t->GetNumberOfRows();

  svtkDebugMacro(<< "Writing row data...");

  scalars = cd->GetScalars();
  if (scalars && scalars->GetNumberOfTuples() <= 0)
    scalars = nullptr;

  vectors = cd->GetVectors();
  if (vectors && vectors->GetNumberOfTuples() <= 0)
    vectors = nullptr;

  normals = cd->GetNormals();
  if (normals && normals->GetNumberOfTuples() <= 0)
    normals = nullptr;

  tcoords = cd->GetTCoords();
  if (tcoords && tcoords->GetNumberOfTuples() <= 0)
    tcoords = nullptr;

  tensors = cd->GetTensors();
  if (tensors && tensors->GetNumberOfTuples() <= 0)
    tensors = nullptr;

  globalIds = cd->GetGlobalIds();
  if (globalIds && globalIds->GetNumberOfTuples() <= 0)
    globalIds = nullptr;

  pedigreeIds = cd->GetPedigreeIds();
  if (pedigreeIds && pedigreeIds->GetNumberOfTuples() <= 0)
    pedigreeIds = nullptr;

  field = cd;
  if (field && field->GetNumberOfTuples() <= 0)
    field = nullptr;

  if (!(scalars || vectors || normals || tcoords || tensors || globalIds || pedigreeIds || field))
  {
    svtkDebugMacro(<< "No row data to write!");
    return 1;
  }

  *fp << "ROW_DATA " << numRows << "\n";
  //
  // Write scalar data
  //
  if (scalars)
  {
    if (!this->WriteScalarData(fp, scalars, numRows))
    {
      return 0;
    }
  }
  //
  // Write vector data
  //
  if (vectors)
  {
    if (!this->WriteVectorData(fp, vectors, numRows))
    {
      return 0;
    }
  }
  //
  // Write normals
  //
  if (normals)
  {
    if (!this->WriteNormalData(fp, normals, numRows))
    {
      return 0;
    }
  }
  //
  // Write texture coords
  //
  if (tcoords)
  {
    if (!this->WriteTCoordData(fp, tcoords, numRows))
    {
      return 0;
    }
  }
  //
  // Write tensors
  //
  if (tensors)
  {
    if (!this->WriteTensorData(fp, tensors, numRows))
    {
      return 0;
    }
  }
  //
  // Write global ids
  //
  if (globalIds)
  {
    if (!this->WriteGlobalIdData(fp, globalIds, numRows))
    {
      return 0;
    }
  }
  //
  // Write pedigree ids
  //
  if (pedigreeIds)
  {
    if (!this->WritePedigreeIdData(fp, pedigreeIds, numRows))
    {
      return 0;
    }
  }
  //
  // Write field
  //
  if (field)
  {
    if (!this->WriteFieldData(fp, field))
    {
      return 0;
    }
  }

  return 1;
}

namespace
{
// Template to handle writing data in ascii or binary
// We could change the format into C++ io standard ...
template <class T>
void svtkWriteDataArray(
  ostream* fp, T* data, int fileType, const char* format, svtkIdType num, svtkIdType numComp)
{
  svtkIdType i, j, idx, sizeT;
  char str[1024];

  sizeT = sizeof(T);

  if (fileType == SVTK_ASCII)
  {
    for (j = 0; j < num; j++)
    {
      for (i = 0; i < numComp; i++)
      {
        idx = i + j * numComp;
        snprintf(str, sizeof(str), format, *data++);
        *fp << str;
        if (!((idx + 1) % 9))
        {
          *fp << "\n";
        }
      }
    }
  }
  else
  {
    if (num * numComp > 0)
    {
      // need to byteswap ??
      switch (sizeT)
      {
        case 2:
          // no typecast needed here; method call takes void* data
          svtkByteSwap::SwapWrite2BERange(data, num * numComp, fp);
          break;
        case 4:
          // no typecast needed here; method call takes void* data
          svtkByteSwap::SwapWrite4BERange(data, num * numComp, fp);
          break;
        case 8:
          // no typecast needed here; method call takes void* data
          svtkByteSwap::SwapWrite8BERange(data, num * numComp, fp);
          break;
        default:
          fp->write(reinterpret_cast<char*>(data), sizeof(T) * (num * numComp));
          break;
      }
    }
  }
  *fp << "\n";
}

// Returns a pointer to the data ordered in original SVTK style ordering
// of the data. If this is an SOA array it has to allocate the memory
// for that in which case the calling function must delete it.
template <class T>
T* GetArrayRawPointer(svtkAbstractArray* array, T* ptr, int isAOSArray)
{
  if (isAOSArray)
  {
    return ptr;
  }
  if (svtkSOADataArrayTemplate<T>* typedArray = svtkSOADataArrayTemplate<T>::SafeDownCast(array))
  {
    T* data = new T[array->GetNumberOfComponents() * array->GetNumberOfTuples()];
    typedArray->ExportToVoidPointer(data);
    return data;
  }
#ifdef SVTK_USE_SCALED_SOA_ARRAYS
  else if (svtkScaledSOADataArrayTemplate<T>* typedScaleArray =
             svtkScaledSOADataArrayTemplate<T>::SafeDownCast(array))
  {
    T* data = new T[array->GetNumberOfComponents() * array->GetNumberOfTuples()];
    typedScaleArray->ExportToVoidPointer(data);
    return data;
  }
#endif
  svtkGenericWarningMacro(
    "Do not know how to handle array type " << array->GetClassName() << " in svtkDataWriter");
  return nullptr;
}

} // end anonymous namespace

// Write out data to file specified.
int svtkDataWriter::WriteArray(ostream* fp, int dataType, svtkAbstractArray* data, const char* format,
  svtkIdType num, svtkIdType numComp)
{
  svtkIdType i, j, idx;
  char str[1024];

  bool isAOSArray = data->HasStandardMemoryLayout();

  char* outputFormat = new char[10];
  switch (dataType)
  {
    case SVTK_BIT:
    { // assume that bit array is always in original AOS ordering
      snprintf(str, sizeof(str), format, "bit");
      *fp << str;
      if (this->FileType == SVTK_ASCII)
      {
        int s;
        for (j = 0; j < num; j++)
        {
          for (i = 0; i < numComp; i++)
          {
            idx = i + j * numComp;
            s = static_cast<svtkBitArray*>(data)->GetValue(idx);
            *fp << (s != 0.0 ? 1 : 0);
            if (!((idx + 1) % 8))
            {
              *fp << "\n";
            }
            else
            {
              *fp << " ";
            }
          }
        }
      }
      else
      {
        unsigned char* cptr = static_cast<svtkBitArray*>(data)->GetPointer(0);
        fp->write(reinterpret_cast<char*>(cptr), (sizeof(unsigned char)) * ((num - 1) / 8 + 1));
      }
      *fp << "\n";
    }
    break;

    case SVTK_CHAR:
    {
      snprintf(str, sizeof(str), format, "char");
      *fp << str;
      char* s =
        GetArrayRawPointer(data, static_cast<svtkCharArray*>(data)->GetPointer(0), isAOSArray);
#if SVTK_TYPE_CHAR_IS_SIGNED
      svtkWriteDataArray(fp, s, this->FileType, "%hhd ", num, numComp);
#else
      svtkWriteDataArray(fp, s, this->FileType, "%hhu ", num, numComp);
#endif
      if (!isAOSArray)
      {
        delete[] s;
      }
    }
    break;

    case SVTK_SIGNED_CHAR:
    {
      snprintf(str, sizeof(str), format, "signed_char");
      *fp << str;
      signed char* s =
        GetArrayRawPointer(data, static_cast<svtkSignedCharArray*>(data)->GetPointer(0), isAOSArray);
      svtkWriteDataArray(fp, s, this->FileType, "%hhd ", num, numComp);
      if (!isAOSArray)
      {
        delete[] s;
      }
    }
    break;

    case SVTK_UNSIGNED_CHAR:
    {
      snprintf(str, sizeof(str), format, "unsigned_char");
      *fp << str;
      unsigned char* s = GetArrayRawPointer(
        data, static_cast<svtkUnsignedCharArray*>(data)->GetPointer(0), isAOSArray);
      svtkWriteDataArray(fp, s, this->FileType, "%hhu ", num, numComp);
      if (!isAOSArray)
      {
        delete[] s;
      }
    }
    break;

    case SVTK_SHORT:
    {
      snprintf(str, sizeof(str), format, "short");
      *fp << str;
      short* s =
        GetArrayRawPointer(data, static_cast<svtkShortArray*>(data)->GetPointer(0), isAOSArray);
      svtkWriteDataArray(fp, s, this->FileType, "%hd ", num, numComp);
      if (!isAOSArray)
      {
        delete[] s;
      }
    }
    break;

    case SVTK_UNSIGNED_SHORT:
    {
      snprintf(str, sizeof(str), format, "unsigned_short");
      *fp << str;
      unsigned short* s = GetArrayRawPointer(
        data, static_cast<svtkUnsignedShortArray*>(data)->GetPointer(0), isAOSArray);
      svtkWriteDataArray(fp, s, this->FileType, "%hu ", num, numComp);
      if (!isAOSArray)
      {
        delete[] s;
      }
    }
    break;

    case SVTK_INT:
    {
      snprintf(str, sizeof(str), format, "int");
      *fp << str;
      int* s = GetArrayRawPointer(data, static_cast<svtkIntArray*>(data)->GetPointer(0), isAOSArray);
      svtkWriteDataArray(fp, s, this->FileType, "%d ", num, numComp);
      if (!isAOSArray)
      {
        delete[] s;
      }
    }
    break;

    case SVTK_UNSIGNED_INT:
    {
      snprintf(str, sizeof(str), format, "unsigned_int");
      *fp << str;
      unsigned int* s = GetArrayRawPointer(
        data, static_cast<svtkUnsignedIntArray*>(data)->GetPointer(0), isAOSArray);
      svtkWriteDataArray(fp, s, this->FileType, "%u ", num, numComp);
      if (!isAOSArray)
      {
        delete[] s;
      }
    }
    break;

    case SVTK_LONG:
    {
      snprintf(str, sizeof(str), format, "long");
      *fp << str;
      long* s =
        GetArrayRawPointer(data, static_cast<svtkLongArray*>(data)->GetPointer(0), isAOSArray);
      svtkWriteDataArray(fp, s, this->FileType, "%ld ", num, numComp);
      if (!isAOSArray)
      {
        delete[] s;
      }
    }
    break;

    case SVTK_UNSIGNED_LONG:
    {
      snprintf(str, sizeof(str), format, "unsigned_long");
      *fp << str;
      unsigned long* s = GetArrayRawPointer(
        data, static_cast<svtkUnsignedLongArray*>(data)->GetPointer(0), isAOSArray);
      svtkWriteDataArray(fp, s, this->FileType, "%lu ", num, numComp);
      if (!isAOSArray)
      {
        delete[] s;
      }
    }
    break;

    case SVTK_LONG_LONG:
    {
      snprintf(str, sizeof(str), format, "svtktypeint64");
      *fp << str;
      long long* s =
        GetArrayRawPointer(data, static_cast<svtkTypeInt64Array*>(data)->GetPointer(0), isAOSArray);
      strcpy(outputFormat, svtkTypeTraits<long long>::ParseFormat());
      strcat(outputFormat, " ");
      svtkWriteDataArray(fp, s, this->FileType, outputFormat, num, numComp);
      if (!isAOSArray)
      {
        delete[] s;
      }
    }
    break;

    case SVTK_UNSIGNED_LONG_LONG:
    {
      snprintf(str, sizeof(str), format, "svtktypeuint64");
      *fp << str;
      unsigned long long* s =
        GetArrayRawPointer(data, static_cast<svtkTypeUInt64Array*>(data)->GetPointer(0), isAOSArray);
      strcpy(outputFormat, svtkTypeTraits<unsigned long long>::ParseFormat());
      strcat(outputFormat, " ");
      svtkWriteDataArray(fp, s, this->FileType, outputFormat, num, numComp);
      if (!isAOSArray)
      {
        delete[] s;
      }
    }
    break;

    case SVTK_FLOAT:
    {
      snprintf(str, sizeof(str), format, "float");
      *fp << str;
      float* s =
        GetArrayRawPointer(data, static_cast<svtkFloatArray*>(data)->GetPointer(0), isAOSArray);
      svtkWriteDataArray(fp, s, this->FileType, "%g ", num, numComp);
      if (!isAOSArray)
      {
        delete[] s;
      }
    }
    break;

    case SVTK_DOUBLE:
    {
      snprintf(str, sizeof(str), format, "double");
      *fp << str;
      double* s =
        GetArrayRawPointer(data, static_cast<svtkDoubleArray*>(data)->GetPointer(0), isAOSArray);
      svtkWriteDataArray(fp, s, this->FileType, "%.11lg ", num, numComp);
      if (!isAOSArray)
      {
        delete[] s;
      }
    }
    break;

    case SVTK_ID_TYPE:
    {
      // currently writing svtkIdType as int.
      svtkIdType size = data->GetNumberOfTuples();
      std::vector<int> intArray(size * numComp);
      snprintf(str, sizeof(str), format, "svtkIdType");
      *fp << str;
      if (isAOSArray)
      {
        svtkIdType* s = static_cast<svtkIdTypeArray*>(data)->GetPointer(0);
        for (svtkIdType jj = 0; jj < size * numComp; jj++)
        {
          intArray[jj] = s[jj];
        }
      }
      else
      {
        svtkSOADataArrayTemplate<svtkIdType>* data2 =
          static_cast<svtkSOADataArrayTemplate<svtkIdType>*>(data);
        std::vector<svtkIdType> vals(numComp);
        for (svtkIdType jj = 0; jj < size; jj++)
        {
          data2->GetTypedTuple(jj, &vals[0]);
          for (i = 0; i < numComp; i++)
          {
            intArray[jj * numComp + i] = vals[i];
          }
        }
      }
      svtkWriteDataArray(fp, &intArray[0], this->FileType, "%d ", num, numComp);
    }
    break;

    case SVTK_STRING:
    {
      snprintf(str, sizeof(str), format, "string");
      *fp << str;
      if (this->FileType == SVTK_ASCII)
      {
        svtkStdString s;
        for (j = 0; j < num; j++)
        {
          for (i = 0; i < numComp; i++)
          {
            idx = i + j * numComp;
            s = static_cast<svtkStringArray*>(data)->GetValue(idx);
            this->EncodeWriteString(fp, s.c_str(), false);
            *fp << "\n";
          }
        }
      }
      else
      {
        svtkStdString s;
        for (j = 0; j < num; j++)
        {
          for (i = 0; i < numComp; i++)
          {
            idx = i + j * numComp;
            s = static_cast<svtkStringArray*>(data)->GetValue(idx);
            svtkTypeUInt64 length = s.length();
            if (length < (static_cast<svtkTypeUInt64>(1) << 6))
            {
              svtkTypeUInt8 len =
                (static_cast<svtkTypeUInt8>(3) << 6) | static_cast<svtkTypeUInt8>(length);
              fp->write(reinterpret_cast<char*>(&len), 1);
            }
            else if (length < (static_cast<svtkTypeUInt64>(1) << 14))
            {
              svtkTypeUInt16 len =
                (static_cast<svtkTypeUInt16>(2) << 14) | static_cast<svtkTypeUInt16>(length);
              svtkByteSwap::SwapWrite2BERange(&len, 1, fp);
            }
            else if (length < (static_cast<svtkTypeUInt64>(1) << 30))
            {
              svtkTypeUInt32 len =
                (static_cast<svtkTypeUInt32>(1) << 30) | static_cast<svtkTypeUInt32>(length);
              svtkByteSwap::SwapWrite4BERange(&len, 1, fp);
            }
            else
            {
              svtkByteSwap::SwapWrite8BERange(&length, 1, fp);
            }
            fp->write(s.c_str(), length);
          }
        }
      }
      *fp << "\n";
    }
    break;

    case SVTK_UNICODE_STRING:
    {
      snprintf(str, sizeof(str), format, "utf8_string");
      *fp << str;
      if (this->FileType == SVTK_ASCII)
      {
        svtkStdString s;
        for (j = 0; j < num; j++)
        {
          for (i = 0; i < numComp; i++)
          {
            idx = i + j * numComp;
            s = static_cast<svtkUnicodeStringArray*>(data)->GetValue(idx).utf8_str();
            this->EncodeWriteString(fp, s.c_str(), false);
            *fp << "\n";
          }
        }
      }
      else
      {
        svtkStdString s;
        for (j = 0; j < num; j++)
        {
          for (i = 0; i < numComp; i++)
          {
            idx = i + j * numComp;
            s = static_cast<svtkUnicodeStringArray*>(data)->GetValue(idx).utf8_str();
            svtkTypeUInt64 length = s.length();
            if (length < (static_cast<svtkTypeUInt64>(1) << 6))
            {
              svtkTypeUInt8 len =
                (static_cast<svtkTypeUInt8>(3) << 6) | static_cast<svtkTypeUInt8>(length);
              fp->write(reinterpret_cast<char*>(&len), 1);
            }
            else if (length < (static_cast<svtkTypeUInt64>(1) << 14))
            {
              svtkTypeUInt16 len =
                (static_cast<svtkTypeUInt16>(2) << 14) | static_cast<svtkTypeUInt16>(length);
              svtkByteSwap::SwapWrite2BERange(&len, 1, fp);
            }
            else if (length < (static_cast<svtkTypeUInt64>(1) << 30))
            {
              svtkTypeUInt32 len =
                (static_cast<svtkTypeUInt32>(1) << 30) | static_cast<svtkTypeUInt32>(length);
              svtkByteSwap::SwapWrite4BERange(&len, 1, fp);
            }
            else
            {
              svtkByteSwap::SwapWrite8BERange(&length, 1, fp);
            }
            fp->write(s.c_str(), length);
          }
        }
      }
      *fp << "\n";
    }
    break;

    case SVTK_VARIANT:
    {
      snprintf(str, sizeof(str), format, "variant");
      *fp << str;
      svtkVariant* v = static_cast<svtkVariantArray*>(data)->GetPointer(0);
      for (j = 0; j < num * numComp; j++)
      {
        *fp << v[j].GetType() << " ";
        this->EncodeWriteString(fp, v[j].ToString().c_str(), false);
        *fp << endl;
      }
    }
    break;

    default:
    {
      svtkErrorMacro(<< "Type currently not supported");
      *fp << "NULL_ARRAY" << endl;
      delete[] outputFormat;
      return 0;
    }
  }
  delete[] outputFormat;

  // Write out metadata if it exists:
  svtkInformation* info = data->GetInformation();
  bool hasComponentNames = data->HasAComponentName();
  bool hasInformation = info && info->GetNumberOfKeys() > 0;
  bool hasMetaData = hasComponentNames || hasInformation;
  if (this->WriteArrayMetaData && hasMetaData)
  {
    *fp << "METADATA" << endl;

    if (hasComponentNames)
    {
      *fp << "COMPONENT_NAMES" << endl;
      for (i = 0; i < numComp; ++i)
      {
        const char* compName = data->GetComponentName(i);
        this->EncodeWriteString(fp, compName, false);
        *fp << endl;
      }
    }

    if (hasInformation)
    {
      this->WriteInformation(fp, info);
    }

    *fp << endl;
  }

  fp->flush();
  if (fp->fail())
  {
    this->SetErrorCode(svtkErrorCode::OutOfDiskSpaceError);
    return 0;
  }
  return 1;
}

int svtkDataWriter::WritePoints(ostream* fp, svtkPoints* points)
{
  svtkIdType numPts;

  if (points == nullptr)
  {
    *fp << "POINTS 0 float\n";
    return 1;
  }

  numPts = points->GetNumberOfPoints();
  *fp << "POINTS " << numPts << " ";
  return this->WriteArray(fp, points->GetDataType(), points->GetData(), "%s\n", numPts, 3);
}

// Write out coordinates for rectilinear grids.
int svtkDataWriter::WriteCoordinates(ostream* fp, svtkDataArray* coords, int axes)
{
  int ncoords = (coords ? coords->GetNumberOfTuples() : 0);

  if (axes == 0)
  {
    *fp << "X_COORDINATES " << ncoords << " ";
  }
  else if (axes == 1)
  {
    *fp << "Y_COORDINATES " << ncoords << " ";
  }
  else
  {
    *fp << "Z_COORDINATES " << ncoords << " ";
  }

  if (coords)
  {
    return this->WriteArray(fp, coords->GetDataType(), coords, "%s\n", ncoords, 1);
  }
  *fp << "float\n";
  return 1;
}

// Write out scalar data.
int svtkDataWriter::WriteScalarData(ostream* fp, svtkDataArray* scalars, svtkIdType num)
{
  svtkIdType i, j, size = 0;
  const char* name;
  svtkLookupTable* lut;
  int dataType = scalars->GetDataType();
  int numComp = scalars->GetNumberOfComponents();

  if ((lut = scalars->GetLookupTable()) == nullptr || (size = lut->GetNumberOfColors()) <= 0)
  {
    name = "default";
  }
  else
  {
    name = this->LookupTableName;
  }

  char* scalarsName;
  // Buffer size is size of array name times four because
  // in theory there could be array name consisting of only
  // weird symbols.
  if (!this->ScalarsName)
  {
    if (scalars->GetName() && strlen(scalars->GetName()))
    {
      scalarsName = new char[strlen(scalars->GetName()) * 4 + 1];
      this->EncodeString(scalarsName, scalars->GetName(), true);
    }
    else
    {
      scalarsName = new char[strlen("scalars") + 1];
      strcpy(scalarsName, "scalars");
    }
  }
  else
  {
    scalarsName = new char[strlen(this->ScalarsName) * 4 + 1];
    this->EncodeString(scalarsName, this->ScalarsName, true);
  }

  if (dataType != SVTK_UNSIGNED_CHAR)
  {
    char format[1024];
    *fp << "SCALARS ";

    if (numComp == 1)
    {
      snprintf(format, sizeof(format), "%s %%s\nLOOKUP_TABLE %s\n", scalarsName, name);
    }
    else
    {
      snprintf(format, sizeof(format), "%s %%s %d\nLOOKUP_TABLE %s\n", scalarsName, numComp, name);
    }
    delete[] scalarsName;
    if (this->WriteArray(fp, scalars->GetDataType(), scalars, format, num, numComp) == 0)
    {
      return 0;
    }
  }

  else // color scalars
  {
    int nvs = scalars->GetNumberOfComponents();
    unsigned char* data = static_cast<svtkUnsignedCharArray*>(scalars)->GetPointer(0);
    *fp << "COLOR_SCALARS " << scalarsName << " " << nvs << "\n";

    if (this->FileType == SVTK_ASCII)
    {
      for (i = 0; i < num; i++)
      {
        for (j = 0; j < nvs; j++)
        {
          *fp << (static_cast<float>(data[nvs * i + j]) / 255.0) << " ";
        }
        if (i != 0 && !(i % 2))
        {
          *fp << "\n";
        }
      }
    }
    else // binary type
    {
      fp->write(reinterpret_cast<char*>(data), (sizeof(unsigned char)) * (nvs * num));
    }

    *fp << "\n";
    delete[] scalarsName;
  }

  // if lookup table, write it out
  if (lut && size > 0)
  {
    *fp << "LOOKUP_TABLE " << this->LookupTableName << " " << size << "\n";
    if (this->FileType == SVTK_ASCII)
    {
      double c[4];
      for (i = 0; i < size; i++)
      {
        lut->GetTableValue(i, c);
        *fp << c[0] << " " << c[1] << " " << c[2] << " " << c[3] << "\n";
      }
    }
    else
    {
      unsigned char* colors = lut->GetPointer(0);
      fp->write(reinterpret_cast<char*>(colors), (sizeof(unsigned char) * 4 * size));
    }
    *fp << "\n";
  }

  fp->flush();
  if (fp->fail())
  {
    this->SetErrorCode(svtkErrorCode::OutOfDiskSpaceError);
    return 0;
  }

  return 1;
}

int svtkDataWriter::WriteVectorData(ostream* fp, svtkDataArray* vectors, svtkIdType num)
{
  *fp << "VECTORS ";

  char* vectorsName;
  // Buffer size is size of array name times four because
  // in theory there could be array name consisting of only
  // weird symbols.
  if (!this->VectorsName)
  {
    if (vectors->GetName() && strlen(vectors->GetName()))
    {
      vectorsName = new char[strlen(vectors->GetName()) * 4 + 1];
      this->EncodeString(vectorsName, vectors->GetName(), true);
    }
    else
    {
      vectorsName = new char[strlen("vectors") + 1];
      strcpy(vectorsName, "vectors");
    }
  }
  else
  {
    vectorsName = new char[strlen(this->VectorsName) * 4 + 1];
    this->EncodeString(vectorsName, this->VectorsName, true);
  }

  char format[1024];
  snprintf(format, sizeof(format), "%s %s\n", vectorsName, "%s");
  delete[] vectorsName;

  return this->WriteArray(fp, vectors->GetDataType(), vectors, format, num, 3);
}

int svtkDataWriter::WriteNormalData(ostream* fp, svtkDataArray* normals, svtkIdType num)
{
  char* normalsName;
  // Buffer size is size of array name times four because
  // in theory there could be array name consisting of only
  // weird symbols.
  if (!this->NormalsName)
  {
    if (normals->GetName() && strlen(normals->GetName()))
    {
      normalsName = new char[strlen(normals->GetName()) * 4 + 1];
      this->EncodeString(normalsName, normals->GetName(), true);
    }
    else
    {
      normalsName = new char[strlen("normals") + 1];
      strcpy(normalsName, "normals");
    }
  }
  else
  {
    normalsName = new char[strlen(this->NormalsName) * 4 + 1];
    this->EncodeString(normalsName, this->NormalsName, true);
  }

  *fp << "NORMALS ";
  char format[1024];
  snprintf(format, sizeof(format), "%s %s\n", normalsName, "%s");
  delete[] normalsName;

  return this->WriteArray(fp, normals->GetDataType(), normals, format, num, 3);
}

int svtkDataWriter::WriteTCoordData(ostream* fp, svtkDataArray* tcoords, svtkIdType num)
{
  int dim = tcoords->GetNumberOfComponents();

  char* tcoordsName;
  // Buffer size is size of array name times four because
  // in theory there could be array name consisting of only
  // weird symbols.
  if (!this->TCoordsName)
  {
    if (tcoords->GetName() && strlen(tcoords->GetName()))
    {
      tcoordsName = new char[strlen(tcoords->GetName()) * 4 + 1];
      this->EncodeString(tcoordsName, tcoords->GetName(), true);
    }
    else
    {
      tcoordsName = new char[strlen("tcoords") + 1];
      strcpy(tcoordsName, "tcoords");
    }
  }
  else
  {
    tcoordsName = new char[strlen(this->TCoordsName) * 4 + 1];
    this->EncodeString(tcoordsName, this->TCoordsName, true);
  }

  *fp << "TEXTURE_COORDINATES ";
  char format[1024];
  snprintf(format, sizeof(format), "%s %d %s\n", tcoordsName, dim, "%s");
  delete[] tcoordsName;

  return this->WriteArray(fp, tcoords->GetDataType(), tcoords, format, num, dim);
}

int svtkDataWriter::WriteTensorData(ostream* fp, svtkDataArray* tensors, svtkIdType num)
{
  char* tensorsName;
  // Buffer size is size of array name times four because
  // in theory there could be array name consisting of only
  // weird symbols.
  if (!this->TensorsName)
  {
    if (tensors->GetName() && strlen(tensors->GetName()))
    {
      tensorsName = new char[strlen(tensors->GetName()) * 4 + 1];
      this->EncodeString(tensorsName, tensors->GetName(), true);
    }
    else
    {
      tensorsName = new char[strlen("tensors") + 1];
      strcpy(tensorsName, "tensors");
    }
  }
  else
  {
    tensorsName = new char[strlen(this->TensorsName) * 4 + 1];
    this->EncodeString(tensorsName, this->TensorsName, true);
  }

  *fp << "TENSORS";
  int numComp = 9;
  if (tensors->GetNumberOfComponents() == 6)
  {
    *fp << "6";
    numComp = 6;
  }
  *fp << " ";
  char format[1024];
  snprintf(format, sizeof(format), "%s %s\n", tensorsName, "%s");
  delete[] tensorsName;

  return this->WriteArray(fp, tensors->GetDataType(), tensors, format, num, numComp);
}

int svtkDataWriter::WriteGlobalIdData(ostream* fp, svtkDataArray* globalIds, svtkIdType num)
{
  *fp << "GLOBAL_IDS ";

  char* globalIdsName;
  // Buffer size is size of array name times four because
  // in theory there could be array name consisting of only
  // weird symbols.
  if (!this->GlobalIdsName)
  {
    if (globalIds->GetName() && strlen(globalIds->GetName()))
    {
      globalIdsName = new char[strlen(globalIds->GetName()) * 4 + 1];
      this->EncodeString(globalIdsName, globalIds->GetName(), true);
    }
    else
    {
      globalIdsName = new char[strlen("global_ids") + 1];
      strcpy(globalIdsName, "global_ids");
    }
  }
  else
  {
    globalIdsName = new char[strlen(this->GlobalIdsName) * 4 + 1];
    this->EncodeString(globalIdsName, this->GlobalIdsName, true);
  }

  char format[1024];
  snprintf(format, sizeof(format), "%s %s\n", globalIdsName, "%s");
  delete[] globalIdsName;

  return this->WriteArray(fp, globalIds->GetDataType(), globalIds, format, num, 1);
}

int svtkDataWriter::WritePedigreeIdData(ostream* fp, svtkAbstractArray* pedigreeIds, svtkIdType num)
{
  *fp << "PEDIGREE_IDS ";

  char* pedigreeIdsName;
  // Buffer size is size of array name times four because
  // in theory there could be array name consisting of only
  // weird symbols.
  if (!this->PedigreeIdsName)
  {
    if (pedigreeIds->GetName() && strlen(pedigreeIds->GetName()))
    {
      pedigreeIdsName = new char[strlen(pedigreeIds->GetName()) * 4 + 1];
      this->EncodeString(pedigreeIdsName, pedigreeIds->GetName(), true);
    }
    else
    {
      pedigreeIdsName = new char[strlen("pedigree_ids") + 1];
      strcpy(pedigreeIdsName, "pedigree_ids");
    }
  }
  else
  {
    pedigreeIdsName = new char[strlen(this->PedigreeIdsName) * 4 + 1];
    this->EncodeString(pedigreeIdsName, this->PedigreeIdsName, true);
  }

  char format[1024];
  snprintf(format, sizeof(format), "%s %s\n", pedigreeIdsName, "%s");
  delete[] pedigreeIdsName;

  return this->WriteArray(fp, pedigreeIds->GetDataType(), pedigreeIds, format, num, 1);
}

int svtkDataWriter::WriteEdgeFlagsData(ostream* fp, svtkDataArray* edgeFlags, svtkIdType num)
{
  *fp << "EDGE_FLAGS ";

  char* edgeFlagsName;
  // Buffer size is size of array name times four because
  // in theory there could be array name consisting of only
  // weird symbols.
  if (!this->EdgeFlagsName)
  {
    if (edgeFlags->GetName() && strlen(edgeFlags->GetName()))
    {
      edgeFlagsName = new char[strlen(edgeFlags->GetName()) * 4 + 1];
      this->EncodeString(edgeFlagsName, edgeFlags->GetName(), true);
    }
    else
    {
      edgeFlagsName = new char[strlen("edge_flags") + 1];
      strcpy(edgeFlagsName, "edge_flags");
    }
  }
  else
  {
    edgeFlagsName = new char[strlen(this->EdgeFlagsName) * 4 + 1];
    this->EncodeString(edgeFlagsName, this->EdgeFlagsName, true);
  }

  char format[1024];
  snprintf(format, sizeof(format), "%s %s\n", edgeFlagsName, "%s");
  delete[] edgeFlagsName;

  return this->WriteArray(fp, edgeFlags->GetDataType(), edgeFlags, format, num, 1);
}

bool svtkDataWriter::CanWriteInformationKey(svtkInformation* info, svtkInformationKey* key)
{
  svtkInformationDoubleKey* dKey = nullptr;
  svtkInformationDoubleVectorKey* dvKey = nullptr;
  if ((dKey = svtkInformationDoubleKey::SafeDownCast(key)))
  {
    // Skip keys with NaNs/infs
    double value = info->Get(dKey);
    if (!svtkMath::IsFinite(value))
    {
      svtkWarningMacro("Skipping key '" << key->GetLocation() << "::" << key->GetName()
                                       << "': bad value: " << value);
      return false;
    }
    return true;
  }
  else if ((dvKey = svtkInformationDoubleVectorKey::SafeDownCast(key)))
  {
    // Skip keys with NaNs/infs
    int length = dvKey->Length(info);
    bool valid = true;
    for (int i = 0; i < length; ++i)
    {
      double value = info->Get(dvKey, i);
      if (!svtkMath::IsFinite(value))
      {
        svtkWarningMacro("Skipping key '" << key->GetLocation() << "::" << key->GetName()
                                         << "': bad value: " << value);
        valid = false;
        break;
      }
    }
    return valid;
  }
  else if (svtkInformationIdTypeKey::SafeDownCast(key) ||
    svtkInformationIntegerKey::SafeDownCast(key) ||
    svtkInformationIntegerVectorKey::SafeDownCast(key) ||
    svtkInformationStringKey::SafeDownCast(key) ||
    svtkInformationStringVectorKey::SafeDownCast(key) ||
    svtkInformationUnsignedLongKey::SafeDownCast(key))
  {
    return true;
  }
  svtkDebugMacro("Could not serialize information with key " << key->GetLocation()
                                                            << "::" << key->GetName()
                                                            << ": "
                                                               "Unsupported data type '"
                                                            << key->GetClassName() << "'.");
  return false;
}

namespace
{
void writeInfoHeader(std::ostream* fp, svtkInformationKey* key)
{
  *fp << "NAME " << key->GetName() << " LOCATION " << key->GetLocation() << "\n"
      << "DATA ";
}
} // end anon namespace

int svtkDataWriter::WriteInformation(std::ostream* fp, svtkInformation* info)
{
  // This will contain the serializable keys:
  svtkNew<svtkInformation> keys;
  svtkInformationKey* key = nullptr;
  svtkNew<svtkInformationIterator> iter;
  iter->SetInformationWeak(info);
  for (iter->InitTraversal(); (key = iter->GetCurrentKey()); iter->GoToNextItem())
  {
    if (this->CanWriteInformationKey(info, key))
    {
      keys->CopyEntry(info, key);
    }
  }

  *fp << "INFORMATION " << keys->GetNumberOfKeys() << "\n";

  iter->SetInformationWeak(keys);
  char buffer[1024];
  for (iter->InitTraversal(); (key = iter->GetCurrentKey()); iter->GoToNextItem())
  {
    svtkInformationDoubleKey* dKey = nullptr;
    svtkInformationDoubleVectorKey* dvKey = nullptr;
    svtkInformationIdTypeKey* idKey = nullptr;
    svtkInformationIntegerKey* iKey = nullptr;
    svtkInformationIntegerVectorKey* ivKey = nullptr;
    svtkInformationStringKey* sKey = nullptr;
    svtkInformationStringVectorKey* svKey = nullptr;
    svtkInformationUnsignedLongKey* ulKey = nullptr;
    if ((dKey = svtkInformationDoubleKey::SafeDownCast(key)))
    {
      writeInfoHeader(fp, key);
      // "%lg" is used to write double array data in ascii, using the same
      // precision here.
      snprintf(buffer, sizeof(buffer), "%lg", dKey->Get(info));
      *fp << buffer << "\n";
    }
    else if ((dvKey = svtkInformationDoubleVectorKey::SafeDownCast(key)))
    {
      writeInfoHeader(fp, key);

      // Size first:
      int length = dvKey->Length(info);
      snprintf(buffer, sizeof(buffer), "%d", length);
      *fp << buffer << " ";

      double* data = dvKey->Get(info);
      for (int i = 0; i < length; ++i)
      {
        // "%lg" is used to write double array data in ascii, using the same
        // precision here.
        snprintf(buffer, sizeof(buffer), "%lg", data[i]);
        *fp << buffer << " ";
      }
      *fp << "\n";
    }
    else if ((idKey = svtkInformationIdTypeKey::SafeDownCast(key)))
    {
      writeInfoHeader(fp, key);
      snprintf(buffer, sizeof(buffer), svtkTypeTraits<svtkIdType>::ParseFormat(), idKey->Get(info));
      *fp << buffer << "\n";
    }
    else if ((iKey = svtkInformationIntegerKey::SafeDownCast(key)))
    {
      writeInfoHeader(fp, key);
      snprintf(buffer, sizeof(buffer), svtkTypeTraits<int>::ParseFormat(), iKey->Get(info));
      *fp << buffer << "\n";
    }
    else if ((ivKey = svtkInformationIntegerVectorKey::SafeDownCast(key)))
    {
      writeInfoHeader(fp, key);

      // Size first:
      int length = ivKey->Length(info);
      snprintf(buffer, sizeof(buffer), "%d", length);
      *fp << buffer << " ";

      int* data = ivKey->Get(info);
      for (int i = 0; i < length; ++i)
      {
        snprintf(buffer, sizeof(buffer), svtkTypeTraits<int>::ParseFormat(), data[i]);
        *fp << buffer << " ";
      }
      *fp << "\n";
    }
    else if ((sKey = svtkInformationStringKey::SafeDownCast(key)))
    {
      writeInfoHeader(fp, key);
      this->EncodeWriteString(fp, sKey->Get(info), false);
      *fp << "\n";
    }
    else if ((svKey = svtkInformationStringVectorKey::SafeDownCast(key)))
    {
      writeInfoHeader(fp, key);

      // Size first:
      int length = svKey->Length(info);
      snprintf(buffer, sizeof(buffer), "%d", length);
      *fp << buffer << "\n";

      for (int i = 0; i < length; ++i)
      {
        this->EncodeWriteString(fp, svKey->Get(info, i), false);
        *fp << "\n";
      }
    }
    else if ((ulKey = svtkInformationUnsignedLongKey::SafeDownCast(key)))
    {
      writeInfoHeader(fp, key);
      snprintf(
        buffer, sizeof(buffer), svtkTypeTraits<unsigned long>::ParseFormat(), ulKey->Get(info));
      *fp << buffer << "\n";
    }
    else
    {
      svtkDebugMacro("Could not serialize information with key " << key->GetLocation()
                                                                << "::" << key->GetName()
                                                                << ": "
                                                                   "Unsupported data type '"
                                                                << key->GetClassName() << "'.");
    }
  }
  return 1;
}

static int svtkIsInTheList(int index, int* list, svtkIdType numElem)
{
  for (svtkIdType i = 0; i < numElem; i++)
  {
    if (index == list[i])
    {
      return 1;
    }
  }
  return 0;
}

int svtkDataWriter::WriteFieldData(ostream* fp, svtkFieldData* f)
{
  char format[1024];
  int i, numArrays = f->GetNumberOfArrays(), actNumArrays = 0;
  svtkIdType numComp, numTuples;
  int attributeIndices[svtkDataSetAttributes::NUM_ATTRIBUTES];
  svtkAbstractArray* array;

  for (i = 0; i < svtkDataSetAttributes::NUM_ATTRIBUTES; i++)
  {
    attributeIndices[i] = -1;
  }
  svtkDataSetAttributes* dsa;
  if ((dsa = svtkDataSetAttributes::SafeDownCast(f)))
  {
    dsa->GetAttributeIndices(attributeIndices);
  }

  for (i = 0; i < numArrays; i++)
  {
    if (!svtkIsInTheList(i, attributeIndices, svtkDataSetAttributes::NUM_ATTRIBUTES))
    {
      actNumArrays++;
    }
  }
  if (actNumArrays < 1)
  {
    return 1;
  }
  *fp << "FIELD " << this->FieldDataName << " " << actNumArrays << "\n";

  for (i = 0; i < numArrays; i++)
  {
    if (!svtkIsInTheList(i, attributeIndices, svtkDataSetAttributes::NUM_ATTRIBUTES))
    {
      array = f->GetAbstractArray(i);
      if (array != nullptr)
      {
        numComp = array->GetNumberOfComponents();
        numTuples = array->GetNumberOfTuples();

        // Buffer size is size of array name times four because
        // in theory there could be array name consisting of only
        // weird symbols.
        char* buffer;
        if (!array->GetName() || strlen(array->GetName()) == 0)
        {
          buffer = strcpy(new char[strlen("unknown") + 1], "unknown");
        }
        else
        {
          buffer = new char[strlen(array->GetName()) * 4 + 1];
          this->EncodeString(buffer, array->GetName(), true);
        }
        snprintf(format, sizeof(format), "%s %" SVTK_ID_TYPE_PRId " %" SVTK_ID_TYPE_PRId " %s\n",
          buffer, numComp, numTuples, "%s");
        this->WriteArray(fp, array->GetDataType(), array, format, numTuples, numComp);
        delete[] buffer;
      }
      else
      {
        *fp << "NULL_ARRAY" << endl;
      }
    }
  }

  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return 0;
  }

  return 1;
}

int svtkDataWriter::WriteCells(ostream* fp, svtkCellArray* cells, const char* label)
{
  if (!cells || cells->GetNumberOfCells() < 1)
  {
    return 1;
  }

  svtkIdType offsetsSize = cells->GetNumberOfOffsets();
  svtkIdType connSize = cells->GetNumberOfConnectivityIds();
  bool is64Bit = cells->IsStorage64Bit();

  int type = is64Bit ? SVTK_TYPE_INT64 : SVTK_TYPE_INT32;

  *fp << label << " " << offsetsSize << " " << connSize << "\n";

  this->WriteArray(fp, type, cells->GetOffsetsArray(), "OFFSETS %s\n", offsetsSize, 1);
  this->WriteArray(fp, type, cells->GetConnectivityArray(), "CONNECTIVITY %s\n", connSize, 1);

  fp->flush();
  if (fp->fail())
  {
    this->SetErrorCode(svtkErrorCode::OutOfDiskSpaceError);
    return 0;
  }

  return 1;
}

void svtkDataWriter::WriteData()
{
  svtkErrorMacro(<< "WriteData() should be implemented in concrete subclass");
}

// Close a svtk file.
void svtkDataWriter::CloseSVTKFile(ostream* fp)
{
  svtkDebugMacro(<< "Closing svtk file\n");

  // Restore the previous locale settings
  std::locale::global(this->CurrentLocale);

  if (fp != nullptr)
  {
    if (this->WriteToOutputString)
    {
      std::ostringstream* ostr = static_cast<std::ostringstream*>(fp);
      delete[] this->OutputString;
      const size_t strlength = ostr->str().size();
      if (strlength > static_cast<size_t>(svtkTypeTraits<svtkIdType>::Max()))
      {
        this->OutputString = nullptr;
        this->OutputStringLength = 0;
        svtkErrorMacro("OutputStringLength overflow: the length of data in the "
                      "writer is greater than what would fit in a variable of type "
                      "`svtkIdType`. You may have to recompile with SVTK_USE_64BIT_IDS."
                      "Presently, svtkIdType is "
          << sizeof(svtkIdType) * 8 << " bits.");
      }
      else
      {
        this->OutputStringLength = static_cast<svtkIdType>(strlength);
        this->OutputString = new char[strlength + 1];
      }
      memcpy(this->OutputString, ostr->str().c_str(), this->OutputStringLength + 1);
    }
    delete fp;
  }
}

char* svtkDataWriter::RegisterAndGetOutputString()
{
  char* tmp = this->OutputString;

  this->OutputString = nullptr;
  this->OutputStringLength = 0;

  return tmp;
}

svtkStdString svtkDataWriter::GetOutputStdString()
{
  return svtkStdString(this->OutputString, this->OutputStringLength);
}

void svtkDataWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "File Name: " << (this->FileName ? this->FileName : "(none)") << "\n";

  if (this->FileType == SVTK_BINARY)
  {
    os << indent << "File Type: BINARY\n";
  }
  else
  {
    os << indent << "File Type: ASCII\n";
  }

  if (this->Header)
  {
    os << indent << "Header: " << this->Header << "\n";
  }
  else
  {
    os << indent << "Header: (None)\n";
  }

  os << indent << "Output String Length: " << this->OutputStringLength << "\n";
  os << indent << "Output String (addr): " << static_cast<void*>(this->OutputString) << "\n";
  os << indent << "WriteToOutputString: " << (this->WriteToOutputString ? "On\n" : "Off\n");

  if (this->ScalarsName)
  {
    os << indent << "Scalars Name: " << this->ScalarsName << "\n";
  }
  else
  {
    os << indent << "Scalars Name: (None)\n";
  }

  if (this->VectorsName)
  {
    os << indent << "Vectors Name: " << this->VectorsName << "\n";
  }
  else
  {
    os << indent << "Vectors Name: (None)\n";
  }

  if (this->NormalsName)
  {
    os << indent << "Normals Name: " << this->NormalsName << "\n";
  }
  else
  {
    os << indent << "Normals Name: (None)\n";
  }

  if (this->TensorsName)
  {
    os << indent << "Tensors Name: " << this->TensorsName << "\n";
  }
  else
  {
    os << indent << "Tensors Name: (None)\n";
  }

  if (this->TCoordsName)
  {
    os << indent << "Texture Coords Name: " << this->TCoordsName << "\n";
  }
  else
  {
    os << indent << "Texture Coordinates Name: (None)\n";
  }

  if (this->GlobalIdsName)
  {
    os << indent << "Global Ids Name: " << this->GlobalIdsName << "\n";
  }
  else
  {
    os << indent << "Global Ids Name: (None)\n";
  }

  if (this->PedigreeIdsName)
  {
    os << indent << "Pedigree Ids Name: " << this->PedigreeIdsName << "\n";
  }
  else
  {
    os << indent << "Pedigree Ids Name: (None)\n";
  }

  if (this->EdgeFlagsName)
  {
    os << indent << "Edge Flags Name: " << this->EdgeFlagsName << "\n";
  }
  else
  {
    os << indent << "Edge Flags Name: (None)\n";
  }

  if (this->LookupTableName)
  {
    os << indent << "Lookup Table Name: " << this->LookupTableName << "\n";
  }
  else
  {
    os << indent << "Lookup Table Name: (None)\n";
  }

  if (this->FieldDataName)
  {
    os << indent << "Field Data Name: " << this->FieldDataName << "\n";
  }
  else
  {
    os << indent << "Field Data Name: (None)\n";
  }
}

int svtkDataWriter::WriteDataSetData(ostream* fp, svtkDataSet* ds)
{
  svtkFieldData* field = ds->GetFieldData();
  if (field && field->GetNumberOfTuples() > 0)
  {
    if (!this->WriteFieldData(fp, field))
    {
      return 0; // we tried to write field data, but we couldn't
    }
  }
  return 1;
}
