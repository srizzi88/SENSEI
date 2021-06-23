/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPParticlePathFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPParticlePathFilter.h"

#include "svtkCellArray.h"
#include "svtkCharArray.h"
#include "svtkDataArray.h"
#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include <cassert>

svtkStandardNewMacro(svtkPParticlePathFilter);

svtkPParticlePathFilter::svtkPParticlePathFilter()
{
  this->It.Initialize(this);
  this->SimulationTime = nullptr;
  this->SimulationTimeStep = nullptr;
}

svtkPParticlePathFilter::~svtkPParticlePathFilter()
{
  if (this->SimulationTime)
  {
    this->SimulationTime->Delete();
    this->SimulationTime = nullptr;
  }
  if (this->SimulationTimeStep)
  {
    this->SimulationTimeStep->Delete();
    this->SimulationTimeStep = nullptr;
  }
}

void svtkPParticlePathFilter::ResetCache()
{
  this->It.Reset();
}

void svtkPParticlePathFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}

int svtkPParticlePathFilter::OutputParticles(svtkPolyData* particles)
{
  svtkNew<svtkPolyData> tailPoly;
  tailPoly->SetPoints(svtkSmartPointer<svtkPoints>::New());

  svtkPointData* tailPD = tailPoly->GetPointData();
  assert(tailPD);
  tailPD->CopyAllocate(particles->GetPointData());

  for (unsigned int i = 0; i < this->Tail.size(); i++)
  {
    svtkParticleTracerBaseNamespace::ParticleInformation& info(this->Tail[i].Previous);
    svtkPointData* pd = this->Tail[i].PreviousPD;

    const double* coord = info.CurrentPosition.x;
    svtkIdType tempId = tailPoly->GetPoints()->InsertNextPoint(coord);
    for (int j = 0; j < pd->GetNumberOfArrays(); j++)
    {
      svtkDataArray* arrFrom = pd->GetArray(j);
      svtkDataArray* arrTo = tailPD->GetArray(arrFrom->GetName());
      assert(arrTo);
      assert(arrTo->GetNumberOfComponents() == arrFrom->GetNumberOfComponents());
      arrTo->InsertTuple(tempId, arrFrom->GetTuple(0));
    }

    this->GetParticleIds(tailPD)->InsertValue(tempId, info.UniqueParticleId);
    this->GetParticleSourceIds(tailPD)->InsertValue(tempId, info.SourceID);
    this->GetInjectedPointIds(tailPD)->InsertValue(tempId, info.InjectedPointId);
    this->GetInjectedStepIds(tailPD)->InsertValue(tempId, info.InjectedStepId);
    this->GetErrorCodeArr(tailPD)->InsertValue(tempId, info.ErrorCode);
    this->GetParticleAge(tailPD)->InsertValue(tempId, info.age);

    svtkArrayDownCast<svtkDoubleArray>(tailPD->GetArray("SimulationTime"))
      ->InsertValue(tempId, info.SimulationTime);
    svtkArrayDownCast<svtkIntArray>(tailPD->GetArray("SimulationTimeStep"))
      ->InsertValue(tempId, info.InjectedStepId + info.TimeStepAge);

    if (this->GetComputeVorticity())
    {
      //      this->GetParticleVorticity(tailPD)->InsertNextTuple(info.vorticity); missing in
      //      implementation
      this->GetParticleAngularVel(tailPD)->InsertValue(tempId, info.angularVel);
      this->GetParticleRotation(tailPD)->InsertValue(tempId, info.rotation);
    }
  }

  this->It.OutputParticles(tailPoly);

  return this->It.OutputParticles(particles); // we've already cleared cache in the first call
}

void svtkPParticlePathFilter::InitializeExtraPointDataArrays(svtkPointData* outputPD)
{
  if (this->SimulationTime == nullptr)
  {
    this->SimulationTime = svtkDoubleArray::New();
    this->SimulationTime->SetName("SimulationTime");
  }
  if (outputPD->GetArray("SimulationTime"))
  {
    outputPD->RemoveArray("SimulationTime");
  }
  this->SimulationTime->SetNumberOfTuples(0);
  outputPD->AddArray(this->SimulationTime);

  if (this->SimulationTimeStep == nullptr)
  {
    this->SimulationTimeStep = svtkIntArray::New();
    this->SimulationTimeStep->SetName("SimulationTimeStep");
  }
  if (outputPD->GetArray("SimulationTimeStep"))
  {
    outputPD->RemoveArray("SimulationTimeStep");
  }
  this->SimulationTimeStep->SetNumberOfTuples(0);
  outputPD->AddArray(this->SimulationTimeStep);
}

void svtkPParticlePathFilter::AppendToExtraPointDataArrays(
  svtkParticleTracerBaseNamespace::ParticleInformation& info)
{
  this->SimulationTime->InsertNextValue(info.SimulationTime);
  this->SimulationTimeStep->InsertNextValue(info.InjectedStepId + info.TimeStepAge);
}

void svtkPParticlePathFilter::Finalize()
{
  this->It.Finalize();
}

int svtkPParticlePathFilter::RequestInformation(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // The output data of this filter has no time associated with it.  It is the
  // result of computations that happen over all time.
  outInfo->Remove(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
  outInfo->Remove(svtkStreamingDemandDrivenPipeline::TIME_RANGE());

  return this->Superclass::RequestInformation(request, inputVector, outputVector);
}
