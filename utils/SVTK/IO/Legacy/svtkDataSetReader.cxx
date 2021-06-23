/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDataSetReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkDataSetReader.h"

#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"
#include "svtkPolyDataReader.h"
#include "svtkRectilinearGrid.h"
#include "svtkRectilinearGridReader.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredGrid.h"
#include "svtkStructuredGridReader.h"
#include "svtkStructuredPoints.h"
#include "svtkStructuredPointsReader.h"
#include "svtkUnstructuredGrid.h"
#include "svtkUnstructuredGridReader.h"

svtkStandardNewMacro(svtkDataSetReader);

svtkDataSetReader::svtkDataSetReader() = default;
svtkDataSetReader::~svtkDataSetReader() = default;

svtkDataObject* svtkDataSetReader::CreateOutput(svtkDataObject* currentOutput)
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
    case SVTK_POLY_DATA:
      output = svtkPolyData::New();
      break;
    case SVTK_STRUCTURED_POINTS:
      output = svtkStructuredPoints::New();
      break;
    case SVTK_STRUCTURED_GRID:
      output = svtkStructuredGrid::New();
      break;
    case SVTK_RECTILINEAR_GRID:
      output = svtkRectilinearGrid::New();
      break;
    case SVTK_UNSTRUCTURED_GRID:
      output = svtkUnstructuredGrid::New();
      break;
  }

  return output;
}

int svtkDataSetReader::ReadMetaDataSimple(const std::string& fname, svtkInformation* metadata)
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
    case SVTK_POLY_DATA:
      reader = svtkPolyDataReader::New();
      break;
    case SVTK_STRUCTURED_POINTS:
      reader = svtkStructuredPointsReader::New();
      break;
    case SVTK_STRUCTURED_GRID:
      reader = svtkStructuredGridReader::New();
      break;
    case SVTK_RECTILINEAR_GRID:
      reader = svtkRectilinearGridReader::New();
      break;
    case SVTK_UNSTRUCTURED_GRID:
      reader = svtkUnstructuredGridReader::New();
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

int svtkDataSetReader::ReadMeshSimple(const std::string& fname, svtkDataObject* output)
{
  svtkDebugMacro(<< "Reading svtk dataset...");

  switch (this->ReadOutputType())
  {
    case SVTK_POLY_DATA:
    {
      svtkPolyDataReader* preader = svtkPolyDataReader::New();
      preader->SetFileName(fname.c_str());
      preader->SetInputArray(this->GetInputArray());
      preader->SetInputString(this->GetInputString(), this->GetInputStringLength());
      preader->SetReadFromInputString(this->GetReadFromInputString());
      preader->SetScalarsName(this->GetScalarsName());
      preader->SetVectorsName(this->GetVectorsName());
      preader->SetNormalsName(this->GetNormalsName());
      preader->SetTensorsName(this->GetTensorsName());
      preader->SetTCoordsName(this->GetTCoordsName());
      preader->SetLookupTableName(this->GetLookupTableName());
      preader->SetFieldDataName(this->GetFieldDataName());
      preader->SetReadAllScalars(this->GetReadAllScalars());
      preader->SetReadAllVectors(this->GetReadAllVectors());
      preader->SetReadAllNormals(this->GetReadAllNormals());
      preader->SetReadAllTensors(this->GetReadAllTensors());
      preader->SetReadAllColorScalars(this->GetReadAllColorScalars());
      preader->SetReadAllTCoords(this->GetReadAllTCoords());
      preader->SetReadAllFields(this->GetReadAllFields());
      preader->Update();
      // Can we use the old output?
      if (!(output && strcmp(output->GetClassName(), "svtkPolyData") == 0))
      {
        // Hack to make sure that the object is not modified
        // with SetNthOutput. Otherwise, extra executions occur.
        svtkTimeStamp ts = this->MTime;
        output = svtkPolyData::New();
        this->GetExecutive()->SetOutputData(0, output);
        output->Delete();
        this->MTime = ts;
      }
      output->ShallowCopy(preader->GetOutput());
      preader->Delete();
      return 1;
    }
    case SVTK_STRUCTURED_POINTS:
    {
      svtkStructuredPointsReader* preader = svtkStructuredPointsReader::New();
      preader->SetFileName(fname.c_str());
      preader->SetInputArray(this->GetInputArray());
      preader->SetInputString(this->GetInputString(), this->GetInputStringLength());
      preader->SetReadFromInputString(this->GetReadFromInputString());
      preader->SetScalarsName(this->GetScalarsName());
      preader->SetVectorsName(this->GetVectorsName());
      preader->SetNormalsName(this->GetNormalsName());
      preader->SetTensorsName(this->GetTensorsName());
      preader->SetTCoordsName(this->GetTCoordsName());
      preader->SetLookupTableName(this->GetLookupTableName());
      preader->SetFieldDataName(this->GetFieldDataName());
      preader->SetReadAllScalars(this->GetReadAllScalars());
      preader->SetReadAllVectors(this->GetReadAllVectors());
      preader->SetReadAllNormals(this->GetReadAllNormals());
      preader->SetReadAllTensors(this->GetReadAllTensors());
      preader->SetReadAllColorScalars(this->GetReadAllColorScalars());
      preader->SetReadAllTCoords(this->GetReadAllTCoords());
      preader->SetReadAllFields(this->GetReadAllFields());
      preader->Update();
      output->ShallowCopy(preader->GetOutput());
      preader->Delete();
      return 1;
    }
    case SVTK_STRUCTURED_GRID:
    {
      svtkStructuredGridReader* preader = svtkStructuredGridReader::New();
      preader->SetFileName(fname.c_str());
      preader->SetInputArray(this->GetInputArray());
      preader->SetInputString(this->GetInputString(), this->GetInputStringLength());
      preader->SetReadFromInputString(this->GetReadFromInputString());
      preader->SetScalarsName(this->GetScalarsName());
      preader->SetVectorsName(this->GetVectorsName());
      preader->SetNormalsName(this->GetNormalsName());
      preader->SetTensorsName(this->GetTensorsName());
      preader->SetTCoordsName(this->GetTCoordsName());
      preader->SetLookupTableName(this->GetLookupTableName());
      preader->SetFieldDataName(this->GetFieldDataName());
      preader->SetReadAllScalars(this->GetReadAllScalars());
      preader->SetReadAllVectors(this->GetReadAllVectors());
      preader->SetReadAllNormals(this->GetReadAllNormals());
      preader->SetReadAllTensors(this->GetReadAllTensors());
      preader->SetReadAllColorScalars(this->GetReadAllColorScalars());
      preader->SetReadAllTCoords(this->GetReadAllTCoords());
      preader->SetReadAllFields(this->GetReadAllFields());
      preader->Update();
      // Can we use the old output?
      if (!(output && strcmp(output->GetClassName(), "svtkStructuredGrid") == 0))
      {
        // Hack to make sure that the object is not modified
        // with SetNthOutput. Otherwise, extra executions occur.
        svtkTimeStamp ts = this->MTime;
        output = svtkStructuredGrid::New();
        this->GetExecutive()->SetOutputData(0, output);
        output->Delete();
        this->MTime = ts;
      }
      output->ShallowCopy(preader->GetOutput());
      preader->Delete();
      return 1;
    }
    case SVTK_RECTILINEAR_GRID:
    {
      svtkRectilinearGridReader* preader = svtkRectilinearGridReader::New();
      preader->SetFileName(fname.c_str());
      preader->SetInputArray(this->GetInputArray());
      preader->SetInputString(this->GetInputString(), this->GetInputStringLength());
      preader->SetReadFromInputString(this->GetReadFromInputString());
      preader->SetScalarsName(this->GetScalarsName());
      preader->SetVectorsName(this->GetVectorsName());
      preader->SetNormalsName(this->GetNormalsName());
      preader->SetTensorsName(this->GetTensorsName());
      preader->SetTCoordsName(this->GetTCoordsName());
      preader->SetLookupTableName(this->GetLookupTableName());
      preader->SetFieldDataName(this->GetFieldDataName());
      preader->SetReadAllScalars(this->GetReadAllScalars());
      preader->SetReadAllVectors(this->GetReadAllVectors());
      preader->SetReadAllNormals(this->GetReadAllNormals());
      preader->SetReadAllTensors(this->GetReadAllTensors());
      preader->SetReadAllColorScalars(this->GetReadAllColorScalars());
      preader->SetReadAllTCoords(this->GetReadAllTCoords());
      preader->SetReadAllFields(this->GetReadAllFields());
      preader->Update();
      // Can we use the old output?
      if (!(output && strcmp(output->GetClassName(), "svtkRectilinearGrid") == 0))
      {
        // Hack to make sure that the object is not modified
        // with SetNthOutput. Otherwise, extra executions occur.
        svtkTimeStamp ts = this->MTime;
        output = svtkRectilinearGrid::New();
        this->GetExecutive()->SetOutputData(0, output);
        output->Delete();
        this->MTime = ts;
      }
      output->ShallowCopy(preader->GetOutput());
      preader->Delete();
      return 1;
    }
    case SVTK_UNSTRUCTURED_GRID:
    {
      svtkUnstructuredGridReader* preader = svtkUnstructuredGridReader::New();
      preader->SetFileName(fname.c_str());
      preader->SetInputArray(this->GetInputArray());
      preader->SetInputString(this->GetInputString(), this->GetInputStringLength());
      preader->SetReadFromInputString(this->GetReadFromInputString());
      preader->SetScalarsName(this->GetScalarsName());
      preader->SetVectorsName(this->GetVectorsName());
      preader->SetNormalsName(this->GetNormalsName());
      preader->SetTensorsName(this->GetTensorsName());
      preader->SetTCoordsName(this->GetTCoordsName());
      preader->SetLookupTableName(this->GetLookupTableName());
      preader->SetFieldDataName(this->GetFieldDataName());
      preader->SetReadAllScalars(this->GetReadAllScalars());
      preader->SetReadAllVectors(this->GetReadAllVectors());
      preader->SetReadAllNormals(this->GetReadAllNormals());
      preader->SetReadAllTensors(this->GetReadAllTensors());
      preader->SetReadAllColorScalars(this->GetReadAllColorScalars());
      preader->SetReadAllTCoords(this->GetReadAllTCoords());
      preader->SetReadAllFields(this->GetReadAllFields());
      preader->Update();
      // Can we use the old output?
      if (!(output && strcmp(output->GetClassName(), "svtkUnstructuredGrid") == 0))
      {
        // Hack to make sure that the object is not modified
        // with SetNthOutput. Otherwise, extra executions occur.
        svtkTimeStamp ts = this->MTime;
        output = svtkUnstructuredGrid::New();
        this->GetExecutive()->SetOutputData(0, output);
        output->Delete();
        this->MTime = ts;
      }
      output->ShallowCopy(preader->GetOutput());
      preader->Delete();
      return 1;
    }
    default:
      svtkErrorMacro("Could not read file " << this->GetFileName());
  }
  return 0;
}

int svtkDataSetReader::ReadOutputType()
{
  char line[256];

  svtkDebugMacro(<< "Reading svtk dataset...");

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

  if (!strncmp(this->LowerCase(line), "dataset", (unsigned long)7))
  {
    // See if type is recognized.
    //
    if (!this->ReadString(line))
    {
      svtkDebugMacro(<< "Premature EOF reading type");
      this->CloseSVTKFile();
      return -1;
    }

    this->CloseSVTKFile();
    if (!strncmp(this->LowerCase(line), "polydata", 8))
    {
      return SVTK_POLY_DATA;
    }
    else if (!strncmp(line, "structured_points", 17))
    {
      return SVTK_STRUCTURED_POINTS;
    }
    else if (!strncmp(line, "structured_grid", 15))
    {
      return SVTK_STRUCTURED_GRID;
    }
    else if (!strncmp(line, "rectilinear_grid", 16))
    {
      return SVTK_RECTILINEAR_GRID;
    }
    else if (!strncmp(line, "unstructured_grid", 17))
    {
      return SVTK_UNSTRUCTURED_GRID;
    }
    else
    {
      svtkDebugMacro(<< "Cannot read dataset type: " << line);
      return -1;
    }
  }
  else if (!strncmp(this->LowerCase(line), "field", (unsigned long)5))
  {
    svtkDebugMacro(<< "This object can only read datasets, not fields");
  }
  else
  {
    svtkDebugMacro(<< "Expecting DATASET keyword, got " << line << " instead");
  }

  return -1;
}

svtkPolyData* svtkDataSetReader::GetPolyDataOutput()
{
  return svtkPolyData::SafeDownCast(this->GetOutput());
}

svtkStructuredPoints* svtkDataSetReader::GetStructuredPointsOutput()
{
  return svtkStructuredPoints::SafeDownCast(this->GetOutput());
}

svtkStructuredGrid* svtkDataSetReader::GetStructuredGridOutput()
{
  return svtkStructuredGrid::SafeDownCast(this->GetOutput());
}

svtkUnstructuredGrid* svtkDataSetReader::GetUnstructuredGridOutput()
{
  return svtkUnstructuredGrid::SafeDownCast(this->GetOutput());
}

svtkRectilinearGrid* svtkDataSetReader::GetRectilinearGridOutput()
{
  return svtkRectilinearGrid::SafeDownCast(this->GetOutput());
}

void svtkDataSetReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

svtkDataSet* svtkDataSetReader::GetOutput(int idx)
{
  return svtkDataSet::SafeDownCast(this->GetOutputDataObject(idx));
}

svtkDataSet* svtkDataSetReader::GetOutput()
{
  return svtkDataSet::SafeDownCast(this->GetOutputDataObject(0));
}

int svtkDataSetReader::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkDataSet");
  return 1;
}
