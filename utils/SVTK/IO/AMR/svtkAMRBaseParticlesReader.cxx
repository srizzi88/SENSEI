/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkAMRBaseParticlesReader.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "svtkAMRBaseParticlesReader.h"
#include "svtkCallbackCommand.h"
#include "svtkDataArraySelection.h"
#include "svtkIndent.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiProcessController.h"
#include "svtkPolyData.h"

#include <cassert>

svtkAMRBaseParticlesReader::svtkAMRBaseParticlesReader() = default;

//------------------------------------------------------------------------------
svtkAMRBaseParticlesReader::~svtkAMRBaseParticlesReader()
{
  this->ParticleDataArraySelection->RemoveObserver(this->SelectionObserver);
  this->SelectionObserver->Delete();
  this->ParticleDataArraySelection->Delete();
}

//------------------------------------------------------------------------------
void svtkAMRBaseParticlesReader::PrintSelf(std::ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
int svtkAMRBaseParticlesReader::FillOutputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkMultiBlockDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int svtkAMRBaseParticlesReader::GetNumberOfParticleArrays()
{
  assert(
    "pre: ParticleDataArraySelection is nullptr!" && (this->ParticleDataArraySelection != nullptr));
  return this->ParticleDataArraySelection->GetNumberOfArrays();
}

//------------------------------------------------------------------------------
const char* svtkAMRBaseParticlesReader::GetParticleArrayName(int index)
{
  assert("pre: array inded out-of-bounds!" && (index >= 0) &&
    (index < this->ParticleDataArraySelection->GetNumberOfArrays()));

  return this->ParticleDataArraySelection->GetArrayName(index);
}

//------------------------------------------------------------------------------
int svtkAMRBaseParticlesReader::GetParticleArrayStatus(const char* name)
{
  assert("pre: array name is nullptr" && (name != nullptr));
  return this->ParticleDataArraySelection->ArrayIsEnabled(name);
}

//------------------------------------------------------------------------------
void svtkAMRBaseParticlesReader::SetParticleArrayStatus(const char* name, int status)
{

  if (status)
  {
    this->ParticleDataArraySelection->EnableArray(name);
  }
  else
  {
    this->ParticleDataArraySelection->DisableArray(name);
  }
}

//------------------------------------------------------------------------------
void svtkAMRBaseParticlesReader::SelectionModifiedCallback(
  svtkObject*, unsigned long, void* clientdata, void*)
{
  static_cast<svtkAMRBaseParticlesReader*>(clientdata)->Modified();
}

//------------------------------------------------------------------------------
void svtkAMRBaseParticlesReader::Initialize()
{
  this->SetNumberOfInputPorts(0);
  this->Frequency = 1;
  this->FilterLocation = 0;
  this->NumberOfBlocks = 0;
  this->Initialized = false;
  this->InitialRequest = true;
  this->FileName = nullptr;
  this->Controller = svtkMultiProcessController::GetGlobalController();

  for (int i = 0; i < 3; ++i)
  {
    this->MinLocation[i] = this->MaxLocation[i] = 0.0;
  }

  this->ParticleDataArraySelection = svtkDataArraySelection::New();
  this->SelectionObserver = svtkCallbackCommand::New();
  this->SelectionObserver->SetCallback(&svtkAMRBaseParticlesReader::SelectionModifiedCallback);
  this->SelectionObserver->SetClientData(this);
  this->ParticleDataArraySelection->AddObserver(svtkCommand::ModifiedEvent, this->SelectionObserver);
}

//------------------------------------------------------------------------------
void svtkAMRBaseParticlesReader::InitializeParticleDataSelections()
{
  if (!this->InitialRequest)
  {
    return;
  }

  this->ParticleDataArraySelection->DisableAllArrays();
  this->InitialRequest = false;
}

//------------------------------------------------------------------------------
void svtkAMRBaseParticlesReader::SetFileName(const char* fileName)
{

  if (this->FileName != nullptr)
  {
    if (strcmp(this->FileName, fileName) != 0)
    {
      this->Initialized = false;
      delete[] this->FileName;
      this->FileName = nullptr;
    }
    else
    {
      return;
    }
  }

  this->FileName = new char[strlen(fileName) + 1];
  strcpy(this->FileName, fileName);

  this->Modified();
}

//------------------------------------------------------------------------------
bool svtkAMRBaseParticlesReader::IsParallel()
{
  if (this->Controller != nullptr && this->Controller->GetNumberOfProcesses() > 1)
  {
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
bool svtkAMRBaseParticlesReader::IsBlockMine(const int blkIdx)
{
  if (!this->IsParallel())
  {
    return true;
  }

  int myRank = this->Controller->GetLocalProcessId();
  if (myRank == this->GetBlockProcessId(blkIdx))
  {
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
int svtkAMRBaseParticlesReader::GetBlockProcessId(const int blkIdx)
{
  if (!this->IsParallel())
  {
    return 0;
  }

  int N = this->Controller->GetNumberOfProcesses();
  return (blkIdx % N);
}

//------------------------------------------------------------------------------
bool svtkAMRBaseParticlesReader::CheckLocation(const double x, const double y, const double z)
{
  if (!this->FilterLocation)
  {
    return true;
  }

  double coords[3];
  coords[0] = x;
  coords[1] = y;
  coords[2] = z;

  for (int i = 0; i < 3; ++i)
  {
    if (this->MinLocation[i] > coords[i] || coords[i] > this->MaxLocation[i])
    {
      return false;
    }
  } // END for all dimensions

  return true;
}

//------------------------------------------------------------------------------
int svtkAMRBaseParticlesReader::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  // STEP 0: Get the output object
  svtkInformation* outInf = outputVector->GetInformationObject(0);
  svtkMultiBlockDataSet* mbds =
    svtkMultiBlockDataSet::SafeDownCast(outInf->Get(svtkDataObject::DATA_OBJECT()));
  assert("pre: output multi-block dataset object is nullptr" && (mbds != nullptr));

  // STEP 1: Read Meta-Data
  this->ReadMetaData();

  // STEP 2: Read blocks
  mbds->SetNumberOfBlocks(this->NumberOfBlocks);
  unsigned int blkidx = 0;
  for (; blkidx < static_cast<unsigned int>(this->NumberOfBlocks); ++blkidx)
  {
    if (this->IsBlockMine(blkidx))
    {
      svtkPolyData* particles = this->ReadParticles(blkidx);
      assert("particles dataset should not be nullptr!" && (particles != nullptr));

      mbds->SetBlock(blkidx, particles);
      particles->Delete();
    }
    else
    {
      mbds->SetBlock(blkidx, nullptr);
    }
  } // END for all blocks

  // STEP 3: Synchronize
  if (this->IsParallel())
  {
    this->Controller->Barrier();
  }

  return 1;
}
