/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLagrangianParticle.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkLagrangianParticle.h"

#include "svtkDataArray.h"
#include "svtkMath.h"
#include "svtkPointData.h"
#include "svtkSetGet.h"

//---------------------------------------------------------------------------
svtkLagrangianParticle::svtkLagrangianParticle(int numberOfVariables, svtkIdType seedId,
  svtkIdType particleId, svtkIdType seedArrayTupleIndex, double integrationTime,
  svtkPointData* seedData, int weightsSize, int numberOfTrackedUserData)
  : Id(particleId)
  , ParentId(-1)
  , SeedId(seedId)
  , NumberOfSteps(0)
  , SeedArrayTupleIndex(seedArrayTupleIndex)
  , SeedData(seedData)
  , StepTime(0)
  , IntegrationTime(integrationTime)
  , PrevIntegrationTime(0)
  , Termination(svtkLagrangianParticle::PARTICLE_TERMINATION_NOT_TERMINATED)
  , Interaction(svtkLagrangianParticle::SURFACE_INTERACTION_NO_INTERACTION)
  , UserFlag(0)
  , NumberOfVariables(numberOfVariables)
  , PInsertPreviousPosition(false)
  , PManualShift(false)
{
  // Initialize equation variables and associated pointers
  this->PrevEquationVariables.resize(this->NumberOfVariables, 0);
  this->PrevVelocity = this->PrevEquationVariables.data() + 3;
  this->PrevUserVariables = this->PrevEquationVariables.data() + 6;

  this->EquationVariables.resize(this->NumberOfVariables, 0);
  this->Velocity = this->EquationVariables.data() + 3;
  this->UserVariables = this->EquationVariables.data() + 6;

  this->NextEquationVariables.resize(this->NumberOfVariables, 0);
  this->NextVelocity = this->NextEquationVariables.data() + 3;
  this->NextUserVariables = this->NextEquationVariables.data() + 6;

  // Initialize cell cache
  this->LastCellId = -1;
  this->LastDataSet = nullptr;
  this->LastLocator = nullptr;
  this->WeightsSize = weightsSize;
  this->LastWeights.resize(this->WeightsSize);

  // Initialize surface cell cache
  this->LastSurfaceCellId = -1;
  this->LastSurfaceDataSet = nullptr;

  // Initialize tracked user data
  this->PrevTrackedUserData.resize(numberOfTrackedUserData, 0);
  this->TrackedUserData.resize(numberOfTrackedUserData, 0);
  this->NextTrackedUserData.resize(numberOfTrackedUserData, 0);
}

//---------------------------------------------------------------------------
// Default destructor in implementation in order to be able to use
// svtkNew in header
svtkLagrangianParticle::~svtkLagrangianParticle() = default;

//---------------------------------------------------------------------------
svtkLagrangianParticle* svtkLagrangianParticle::NewInstance(int numberOfVariables, svtkIdType seedId,
  svtkIdType particleId, svtkIdType seedArrayTupleIndex, double integrationTime,
  svtkPointData* seedData, int weightsSize, int numberOfTrackedUserData, svtkIdType numberOfSteps,
  double previousIntegrationTime)
{
  svtkLagrangianParticle* particle = new svtkLagrangianParticle(numberOfVariables, seedId, particleId,
    seedArrayTupleIndex, integrationTime, seedData, weightsSize, numberOfTrackedUserData);
  particle->NumberOfSteps = numberOfSteps;
  particle->PrevIntegrationTime = previousIntegrationTime;
  return particle;
}

//---------------------------------------------------------------------------
svtkLagrangianParticle* svtkLagrangianParticle::NewParticle(svtkIdType particleId)
{
  // Create particle and copy members
  svtkLagrangianParticle* particle =
    svtkLagrangianParticle::NewInstance(this->GetNumberOfVariables(), this->GetSeedId(), particleId,
      this->SeedArrayTupleIndex, this->IntegrationTime + this->StepTime, this->SeedData,
      this->WeightsSize, static_cast<int>(this->TrackedUserData.size()));
  particle->ParentId = this->GetId();
  particle->NumberOfSteps = this->GetNumberOfSteps() + 1;

  // Copy Variables
  std::copy(this->EquationVariables.begin(), this->EquationVariables.end(),
    particle->PrevEquationVariables.begin());
  std::copy(this->NextEquationVariables.begin(), this->NextEquationVariables.end(),
    particle->EquationVariables.begin());
  std::fill(particle->NextEquationVariables.begin(), particle->NextEquationVariables.end(), 0);

  // Copy UserData
  std::copy(this->TrackedUserData.begin(), this->TrackedUserData.end(),
    particle->PrevTrackedUserData.begin());
  std::copy(this->NextTrackedUserData.begin(), this->NextTrackedUserData.end(),
    particle->TrackedUserData.begin());
  std::fill(particle->NextTrackedUserData.begin(), particle->NextTrackedUserData.end(), 0);

  // Copy thread-specific data as well
  particle->ThreadedData = this->ThreadedData;

  return particle;
}

//---------------------------------------------------------------------------
svtkLagrangianParticle* svtkLagrangianParticle::CloneParticle()
{
  svtkLagrangianParticle* clone = svtkLagrangianParticle::NewInstance(this->GetNumberOfVariables(),
    this->GetSeedId(), this->GetId(), this->SeedArrayTupleIndex, this->IntegrationTime,
    this->GetSeedData(), this->WeightsSize, static_cast<int>(this->TrackedUserData.size()));
  clone->Id = this->Id;
  clone->ParentId = this->ParentId;
  clone->NumberOfSteps = this->NumberOfSteps;

  std::copy(this->PrevEquationVariables.begin(), this->PrevEquationVariables.end(),
    clone->PrevEquationVariables.begin());
  std::copy(this->EquationVariables.begin(), this->EquationVariables.end(),
    clone->EquationVariables.begin());
  std::copy(this->NextEquationVariables.begin(), this->NextEquationVariables.end(),
    clone->NextEquationVariables.begin());
  std::copy(this->PrevTrackedUserData.begin(), this->PrevTrackedUserData.end(),
    clone->PrevTrackedUserData.begin());
  std::copy(
    this->TrackedUserData.begin(), this->TrackedUserData.end(), clone->TrackedUserData.begin());
  std::copy(this->NextTrackedUserData.begin(), this->NextTrackedUserData.end(),
    clone->NextTrackedUserData.begin());
  clone->StepTime = this->StepTime;

  // Copy thread-specific data as well
  clone->ThreadedData = this->ThreadedData;

  return clone;
}

//---------------------------------------------------------------------------
double* svtkLagrangianParticle::GetLastWeights()
{
  return this->LastWeights.data();
}

//---------------------------------------------------------------------------
svtkIdType svtkLagrangianParticle::GetLastCellId()
{
  return this->LastCellId;
}

//---------------------------------------------------------------------------
double* svtkLagrangianParticle::GetLastCellPosition()
{
  return this->LastCellPosition;
}

//---------------------------------------------------------------------------
svtkDataSet* svtkLagrangianParticle::GetLastDataSet()
{
  return this->LastDataSet;
}

//---------------------------------------------------------------------------
svtkAbstractCellLocator* svtkLagrangianParticle::GetLastLocator()
{
  return this->LastLocator;
}

//---------------------------------------------------------------------------
void svtkLagrangianParticle::SetLastCell(
  svtkAbstractCellLocator* locator, svtkDataSet* dataset, svtkIdType cellId, double cellPosition[3])
{
  this->LastLocator = locator;
  this->LastDataSet = dataset;
  this->LastCellId = cellId;
  std::copy(cellPosition, cellPosition + 3, this->LastCellPosition);
}

//---------------------------------------------------------------------------
void svtkLagrangianParticle::SetLastSurfaceCell(svtkDataSet* dataset, svtkIdType cellId)
{
  this->LastSurfaceDataSet = dataset;
  this->LastSurfaceCellId = cellId;
}

//---------------------------------------------------------------------------
svtkIdType svtkLagrangianParticle::GetLastSurfaceCellId()
{
  return this->LastSurfaceCellId;
}

//---------------------------------------------------------------------------
svtkDataSet* svtkLagrangianParticle::GetLastSurfaceDataSet()
{
  return this->LastSurfaceDataSet;
}

//---------------------------------------------------------------------------
svtkIdType svtkLagrangianParticle::GetId()
{
  return this->Id;
}

//---------------------------------------------------------------------------
void svtkLagrangianParticle::SetParentId(svtkIdType parentId)
{
  this->ParentId = parentId;
}

//---------------------------------------------------------------------------
svtkIdType svtkLagrangianParticle::GetParentId()
{
  return this->ParentId;
}

//---------------------------------------------------------------------------
svtkIdType svtkLagrangianParticle::GetSeedId()
{
  return this->SeedId;
}

//---------------------------------------------------------------------------
svtkIdType svtkLagrangianParticle::GetNumberOfSteps()
{
  return this->NumberOfSteps;
}

//---------------------------------------------------------------------------
int svtkLagrangianParticle::GetNumberOfVariables()
{
  return this->NumberOfVariables;
}

//---------------------------------------------------------------------------
int svtkLagrangianParticle::GetNumberOfUserVariables()
{
  return this->NumberOfVariables - 7;
}

//---------------------------------------------------------------------------
svtkPointData* svtkLagrangianParticle::GetSeedData()
{
  return this->SeedData;
}

//---------------------------------------------------------------------------
svtkIdType svtkLagrangianParticle::GetSeedArrayTupleIndex() const
{
  return this->SeedArrayTupleIndex;
}

//---------------------------------------------------------------------------
double& svtkLagrangianParticle::GetStepTimeRef()
{
  return this->StepTime;
}

//---------------------------------------------------------------------------
double svtkLagrangianParticle::GetIntegrationTime()
{
  return this->IntegrationTime;
}

//---------------------------------------------------------------------------
double svtkLagrangianParticle::GetPrevIntegrationTime()
{
  return this->PrevIntegrationTime;
}

//---------------------------------------------------------------------------
void svtkLagrangianParticle::SetIntegrationTime(double time)
{
  this->IntegrationTime = time;
}

//---------------------------------------------------------------------------
void svtkLagrangianParticle::SetTermination(int termination)
{
  this->Termination = termination;
}

//---------------------------------------------------------------------------
int svtkLagrangianParticle::GetTermination()
{
  return this->Termination;
}

//---------------------------------------------------------------------------
void svtkLagrangianParticle::SetUserFlag(int flag)
{
  this->UserFlag = flag;
}

//---------------------------------------------------------------------------
int svtkLagrangianParticle::GetUserFlag()
{
  return this->UserFlag;
}

//---------------------------------------------------------------------------
void svtkLagrangianParticle::SetInteraction(int interaction)
{
  this->Interaction = interaction;
}

//---------------------------------------------------------------------------
int svtkLagrangianParticle::GetInteraction()
{
  return this->Interaction;
}

//---------------------------------------------------------------------------
void svtkLagrangianParticle::SetPInsertPreviousPosition(bool val)
{
  this->PInsertPreviousPosition = val;
}

//---------------------------------------------------------------------------
bool svtkLagrangianParticle::GetPInsertPreviousPosition()
{
  return this->PInsertPreviousPosition;
}

//---------------------------------------------------------------------------
void svtkLagrangianParticle::SetPManualShift(bool val)
{
  this->PManualShift = val;
}

//---------------------------------------------------------------------------
bool svtkLagrangianParticle::GetPManualShift()
{
  return this->PManualShift;
}

//---------------------------------------------------------------------------
double svtkLagrangianParticle::GetPositionVectorMagnitude()
{
  double* current = this->GetPosition();
  double* next = this->GetNextPosition();
  double vector[3];
  svtkMath::Subtract(next, current, vector);
  return svtkMath::Norm(vector, 3);
}

//---------------------------------------------------------------------------
void svtkLagrangianParticle::MoveToNextPosition()
{
  std::copy(this->EquationVariables.begin(), this->EquationVariables.end(),
    this->PrevEquationVariables.begin());
  std::copy(this->NextEquationVariables.begin(), this->NextEquationVariables.end(),
    this->EquationVariables.begin());
  std::fill(this->NextEquationVariables.begin(), this->NextEquationVariables.end(), 0);
  std::copy(
    this->TrackedUserData.begin(), this->TrackedUserData.end(), this->PrevTrackedUserData.begin());
  std::copy(this->NextTrackedUserData.begin(), this->NextTrackedUserData.end(),
    this->TrackedUserData.begin());
  std::fill(this->NextTrackedUserData.begin(), this->NextTrackedUserData.end(), 0);

  this->NumberOfSteps++;
  this->PrevIntegrationTime = this->IntegrationTime;
  this->IntegrationTime += this->StepTime;
}

//---------------------------------------------------------------------------
void svtkLagrangianParticle::PrintSelf(ostream& os, svtkIndent indent)
{
  os << indent << "Id: " << this->Id << std::endl;
  os << indent << "LastCellId: " << this->LastCellId << std::endl;
  os << indent << "LastDataSet: " << this->LastDataSet << std::endl;
  os << indent << "LastLocator: " << this->LastLocator << std::endl;
  os << indent << "NumberOfSteps: " << this->NumberOfSteps << std::endl;
  os << indent << "NumberOfVariables: " << this->NumberOfVariables << std::endl;
  os << indent << "ParentId: " << this->ParentId << std::endl;
  os << indent << "SeedData: " << this->SeedData << std::endl;
  os << indent << "SeedArrayTupleIndex: " << this->SeedArrayTupleIndex << std::endl;
  os << indent << "SeedId: " << this->SeedId << std::endl;
  os << indent << "StepTime: " << this->StepTime << std::endl;
  os << indent << "IntegrationTime: " << this->IntegrationTime << std::endl;
  os << indent << "Termination: " << this->Termination << std::endl;
  os << indent << "UserFlag: " << this->UserFlag << std::endl;
  os << indent << "Interaction: " << this->Interaction << std::endl;

  os << indent << "PrevEquationVariables:";
  for (auto var : this->PrevEquationVariables)
  {
    os << indent << " " << var;
  }
  os << std::endl;

  os << indent << "EquationVariables:";
  for (auto var : this->EquationVariables)
  {
    os << indent << " " << var;
  }
  os << std::endl;

  os << indent << "NextEquationVariables:";
  for (auto var : this->NextEquationVariables)
  {
    os << indent << " " << var;
  }
  os << std::endl;

  os << indent << "PrevTrackedUserData:";
  for (auto var : this->PrevTrackedUserData)
  {
    os << indent << " " << var;
  }
  os << std::endl;

  os << indent << "TrackedUserData:";
  for (auto var : this->TrackedUserData)
  {
    os << indent << " " << var;
  }
  os << std::endl;

  os << indent << "NextTrackedUserData:";
  for (auto var : this->NextTrackedUserData)
  {
    os << indent << " " << var;
  }
  os << std::endl;

  os << indent << "ThreadedData: " << this->ThreadedData << std::endl;
}
