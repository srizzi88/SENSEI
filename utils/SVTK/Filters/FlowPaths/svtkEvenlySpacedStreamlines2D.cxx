/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkEvenlySpacedStreamlines2D.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkEvenlySpacedStreamlines2D.h"

#include "svtkAMRInterpolatedVelocityField.h"
#include "svtkAbstractInterpolatedVelocityField.h"
#include "svtkAppendPolyData.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCellLocatorInterpolatedVelocityField.h"
#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkCompositeDataSet.h"
#include "svtkDataSetAttributes.h"
#include "svtkDoubleArray.h"
#include "svtkExecutive.h"
#include "svtkGenericCell.h"
#include "svtkIdList.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkInterpolatedVelocityField.h"
#include "svtkMath.h"
#include "svtkMathUtilities.h"
#include "svtkModifiedBSPTree.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkOverlappingAMR.h"
#include "svtkPointData.h"
#include "svtkPointSet.h"
#include "svtkPolyData.h"
#include "svtkPolyDataCollection.h"
#include "svtkPolyLine.h"
#include "svtkRungeKutta2.h"
#include "svtkRungeKutta4.h"
#include "svtkRungeKutta45.h"
#include "svtkSmartPointer.h"
#include "svtkStreamTracer.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <iterator>
#include <vector>

svtkObjectFactoryNewMacro(svtkEvenlySpacedStreamlines2D);
svtkCxxSetObjectMacro(svtkEvenlySpacedStreamlines2D, Integrator, svtkInitialValueProblemSolver);
svtkCxxSetObjectMacro(
  svtkEvenlySpacedStreamlines2D, InterpolatorPrototype, svtkAbstractInterpolatedVelocityField);

svtkEvenlySpacedStreamlines2D::svtkEvenlySpacedStreamlines2D()
{
  this->Integrator = svtkRungeKutta2::New();
  for (int i = 0; i < 3; i++)
  {
    this->StartPosition[i] = 0.0;
  }

  this->IntegrationStepUnit = svtkStreamTracer::CELL_LENGTH_UNIT;
  this->InitialIntegrationStep = 0.5;
  this->ClosedLoopMaximumDistance = 1.0e-6;
  this->ClosedLoopMaximumDistanceArcLength = 1.0e-6;
  this->LoopAngle = 0.349066; // 20 degrees in radians
  this->MaximumNumberOfSteps = 2000;
  this->MinimumNumberOfLoopPoints = 4;
  this->DirectionStart = 0;
  // invalid integration direction so that we trigger a change the first time
  this->PreviousDirection = 0;

  this->TerminalSpeed = 1.0E-12;

  this->ComputeVorticity = true;

  this->InterpolatorPrototype = nullptr;

  // by default process active point vectors
  this->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, svtkDataSetAttributes::VECTORS);
  this->SeparatingDistance = 1;
  this->SeparatingDistanceArcLength = 1;
  this->SeparatingDistanceRatio = 0.5;
  this->SuperposedGrid = svtkImageData::New();
  this->Streamlines = svtkPolyDataCollection::New();
  // by default process active point vectors
  this->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, svtkDataSetAttributes::VECTORS);
}

svtkEvenlySpacedStreamlines2D::~svtkEvenlySpacedStreamlines2D()
{
  this->SetIntegrator(nullptr);
  this->SetInterpolatorPrototype(nullptr);
  this->SuperposedGrid->Delete();
  this->Streamlines->Delete();
}

int svtkEvenlySpacedStreamlines2D::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (!this->SetupOutput(inInfo, outInfo))
  {
    return 0;
  }
  double bounds[6];
  svtkEvenlySpacedStreamlines2D::GetBounds(this->InputData, bounds);
  if (!svtkMathUtilities::FuzzyCompare(bounds[4], bounds[5]))
  {
    this->InputData->UnRegister(this);
    svtkErrorMacro("svtkEvenlySpacedStreamlines2D does not support planes not aligned with XY.");
    return 0;
  }
  std::array<double, 3> v = { { bounds[1] - bounds[0], bounds[3] - bounds[2],
    bounds[5] - bounds[4] } };
  double length = svtkMath::Norm(&v[0]);

  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // compute the separating distance arc length
  double cellLength = 0;
  if (!this->ComputeCellLength(&cellLength))
  {
    this->InputData->UnRegister(this);
    return 0;
  }
  this->SeparatingDistanceArcLength =
    this->ConvertToLength(this->SeparatingDistance, this->IntegrationStepUnit, cellLength);
  this->ClosedLoopMaximumDistanceArcLength =
    this->ConvertToLength(this->ClosedLoopMaximumDistance, this->IntegrationStepUnit, cellLength);
  this->InitializeSuperposedGrid(bounds);
  auto streamTracer = svtkSmartPointer<svtkStreamTracer>::New();
  streamTracer->SetInputDataObject(this->InputData);
  streamTracer->SetMaximumPropagation(length);
  streamTracer->SetMaximumNumberOfSteps(this->MaximumNumberOfSteps);
  streamTracer->SetIntegrationDirection(svtkStreamTracer::BOTH);
  streamTracer->SetInputArrayToProcess(0, this->GetInputArrayInformation(0));
  streamTracer->SetStartPosition(this->StartPosition);
  streamTracer->SetTerminalSpeed(this->TerminalSpeed);
  streamTracer->SetInitialIntegrationStep(this->InitialIntegrationStep);
  streamTracer->SetIntegrationStepUnit(this->IntegrationStepUnit);
  streamTracer->SetIntegrator(this->Integrator);
  streamTracer->SetComputeVorticity(this->ComputeVorticity);
  streamTracer->SetInterpolatorPrototype(this->InterpolatorPrototype);
  // we end streamlines after one loop iteration
  streamTracer->AddCustomTerminationCallback(&svtkEvenlySpacedStreamlines2D::IsStreamlineLooping,
    this, svtkStreamTracer::FIXED_REASONS_FOR_TERMINATION_COUNT);
  streamTracer->Update();

  auto streamline = svtkSmartPointer<svtkPolyData>::New();
  streamline->ShallowCopy(streamTracer->GetOutput());
  this->AddToAllPoints(streamline);

  auto append = svtkSmartPointer<svtkAppendPolyData>::New();
  append->UserManagedInputsOn();
  append->SetNumberOfInputs(2);
  output->ShallowCopy(streamline);
  int currentSeedId = 1;
  int processedSeedId = 0;

  this->Streamlines->RemoveAllItems();
  this->Streamlines->AddItem(streamline);
  // we also end streamlines when they are close to other streamlines
  streamTracer->AddCustomTerminationCallback(
    &svtkEvenlySpacedStreamlines2D::IsStreamlineTooCloseToOthers, this,
    svtkStreamTracer::FIXED_REASONS_FOR_TERMINATION_COUNT + 1);

  const char* velocityName = this->GetInputArrayToProcessName();
  double deltaOne = this->SeparatingDistanceArcLength / 1000;
  double delta[3] = { deltaOne, deltaOne, deltaOne };
  int maxNumberOfItems = 0;
  float lastProgress = 0.0;
  while (this->Streamlines->GetNumberOfItems())
  {
    int numberOfItems = this->Streamlines->GetNumberOfItems();
    if (numberOfItems > maxNumberOfItems)
    {
      maxNumberOfItems = numberOfItems;
    }
    if (processedSeedId % 10 == 0)
    {
      float progress = (static_cast<float>(maxNumberOfItems) - numberOfItems) / maxNumberOfItems;
      if (progress > lastProgress)
      {
        this->UpdateProgress(progress);
        lastProgress = progress;
      }
    }

    streamline = svtkPolyData::SafeDownCast(this->Streamlines->GetItemAsObject(0));
    svtkDataArray* velocity = streamline->GetPointData()->GetArray(velocityName);
    for (svtkIdType pointId = 0; pointId < streamline->GetNumberOfPoints(); ++pointId)
    {
      // generate 2 new seeds for every streamline point
      double newSeedVector[3];
      double normal[3] = { 0, 0, 1 };
      svtkMath::Cross(normal, velocity->GetTuple(pointId), newSeedVector);
      // floating point errors move newSeedVector out of XY plane.
      newSeedVector[2] = 0;
      svtkMath::Normalize(newSeedVector);
      svtkMath::MultiplyScalar(newSeedVector, this->SeparatingDistanceArcLength);
      double point[3];
      streamline->GetPoint(pointId, point);
      std::array<std::array<double, 3>, 2> newSeeds;
      svtkMath::Add(point, newSeedVector, &newSeeds[0][0]);
      svtkMath::Subtract(point, newSeedVector, &newSeeds[1][0]);

      for (auto newSeed : newSeeds)
      {
        if (svtkMath::PointIsWithinBounds(&newSeed[0], bounds, delta) &&
          !this->ForEachCell(&newSeed[0], &svtkEvenlySpacedStreamlines2D::IsTooClose<DISTANCE>))
        {
          streamTracer->SetStartPosition(&newSeed[0]);
          streamTracer->Update();
          auto newStreamline = svtkSmartPointer<svtkPolyData>::New();
          newStreamline->ShallowCopy(streamTracer->GetOutput());

          svtkIntArray* seedIds =
            svtkIntArray::SafeDownCast(newStreamline->GetCellData()->GetArray("SeedIds"));
          for (int cellId = 0; cellId < newStreamline->GetNumberOfCells(); ++cellId)
          {
            seedIds->SetValue(cellId, currentSeedId);
          }
          currentSeedId++;
          this->AddToAllPoints(newStreamline);
          append->SetInputDataByNumber(0, output);
          append->SetInputDataByNumber(1, newStreamline);
          append->Update();
          output->ShallowCopy(append->GetOutput());
          this->Streamlines->AddItem(newStreamline);
        }
      }
    }
    this->Streamlines->RemoveItem(0);
    ++processedSeedId;
  }
  this->InputData->UnRegister(this);
  return 1;
}

int svtkEvenlySpacedStreamlines2D::ComputeCellLength(double* cellLength)
{
  svtkAbstractInterpolatedVelocityField* func;
  int maxCellSize = 0;
  if (this->CheckInputs(func, &maxCellSize) != SVTK_OK)
  {
    if (func)
    {
      func->Delete();
    }
    return 0;
  }
  svtkDataSet* input;
  auto cell = svtkSmartPointer<svtkGenericCell>::New();
  double velocity[3];
  // access the start position
  if (!func->FunctionValues(this->StartPosition, velocity))
  {
    func->Delete();
    return 0;
  }
  // Make sure we use the dataset found by the svtkAbstractInterpolatedVelocityField
  input = func->GetLastDataSet();
  input->GetCell(func->GetLastCellId(), cell);
  *cellLength = sqrt(static_cast<double>(cell->GetLength2()));
  func->Delete();
  return 1;
}

int svtkEvenlySpacedStreamlines2D::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataObject");
  }
  return 1;
}

bool svtkEvenlySpacedStreamlines2D::IsStreamlineTooCloseToOthers(
  void* clientdata, svtkPoints* points, svtkDataArray* velocity, int direction)
{
  (void)velocity;
  (void)direction;
  svtkEvenlySpacedStreamlines2D* This = static_cast<svtkEvenlySpacedStreamlines2D*>(clientdata);
  svtkIdType count = points->GetNumberOfPoints();
  double point[3];
  points->GetPoint(count - 1, point);
  return This->ForEachCell(point, &svtkEvenlySpacedStreamlines2D::IsTooClose<DISTANCE_RATIO>);
}

bool svtkEvenlySpacedStreamlines2D::IsStreamlineLooping(
  void* clientdata, svtkPoints* points, svtkDataArray* velocity, int direction)
{
  svtkEvenlySpacedStreamlines2D* This = static_cast<svtkEvenlySpacedStreamlines2D*>(clientdata);
  svtkIdType p0 = points->GetNumberOfPoints() - 1;

  // reinitialize when changing direction
  if (direction != This->PreviousDirection)
  {
    This->InitializePoints(This->CurrentPoints);
    This->InitializeMinPointIds();
    This->PreviousDirection = direction;
    This->DirectionStart = p0;
  }

  double p0Point[3];
  points->GetPoint(p0, p0Point);
  int ijk[3] = { 0, 0, 0 };
  ijk[0] = floor(p0Point[0] / This->SeparatingDistanceArcLength);
  ijk[1] = floor(p0Point[1] / This->SeparatingDistanceArcLength);
  svtkIdType cellId = This->SuperposedGrid->ComputeCellId(&ijk[0]);

  bool retVal = This->ForEachCell(
    p0Point, &svtkEvenlySpacedStreamlines2D::IsLooping, points, velocity, direction);

  // add the point to the list
  This->CurrentPoints[cellId].push_back(p0);
  if (p0 < This->MinPointIds[cellId])
  {
    This->MinPointIds[cellId] = p0;
  }
  return retVal;
}

template <typename CellCheckerType>
bool svtkEvenlySpacedStreamlines2D::ForEachCell(
  double* point, CellCheckerType checker, svtkPoints* points, svtkDataArray* velocity, int direction)
{
  // point current cell
  int ijk[3] = { 0, 0, 0 };
  ijk[0] = floor(point[0] / this->SeparatingDistanceArcLength);
  ijk[1] = floor(point[1] / this->SeparatingDistanceArcLength);
  svtkIdType cellId = this->SuperposedGrid->ComputeCellId(&ijk[0]);
  if ((this->*checker)(point, cellId, points, velocity, direction))
  {
    return true;
  }
  // and check cells around the current cell
  std::array<std::array<int, 3>, 8> around = { {
    { { ijk[0] - 1, ijk[1] + 1, ijk[2] } },
    { { ijk[0], ijk[1] + 1, ijk[2] } },
    { { ijk[0] + 1, ijk[1] + 1, ijk[2] } },
    { { ijk[0] - 1, ijk[1], ijk[2] } },
    { { ijk[0] + 1, ijk[1], ijk[2] } },
    { { ijk[0] - 1, ijk[1] - 1, ijk[2] } },
    { { ijk[0], ijk[1] - 1, ijk[2] } },
    { { ijk[0] + 1, ijk[1] - 1, ijk[2] } },
  } };
  int extent[6];
  this->SuperposedGrid->GetExtent(extent);
  for (auto cellPos : around)
  {
    cellId = this->SuperposedGrid->ComputeCellId(&cellPos[0]);
    if (cellPos[0] >= extent[0] && cellPos[0] < extent[1] && cellPos[1] >= extent[2] &&
      cellPos[1] < extent[3] && (this->*checker)(point, cellId, points, velocity, direction))
    {
      return true;
    }
  }
  return false;
}

bool svtkEvenlySpacedStreamlines2D::IsLooping(
  double* point, svtkIdType cellId, svtkPoints* points, svtkDataArray* velocity, int direction)
{
  (void)point;
  // do we have enough points to form a loop
  svtkIdType p0 = points->GetNumberOfPoints() - 1;
  svtkIdType minLoopPoints = std::max(svtkIdType(3), this->MinimumNumberOfLoopPoints);
  if (!this->CurrentPoints[cellId].empty() && p0 - this->MinPointIds[cellId] + 1 >= minLoopPoints)
  {
    svtkIdType p1 = p0 - 1;
    double testDistance2 = this->SeparatingDistanceArcLength * this->SeparatingDistanceArcLength *
      this->SeparatingDistanceRatio * this->SeparatingDistanceRatio;
    double maxDistance2 =
      this->ClosedLoopMaximumDistanceArcLength * this->ClosedLoopMaximumDistanceArcLength;
    for (svtkIdType q : this->CurrentPoints[cellId])
    {
      // do we have enough points to form a loop
      if (p0 - q + 1 < minLoopPoints)
      {
        continue;
      }
      double p0Point[3];
      points->GetPoint(p0, p0Point);
      double qPoint[3];
      points->GetPoint(q, qPoint);
      double distance2 = svtkMath::Distance2BetweenPoints(p0Point, qPoint);
      if (distance2 <= maxDistance2)
      {
        // closed loop
        return true;
      }
      if (distance2 >= testDistance2)
      {
        // we might loop but points are too far.
        continue;
      }
      double p1Point[3];
      points->GetPoint(p1, p1Point);
      double v1[3];
      svtkMath::Subtract(p0Point, p1Point, v1);
      svtkMath::MultiplyScalar(v1, direction);
      double* qVector = velocity->GetTuple(q);
      if (svtkMath::Dot(qVector, v1) < cos(this->LoopAngle))
      {
        // qVector makes a large angle with p0p1
        continue;
      }
      double u0[3], u1[3];
      svtkMath::Subtract(p0Point, qPoint, u0);
      svtkMath::MultiplyScalar(u0, direction);
      svtkMath::Subtract(p1Point, qPoint, u1);
      svtkMath::MultiplyScalar(u1, direction);
      if (svtkMath::Dot(u0, v1) >= 0 && svtkMath::Dot(u1, v1) >= 0)
      {
        // we found a "proponent point" See Liu et al.
        continue;
      }
      // the algorithm in Liu at al. has another test that determines if the
      // loop is closed or spiraling. We don't care about that so we skip it.
      return true;
    }
  }
  return false;
}

template <int distanceType>
bool svtkEvenlySpacedStreamlines2D::IsTooClose(
  double* point, svtkIdType cellId, svtkPoints* points, svtkDataArray* velocity, int direction)
{
  (void)points;
  (void)velocity;
  (void)direction;
  double testDistance2 = this->SeparatingDistanceArcLength * this->SeparatingDistanceArcLength;
  if (distanceType == DISTANCE_RATIO)
  {
    testDistance2 *= (this->SeparatingDistanceRatio * this->SeparatingDistanceRatio);
  }
  for (auto cellPoint : this->AllPoints[cellId])
  {
    double distance2 = svtkMath::Distance2BetweenPoints(point, &cellPoint[0]);
    if (distance2 < testDistance2)
    {
      return true;
    }
  }
  return false;
}

int svtkEvenlySpacedStreamlines2D::GetIntegratorType()
{
  if (!this->Integrator)
  {
    return svtkStreamTracer::NONE;
  }
  if (!strcmp(this->Integrator->GetClassName(), "svtkRungeKutta2"))
  {
    return svtkStreamTracer::RUNGE_KUTTA2;
  }
  if (!strcmp(this->Integrator->GetClassName(), "svtkRungeKutta4"))
  {
    return svtkStreamTracer::RUNGE_KUTTA4;
  }
  return svtkStreamTracer::UNKNOWN;
}

void svtkEvenlySpacedStreamlines2D::SetInterpolatorTypeToDataSetPointLocator()
{
  this->SetInterpolatorType(
    static_cast<int>(svtkStreamTracer::INTERPOLATOR_WITH_DATASET_POINT_LOCATOR));
}

void svtkEvenlySpacedStreamlines2D::SetInterpolatorTypeToCellLocator()
{
  this->SetInterpolatorType(static_cast<int>(svtkStreamTracer::INTERPOLATOR_WITH_CELL_LOCATOR));
}

void svtkEvenlySpacedStreamlines2D::SetInterpolatorType(int interpType)
{
  if (interpType == svtkStreamTracer::INTERPOLATOR_WITH_CELL_LOCATOR)
  {
    // create an interpolator equipped with a cell locator
    svtkSmartPointer<svtkCellLocatorInterpolatedVelocityField> cellLoc =
      svtkSmartPointer<svtkCellLocatorInterpolatedVelocityField>::New();

    // specify the type of the cell locator attached to the interpolator
    svtkSmartPointer<svtkModifiedBSPTree> cellLocType = svtkSmartPointer<svtkModifiedBSPTree>::New();
    cellLoc->SetCellLocatorPrototype(cellLocType);

    this->SetInterpolatorPrototype(cellLoc);
  }
  else
  {
    // create an interpolator equipped with a point locator (by default)
    svtkSmartPointer<svtkInterpolatedVelocityField> pntLoc =
      svtkSmartPointer<svtkInterpolatedVelocityField>::New();
    this->SetInterpolatorPrototype(pntLoc);
  }
}

void svtkEvenlySpacedStreamlines2D::SetIntegratorType(int type)
{
  svtkInitialValueProblemSolver* ivp = nullptr;
  switch (type)
  {
    case svtkStreamTracer::RUNGE_KUTTA2:
      ivp = svtkRungeKutta2::New();
      break;
    case svtkStreamTracer::RUNGE_KUTTA4:
      ivp = svtkRungeKutta4::New();
      break;
    default:
      svtkWarningMacro("Unrecognized integrator type. Keeping old one.");
      break;
  }
  if (ivp)
  {
    this->SetIntegrator(ivp);
    ivp->Delete();
  }
}

void svtkEvenlySpacedStreamlines2D::SetIntegrationStepUnit(int unit)
{
  if (unit != svtkStreamTracer::LENGTH_UNIT && unit != svtkStreamTracer::CELL_LENGTH_UNIT)
  {
    unit = svtkStreamTracer::CELL_LENGTH_UNIT;
  }

  if (unit == this->IntegrationStepUnit)
  {
    return;
  }

  this->IntegrationStepUnit = unit;
  this->Modified();
}

double svtkEvenlySpacedStreamlines2D::ConvertToLength(double interval, int unit, double cellLength)
{
  double retVal = 0.0;
  if (unit == svtkStreamTracer::LENGTH_UNIT)
  {
    retVal = interval;
  }
  else if (unit == svtkStreamTracer::CELL_LENGTH_UNIT)
  {
    retVal = interval * cellLength;
  }
  return retVal;
}

int svtkEvenlySpacedStreamlines2D::SetupOutput(svtkInformation* inInfo, svtkInformation* outInfo)
{
  int piece = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int numPieces = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  svtkDataObject* input = inInfo->Get(svtkDataObject::DATA_OBJECT());

  svtkCompositeDataSet* hdInput = svtkCompositeDataSet::SafeDownCast(input);
  svtkDataSet* dsInput = svtkDataSet::SafeDownCast(input);
  if (hdInput)
  {
    this->InputData = hdInput;
    hdInput->Register(this);
    return 1;
  }
  else if (dsInput)
  {
    auto mb = svtkSmartPointer<svtkMultiBlockDataSet>::New();
    mb->SetNumberOfBlocks(numPieces);
    mb->SetBlock(piece, dsInput);
    this->InputData = mb;
    mb->Register(this);
    return 1;
  }
  else
  {
    svtkErrorMacro(
      "This filter cannot handle input of type: " << (input ? input->GetClassName() : "(none)"));
    return 0;
  }
}

int svtkEvenlySpacedStreamlines2D::CheckInputs(
  svtkAbstractInterpolatedVelocityField*& func, int* maxCellSize)
{
  if (!this->InputData)
  {
    return SVTK_ERROR;
  }

  svtkOverlappingAMR* amrData = svtkOverlappingAMR::SafeDownCast(this->InputData);

  svtkSmartPointer<svtkCompositeDataIterator> iter;
  iter.TakeReference(this->InputData->NewIterator());

  svtkDataSet* input0 = nullptr;
  iter->GoToFirstItem();
  while (!iter->IsDoneWithTraversal() && input0 == nullptr)
  {
    input0 = svtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    iter->GoToNextItem();
  }
  if (!input0)
  {
    return SVTK_ERROR;
  }

  int vecType(0);
  svtkDataArray* vectors = this->GetInputArrayToProcess(0, input0, vecType);
  if (!vectors)
  {
    return SVTK_ERROR;
  }

  // Set the function set to be integrated
  if (!this->InterpolatorPrototype)
  {
    if (amrData)
    {
      func = svtkAMRInterpolatedVelocityField::New();
    }
    else
    {
      func = svtkInterpolatedVelocityField::New();
    }
  }
  else
  {
    if (amrData &&
      svtkAMRInterpolatedVelocityField::SafeDownCast(this->InterpolatorPrototype) == nullptr)
    {
      this->InterpolatorPrototype = svtkAMRInterpolatedVelocityField::New();
    }
    func = this->InterpolatorPrototype->NewInstance();
    func->CopyParameters(this->InterpolatorPrototype);
  }

  if (svtkAMRInterpolatedVelocityField::SafeDownCast(func))
  {
    assert(amrData);
    svtkAMRInterpolatedVelocityField::SafeDownCast(func)->SetAMRData(amrData);
    if (maxCellSize)
    {
      *maxCellSize = 8;
    }
  }
  else if (svtkCompositeInterpolatedVelocityField::SafeDownCast(func))
  {
    iter->GoToFirstItem();
    while (!iter->IsDoneWithTraversal())
    {
      svtkDataSet* inp = svtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (inp)
      {
        int cellSize = inp->GetMaxCellSize();
        if (cellSize > *maxCellSize)
        {
          *maxCellSize = cellSize;
        }
        svtkCompositeInterpolatedVelocityField::SafeDownCast(func)->AddDataSet(inp);
      }
      iter->GoToNextItem();
    }
  }
  else
  {
    assert(false);
  }

  const char* vecName = vectors->GetName();
  func->SelectVectors(vecType, vecName);
  return SVTK_OK;
}

void svtkEvenlySpacedStreamlines2D::InitializeSuperposedGrid(double* bounds)
{
  this->SuperposedGrid->SetExtent(floor(bounds[0] / this->SeparatingDistanceArcLength),
    ceil(bounds[1] / this->SeparatingDistanceArcLength),
    floor(bounds[2] / this->SeparatingDistanceArcLength),
    ceil(bounds[3] / this->SeparatingDistanceArcLength), 0, 0);
  this->SuperposedGrid->SetSpacing(this->SeparatingDistanceArcLength,
    this->SeparatingDistanceArcLength, this->SeparatingDistanceArcLength);
  this->InitializePoints(this->AllPoints);
  this->InitializePoints(this->CurrentPoints);
}

template <typename T>
void svtkEvenlySpacedStreamlines2D::InitializePoints(T& points)
{
  points.resize(this->SuperposedGrid->GetNumberOfCells());
  for (std::size_t i = 0; i < points.size(); ++i)
  {
    points[i].clear();
  }
}

void svtkEvenlySpacedStreamlines2D::InitializeMinPointIds()
{
  this->MinPointIds.resize(this->SuperposedGrid->GetNumberOfCells());
  for (std::size_t i = 0; i < this->MinPointIds.size(); ++i)
  {
    this->MinPointIds[i] = std::numeric_limits<svtkIdType>::max();
  }
}

void svtkEvenlySpacedStreamlines2D::AddToAllPoints(svtkPolyData* streamline)
{
  svtkPoints* points = streamline->GetPoints();
  if (points)
  {
    for (svtkIdType i = 0; i < points->GetNumberOfPoints(); ++i)
    {
      double point[3];
      points->GetPoint(i, point);
      int ijk[3] = { 0, 0, 0 };
      ijk[0] = floor(point[0] / this->SeparatingDistanceArcLength);
      ijk[1] = floor(point[1] / this->SeparatingDistanceArcLength);
      svtkIdType cellId = this->SuperposedGrid->ComputeCellId(ijk);
      this->AllPoints[cellId].push_back({ { point[0], point[1], point[2] } });
    }
  }
}

void svtkEvenlySpacedStreamlines2D::GetBounds(svtkCompositeDataSet* cds, double bounds[6])
{
  if (svtkOverlappingAMR::SafeDownCast(cds))
  {
    svtkOverlappingAMR* amr = svtkOverlappingAMR::SafeDownCast(cds);
    amr->GetBounds(bounds);
  }
  else
  {
    // initialize bounds
    for (int i : { 0, 2, 4 })
    {
      bounds[i] = std::numeric_limits<double>::max();
    }
    for (int i : { 1, 3, 5 })
    {
      bounds[i] = -std::numeric_limits<double>::max();
    }
    // go over all datasets in the composite data and find min,max
    // for components of all bounds
    svtkSmartPointer<svtkCompositeDataIterator> iter;
    iter.TakeReference(cds->NewIterator());
    iter->GoToFirstItem();
    while (!iter->IsDoneWithTraversal())
    {
      svtkDataSet* input = svtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (input)
      {
        double b[6];
        input->GetBounds(b);
        for (int i : { 0, 2, 4 })
        {
          if (b[i] < bounds[i])
          {
            bounds[i] = b[i];
          }
        }
        for (int i : { 1, 3, 5 })
        {
          if (b[i] > bounds[i])
          {
            bounds[i] = b[i];
          }
        }
      }
      iter->GoToNextItem();
    }
  }
}

const char* svtkEvenlySpacedStreamlines2D::GetInputArrayToProcessName()
{
  svtkSmartPointer<svtkCompositeDataIterator> iter;
  iter.TakeReference(this->InputData->NewIterator());

  svtkDataSet* input0 = nullptr;
  iter->GoToFirstItem();
  while (!iter->IsDoneWithTraversal() && input0 == nullptr)
  {
    input0 = svtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    iter->GoToNextItem();
  }
  if (!input0)
  {
    return "";
  }
  int vecType(0);
  svtkDataArray* vectors = this->GetInputArrayToProcess(0, input0, vecType);
  if (vectors)
  {
    return vectors->GetName();
  }
  else
  {
    svtkErrorMacro("svtkEvenlySpacedStreamlines2D::SetInputArrayToProcess was not called");
    return nullptr;
  }
}

void svtkEvenlySpacedStreamlines2D::SetIntegratorTypeToRungeKutta2()
{
  this->SetIntegratorType(svtkStreamTracer::RUNGE_KUTTA2);
}

void svtkEvenlySpacedStreamlines2D::SetIntegratorTypeToRungeKutta4()
{
  this->SetIntegratorType(svtkStreamTracer::RUNGE_KUTTA4);
}

void svtkEvenlySpacedStreamlines2D::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Start position: " << this->StartPosition[0] << " " << this->StartPosition[1]
     << " " << this->StartPosition[2] << endl;
  os << indent << "Terminal speed: " << this->TerminalSpeed << endl;

  os << indent << "Integration step unit: "
     << ((this->IntegrationStepUnit == svtkStreamTracer::LENGTH_UNIT) ? "length." : "cell length.")
     << endl;

  os << indent << "Initial integration step: " << this->InitialIntegrationStep << endl;
  os << indent << "Separation distance: " << this->SeparatingDistance << endl;

  os << indent << "Integrator: " << this->Integrator << endl;
  os << indent << "Vorticity computation: " << (this->ComputeVorticity ? " On" : " Off") << endl;
}
