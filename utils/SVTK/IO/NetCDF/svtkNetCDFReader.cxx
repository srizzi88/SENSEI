// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkNetCDFReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "svtkNetCDFReader.h"

#include "svtkCallbackCommand.h"
#include "svtkCellData.h"
#include "svtkDataArraySelection.h"
#include "svtkDoubleArray.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkRectilinearGrid.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStringArray.h"
#include "svtkStructuredGrid.h"

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <string>

#include <svtksys/SystemTools.hxx>

#include "svtk_netcdf.h"

#define CALL_NETCDF(call)                                                                          \
  {                                                                                                \
    int errorcode = call;                                                                          \
    if (errorcode != NC_NOERR)                                                                     \
    {                                                                                              \
      svtkErrorMacro(<< "netCDF Error: " << nc_strerror(errorcode));                                \
      return 0;                                                                                    \
    }                                                                                              \
  }

#include <cctype>

class svtkNetCDFReaderPrivate
{
public:
  svtkNetCDFReaderPrivate() = default;
  ~svtkNetCDFReaderPrivate() { this->ArrayUnits.clear(); }
  void AddUnit(const std::string& arrayName, const std::string& unit)
  {
    this->ArrayUnits[arrayName] = unit;
  }
  std::map<std::string, std::string> ArrayUnits;
};

//=============================================================================
static int NetCDFTypeToSVTKType(nc_type type)
{
  switch (type)
  {
    case NC_BYTE:
      return SVTK_UNSIGNED_CHAR;
    case NC_CHAR:
      return SVTK_CHAR;
    case NC_SHORT:
      return SVTK_SHORT;
    case NC_INT:
      return SVTK_INT;
    case NC_FLOAT:
      return SVTK_FLOAT;
    case NC_DOUBLE:
      return SVTK_DOUBLE;
    default:
      svtkGenericWarningMacro(<< "Unknown netCDF variable type " << type);
      return -1;
  }
}

//=============================================================================
svtkStandardNewMacro(svtkNetCDFReader);

//-----------------------------------------------------------------------------
svtkNetCDFReader::svtkNetCDFReader()
{
  this->SetNumberOfInputPorts(0);

  this->FileName = nullptr;
  this->ReplaceFillValueWithNan = 0;

  this->LoadingDimensions = svtkSmartPointer<svtkIntArray>::New();

  this->VariableArraySelection = svtkSmartPointer<svtkDataArraySelection>::New();
  SVTK_CREATE(svtkCallbackCommand, cbc);
  cbc->SetCallback(&svtkNetCDFReader::SelectionModifiedCallback);
  cbc->SetClientData(this);
  this->VariableArraySelection->AddObserver(svtkCommand::ModifiedEvent, cbc);

  this->AllVariableArrayNames = svtkSmartPointer<svtkStringArray>::New();

  this->VariableDimensions = svtkStringArray::New();
  this->AllDimensions = svtkStringArray::New();

  this->WholeExtent[0] = this->WholeExtent[1] = this->WholeExtent[2] = this->WholeExtent[3] =
    this->WholeExtent[4] = this->WholeExtent[5] = 0;

  this->TimeUnits = nullptr;
  this->Calendar = nullptr;
  this->Private = new svtkNetCDFReaderPrivate();
}

svtkNetCDFReader::~svtkNetCDFReader()
{
  this->SetFileName(nullptr);
  this->VariableDimensions->Delete();
  this->AllDimensions->Delete();
  delete[] this->TimeUnits;
  delete[] this->Calendar;
  delete this->Private;
}

void svtkNetCDFReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FileName: " << (this->FileName ? this->FileName : "(nullptr)") << endl;
  os << indent << "ReplaceFillValueWithNan: " << this->ReplaceFillValueWithNan << endl;

  os << indent << "VariableArraySelection:" << endl;
  this->VariableArraySelection->PrintSelf(os, indent.GetNextIndent());
  os << indent << "AllVariableArrayNames:" << endl;
  this->GetAllVariableArrayNames()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "VariableDimensions: " << this->VariableDimensions << endl;
  os << indent << "AllDimensions: " << this->AllDimensions << endl;
}

//-----------------------------------------------------------------------------
int svtkNetCDFReader::RequestDataObject(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkDataObject* output = svtkDataObject::GetData(outInfo);

  if (!output || !output->IsA("svtkImageData"))
  {
    output = svtkImageData::New();
    outInfo->Set(svtkDataObject::DATA_OBJECT(), output);
    output->Delete(); // Not really deleted.
  }

  return 1;
}

//-----------------------------------------------------------------------------
int svtkNetCDFReader::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  if (!this->UpdateMetaData())
    return 0;

  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  int ncFD;
  CALL_NETCDF(nc_open(this->FileName, NC_NOWRITE, &ncFD));

  SVTK_CREATE(svtkDoubleArray, timeValues);
  SVTK_CREATE(svtkIntArray, currentDimensions);
  this->LoadingDimensions->Initialize();
  int numArrays = this->VariableArraySelection->GetNumberOfArrays();
  int numDims = 0;
  for (int arrayIndex = 0; arrayIndex < numArrays; arrayIndex++)
  {
    if (!this->VariableArraySelection->GetArraySetting(arrayIndex))
      continue;

    const char* name = this->VariableArraySelection->GetArrayName(arrayIndex);
    int varId;
    CALL_NETCDF(nc_inq_varid(ncFD, name, &varId));

    int currentNumDims;
    CALL_NETCDF(nc_inq_varndims(ncFD, varId, &currentNumDims));
    if (currentNumDims < 1)
      continue;
    currentDimensions->SetNumberOfComponents(1);
    currentDimensions->SetNumberOfTuples(currentNumDims);
    CALL_NETCDF(nc_inq_vardimid(ncFD, varId, currentDimensions->GetPointer(0)));

    // get units
    int status;
    size_t len = 0;
    char* buffer = nullptr;
    status = nc_inq_attlen(ncFD, varId, "units", &len);
    if (status == NC_NOERR)
    {
      buffer = new char[len + 1];
      status = nc_get_att_text(ncFD, varId, "units", buffer);
      buffer[len] = '\0';
    }
    if (status == NC_NOERR)
    {
      this->Private->AddUnit(name, buffer);
    }
    delete[] buffer;

    // Assumption: time dimension is first.
    int timeDim = currentDimensions->GetValue(0); // Not determined yet.
    if (this->IsTimeDimension(ncFD, timeDim))
    {
      // Get time step information.
      SVTK_CREATE(svtkDoubleArray, compositeTimeValues);
      svtkSmartPointer<svtkDoubleArray> currentTimeValues = this->GetTimeValues(ncFD, timeDim);
      double* oldTime = timeValues->GetPointer(0);
      double* newTime = currentTimeValues->GetPointer(0);
      double* oldTimeEnd = oldTime + timeValues->GetNumberOfTuples();
      double* newTimeEnd = newTime + currentTimeValues->GetNumberOfTuples();
      compositeTimeValues->Allocate(
        timeValues->GetNumberOfTuples() + currentTimeValues->GetNumberOfTuples());
      compositeTimeValues->SetNumberOfComponents(1);
      while ((oldTime < oldTimeEnd) || (newTime < newTimeEnd))
      {
        double nextTimeValue;
        if ((newTime >= newTimeEnd) || ((oldTime < oldTimeEnd) && (*oldTime < *newTime)))
        {
          nextTimeValue = *oldTime;
          oldTime++;
        }
        else if ((oldTime >= oldTimeEnd) || (*newTime < *oldTime))
        {
          nextTimeValue = *newTime;
          newTime++;
        }
        else // *oldTime == *newTime
        {
          nextTimeValue = *oldTime;
          oldTime++;
          newTime++;
        }
        compositeTimeValues->InsertNextTuple1(nextTimeValue);

        // Obviously, time values should be monotonically increasing. If they
        // are not, that is indicative of an error. Often this means just a
        // garbage time slice. Because we still want to see the other time
        // slices, just dump those with bad time values.
        while ((newTime < newTimeEnd) && (*newTime <= nextTimeValue))
        {
          newTime++;
        }
      }
      timeValues = compositeTimeValues;

      // Strip off time dimension from what we load (we will use it to
      // subset instead).
      currentDimensions->RemoveTuple(0);
      currentNumDims--;
    }

    // Remember the first variable we encounter.  Use it to determine extents
    // (below).
    if (numDims == 0)
    {
      numDims = currentNumDims;
      this->LoadingDimensions->DeepCopy(currentDimensions);
    }
  }

  // Capture the extent information from this->LoadingDimensions.
  bool pointData = this->DimensionsAreForPointData(this->LoadingDimensions);
  for (int i = 0; i < 3; i++)
  {
    this->WholeExtent[2 * i] = 0;
    if (i < this->LoadingDimensions->GetNumberOfTuples())
    {
      size_t dimlength;
      // Remember that netCDF arrays are indexed backward from SVTK images.
      int dim = this->LoadingDimensions->GetValue(numDims - i - 1);
      CALL_NETCDF(nc_inq_dimlen(ncFD, dim, &dimlength));
      this->WholeExtent[2 * i + 1] = static_cast<int>(dimlength - 1);
      // For cell data, add one to the extent (which is for points).
      if (!pointData)
        this->WholeExtent[2 * i + 1]++;
    }
    else
    {
      this->WholeExtent[2 * i + 1] = 0;
    }
  }
  svtkDebugMacro(<< "Whole extents: " << this->WholeExtent[0] << ", " << this->WholeExtent[1] << ", "
                << this->WholeExtent[2] << ", " << this->WholeExtent[3] << ", "
                << this->WholeExtent[4] << ", " << this->WholeExtent[5]);

  // Report extents.
  svtkDataObject* output = svtkDataObject::GetData(outInfo);
  if (output && (output->GetExtentType() == SVTK_3D_EXTENT))
  {
    outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), this->WholeExtent, 6);
  }

  // Free old time units.
  delete[] this->TimeUnits;
  this->TimeUnits = nullptr;
  delete[] this->Calendar;
  this->Calendar = nullptr;

  // If we have time, report that.
  if (timeValues && (timeValues->GetNumberOfTuples() > 0))
  {
    outInfo->Set(svtkStreamingDemandDrivenPipeline::TIME_STEPS(), timeValues->GetPointer(0),
      timeValues->GetNumberOfTuples());
    double timeRange[2];
    timeRange[0] = timeValues->GetValue(0);
    timeRange[1] = timeValues->GetValue(timeValues->GetNumberOfTuples() - 1);
    outInfo->Set(svtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);

    // Get time units
    int status, varId;
    size_t len = 0;
    char* buffer = nullptr;
    status = nc_inq_varid(ncFD, "time", &varId);
    if (status == NC_NOERR)
    {
      status = nc_inq_attlen(ncFD, varId, "units", &len);
    }
    if (status == NC_NOERR)
    {
      buffer = new char[len + 1];
      status = nc_get_att_text(ncFD, varId, "units", buffer);
      buffer[len] = '\0';
      if (status == NC_NOERR)
      {
        this->TimeUnits = buffer;
      }
      else
      {
        delete[] buffer;
      }
    }

    // Get calendar that time units are in
    if (status == NC_NOERR)
    {
      status = nc_inq_attlen(ncFD, varId, "calendar", &len);
    }
    if (status == NC_NOERR)
    {
      buffer = new char[len + 1];
      status = nc_get_att_text(ncFD, varId, "calendar", buffer);
      buffer[len] = '\0';
      if (status == NC_NOERR)
      {
        this->Calendar = buffer;
      }
      else
      {
        delete[] buffer;
      }
    }
  }
  else
  {
    outInfo->Remove(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
    outInfo->Remove(svtkStreamingDemandDrivenPipeline::TIME_RANGE());
  }

  CALL_NETCDF(nc_close(ncFD));

  return 1;
}

//-----------------------------------------------------------------------------
int svtkNetCDFReader::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  // If the output is not a svtkDataSet, then the subclass needs to override
  // this method.
  svtkDataSet* output = svtkDataSet::GetData(outInfo);
  if (!output)
  {
    svtkErrorMacro(<< "Bad output type.");
    return 0;
  }

  // Set up the extent for regular-grid type data sets.
  svtkImageData* imageOutput = svtkImageData::SafeDownCast(output);
  svtkRectilinearGrid* rectOutput = svtkRectilinearGrid::SafeDownCast(output);
  svtkStructuredGrid* structOutput = svtkStructuredGrid::SafeDownCast(output);
  outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), this->UpdateExtent);
  if (imageOutput)
  {
    imageOutput->SetExtent(this->UpdateExtent);
  }
  else if (rectOutput)
  {
    rectOutput->SetExtent(this->UpdateExtent);
  }
  else if (structOutput)
  {
    structOutput->SetExtent(this->UpdateExtent);
  }
  else
  {
    // Superclass should handle extent setup if necessary.
  }

  // Get requested time step.
  double time = 0.0;
  if (outInfo->Has(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    time = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
  }

  int ncFD;
  CALL_NETCDF(nc_open(this->FileName, NC_NOWRITE, &ncFD));

  this->ComputeArraySelection();
  // Iterate over arrays and load selected ones.
  int numArrays = this->VariableArraySelection->GetNumberOfArrays();
  for (int arrayIndex = 0; arrayIndex < numArrays; arrayIndex++)
  {
    if (!this->VariableArraySelection->GetArraySetting(arrayIndex))
      continue;

    const char* name = this->VariableArraySelection->GetArrayName(arrayIndex);

    if (!this->LoadVariable(ncFD, name, time, output))
      return 0;
  }

  // Add time units and time calendar as field arrays
  if (this->TimeUnits)
  {
    svtkNew<svtkStringArray> arr;
    arr->SetName("time_units");
    arr->InsertNextValue(this->TimeUnits);
    output->GetFieldData()->AddArray(arr);
  }
  if (this->Calendar)
  {
    svtkNew<svtkStringArray> arr;
    arr->SetName("time_calendar");
    arr->InsertNextValue(this->Calendar);
    output->GetFieldData()->AddArray(arr);
  }

  // Add data array units as field arrays
  for (const auto& pair : this->Private->ArrayUnits)
  {
    svtkNew<svtkStringArray> arr;
    std::stringstream ss;
    ss << pair.first << "_units";
    arr->SetName(ss.str().c_str());
    arr->InsertNextValue(pair.second);
    output->GetFieldData()->AddArray(arr);
  }

  CALL_NETCDF(nc_close(ncFD));

  return 1;
}

//-----------------------------------------------------------------------------
void svtkNetCDFReader::SetFileName(const char* filename)
{
  if (this->FileName == filename)
    return;
  if (this->FileName && filename && (strcmp(this->FileName, filename) == 0))
  {
    return;
  }

  delete[] this->FileName;
  this->FileName = nullptr;

  if (filename)
  {
    this->FileName = new char[strlen(filename) + 1];
    strcpy(this->FileName, filename);
  }

  this->Modified();
  this->FileNameMTime.Modified();
}

//----------------------------------------------------------------------------
void svtkNetCDFReader::SelectionModifiedCallback(svtkObject*, unsigned long, void* clientdata, void*)
{
  static_cast<svtkNetCDFReader*>(clientdata)->Modified();
}

//-----------------------------------------------------------------------------
int svtkNetCDFReader::GetNumberOfVariableArrays()
{
  return this->VariableArraySelection->GetNumberOfArrays();
}

//-----------------------------------------------------------------------------
const char* svtkNetCDFReader::GetVariableArrayName(int index)
{
  return this->VariableArraySelection->GetArrayName(index);
}

//-----------------------------------------------------------------------------
int svtkNetCDFReader::GetVariableArrayStatus(const char* name)
{
  return this->VariableArraySelection->ArrayIsEnabled(name);
}

//-----------------------------------------------------------------------------
void svtkNetCDFReader::SetVariableArrayStatus(const char* name, int status)
{
  svtkDebugMacro("Set cell array \"" << name << "\" status to: " << status);
  if (status)
  {
    this->VariableArraySelection->EnableArray(name);
  }
  else
  {
    this->VariableArraySelection->DisableArray(name);
  }
}

//-----------------------------------------------------------------------------
svtkStringArray* svtkNetCDFReader::GetAllVariableArrayNames()
{
  int numArrays = this->GetNumberOfVariableArrays();
  this->AllVariableArrayNames->SetNumberOfValues(numArrays);
  for (int arrayIdx = 0; arrayIdx < numArrays; arrayIdx++)
  {
    const char* arrayName = this->GetVariableArrayName(arrayIdx);
    this->AllVariableArrayNames->SetValue(arrayIdx, arrayName);
  }

  return this->AllVariableArrayNames;
}

//-----------------------------------------------------------------------------
bool svtkNetCDFReader::ComputeArraySelection()
{
  if (this->VariableArraySelection->GetNumberOfArrays() == 0 || this->CurrentDimensions.empty())
  {
    return false;
  }

  this->VariableArraySelection->DisableAllArrays();

  for (svtkIdType i = 0; i < this->VariableDimensions->GetNumberOfValues(); i++)
  {
    if (this->VariableDimensions->GetValue(i) == this->CurrentDimensions)
    {
      const char* variableName = this->VariableArraySelection->GetArrayName(i);
      this->VariableArraySelection->EnableArray(variableName);
      return true;
    }
  }

  svtkWarningMacro("Variable dimensions (" << this->CurrentDimensions << ") not found.");
  return false;
}

//-----------------------------------------------------------------------------
void svtkNetCDFReader::SetDimensions(const char* dimensions)
{
  this->CurrentDimensions = dimensions;
  this->ComputeArraySelection();
}

//-----------------------------------------------------------------------------
int svtkNetCDFReader::UpdateMetaData()
{
  if (this->MetaDataMTime < this->FileNameMTime)
  {
    if (!this->FileName)
    {
      svtkErrorMacro(<< "FileName not set.");
      return 0;
    }

    int ncFD;
    CALL_NETCDF(nc_open(this->FileName, NC_NOWRITE, &ncFD));

    int retval = this->ReadMetaData(ncFD);

    if (retval)
      retval = this->FillVariableDimensions(ncFD);

    if (retval)
      this->MetaDataMTime.Modified();

    CALL_NETCDF(nc_close(ncFD));

    return retval;
  }
  else
  {
    return 1;
  }
}

//-----------------------------------------------------------------------------
svtkStdString svtkNetCDFReader::DescribeDimensions(int ncFD, const int* dimIds, int numDims)
{
  svtkStdString description;
  for (int i = 0; i < numDims; i++)
  {
    char name[NC_MAX_NAME + 1];
    CALL_NETCDF(nc_inq_dimname(ncFD, dimIds[i], name));
    if (i > 0)
      description += " ";
    description += name;
  }
  return description;
}

//-----------------------------------------------------------------------------
int svtkNetCDFReader::ReadMetaData(int ncFD)
{
  svtkDebugMacro("ReadMetaData");

  // Look at all variables and record them so that the user can select which
  // ones he wants.  This oddness of adding and removing from
  // VariableArraySelection is to preserve any current settings for variables.
  typedef std::set<svtkStdString> stringSet;
  stringSet variablesToAdd;
  stringSet variablesToRemove;

  // Initialize variablesToRemove with all the variables.  Then remove them from
  // the list as we find them.
  for (int i = 0; i < this->VariableArraySelection->GetNumberOfArrays(); i++)
  {
    variablesToRemove.insert(this->VariableArraySelection->GetArrayName(i));
  }

  int numVariables;
  CALL_NETCDF(nc_inq_nvars(ncFD, &numVariables));

  for (int i = 0; i < numVariables; i++)
  {
    char name[NC_MAX_NAME + 1];
    CALL_NETCDF(nc_inq_varname(ncFD, i, name));
    if (variablesToRemove.find(name) == variablesToRemove.end())
    {
      // Variable not already here.  Insert it in the variables to add.
      variablesToAdd.insert(name);
    }
    else
    {
      // Variable already exists.  Leave it be.  Remove it from the
      // variablesToRemove list.
      variablesToRemove.erase(name);
    }
  }

  // Add and remove variables.  This will be a no-op if the variables have not
  // changed.
  for (stringSet::iterator removeItr = variablesToRemove.begin();
       removeItr != variablesToRemove.end(); ++removeItr)
  {
    this->VariableArraySelection->RemoveArrayByName(removeItr->c_str());
  }
  for (stringSet::iterator addItr = variablesToAdd.begin(); addItr != variablesToAdd.end();
       ++addItr)
  {
    this->VariableArraySelection->AddArray(addItr->c_str());
  }

  return 1;
}

//-----------------------------------------------------------------------------
int svtkNetCDFReader::FillVariableDimensions(int ncFD)
{
  int numVar = this->GetNumberOfVariableArrays();
  this->VariableDimensions->SetNumberOfValues(numVar);
  this->AllDimensions->SetNumberOfValues(0);

  for (int i = 0; i < numVar; i++)
  {
    // Get the dimensions of this variable and encode them in a string.
    const char* varName = this->GetVariableArrayName(i);
    int varId, numDim, dimIds[NC_MAX_VAR_DIMS];
    CALL_NETCDF(nc_inq_varid(ncFD, varName, &varId));
    CALL_NETCDF(nc_inq_varndims(ncFD, varId, &numDim));
    CALL_NETCDF(nc_inq_vardimid(ncFD, varId, dimIds));
    svtkStdString dimEncoding("(");
    for (int j = 0; j < numDim; j++)
    {
      if ((j == 0) && (this->IsTimeDimension(ncFD, dimIds[j])))
        continue;
      char dimName[NC_MAX_NAME + 1];
      CALL_NETCDF(nc_inq_dimname(ncFD, dimIds[j], dimName));
      if (dimEncoding.size() > 1)
        dimEncoding += ", ";
      dimEncoding += dimName;
    }
    dimEncoding += ")";
    // Store all dimensions in the VariableDimensions array.
    this->VariableDimensions->SetValue(i, dimEncoding.c_str());
    // Store unique dimensions in the AllDimensions array.
    bool unique = true;
    for (svtkIdType j = 0; j < this->AllDimensions->GetNumberOfValues(); j++)
    {
      if (this->AllDimensions->GetValue(j) == dimEncoding)
      {
        unique = false;
        break;
      }
    }
    if (unique)
      this->AllDimensions->InsertNextValue(dimEncoding);
  }

  return 1;
}

//-----------------------------------------------------------------------------
int svtkNetCDFReader::IsTimeDimension(int ncFD, int dimId)
{
  char name[NC_MAX_NAME + 1];
  CALL_NETCDF(nc_inq_dimname(ncFD, dimId, name));
  name[4] = '\0'; // Truncate to 4 characters.
  return (svtksys::SystemTools::Strucmp(name, "time") == 0);
}

//-----------------------------------------------------------------------------
svtkSmartPointer<svtkDoubleArray> svtkNetCDFReader::GetTimeValues(int ncFD, int dimId)
{
  SVTK_CREATE(svtkDoubleArray, timeValues);
  size_t dimLength;
  CALL_NETCDF(nc_inq_dimlen(ncFD, dimId, &dimLength));
  timeValues->SetNumberOfComponents(1);
  timeValues->SetNumberOfTuples(static_cast<svtkIdType>(dimLength));
  for (size_t j = 0; j < dimLength; j++)
  {
    timeValues->SetValue(static_cast<svtkIdType>(j), static_cast<double>(j));
  }
  return timeValues;
}

//-----------------------------------------------------------------------------
void svtkNetCDFReader::GetUpdateExtentForOutput(svtkDataSet*, int extent[6])
{
  memcpy(extent, this->UpdateExtent, 6 * sizeof(int));
}

//-----------------------------------------------------------------------------
int svtkNetCDFReader::LoadVariable(int ncFD, const char* varName, double time, svtkDataSet* output)
{
  // Get the variable id.
  int varId;
  CALL_NETCDF(nc_inq_varid(ncFD, varName, &varId));

  // Get dimension info.
  int numDims;
  CALL_NETCDF(nc_inq_varndims(ncFD, varId, &numDims));
  if (numDims > 4)
  {
    svtkErrorMacro(<< "More than 3 dims + time not supported in variable " << varName);
    return 0;
  }
  int dimIds[4];
  CALL_NETCDF(nc_inq_vardimid(ncFD, varId, dimIds));

  // Number of values to read.
  svtkIdType arraySize = 1;

  // Indices to read from.
  size_t start[4], count[4];

  // Are we using time?
  int timeIndexOffset = 0;
  if ((numDims > 0) && this->IsTimeDimension(ncFD, dimIds[0]))
  {
    svtkSmartPointer<svtkDoubleArray> timeValues = this->GetTimeValues(ncFD, dimIds[0]);
    timeIndexOffset = 1;
    // Find the index for the given time.
    // We could speed this up with a binary search or something.
    for (start[0] = 0; start[0] < static_cast<size_t>(timeValues->GetNumberOfTuples()); start[0]++)
    {
      if (timeValues->GetValue(static_cast<svtkIdType>(start[0])) >= time)
        break;
    }
    count[0] = 1;
    numDims--;
  }

  if (numDims > 3)
  {
    svtkErrorMacro(<< "More than 3 dims without time not supported in variable " << varName);
    return 0;
  }

  bool loadingPointData = this->DimensionsAreForPointData(this->LoadingDimensions);

  // Set up read indices.  Also check to make sure the dimensions are consistent
  // with other loaded variables.
  int extent[6];
  this->GetUpdateExtentForOutput(output, extent);
  if (numDims != this->LoadingDimensions->GetNumberOfTuples())
  {
    svtkWarningMacro(<< "Variable " << varName << " dimensions ("
                    << this->DescribeDimensions(ncFD, dimIds + timeIndexOffset, numDims).c_str()
                    << ") are different than the other variable dimensions ("
                    << this
                         ->DescribeDimensions(ncFD, this->LoadingDimensions->GetPointer(0),
                           this->LoadingDimensions->GetNumberOfTuples())
                         .c_str()
                    << ").  Skipping");
    return 1;
  }
  for (int i = 0; i < numDims; i++)
  {
    if (dimIds[i + timeIndexOffset] != this->LoadingDimensions->GetValue(i))
    {
      svtkWarningMacro(<< "Variable " << varName << " dimensions ("
                      << this->DescribeDimensions(ncFD, dimIds + timeIndexOffset, numDims).c_str()
                      << ") are different than the other variable dimensions ("
                      << this
                           ->DescribeDimensions(ncFD, this->LoadingDimensions->GetPointer(0),
                             this->LoadingDimensions->GetNumberOfTuples())
                           .c_str()
                      << ").  Skipping");
      return 1;
    }
    // Remember that netCDF arrays are indexed backward from SVTK images.
    start[i + timeIndexOffset] = extent[2 * (numDims - i - 1)];
    count[i + timeIndexOffset] =
      extent[2 * (numDims - i - 1) + 1] - extent[2 * (numDims - i - 1)] + 1;

    // If loading cell data, subtract one from the data being loaded.
    if (!loadingPointData)
      count[i + timeIndexOffset]--;

    arraySize *= static_cast<svtkIdType>(count[i + timeIndexOffset]);
  }

  // Allocate an array of the right type.
  nc_type ncType;
  CALL_NETCDF(nc_inq_vartype(ncFD, varId, &ncType));
  int svtkType = NetCDFTypeToSVTKType(ncType);
  if (svtkType < 1)
    return 0;
  svtkSmartPointer<svtkDataArray> dataArray;
  dataArray.TakeReference(svtkDataArray::CreateDataArray(svtkType));
  dataArray->SetNumberOfComponents(1);
  dataArray->SetNumberOfTuples(arraySize);

  // Read the array from the file.
  CALL_NETCDF(nc_get_vars(ncFD, varId, start, count, nullptr, dataArray->GetVoidPointer(0)));

  // Check for a fill value.
  size_t attribLength;
  if ((nc_inq_attlen(ncFD, varId, "_FillValue", &attribLength) == NC_NOERR) && (attribLength == 1))
  {
    if (this->ReplaceFillValueWithNan)
    {
      // NaN only available with float and double.
      if (dataArray->GetDataType() == SVTK_FLOAT)
      {
        float fillValue;
        nc_get_att_float(ncFD, varId, "_FillValue", &fillValue);
        std::replace(reinterpret_cast<float*>(dataArray->GetVoidPointer(0)),
          reinterpret_cast<float*>(dataArray->GetVoidPointer(dataArray->GetNumberOfTuples())),
          fillValue, static_cast<float>(svtkMath::Nan()));
      }
      else if (dataArray->GetDataType() == SVTK_DOUBLE)
      {
        double fillValue;
        nc_get_att_double(ncFD, varId, "_FillValue", &fillValue);
        std::replace(reinterpret_cast<double*>(dataArray->GetVoidPointer(0)),
          reinterpret_cast<double*>(dataArray->GetVoidPointer(dataArray->GetNumberOfTuples())),
          fillValue, svtkMath::Nan());
      }
      else
      {
        svtkDebugMacro(<< "No NaN available for data of type " << dataArray->GetDataType());
      }
    }
  }

  // Check to see if there is a scale or offset.
  double scale = 1.0;
  double offset = 0.0;
  if ((nc_inq_attlen(ncFD, varId, "scale_factor", &attribLength) == NC_NOERR) &&
    (attribLength == 1))
  {
    CALL_NETCDF(nc_get_att_double(ncFD, varId, "scale_factor", &scale));
  }
  if ((nc_inq_attlen(ncFD, varId, "add_offset", &attribLength) == NC_NOERR) && (attribLength == 1))
  {
    CALL_NETCDF(nc_get_att_double(ncFD, varId, "add_offset", &offset));
  }

  if ((scale != 1.0) || (offset != 0.0))
  {
    SVTK_CREATE(svtkDoubleArray, adjustedArray);
    adjustedArray->SetNumberOfComponents(1);
    adjustedArray->SetNumberOfTuples(arraySize);
    for (svtkIdType i = 0; i < arraySize; i++)
    {
      adjustedArray->SetValue(i, dataArray->GetTuple1(i) * scale + offset);
    }
    dataArray = adjustedArray;
  }

  // Add data to the output.
  dataArray->SetName(varName);
  if (loadingPointData)
  {
    output->GetPointData()->AddArray(dataArray);
  }
  else
  {
    output->GetCellData()->AddArray(dataArray);
  }

  return 1;
}

//-----------------------------------------------------------------------------
std::string svtkNetCDFReader::QueryArrayUnits(const char* name)
{
  return this->Private->ArrayUnits[name];
}
