/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkParallelReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkParallelReader.h"

#include "svtkInformation.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkReaderExecutive.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include <numeric>
#include <vector>

struct svtkParallelReaderInternal
{
  using FileNamesType = std::vector<std::string>;
  FileNamesType FileNames;
};

//----------------------------------------------------------------------------
svtkParallelReader::svtkParallelReader()
{
  this->Internal = new svtkParallelReaderInternal;
  this->CurrentFileIndex = -1;
}

//----------------------------------------------------------------------------
svtkParallelReader::~svtkParallelReader()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
svtkExecutive* svtkParallelReader::CreateDefaultExecutive()
{
  return svtkReaderExecutive::New();
}

//----------------------------------------------------------------------------
void svtkParallelReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void svtkParallelReader::AddFileName(const char* fname)
{
  if (fname == nullptr || strlen(fname) == 0)
  {
    return;
  }
  this->Internal->FileNames.push_back(fname);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkParallelReader::ClearFileNames()
{
  this->Internal->FileNames.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
int svtkParallelReader::GetNumberOfFileNames() const
{
  return static_cast<int>(this->Internal->FileNames.size());
}

//----------------------------------------------------------------------------
const char* svtkParallelReader::GetFileName(int i) const
{
  return this->Internal->FileNames[i].c_str();
}

//----------------------------------------------------------------------------
const char* svtkParallelReader::GetCurrentFileName() const
{
  if (this->CurrentFileIndex < 0 || this->CurrentFileIndex >= (int)this->Internal->FileNames.size())
  {
    return nullptr;
  }
  return this->Internal->FileNames[this->CurrentFileIndex].c_str();
}

//----------------------------------------------------------------------------
int svtkParallelReader::ReadMetaData(svtkInformation* metadata)
{
  metadata->Set(svtkAlgorithm::CAN_HANDLE_PIECE_REQUEST(), 1);

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
int svtkParallelReader::ReadMesh(
  int piece, int npieces, int nghosts, int timestep, svtkDataObject* output)
{
  int nTimes = static_cast<int>(this->Internal->FileNames.size());
  if (timestep >= nTimes)
  {
    svtkErrorMacro(
      "Cannot read time step " << timestep << ". Only " << nTimes << " time steps are available.");
    return 0;
  }

  if (this->ReadMesh(this->Internal->FileNames[timestep], piece, npieces, nghosts, output))
  {
    this->CurrentFileIndex = timestep;
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
int svtkParallelReader::ReadPoints(
  int piece, int npieces, int nghosts, int timestep, svtkDataObject* output)
{
  int nTimes = static_cast<int>(this->Internal->FileNames.size());
  if (timestep >= nTimes)
  {
    svtkErrorMacro(
      "Cannot read time step " << timestep << ". Only " << nTimes << " time steps are available.");
    return 0;
  }

  return this->ReadPoints(this->Internal->FileNames[timestep], piece, npieces, nghosts, output);
}

//----------------------------------------------------------------------------
int svtkParallelReader::ReadArrays(
  int piece, int npieces, int nghosts, int timestep, svtkDataObject* output)
{
  int nTimes = static_cast<int>(this->Internal->FileNames.size());
  if (timestep >= nTimes)
  {
    svtkErrorMacro(
      "Cannot read time step " << timestep << ". Only " << nTimes << " time steps are available.");
    return 0;
  }

  return this->ReadArrays(this->Internal->FileNames[timestep], piece, npieces, nghosts, output);
}

//----------------------------------------------------------------------------
double svtkParallelReader::GetTimeValue(const std::string&)
{
  return svtkMath::Nan();
}
