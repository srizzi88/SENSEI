/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkParticlePathFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkParticlePathFilter.h"

#include "svtkCell.h"
#include "svtkCellArray.h"
#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSetGet.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include <vector>

svtkObjectFactoryNewMacro(svtkParticlePathFilter);

void ParticlePathFilterInternal::Initialize(svtkParticleTracerBase* filter)
{
  this->Filter = filter;
  this->Filter->SetForceReinjectionEveryNSteps(0);
  this->Filter->SetIgnorePipelineTime(1);
  this->ClearCache = false;
}

void ParticlePathFilterInternal::Reset()
{
  this->Filter->svtkParticleTracerBase::ResetCache();
  this->Paths.clear();
}

int ParticlePathFilterInternal::OutputParticles(svtkPolyData* particles)
{
  if (!this->Filter->Output || this->ClearCache)
  {
    this->Filter->Output = svtkSmartPointer<svtkPolyData>::New();
    this->Filter->Output->SetPoints(svtkSmartPointer<svtkPoints>::New());
    this->Filter->Output->GetPointData()->CopyAllocate(particles->GetPointData());
  }
  if (this->ClearCache)
  { // clear cache no matter what
    this->Paths.clear();
  }

  svtkPoints* pts = particles->GetPoints();
  if (!pts || pts->GetNumberOfPoints() == 0)
  {
    return 0;
  }

  svtkPointData* outPd = this->Filter->Output->GetPointData();
  svtkPoints* outPoints = this->Filter->Output->GetPoints();

  // Get the input arrays
  svtkPointData* pd = particles->GetPointData();
  svtkIntArray* particleIds = svtkArrayDownCast<svtkIntArray>(pd->GetArray("ParticleId"));

  // Append the input arrays to the output arrays
  int begin = outPoints->GetNumberOfPoints();
  for (int i = 0; i < pts->GetNumberOfPoints(); i++)
  {
    outPoints->InsertNextPoint(pts->GetPoint(i));
  }
  svtkDataSetAttributes::FieldList ptList(1);
  ptList.InitializeFieldList(pd);
  for (int i = 0, j = begin; i < pts->GetNumberOfPoints(); i++, j++)
  {
    outPd->CopyData(ptList, pd, 0, i, j);
  }

  // Augment the paths
  for (svtkIdType i = 0; i < pts->GetNumberOfPoints(); i++)
  {
    int outId = i + begin;

    int pid = particleIds->GetValue(i);
    for (int j = static_cast<int>(this->Paths.size()); j <= pid; j++)
    {
      this->Paths.push_back(svtkSmartPointer<svtkIdList>::New());
    }

    svtkIdList* path = this->Paths[pid];

#ifdef DEBUG
    if (path->GetNumberOfIds() > 0)
    {
      svtkFloatArray* outParticleAge =
        svtkArrayDownCast<svtkFloatArray>(outPd->GetArray("ParticleAge"));
      if (outParticleAge->GetValue(outId) <
        outParticleAge->GetValue(path->GetId(path->GetNumberOfIds() - 1)))
      {
        svtkOStrStreamWrapper svtkmsg;
        svtkmsg << "ERROR: In " __FILE__ ", line " << __LINE__ << "\n"
               << "): "
               << " new particles have wrong ages"
               << "\n\n";
      }
    }
#endif
    path->InsertNextId(outId);
  }

  return 1;
}
void ParticlePathFilterInternal::Finalize()
{
  this->Filter->Output->SetLines(svtkSmartPointer<svtkCellArray>::New());
  svtkCellArray* outLines = this->Filter->Output->GetLines();
  if (!outLines)
  {
    svtkOStrStreamWrapper svtkmsg;
    svtkmsg << "ERROR: In " __FILE__ ", line " << __LINE__ << "\n"
           << "): "
           << " no lines in the output"
           << "\n\n";
    return;
  }
  // if we have a path that leaves a process and than comes back we need
  // to add that as separate cells. we use the simulation time step to check
  // on that assuming that the particle path filter is updated every time step.
  svtkIntArray* sourceSimulationTimeStepArray = svtkArrayDownCast<svtkIntArray>(
    this->Filter->Output->GetPointData()->GetArray("SimulationTimeStep"));
  svtkNew<svtkIdList> tmpIds;
  for (size_t i = 0; i < this->Paths.size(); i++)
  {
    if (this->Paths[i]->GetNumberOfIds() > 1)
    {
      svtkIdList* ids = this->Paths[i];
      int previousTimeStep = sourceSimulationTimeStepArray->GetTypedComponent(ids->GetId(0), 0);
      tmpIds->Reset();
      tmpIds->InsertNextId(ids->GetId(0));
      for (svtkIdType j = 1; j < ids->GetNumberOfIds(); j++)
      {
        int currentTimeStep = sourceSimulationTimeStepArray->GetTypedComponent(ids->GetId(j), 0);
        if (currentTimeStep != (previousTimeStep + 1))
        {
          if (tmpIds->GetNumberOfIds() > 1)
          {
            outLines->InsertNextCell(tmpIds);
          }
          tmpIds->Reset();
        }
        tmpIds->InsertNextId(ids->GetId(j));
        previousTimeStep = currentTimeStep;
      }
      if (tmpIds->GetNumberOfIds() > 1)
      {
        outLines->InsertNextCell(tmpIds);
      }
    }
  }
}

svtkParticlePathFilter::svtkParticlePathFilter()
{
  this->It.Initialize(this);
  this->SimulationTime = nullptr;
  this->SimulationTimeStep = nullptr;
}

svtkParticlePathFilter::~svtkParticlePathFilter()
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

void svtkParticlePathFilter::ResetCache()
{
  Superclass::ResetCache();
  this->It.Reset();
}

void svtkParticlePathFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}

int svtkParticlePathFilter::OutputParticles(svtkPolyData* particles)
{
  return this->It.OutputParticles(particles);
}

void svtkParticlePathFilter::InitializeExtraPointDataArrays(svtkPointData* outputPD)
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

void svtkParticlePathFilter::AppendToExtraPointDataArrays(
  svtkParticleTracerBaseNamespace::ParticleInformation& info)
{
  this->SimulationTime->InsertNextValue(info.SimulationTime);
  this->SimulationTimeStep->InsertNextValue(info.InjectedStepId + info.TimeStepAge);
}

void svtkParticlePathFilter::Finalize()
{
  this->It.Finalize();
}

int svtkParticlePathFilter::RequestInformation(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // The output data of this filter has no time associated with it.  It is the
  // result of computations that happen over all time.
  outInfo->Remove(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
  outInfo->Remove(svtkStreamingDemandDrivenPipeline::TIME_RANGE());

  return this->Superclass::RequestInformation(request, inputVector, outputVector);
}
