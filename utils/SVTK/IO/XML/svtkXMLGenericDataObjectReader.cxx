/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLGenericDataObjectReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLGenericDataObjectReader.h"

#include "svtkCommand.h"
#include "svtkHierarchicalBoxDataSet.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNonOverlappingAMR.h"
#include "svtkObjectFactory.h"
#include "svtkOverlappingAMR.h"
#include "svtkPolyData.h"
#include "svtkRectilinearGrid.h"
#include "svtkSmartPointer.h"
#include "svtkStructuredGrid.h"
#include "svtkUnstructuredGrid.h"
#include "svtkXMLFileReadTester.h"
#include "svtkXMLImageDataReader.h"
#include "svtkXMLMultiBlockDataReader.h"
#include "svtkXMLPImageDataReader.h"
#include "svtkXMLPPolyDataReader.h"
#include "svtkXMLPRectilinearGridReader.h"
#include "svtkXMLPStructuredGridReader.h"
#include "svtkXMLPUnstructuredGridReader.h"
#include "svtkXMLPolyDataReader.h"
#include "svtkXMLRectilinearGridReader.h"
#include "svtkXMLStructuredGridReader.h"
#include "svtkXMLUniformGridAMRReader.h"
#include "svtkXMLUnstructuredGridReader.h"

#include <cassert>

svtkStandardNewMacro(svtkXMLGenericDataObjectReader);

// ---------------------------------------------------------------------------
svtkXMLGenericDataObjectReader::svtkXMLGenericDataObjectReader()
{
  this->Reader = nullptr;
}

// ---------------------------------------------------------------------------
svtkXMLGenericDataObjectReader::~svtkXMLGenericDataObjectReader()
{
  if (this->Reader != nullptr)
  {
    this->Reader->Delete();
  }
}

// ---------------------------------------------------------------------------
int svtkXMLGenericDataObjectReader::ReadOutputType(const char* name, bool& parallel)
{
  parallel = false;

  // Test if the file with the given name is a SVTKFile with the given
  // type.
  svtkSmartPointer<svtkXMLFileReadTester> tester = svtkSmartPointer<svtkXMLFileReadTester>::New();

  tester->SetFileName(name);
  if (tester->TestReadFile())
  {
    char* cfileDataType = tester->GetFileDataType();
    if (cfileDataType != nullptr)
    {
      std::string fileDataType(cfileDataType);
      if (fileDataType.compare("HierarchicalBoxDataSet") == 0 ||
        fileDataType.compare("svtkHierarchicalBoxDataSet") == 0)
      {
        return SVTK_HIERARCHICAL_BOX_DATA_SET;
      }
      if (fileDataType.compare("svtkOverlappingAMR") == 0)
      {
        return SVTK_OVERLAPPING_AMR;
      }
      if (fileDataType.compare("svtkNonOverlappingAMR") == 0)
      {
        return SVTK_NON_OVERLAPPING_AMR;
      }
      if (fileDataType.compare("ImageData") == 0)
      {
        return SVTK_IMAGE_DATA;
      }
      if (fileDataType.compare("PImageData") == 0)
      {
        parallel = true;
        return SVTK_IMAGE_DATA;
      }
      if (fileDataType.compare("svtkMultiBlockDataSet") == 0)
      {
        return SVTK_MULTIBLOCK_DATA_SET;
      }
      if (fileDataType.compare("PolyData") == 0)
      {
        return SVTK_POLY_DATA;
      }
      if (fileDataType.compare("PPolyData") == 0)
      {
        parallel = true;
        return SVTK_POLY_DATA;
      }
      if (fileDataType.compare("RectilinearGrid") == 0)
      {
        return SVTK_RECTILINEAR_GRID;
      }
      if (fileDataType.compare("PRectilinearGrid") == 0)
      {
        parallel = true;
        return SVTK_RECTILINEAR_GRID;
      }
      if (fileDataType.compare("StructuredGrid") == 0)
      {
        return SVTK_STRUCTURED_GRID;
      }
      if (fileDataType.compare("PStructuredGrid") == 0)
      {
        parallel = true;
        return SVTK_STRUCTURED_GRID;
      }
      if (fileDataType.compare("UnstructuredGrid") == 0 ||
        fileDataType.compare("UnstructuredGridBase") == 0)
      {
        return SVTK_UNSTRUCTURED_GRID;
      }
      if (fileDataType.compare("PUnstructuredGrid") == 0 ||
        fileDataType.compare("PUnstructuredGridBase") == 0)
      {
        parallel = true;
        return SVTK_UNSTRUCTURED_GRID;
      }
    }
  }

  svtkErrorMacro(<< "could not load " << name);
  return -1;
}

// ---------------------------------------------------------------------------
int svtkXMLGenericDataObjectReader::RequestDataObject(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  if (!this->Stream && !this->FileName)
  {
    svtkErrorMacro("File name not specified");
    return 0;
  }

  if (this->Reader != nullptr)
  {
    this->Reader->Delete();
    this->Reader = nullptr;
  }

  svtkDataObject* output = nullptr;

  // Create reader.
  bool parallel = false;
  switch (this->ReadOutputType(this->FileName, parallel))
  {
    case SVTK_HIERARCHICAL_BOX_DATA_SET:
      this->Reader = svtkXMLUniformGridAMRReader::New();
      output = svtkHierarchicalBoxDataSet::New();
      break;
    case SVTK_OVERLAPPING_AMR:
      this->Reader = svtkXMLUniformGridAMRReader::New();
      output = svtkOverlappingAMR::New();
      break;
    case SVTK_NON_OVERLAPPING_AMR:
      this->Reader = svtkXMLUniformGridAMRReader::New();
      output = svtkNonOverlappingAMR::New();
      break;
    case SVTK_IMAGE_DATA:
      if (parallel)
      {
        this->Reader = svtkXMLPImageDataReader::New();
      }
      else
      {
        this->Reader = svtkXMLImageDataReader::New();
      }
      output = svtkImageData::New();
      break;
    case SVTK_MULTIBLOCK_DATA_SET:
      this->Reader = svtkXMLMultiBlockDataReader::New();
      output = svtkMultiBlockDataSet::New();
      break;
    case SVTK_POLY_DATA:
      if (parallel)
      {
        this->Reader = svtkXMLPPolyDataReader::New();
      }
      else
      {
        this->Reader = svtkXMLPolyDataReader::New();
      }
      output = svtkPolyData::New();
      break;
    case SVTK_RECTILINEAR_GRID:
      if (parallel)
      {
        this->Reader = svtkXMLPRectilinearGridReader::New();
      }
      else
      {
        this->Reader = svtkXMLRectilinearGridReader::New();
      }
      output = svtkRectilinearGrid::New();
      break;
    case SVTK_STRUCTURED_GRID:
      if (parallel)
      {
        this->Reader = svtkXMLPStructuredGridReader::New();
      }
      else
      {
        this->Reader = svtkXMLStructuredGridReader::New();
      }
      output = svtkStructuredGrid::New();
      break;
    case SVTK_UNSTRUCTURED_GRID:
      if (parallel)
      {
        this->Reader = svtkXMLPUnstructuredGridReader::New();
      }
      else
      {
        this->Reader = svtkXMLUnstructuredGridReader::New();
      }
      output = svtkUnstructuredGrid::New();
      break;
    default:
      this->Reader = nullptr;
  }

  if (this->Reader != nullptr)
  {
    this->Reader->SetFileName(this->GetFileName());
    //    this->Reader->SetStream(this->GetStream());
    // Delegate the error observers
    if (this->GetReaderErrorObserver())
    {
      this->Reader->AddObserver(svtkCommand::ErrorEvent, this->GetReaderErrorObserver());
    }
    if (this->GetParserErrorObserver())
    {
      this->Reader->SetParserErrorObserver(this->GetParserErrorObserver());
    }
    // Delegate call. RequestDataObject() would be more appropriate but it is
    // protected.
    int result = this->Reader->ProcessRequest(request, inputVector, outputVector);
    if (result)
    {
      svtkInformation* outInfo = outputVector->GetInformationObject(0);
      outInfo->Set(svtkDataObject::DATA_OBJECT(), output);

      //      outInfo->Set(svtkDataObject::DATA_OBJECT(),output);
      if (output != nullptr)
      {
        output->Delete();
      }
    }
    return result;
  }
  else
  {
    return 0;
  }
}

// ----------------------------------------------------------------------------
int svtkXMLGenericDataObjectReader::RequestInformation(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // reader is created in RequestDataObject.
  if (this->Reader != nullptr)
  {
    // RequestInformation() would be more appropriate but it is protected.
    return this->Reader->ProcessRequest(request, inputVector, outputVector);
  }
  else
  {
    return 0;
  }
}

// ----------------------------------------------------------------------------
int svtkXMLGenericDataObjectReader::RequestUpdateExtent(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // reader is created in RequestDataObject.
  if (this->Reader != nullptr)
  {
    // RequestUpdateExtent() would be more appropriate but it is protected.
    return this->Reader->ProcessRequest(request, inputVector, outputVector);
  }
  else
  {
    return 0;
  }
}

// ----------------------------------------------------------------------------
int svtkXMLGenericDataObjectReader::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // reader is created in RequestDataObject.
  if (this->Reader != nullptr)
  {
    // RequestData() would be more appropriate but it is protected.
    return this->Reader->ProcessRequest(request, inputVector, outputVector);
  }
  else
  {
    return 0;
  }
}

// ----------------------------------------------------------------------------
void svtkXMLGenericDataObjectReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------------
svtkDataObject* svtkXMLGenericDataObjectReader::GetOutput()
{
  return this->GetOutput(0);
}

// ----------------------------------------------------------------------------
svtkDataObject* svtkXMLGenericDataObjectReader::GetOutput(int idx)
{
  return this->GetOutputDataObject(idx);
}

// ---------------------------------------------------------------------------
svtkHierarchicalBoxDataSet* svtkXMLGenericDataObjectReader::GetHierarchicalBoxDataSetOutput()
{
  return svtkHierarchicalBoxDataSet::SafeDownCast(this->GetOutput());
}

// ---------------------------------------------------------------------------
svtkImageData* svtkXMLGenericDataObjectReader::GetImageDataOutput()
{
  return svtkImageData::SafeDownCast(this->GetOutput());
}

// ---------------------------------------------------------------------------
svtkMultiBlockDataSet* svtkXMLGenericDataObjectReader::GetMultiBlockDataSetOutput()
{
  return svtkMultiBlockDataSet::SafeDownCast(this->GetOutput());
}

// ---------------------------------------------------------------------------
svtkPolyData* svtkXMLGenericDataObjectReader::GetPolyDataOutput()
{
  return svtkPolyData::SafeDownCast(this->GetOutput());
}

// ---------------------------------------------------------------------------
svtkRectilinearGrid* svtkXMLGenericDataObjectReader::GetRectilinearGridOutput()
{
  return svtkRectilinearGrid::SafeDownCast(this->GetOutput());
}

// ---------------------------------------------------------------------------
svtkStructuredGrid* svtkXMLGenericDataObjectReader::GetStructuredGridOutput()
{
  return svtkStructuredGrid::SafeDownCast(this->GetOutput());
}

// ---------------------------------------------------------------------------
svtkUnstructuredGrid* svtkXMLGenericDataObjectReader::GetUnstructuredGridOutput()
{
  return svtkUnstructuredGrid::SafeDownCast(this->GetOutput());
}

// ----------------------------------------------------------------------------
const char* svtkXMLGenericDataObjectReader::GetDataSetName()
{
  assert("check: not_used" && 0); // should not be used.
  return "DataObject";            // not used.
}

// ----------------------------------------------------------------------------
void svtkXMLGenericDataObjectReader::SetupEmptyOutput()
{
  this->GetCurrentOutput()->Initialize();
}

// ----------------------------------------------------------------------------
int svtkXMLGenericDataObjectReader::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkDataObject");
  return 1;
}

// ----------------------------------------------------------------------------
svtkIdType svtkXMLGenericDataObjectReader::GetNumberOfPoints()
{
  svtkIdType numPts = 0;
  svtkDataSet* output = svtkDataSet::SafeDownCast(this->GetCurrentOutput());
  if (output)
  {
    numPts = output->GetNumberOfPoints();
  }
  return numPts;
}

// ----------------------------------------------------------------------------
svtkIdType svtkXMLGenericDataObjectReader::GetNumberOfCells()
{
  svtkIdType numCells = 0;
  svtkDataSet* output = svtkDataSet::SafeDownCast(this->GetCurrentOutput());
  if (output)
  {
    numCells = output->GetNumberOfCells();
  }
  return numCells;
}
