/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGenericDataObjectReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkGenericDataObjectReader.h"

#include "svtkCompositeDataReader.h"
#include "svtkDirectedGraph.h"
#include "svtkGraph.h"
#include "svtkGraphReader.h"
#include "svtkHierarchicalBoxDataSet.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMolecule.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiPieceDataSet.h"
#include "svtkNonOverlappingAMR.h"
#include "svtkObjectFactory.h"
#include "svtkOverlappingAMR.h"
#include "svtkPartitionedDataSet.h"
#include "svtkPartitionedDataSetCollection.h"
#include "svtkPolyData.h"
#include "svtkPolyDataReader.h"
#include "svtkRectilinearGrid.h"
#include "svtkRectilinearGridReader.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredGrid.h"
#include "svtkStructuredGridReader.h"
#include "svtkStructuredPoints.h"
#include "svtkStructuredPointsReader.h"
#include "svtkTable.h"
#include "svtkTableReader.h"
#include "svtkTree.h"
#include "svtkTreeReader.h"
#include "svtkUndirectedGraph.h"
#include "svtkUnstructuredGrid.h"
#include "svtkUnstructuredGridReader.h"

svtkStandardNewMacro(svtkGenericDataObjectReader);

template <typename ReaderT, typename DataT>
void svtkGenericDataObjectReader::ReadData(
  const char* fname, const char* DataClass, svtkDataObject* Output)
{
  ReaderT* const reader = ReaderT::New();

  reader->SetFileName(fname);
  reader->SetInputArray(this->GetInputArray());
  reader->SetInputString(this->GetInputString(), this->GetInputStringLength());
  reader->SetReadFromInputString(this->GetReadFromInputString());
  reader->SetScalarsName(this->GetScalarsName());
  reader->SetVectorsName(this->GetVectorsName());
  reader->SetNormalsName(this->GetNormalsName());
  reader->SetTensorsName(this->GetTensorsName());
  reader->SetTCoordsName(this->GetTCoordsName());
  reader->SetLookupTableName(this->GetLookupTableName());
  reader->SetFieldDataName(this->GetFieldDataName());
  reader->SetReadAllScalars(this->GetReadAllScalars());
  reader->SetReadAllVectors(this->GetReadAllVectors());
  reader->SetReadAllNormals(this->GetReadAllNormals());
  reader->SetReadAllTensors(this->GetReadAllTensors());
  reader->SetReadAllColorScalars(this->GetReadAllColorScalars());
  reader->SetReadAllTCoords(this->GetReadAllTCoords());
  reader->SetReadAllFields(this->GetReadAllFields());
  reader->Update();

  // copy the header from the reader.
  this->SetHeader(reader->GetHeader());

  // Can we use the old output?
  if (!(Output && strcmp(Output->GetClassName(), DataClass) == 0))
  {
    // Hack to make sure that the object is not modified
    // with SetNthOutput. Otherwise, extra executions occur.
    const svtkTimeStamp mtime = this->MTime;
    Output = DataT::New();
    this->GetExecutive()->SetOutputData(0, Output);
    Output->Delete();
    this->MTime = mtime;
  }
  Output->ShallowCopy(reader->GetOutput());
  reader->Delete();
}

svtkGenericDataObjectReader::svtkGenericDataObjectReader() = default;
svtkGenericDataObjectReader::~svtkGenericDataObjectReader() = default;

svtkDataObject* svtkGenericDataObjectReader::CreateOutput(svtkDataObject* currentOutput)
{
  if (this->GetFileName() == nullptr &&
    (this->GetReadFromInputString() == 0 ||
      (this->GetInputArray() == nullptr && this->GetInputString() == nullptr)))
  {
    svtkWarningMacro(<< "FileName must be set");
    return nullptr;
  }

  int outputType = this->ReadOutputType();

  if (currentOutput && (currentOutput->GetDataObjectType() == outputType))
  {
    return currentOutput;
  }

  svtkDataObject* output = nullptr;
  switch (outputType)
  {
    case SVTK_DIRECTED_GRAPH:
      output = svtkDirectedGraph::New();
      break;
    case SVTK_MOLECULE:
    case SVTK_UNDIRECTED_GRAPH:
      output = svtkUndirectedGraph::New();
      break;
    case SVTK_IMAGE_DATA:
      output = svtkImageData::New();
      break;
    case SVTK_POLY_DATA:
      output = svtkPolyData::New();
      break;
    case SVTK_RECTILINEAR_GRID:
      output = svtkRectilinearGrid::New();
      break;
    case SVTK_STRUCTURED_GRID:
      output = svtkStructuredGrid::New();
      break;
    case SVTK_STRUCTURED_POINTS:
      output = svtkStructuredPoints::New();
      break;
    case SVTK_TABLE:
      output = svtkTable::New();
      break;
    case SVTK_TREE:
      output = svtkTree::New();
      break;
    case SVTK_UNSTRUCTURED_GRID:
      output = svtkUnstructuredGrid::New();
      break;
    case SVTK_MULTIBLOCK_DATA_SET:
      output = svtkMultiBlockDataSet::New();
      break;
    case SVTK_MULTIPIECE_DATA_SET:
      output = svtkMultiPieceDataSet::New();
      break;
    case SVTK_HIERARCHICAL_BOX_DATA_SET:
      output = svtkHierarchicalBoxDataSet::New();
      break;
    case SVTK_OVERLAPPING_AMR:
      output = svtkOverlappingAMR::New();
      break;
    case SVTK_NON_OVERLAPPING_AMR:
      output = svtkNonOverlappingAMR::New();
      break;
    case SVTK_PARTITIONED_DATA_SET:
      output = svtkPartitionedDataSet::New();
      break;
    case SVTK_PARTITIONED_DATA_SET_COLLECTION:
      output = svtkPartitionedDataSetCollection::New();
      break;
  }

  return output;
}

int svtkGenericDataObjectReader::ReadMetaDataSimple(
  const std::string& fname, svtkInformation* metadata)
{
  if (fname.empty() &&
    (this->GetReadFromInputString() == 0 ||
      (this->GetInputArray() == nullptr && this->GetInputString() == nullptr)))
  {
    svtkWarningMacro(<< "FileName must be set");
    return 0;
  }

  svtkDataReader* reader = nullptr;
  int retVal;
  switch (this->ReadOutputType())
  {
    case SVTK_MOLECULE:
    case SVTK_UNDIRECTED_GRAPH:
    case SVTK_DIRECTED_GRAPH:
      reader = svtkGraphReader::New();
      break;
    case SVTK_IMAGE_DATA:
      reader = svtkStructuredPointsReader::New();
      break;
    case SVTK_POLY_DATA:
      reader = svtkPolyDataReader::New();
      break;
    case SVTK_RECTILINEAR_GRID:
      reader = svtkRectilinearGridReader::New();
      break;
    case SVTK_STRUCTURED_GRID:
      reader = svtkStructuredGridReader::New();
      break;
    case SVTK_STRUCTURED_POINTS:
      reader = svtkStructuredPointsReader::New();
      break;
    case SVTK_TABLE:
      reader = svtkTableReader::New();
      break;
    case SVTK_TREE:
      reader = svtkTreeReader::New();
      break;
    case SVTK_UNSTRUCTURED_GRID:
      reader = svtkUnstructuredGridReader::New();
      break;
    case SVTK_MULTIBLOCK_DATA_SET:
    case SVTK_HIERARCHICAL_BOX_DATA_SET:
    case SVTK_MULTIPIECE_DATA_SET:
    case SVTK_OVERLAPPING_AMR:
    case SVTK_NON_OVERLAPPING_AMR:
    case SVTK_PARTITIONED_DATA_SET:
    case SVTK_PARTITIONED_DATA_SET_COLLECTION:
      reader = svtkCompositeDataReader::New();
      break;
    default:
      reader = nullptr;
  }

  if (reader)
  {
    reader->SetReadFromInputString(this->GetReadFromInputString());
    reader->SetInputArray(this->GetInputArray());
    reader->SetInputString(this->GetInputString());
    retVal = reader->ReadMetaDataSimple(fname.c_str(), metadata);
    reader->Delete();
    return retVal;
  }
  return 1;
}

int svtkGenericDataObjectReader::ReadMeshSimple(const std::string& fname, svtkDataObject* output)
{

  svtkDebugMacro(<< "Reading svtk dataset...");

  switch (this->ReadOutputType())
  {
    case SVTK_MOLECULE:
    {
      this->ReadData<svtkGraphReader, svtkMolecule>(fname.c_str(), "svtkMolecule", output);
      return 1;
    }
    case SVTK_DIRECTED_GRAPH:
    {
      this->ReadData<svtkGraphReader, svtkDirectedGraph>(fname.c_str(), "svtkDirectedGraph", output);
      return 1;
    }
    case SVTK_UNDIRECTED_GRAPH:
    {
      this->ReadData<svtkGraphReader, svtkUndirectedGraph>(
        fname.c_str(), "svtkUndirectedGraph", output);
      return 1;
    }
    case SVTK_IMAGE_DATA:
    {
      this->ReadData<svtkStructuredPointsReader, svtkImageData>(
        fname.c_str(), "svtkImageData", output);
      return 1;
    }
    case SVTK_POLY_DATA:
    {
      this->ReadData<svtkPolyDataReader, svtkPolyData>(fname.c_str(), "svtkPolyData", output);
      return 1;
    }
    case SVTK_RECTILINEAR_GRID:
    {
      this->ReadData<svtkRectilinearGridReader, svtkRectilinearGrid>(
        fname.c_str(), "svtkRectilinearGrid", output);
      return 1;
    }
    case SVTK_STRUCTURED_GRID:
    {
      this->ReadData<svtkStructuredGridReader, svtkStructuredGrid>(
        fname.c_str(), "svtkStructuredGrid", output);
      return 1;
    }
    case SVTK_STRUCTURED_POINTS:
    {
      this->ReadData<svtkStructuredPointsReader, svtkStructuredPoints>(
        fname.c_str(), "svtkStructuredPoints", output);
      return 1;
    }
    case SVTK_TABLE:
    {
      this->ReadData<svtkTableReader, svtkTable>(fname.c_str(), "svtkTable", output);
      return 1;
    }
    case SVTK_TREE:
    {
      this->ReadData<svtkTreeReader, svtkTree>(fname.c_str(), "svtkTree", output);
      return 1;
    }
    case SVTK_UNSTRUCTURED_GRID:
    {
      this->ReadData<svtkUnstructuredGridReader, svtkUnstructuredGrid>(
        fname.c_str(), "svtkUnstructuredGrid", output);
      return 1;
    }
    case SVTK_MULTIBLOCK_DATA_SET:
    {
      this->ReadData<svtkCompositeDataReader, svtkMultiBlockDataSet>(
        fname.c_str(), "svtkMultiBlockDataSet", output);
      return 1;
    }
    case SVTK_MULTIPIECE_DATA_SET:
    {
      this->ReadData<svtkCompositeDataReader, svtkMultiPieceDataSet>(
        fname.c_str(), "svtkMultiPieceDataSet", output);
      return 1;
    }
    case SVTK_HIERARCHICAL_BOX_DATA_SET:
    {
      this->ReadData<svtkCompositeDataReader, svtkHierarchicalBoxDataSet>(
        fname.c_str(), "svtkHierarchicalBoxDataSet", output);
      return 1;
    }
    case SVTK_OVERLAPPING_AMR:
    {
      this->ReadData<svtkCompositeDataReader, svtkOverlappingAMR>(
        fname.c_str(), "svtkHierarchicalBoxDataSet", output);
      return 1;
    }
    case SVTK_NON_OVERLAPPING_AMR:
    {
      this->ReadData<svtkCompositeDataReader, svtkNonOverlappingAMR>(
        fname.c_str(), "svtkHierarchicalBoxDataSet", output);
      return 1;
    }
    case SVTK_PARTITIONED_DATA_SET:
    {
      this->ReadData<svtkCompositeDataReader, svtkPartitionedDataSet>(
        fname.c_str(), "svtkPartitionedDataSet", output);
      return 1;
    }
    case SVTK_PARTITIONED_DATA_SET_COLLECTION:
    {
      this->ReadData<svtkCompositeDataReader, svtkPartitionedDataSetCollection>(
        fname.c_str(), "svtkPartitionedDataSetCollection", output);
      return 1;
    }
    default:
      svtkErrorMacro("Could not read file " << this->GetFileName());
  }
  return 0;
}

int svtkGenericDataObjectReader::ReadOutputType()
{
  char line[256];

  svtkDebugMacro(<< "Reading svtk data object...");

  if (!this->OpenSVTKFile() || !this->ReadHeader())
  {
    return -1;
  }

  // Determine dataset type
  //
  if (!this->ReadString(line))
  {
    svtkDebugMacro(<< "Premature EOF reading dataset keyword");
    return -1;
  }

  if (!strncmp(this->LowerCase(line), "dataset", static_cast<unsigned long>(7)))
  {
    // See iftype is recognized.
    //
    if (!this->ReadString(line))
    {
      svtkDebugMacro(<< "Premature EOF reading type");
      this->CloseSVTKFile();
      return -1;
    }

    this->CloseSVTKFile();

    if (!strncmp(this->LowerCase(line), "molecule", 8))
    {
      return SVTK_MOLECULE;
    }
    if (!strncmp(this->LowerCase(line), "directed_graph", 14))
    {
      return SVTK_DIRECTED_GRAPH;
    }
    if (!strncmp(this->LowerCase(line), "undirected_graph", 16))
    {
      return SVTK_UNDIRECTED_GRAPH;
    }
    if (!strncmp(this->LowerCase(line), "polydata", 8))
    {
      return SVTK_POLY_DATA;
    }
    if (!strncmp(this->LowerCase(line), "rectilinear_grid", 16))
    {
      return SVTK_RECTILINEAR_GRID;
    }
    if (!strncmp(this->LowerCase(line), "structured_grid", 15))
    {
      return SVTK_STRUCTURED_GRID;
    }
    if (!strncmp(this->LowerCase(line), "structured_points", 17))
    {
      return SVTK_STRUCTURED_POINTS;
    }
    if (!strncmp(this->LowerCase(line), "table", 5))
    {
      return SVTK_TABLE;
    }
    if (!strncmp(this->LowerCase(line), "tree", 4))
    {
      return SVTK_TREE;
    }
    if (!strncmp(this->LowerCase(line), "unstructured_grid", 17))
    {
      return SVTK_UNSTRUCTURED_GRID;
    }
    if (!strncmp(this->LowerCase(line), "multiblock", strlen("multiblock")))
    {
      return SVTK_MULTIBLOCK_DATA_SET;
    }
    if (!strncmp(this->LowerCase(line), "multipiece", strlen("multipiece")))
    {
      return SVTK_MULTIPIECE_DATA_SET;
    }
    if (!strncmp(this->LowerCase(line), "hierarchical_box", strlen("hierarchical_box")))
    {
      return SVTK_HIERARCHICAL_BOX_DATA_SET;
    }
    if (strncmp(this->LowerCase(line), "overlapping_amr", strlen("overlapping_amr")) == 0)
    {
      return SVTK_OVERLAPPING_AMR;
    }
    if (strncmp(this->LowerCase(line), "non_overlapping_amr", strlen("non_overlapping_amr")) == 0)
    {
      return SVTK_NON_OVERLAPPING_AMR;
    }
    if (strncmp(this->LowerCase(line), "partitioned", strlen("partitioned")) == 0)
    {
      return SVTK_PARTITIONED_DATA_SET;
    }
    if (strncmp(
          this->LowerCase(line), "partitioned_collection", strlen("partitioned_collection")) == 0)
    {
      return SVTK_PARTITIONED_DATA_SET_COLLECTION;
    }

    svtkDebugMacro(<< "Cannot read dataset type: " << line);
    return -1;
  }
  else if (!strncmp(this->LowerCase(line), "field", static_cast<unsigned long>(5)))
  {
    svtkDebugMacro(<< "This object can only read data objects, not fields");
  }
  else
  {
    svtkDebugMacro(<< "Expecting DATASET keyword, got " << line << " instead");
  }

  return -1;
}

svtkGraph* svtkGenericDataObjectReader::GetGraphOutput()
{
  return svtkGraph::SafeDownCast(this->GetOutput());
}

svtkMolecule* svtkGenericDataObjectReader::GetMoleculeOutput()
{
  return svtkMolecule::SafeDownCast(this->GetOutput());
}

svtkPolyData* svtkGenericDataObjectReader::GetPolyDataOutput()
{
  return svtkPolyData::SafeDownCast(this->GetOutput());
}

svtkRectilinearGrid* svtkGenericDataObjectReader::GetRectilinearGridOutput()
{
  return svtkRectilinearGrid::SafeDownCast(this->GetOutput());
}

svtkStructuredGrid* svtkGenericDataObjectReader::GetStructuredGridOutput()
{
  return svtkStructuredGrid::SafeDownCast(this->GetOutput());
}

svtkStructuredPoints* svtkGenericDataObjectReader::GetStructuredPointsOutput()
{
  return svtkStructuredPoints::SafeDownCast(this->GetOutput());
}

svtkTable* svtkGenericDataObjectReader::GetTableOutput()
{
  return svtkTable::SafeDownCast(this->GetOutput());
}

svtkTree* svtkGenericDataObjectReader::GetTreeOutput()
{
  return svtkTree::SafeDownCast(this->GetOutput());
}

svtkUnstructuredGrid* svtkGenericDataObjectReader::GetUnstructuredGridOutput()
{
  return svtkUnstructuredGrid::SafeDownCast(this->GetOutput());
}

void svtkGenericDataObjectReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

svtkDataObject* svtkGenericDataObjectReader::GetOutput()
{
  return this->GetOutputDataObject(0);
}

svtkDataObject* svtkGenericDataObjectReader::GetOutput(int idx)
{
  return this->GetOutputDataObject(idx);
}

int svtkGenericDataObjectReader::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkDataObject");
  return 1;
}
