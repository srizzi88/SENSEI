/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSimpleReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSimpleReader.h"

#include "svtkInformation.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkReaderExecutive.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include <numeric>
#include <vector>

struct svtkSimpleReaderInternal
{
  using FileNamesType = std::vector<std::string>;
  FileNamesType FileNames;
};

//----------------------------------------------------------------------------
svtkSimpleReader::svtkSimpleReader()
{
  this->Internal = new svtkSimpleReaderInternal;
  this->CurrentFileIndex = -1;
  this->HasTemporalMetaData = false;
}

//----------------------------------------------------------------------------
svtkSimpleReader::~svtkSimpleReader()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
svtkExecutive* svtkSimpleReader::CreateDefaultExecutive()
{
  return svtkReaderExecutive::New();
}

//----------------------------------------------------------------------------
void svtkSimpleReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void svtkSimpleReader::AddFileName(const char* fname)
{
  if (fname == nullptr || strlen(fname) == 0)
  {
    return;
  }
  this->Internal->FileNames.push_back(fname);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkSimpleReader::ClearFileNames()
{
  this->Internal->FileNames.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
int svtkSimpleReader::GetNumberOfFileNames() const
{
  return static_cast<int>(this->Internal->FileNames.size());
}

//----------------------------------------------------------------------------
const char* svtkSimpleReader::GetFileName(int i) const
{
  return this->Internal->FileNames[i].c_str();
}

//----------------------------------------------------------------------------
const char* svtkSimpleReader::GetCurrentFileName() const
{
  if (this->CurrentFileIndex < 0 || this->CurrentFileIndex >= (int)this->Internal->FileNames.size())
  {
    return nullptr;
  }
  return this->Internal->FileNames[this->CurrentFileIndex].c_str();
}

//----------------------------------------------------------------------------
int svtkSimpleReader::ReadTimeDependentMetaData(int timestep, svtkInformation* metadata)
{
  if (!this->HasTemporalMetaData)
  {
    return 1;
  }

  int nTimes = static_cast<int>(this->Internal->FileNames.size());
  if (timestep >= nTimes)
  {
    svtkErrorMacro(
      "Cannot read time step " << timestep << ". Only " << nTimes << " time steps are available.");
    return 0;
  }

  return this->ReadMetaDataSimple(this->Internal->FileNames[timestep], metadata);
}

//----------------------------------------------------------------------------
int svtkSimpleReader::ReadMetaData(svtkInformation* metadata)
{
  if (this->HasTemporalMetaData)
  {
    metadata->Set(svtkStreamingDemandDrivenPipeline::TIME_DEPENDENT_INFORMATION(), 1);
  }
  else
  {
    if (!this->Internal->FileNames.empty())
    {
      // Call the meta-data function on the first file.
      int retval = this->ReadMetaDataSimple(this->Internal->FileNames[0], metadata);
      if (!retval)
      {
        return retval;
      }
    }
  }

  if (this->Internal->FileNames.empty())
  {
    // No file names specified. No meta-data. There is still
    // no need to return with an error.
    return 1;
  }

  size_t nTimes = this->Internal->FileNames.size();
  std::vector<double> times(nTimes);

  bool hasTime = true;
  auto iter = times.begin();
  for (const auto& fname : this->Internal->FileNames)
  {
    auto time = this->GetTimeValue(fname);
    if (svtkMath::IsNan(time))
    {
      hasTime = false;
      break;
    }
    *iter++ = time;
  }

  if (!hasTime)
  {
    std::iota(times.begin(), times.end(), 0);
  }

  double timeRange[2];
  timeRange[0] = times[0];
  timeRange[1] = times[nTimes - 1];

  metadata->Set(svtkStreamingDemandDrivenPipeline::TIME_STEPS(), &times[0], (int)nTimes);
  metadata->Set(svtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);

  return 1;
}

//----------------------------------------------------------------------------
int svtkSimpleReader::ReadMesh(int piece, int, int, int timestep, svtkDataObject* output)
{
  // Not a parallel reader. Cannot handle anything other than the first piece,
  // which will have everything.
  if (piece > 0)
  {
    return 1;
  }

  int nTimes = static_cast<int>(this->Internal->FileNames.size());
  if (timestep >= nTimes)
  {
    svtkErrorMacro(
      "Cannot read time step " << timestep << ". Only " << nTimes << " time steps are available.");
    return 0;
  }

  if (this->ReadMeshSimple(this->Internal->FileNames[timestep], output))
  {
    this->CurrentFileIndex = timestep;
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
int svtkSimpleReader::ReadPoints(int piece, int, int, int timestep, svtkDataObject* output)
{
  // Not a parallel reader. Cannot handle anything other than the first piece,
  // which will have everything.
  if (piece > 0)
  {
    return 1;
  }

  int nTimes = static_cast<int>(this->Internal->FileNames.size());
  if (timestep >= nTimes)
  {
    svtkErrorMacro(
      "Cannot read time step " << timestep << ". Only " << nTimes << " time steps are available.");
    return 0;
  }

  return this->ReadPointsSimple(this->Internal->FileNames[timestep], output);
}

//----------------------------------------------------------------------------
int svtkSimpleReader::ReadArrays(int piece, int, int, int timestep, svtkDataObject* output)
{
  // Not a parallel reader. Cannot handle anything other than the first piece,
  // which will have everything.
  if (piece > 0)
  {
    return 1;
  }

  int nTimes = static_cast<int>(this->Internal->FileNames.size());
  if (timestep >= nTimes)
  {
    svtkErrorMacro(
      "Cannot read time step " << timestep << ". Only " << nTimes << " time steps are available.");
    return 0;
  }

  return this->ReadArraysSimple(this->Internal->FileNames[timestep], output);
}

//----------------------------------------------------------------------------
double svtkSimpleReader::GetTimeValue(const std::string&)
{
  return svtkMath::Nan();
}
