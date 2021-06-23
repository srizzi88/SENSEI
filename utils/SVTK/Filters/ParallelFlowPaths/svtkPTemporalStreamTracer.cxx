/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkTemporalStreamTracer.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPTemporalStreamTracer.h"
#include "svtkTemporalStreamTracer.h"

#include "svtkAbstractParticleWriter.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCharArray.h"
#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkDataSetAttributes.h"
#include "svtkDoubleArray.h"
#include "svtkExecutive.h"
#include "svtkFloatArray.h"
#include "svtkGenericCell.h"
#include "svtkIdList.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkMPIController.h"
#include "svtkMath.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkOutputWindow.h"
#include "svtkPointData.h"
#include "svtkPointSet.h"
#include "svtkPolyData.h"
#include "svtkPolyLine.h"
#include "svtkRungeKutta2.h"
#include "svtkRungeKutta4.h"
#include "svtkRungeKutta45.h"
#include "svtkSmartPointer.h"
#include "svtkTemporalInterpolatedVelocityField.h"
#include "svtkToolkits.h"
#include <cassert>

using namespace svtkTemporalStreamTracerNamespace;

//---------------------------------------------------------------------------
svtkStandardNewMacro(svtkPTemporalStreamTracer);
svtkCxxSetObjectMacro(svtkPTemporalStreamTracer, Controller, svtkMultiProcessController);
//---------------------------------------------------------------------------
svtkPTemporalStreamTracer::svtkPTemporalStreamTracer()
{
  this->Controller = nullptr;
  this->SetController(svtkMultiProcessController::GetGlobalController());
  SVTK_LEGACY_BODY(svtkPTemporalStreamTracer::svtkPTemporalStreamTracer, "SVTK 9.0");
}
//---------------------------------------------------------------------------
svtkPTemporalStreamTracer::~svtkPTemporalStreamTracer()
{
  this->SetController(nullptr);
  this->SetParticleWriter(nullptr);
}
//---------------------------------------------------------------------------
void svtkPTemporalStreamTracer::AssignSeedsToProcessors(svtkDataSet* source, int sourceID, int ptId,
  ParticleVector& LocalSeedPoints, int& LocalAssignedCount)
{
  if (!this->Controller)
  {
    return Superclass::AssignSeedsToProcessors(
      source, sourceID, ptId, LocalSeedPoints, LocalAssignedCount);
  }

  ParticleVector candidates;
  //
  // take points from the source object and create a particle list
  //
  int numSeeds = source->GetNumberOfPoints();
#ifndef NDEBUG
  int numTested = numSeeds;
#endif
  candidates.resize(numSeeds);
  //
  for (int i = 0; i < numSeeds; i++)
  {
    ParticleInformation& info = candidates[i];
    memcpy(&(info.CurrentPosition.x[0]), source->GetPoint(i), sizeof(double) * 3);
    info.CurrentPosition.x[3] = this->CurrentTimeSteps[0];
    info.LocationState = 0;
    info.CachedCellId[0] = -1;
    info.CachedCellId[1] = -1;
    info.CachedDataSetId[0] = 0;
    info.CachedDataSetId[1] = 0;
    info.SourceID = sourceID;
    info.InjectedPointId = i + ptId;
    info.InjectedStepId = this->ReinjectionCounter;
    info.TimeStepAge = 0;
    info.UniqueParticleId = -1;
    info.rotation = 0.0;
    info.angularVel = 0.0;
    info.time = 0.0;
    info.age = 0.0;
    info.speed = 0.0;
    info.ErrorCode = 0;
  }
  //
  // Gather all Seeds to all processors for classification
  //
  // TODO : can we just use the same array here for send and receive
  ParticleVector allCandidates;
  if (this->UpdateNumPieces > 1)
  {
    // Gather all seed particles to all processes
    this->TransmitReceiveParticles(candidates, allCandidates, false);
#ifndef NDEBUG
    numTested = static_cast<int>(allCandidates.size());
#endif
    svtkDebugMacro(<< "Local Particles " << numSeeds << " TransmitReceive Total " << numTested);
    // Test to see which ones belong to us
    this->TestParticles(allCandidates, LocalSeedPoints, LocalAssignedCount);
  }
  else
  {
#ifndef NDEBUG
    numTested = static_cast<int>(candidates.size());
#endif
    this->TestParticles(candidates, LocalSeedPoints, LocalAssignedCount);
  }
  int TotalAssigned = 0;
  this->Controller->Reduce(&LocalAssignedCount, &TotalAssigned, 1, svtkCommunicator::SUM_OP, 0);

  // Assign unique identifiers taking into account uneven distribution
  // across processes and seeds which were rejected
  this->AssignUniqueIds(LocalSeedPoints);
  //
  svtkDebugMacro(<< "Tested " << numTested << " LocallyAssigned " << LocalAssignedCount);
  if (this->UpdatePieceId == 0)
  {
    svtkDebugMacro(<< "Total Assigned to all processes " << TotalAssigned);
  }
}
//---------------------------------------------------------------------------
void svtkPTemporalStreamTracer::AssignUniqueIds(
  svtkTemporalStreamTracerNamespace::ParticleVector& LocalSeedPoints)
{
  if (!this->Controller)
  {
    return Superclass::AssignUniqueIds(LocalSeedPoints);
  }

  svtkIdType ParticleCountOffset = 0;
  svtkIdType numParticles = static_cast<svtkIdType>(LocalSeedPoints.size());
  if (this->UpdateNumPieces > 1)
  {
    svtkMPICommunicator* com = svtkMPICommunicator::SafeDownCast(this->Controller->GetCommunicator());
    if (com == 0)
    {
      svtkErrorMacro("MPICommunicator needed for this operation.");
      return;
    }
    // everyone starts with the master index
    com->Broadcast(&this->UniqueIdCounter, 1, 0);
    //    svtkErrorMacro("UniqueIdCounter " << this->UniqueIdCounter);
    // setup arrays used by the AllGather call.
    std::vector<svtkIdType> recvNumParticles(this->UpdateNumPieces, 0);
    // Broadcast and receive count to/from all other processes.
    com->AllGather(&numParticles, &recvNumParticles[0], 1);
    // Each process is allocating a certain number.
    // start our indices from sum[0,this->UpdatePieceId](numparticles)
    for (int i = 0; i < this->UpdatePieceId; ++i)
    {
      ParticleCountOffset += recvNumParticles[i];
    }
    for (svtkIdType i = 0; i < numParticles; i++)
    {
      LocalSeedPoints[i].UniqueParticleId = this->UniqueIdCounter + ParticleCountOffset + i;
    }
    for (int i = 0; i < this->UpdateNumPieces; ++i)
    {
      this->UniqueIdCounter += recvNumParticles[i];
    }
  }
  else
  {
    for (svtkIdType i = 0; i < numParticles; i++)
    {
      LocalSeedPoints[i].UniqueParticleId = this->UniqueIdCounter + ParticleCountOffset + i;
    }
    this->UniqueIdCounter += numParticles;
  }
}
//---------------------------------------------------------------------------
void svtkPTemporalStreamTracer::TransmitReceiveParticles(
  ParticleVector& sending, ParticleVector& received, bool removeself)
{
  svtkMPICommunicator* com = svtkMPICommunicator::SafeDownCast(this->Controller->GetCommunicator());
  if (com == 0)
  {
    svtkErrorMacro("MPICommunicator needed for this operation.");
    return;
  }
  //
  // We must allocate buffers for all processor particles
  //
  svtkIdType OurParticles = static_cast<svtkIdType>(sending.size());
  svtkIdType TotalParticles = 0;
  // setup arrays used by the AllGatherV call.
  std::vector<svtkIdType> recvLengths(this->UpdateNumPieces, 0);
  std::vector<svtkIdType> recvOffsets(this->UpdateNumPieces, 0);
  // Broadcast and receive size to/from all other processes.
  com->AllGather(&OurParticles, &recvLengths[0], 1);
  // Compute the displacements.
  const svtkIdType TypeSize = sizeof(ParticleInformation);
  for (int i = 0; i < this->UpdateNumPieces; ++i)
  {
    //  << i << ": " << recvLengths[i] << "   ";
    recvOffsets[i] = TotalParticles * TypeSize;
    TotalParticles += recvLengths[i];
    recvLengths[i] *= TypeSize;
  }
  //  << '\n';
  // Allocate the space for all particles
  received.resize(TotalParticles);
  if (TotalParticles == 0)
    return;
  // Gather the data from all procs.
  char* sendbuf = (char*)((sending.size() > 0) ? &(sending[0]) : nullptr);
  char* recvbuf = (char*)(&(received[0]));
  com->AllGatherV(sendbuf, recvbuf, OurParticles * TypeSize, &recvLengths[0], &recvOffsets[0]);
  // Now all particles from all processors are in one big array
  // remove any from ourself that we have already tested
  if (removeself)
  {
    std::vector<ParticleInformation>::iterator first =
      received.begin() + recvOffsets[this->UpdatePieceId] / TypeSize;
    std::vector<ParticleInformation>::iterator last =
      first + recvLengths[this->UpdatePieceId] / TypeSize;
    received.erase(first, last);
  }
}
//---------------------------------------------------------------------------
int svtkPTemporalStreamTracer::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  int rvalue = this->Superclass::RequestData(request, inputVector, outputVector);

  if (this->Controller)
  {
    this->Controller->Barrier();
  }

  return rvalue;
}
//---------------------------------------------------------------------------
void svtkPTemporalStreamTracer::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Controller: " << this->Controller << endl;
}
//---------------------------------------------------------------------------
void svtkPTemporalStreamTracer::AddParticleToMPISendList(ParticleInformation& info)
{
  double eps = (this->CurrentTimeSteps[1] - this->CurrentTimeSteps[0]) / 100;
  if (info.CurrentPosition.x[3] < (this->CurrentTimeSteps[0] - eps) ||
    info.CurrentPosition.x[3] > (this->CurrentTimeSteps[1] + eps))
  {
    svtkDebugMacro(<< "Unexpected time value in MPISendList - expected ("
                  << this->CurrentTimeSteps[0] << "-" << this->CurrentTimeSteps[1] << ") got "
                  << info.CurrentPosition.x[3]);
  }
  if (this->MPISendList.capacity() < (this->MPISendList.size() + 1))
  {
    this->MPISendList.reserve(static_cast<int>(this->MPISendList.size() * 1.5));
  }
  this->MPISendList.push_back(info);
}
