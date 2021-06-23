/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGenericDataObjectWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkGenericDataObjectWriter.h"

#include "svtkCompositeDataSet.h"
#include "svtkCompositeDataWriter.h"
#include "svtkDataObject.h"
#include "svtkErrorCode.h"
#include "svtkGraph.h"
#include "svtkGraphWriter.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"
#include "svtkPolyDataWriter.h"
#include "svtkRectilinearGrid.h"
#include "svtkRectilinearGridWriter.h"
#include "svtkStructuredGrid.h"
#include "svtkStructuredGridWriter.h"
#include "svtkStructuredPoints.h"
#include "svtkStructuredPointsWriter.h"
#include "svtkTable.h"
#include "svtkTableWriter.h"
#include "svtkTree.h"
#include "svtkTreeWriter.h"
#include "svtkUnstructuredGrid.h"
#include "svtkUnstructuredGridWriter.h"

svtkStandardNewMacro(svtkGenericDataObjectWriter);

template <typename WriterT>
svtkDataWriter* CreateWriter(svtkAlgorithmOutput* input)
{
  WriterT* const writer = WriterT::New();
  writer->SetInputConnection(input);
  return writer;
}

svtkGenericDataObjectWriter::svtkGenericDataObjectWriter() = default;

svtkGenericDataObjectWriter::~svtkGenericDataObjectWriter() = default;

void svtkGenericDataObjectWriter::WriteData()
{
  svtkDebugMacro(<< "Writing svtk data object ...");

  svtkDataWriter* writer = nullptr;

  svtkAlgorithmOutput* input = this->GetInputConnection(0, 0);
  switch (this->GetInput()->GetDataObjectType())
  {
    case SVTK_COMPOSITE_DATA_SET:
      svtkErrorMacro(<< "Cannot write composite data set");
      return;
    case SVTK_DATA_OBJECT:
      svtkErrorMacro(<< "Cannot write data object");
      return;
    case SVTK_DATA_SET:
      svtkErrorMacro(<< "Cannot write data set");
      return;
    case SVTK_GENERIC_DATA_SET:
      svtkErrorMacro(<< "Cannot write generic data set");
      return;
    case SVTK_DIRECTED_GRAPH:
    case SVTK_UNDIRECTED_GRAPH:
    case SVTK_MOLECULE:
      writer = CreateWriter<svtkGraphWriter>(input);
      break;
    case SVTK_HIERARCHICAL_DATA_SET:
      svtkErrorMacro(<< "Cannot write hierarchical data set");
      return;
    case SVTK_HYPER_OCTREE:
      svtkErrorMacro(<< "Cannot write hyper octree");
      return;
    case SVTK_IMAGE_DATA:
      writer = CreateWriter<svtkStructuredPointsWriter>(input);
      break;
    case SVTK_MULTIBLOCK_DATA_SET:
    case SVTK_HIERARCHICAL_BOX_DATA_SET:
    case SVTK_MULTIPIECE_DATA_SET:
    case SVTK_OVERLAPPING_AMR:
    case SVTK_NON_OVERLAPPING_AMR:
    case SVTK_PARTITIONED_DATA_SET:
    case SVTK_PARTITIONED_DATA_SET_COLLECTION:
      writer = CreateWriter<svtkCompositeDataWriter>(input);
      break;
    case SVTK_MULTIGROUP_DATA_SET:
      svtkErrorMacro(<< "Cannot write multigroup data set");
      return;
    case SVTK_PIECEWISE_FUNCTION:
      svtkErrorMacro(<< "Cannot write piecewise function");
      return;
    case SVTK_POINT_SET:
      svtkErrorMacro(<< "Cannot write point set");
      return;
    case SVTK_POLY_DATA:
      writer = CreateWriter<svtkPolyDataWriter>(input);
      break;
    case SVTK_RECTILINEAR_GRID:
      writer = CreateWriter<svtkRectilinearGridWriter>(input);
      break;
    case SVTK_STRUCTURED_GRID:
      writer = CreateWriter<svtkStructuredGridWriter>(input);
      break;
    case SVTK_STRUCTURED_POINTS:
      writer = CreateWriter<svtkStructuredPointsWriter>(input);
      break;
    case SVTK_TABLE:
      writer = CreateWriter<svtkTableWriter>(input);
      break;
    case SVTK_TREE:
      writer = CreateWriter<svtkTreeWriter>(input);
      break;
    case SVTK_TEMPORAL_DATA_SET:
      svtkErrorMacro(<< "Cannot write temporal data set");
      return;
    case SVTK_UNIFORM_GRID:
      svtkErrorMacro(<< "Cannot write uniform grid");
      return;
    case SVTK_UNSTRUCTURED_GRID:
      writer = CreateWriter<svtkUnstructuredGridWriter>(input);
      break;
  }

  if (!writer)
  {
    svtkErrorMacro(<< "null data object writer");
    return;
  }

  writer->SetFileName(this->FileName);
  writer->SetScalarsName(this->ScalarsName);
  writer->SetVectorsName(this->VectorsName);
  writer->SetNormalsName(this->NormalsName);
  writer->SetTensorsName(this->TensorsName);
  writer->SetTCoordsName(this->TCoordsName);
  writer->SetHeader(this->Header);
  writer->SetLookupTableName(this->LookupTableName);
  writer->SetFieldDataName(this->FieldDataName);
  writer->SetFileType(this->FileType);
  writer->SetDebug(this->Debug);
  writer->SetWriteToOutputString(this->WriteToOutputString);
  writer->Write();
  if (writer->GetErrorCode() == svtkErrorCode::OutOfDiskSpaceError)
  {
    this->SetErrorCode(svtkErrorCode::OutOfDiskSpaceError);
  }
  if (this->WriteToOutputString)
  {
    delete[] this->OutputString;
    this->OutputStringLength = writer->GetOutputStringLength();
    this->OutputString = writer->RegisterAndGetOutputString();
  }
  writer->Delete();
}

int svtkGenericDataObjectWriter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataObject");
  return 1;
}

void svtkGenericDataObjectWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
