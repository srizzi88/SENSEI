/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkStreamTracer.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkStreamTracer.h"

#include "svtkAMRInterpolatedVelocityField.h"
#include "svtkAbstractInterpolatedVelocityField.h"
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
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkInterpolatedVelocityField.h"
#include "svtkMath.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkOverlappingAMR.h"
#include "svtkPointData.h"
#include "svtkPointSet.h"
#include "svtkPolyData.h"
#include "svtkPolyLine.h"
#include "svtkRungeKutta2.h"
#include "svtkRungeKutta4.h"
#include "svtkRungeKutta45.h"
#include "svtkSmartPointer.h"
#include "svtkStaticCellLocator.h"

#include <vector>

svtkObjectFactoryNewMacro(svtkStreamTracer);
svtkCxxSetObjectMacro(svtkStreamTracer, Integrator, svtkInitialValueProblemSolver);
svtkCxxSetObjectMacro(svtkStreamTracer, InterpolatorPrototype, svtkAbstractInterpolatedVelocityField);

const double svtkStreamTracer::EPSILON = 1.0E-12;

namespace
{
// special function to interpolate the point data from the input to the output
// if fast == true, then it just calls the usual InterpolatePoint function,
// otherwise,
// it makes sure the array exists in the input before trying to copy it to the
// output. if it doesn't exist in the input but is in the output then we
// remove it from the output instead of having bad values there.
// this is meant for multiblock data sets where the grids may not have the
// same point data arrays or have them in different orders.
void InterpolatePoint(svtkDataSetAttributes* outPointData, svtkDataSetAttributes* inPointData,
  svtkIdType toId, svtkIdList* ids, double* weights, bool fast)
{
  if (fast)
  {
    outPointData->InterpolatePoint(inPointData, toId, ids, weights);
  }
  else
  {
    for (int i = outPointData->GetNumberOfArrays() - 1; i >= 0; i--)
    {
      svtkAbstractArray* toArray = outPointData->GetAbstractArray(i);
      if (svtkAbstractArray* fromArray = inPointData->GetAbstractArray(toArray->GetName()))
      {
        toArray->InterpolateTuple(toId, ids, fromArray, weights);
      }
      else
      {
        outPointData->RemoveArray(toArray->GetName());
      }
    }
  }
}

}

//---------------------------------------------------------------------------
svtkStreamTracer::svtkStreamTracer()
{
  this->Integrator = svtkRungeKutta2::New();
  this->IntegrationDirection = FORWARD;
  for (int i = 0; i < 3; i++)
  {
    this->StartPosition[i] = 0.0;
  }

  this->MaximumPropagation = 1.0;
  this->IntegrationStepUnit = CELL_LENGTH_UNIT;
  this->InitialIntegrationStep = 0.5;
  this->MinimumIntegrationStep = 1.0E-2;
  this->MaximumIntegrationStep = 1.0;

  this->MaximumError = 1.0e-6;
  this->MaximumNumberOfSteps = 2000;
  this->TerminalSpeed = EPSILON;

  this->ComputeVorticity = true;
  this->RotationScale = 1.0;

  this->LastUsedStepSize = 0.0;

  this->GenerateNormalsInIntegrate = true;

  this->InterpolatorPrototype = nullptr;

  this->SetNumberOfInputPorts(2);

  // by default process active point vectors
  this->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, svtkDataSetAttributes::VECTORS);

  this->HasMatchingPointAttributes = true;

  this->SurfaceStreamlines = false;
}

//---------------------------------------------------------------------------
svtkStreamTracer::~svtkStreamTracer()
{
  this->SetIntegrator(nullptr);
  this->SetInterpolatorPrototype(nullptr);
}

//---------------------------------------------------------------------------
void svtkStreamTracer::SetSourceConnection(svtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//---------------------------------------------------------------------------
void svtkStreamTracer::SetSourceData(svtkDataSet* source)
{
  this->SetInputData(1, source);
}

//---------------------------------------------------------------------------
svtkDataSet* svtkStreamTracer::GetSource()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }
  return svtkDataSet::SafeDownCast(this->GetExecutive()->GetInputData(1, 0));
}

//---------------------------------------------------------------------------
int svtkStreamTracer::GetIntegratorType()
{
  if (!this->Integrator)
  {
    return NONE;
  }
  if (!strcmp(this->Integrator->GetClassName(), "svtkRungeKutta2"))
  {
    return RUNGE_KUTTA2;
  }
  if (!strcmp(this->Integrator->GetClassName(), "svtkRungeKutta4"))
  {
    return RUNGE_KUTTA4;
  }
  if (!strcmp(this->Integrator->GetClassName(), "svtkRungeKutta45"))
  {
    return RUNGE_KUTTA45;
  }
  return UNKNOWN;
}

//---------------------------------------------------------------------------
void svtkStreamTracer::SetInterpolatorTypeToDataSetPointLocator()
{
  this->SetInterpolatorType(static_cast<int>(INTERPOLATOR_WITH_DATASET_POINT_LOCATOR));
}

//---------------------------------------------------------------------------
void svtkStreamTracer::SetInterpolatorTypeToCellLocator()
{
  this->SetInterpolatorType(static_cast<int>(INTERPOLATOR_WITH_CELL_LOCATOR));
}

//---------------------------------------------------------------------------
void svtkStreamTracer::SetInterpolatorType(int interpType)
{
  if (interpType == INTERPOLATOR_WITH_CELL_LOCATOR)
  {
    // create an interpolator equipped with a cell locator
    svtkSmartPointer<svtkCellLocatorInterpolatedVelocityField> cellLoc =
      svtkSmartPointer<svtkCellLocatorInterpolatedVelocityField>::New();

    // specify the type of the cell locator attached to the interpolator
    svtkSmartPointer<svtkStaticCellLocator> cellLocType =
      svtkSmartPointer<svtkStaticCellLocator>::New();
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

//---------------------------------------------------------------------------
void svtkStreamTracer::SetIntegratorType(int type)
{
  svtkInitialValueProblemSolver* ivp = nullptr;
  switch (type)
  {
    case RUNGE_KUTTA2:
      ivp = svtkRungeKutta2::New();
      break;
    case RUNGE_KUTTA4:
      ivp = svtkRungeKutta4::New();
      break;
    case RUNGE_KUTTA45:
      ivp = svtkRungeKutta45::New();
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

//---------------------------------------------------------------------------
void svtkStreamTracer::SetIntegrationStepUnit(int unit)
{
  if (unit != LENGTH_UNIT && unit != CELL_LENGTH_UNIT)
  {
    unit = CELL_LENGTH_UNIT;
  }

  if (unit == this->IntegrationStepUnit)
  {
    return;
  }

  this->IntegrationStepUnit = unit;
  this->Modified();
}

//---------------------------------------------------------------------------
double svtkStreamTracer::ConvertToLength(double interval, int unit, double cellLength)
{
  double retVal = 0.0;
  if (unit == LENGTH_UNIT)
  {
    retVal = interval;
  }
  else if (unit == CELL_LENGTH_UNIT)
  {
    retVal = interval * cellLength;
  }
  return retVal;
}

//---------------------------------------------------------------------------
double svtkStreamTracer::ConvertToLength(
  svtkStreamTracer::IntervalInformation& interval, double cellLength)
{
  return ConvertToLength(interval.Interval, interval.Unit, cellLength);
}

//---------------------------------------------------------------------------
void svtkStreamTracer::ConvertIntervals(
  double& step, double& minStep, double& maxStep, int direction, double cellLength)
{
  minStep = maxStep = step = direction *
    this->ConvertToLength(this->InitialIntegrationStep, this->IntegrationStepUnit, cellLength);

  if (this->MinimumIntegrationStep > 0.0)
  {
    minStep =
      this->ConvertToLength(this->MinimumIntegrationStep, this->IntegrationStepUnit, cellLength);
  }

  if (this->MaximumIntegrationStep > 0.0)
  {
    maxStep =
      this->ConvertToLength(this->MaximumIntegrationStep, this->IntegrationStepUnit, cellLength);
  }
}

//---------------------------------------------------------------------------
void svtkStreamTracer::CalculateVorticity(
  svtkGenericCell* cell, double pcoords[3], svtkDoubleArray* cellVectors, double vorticity[3])
{
  double* cellVel;
  double derivs[9];

  cellVel = cellVectors->GetPointer(0);
  cell->Derivatives(0, pcoords, cellVel, 3, derivs);
  vorticity[0] = derivs[7] - derivs[5];
  vorticity[1] = derivs[2] - derivs[6];
  vorticity[2] = derivs[3] - derivs[1];
}

//---------------------------------------------------------------------------
void svtkStreamTracer::InitializeSeeds(svtkDataArray*& seeds, svtkIdList*& seedIds,
  svtkIntArray*& integrationDirections, svtkDataSet* source)
{
  seedIds = svtkIdList::New();
  integrationDirections = svtkIntArray::New();
  seeds = nullptr;

  if (source)
  {
    svtkIdType numSeeds = source->GetNumberOfPoints();
    if (numSeeds > 0)
    {
      // For now, one thread will do all

      if (this->IntegrationDirection == BOTH)
      {
        seedIds->SetNumberOfIds(2 * numSeeds);
        for (svtkIdType i = 0; i < numSeeds; ++i)
        {
          seedIds->SetId(i, i);
          seedIds->SetId(numSeeds + i, i);
        }
      }
      else
      {
        seedIds->SetNumberOfIds(numSeeds);
        for (svtkIdType i = 0; i < numSeeds; ++i)
        {
          seedIds->SetId(i, i);
        }
      }
      // Check if the source is a PointSet
      svtkPointSet* seedPts = svtkPointSet::SafeDownCast(source);
      if (seedPts)
      {
        // If it is, use it's points as source
        svtkDataArray* orgSeeds = seedPts->GetPoints()->GetData();
        seeds = orgSeeds->NewInstance();
        seeds->DeepCopy(orgSeeds);
      }
      else
      {
        // Else, create a seed source
        seeds = svtkDoubleArray::New();
        seeds->SetNumberOfComponents(3);
        seeds->SetNumberOfTuples(numSeeds);
        for (svtkIdType i = 0; i < numSeeds; ++i)
        {
          seeds->SetTuple(i, source->GetPoint(i));
        }
      }
    }
  }
  else
  {
    seeds = svtkDoubleArray::New();
    seeds->SetNumberOfComponents(3);
    seeds->InsertNextTuple(this->StartPosition);
    seedIds->InsertNextId(0);
    if (this->IntegrationDirection == BOTH)
    {
      seedIds->InsertNextId(0);
    }
  }

  if (seeds)
  {
    svtkIdType i;
    svtkIdType numSeeds = seeds->GetNumberOfTuples();
    if (this->IntegrationDirection == BOTH)
    {
      for (i = 0; i < numSeeds; i++)
      {
        integrationDirections->InsertNextValue(FORWARD);
      }
      for (i = 0; i < numSeeds; i++)
      {
        integrationDirections->InsertNextValue(BACKWARD);
      }
    }
    else
    {
      for (i = 0; i < numSeeds; i++)
      {
        integrationDirections->InsertNextValue(this->IntegrationDirection);
      }
    }
  }
}

//---------------------------------------------------------------------------
int svtkStreamTracer::SetupOutput(svtkInformation* inInfo, svtkInformation* outInfo)
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
    svtkMultiBlockDataSet* mb = svtkMultiBlockDataSet::New();
    mb->SetNumberOfBlocks(numPieces);
    mb->SetBlock(piece, dsInput);
    this->InputData = mb;
    mb->Register(this);
    mb->Delete();
    return 1;
  }
  else
  {
    svtkErrorMacro(
      "This filter cannot handle input of type: " << (input ? input->GetClassName() : "(none)"));
    return 0;
  }
}

//---------------------------------------------------------------------------
int svtkStreamTracer::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (!this->SetupOutput(inInfo, outInfo))
  {
    return 0;
  }

  svtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  svtkDataSet* source = nullptr;
  if (sourceInfo)
  {
    source = svtkDataSet::SafeDownCast(sourceInfo->Get(svtkDataObject::DATA_OBJECT()));
  }
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkDataArray* seeds = nullptr;
  svtkIdList* seedIds = nullptr;
  svtkIntArray* integrationDirections = nullptr;
  this->InitializeSeeds(seeds, seedIds, integrationDirections, source);

  if (seeds)
  {
    double lastPoint[3];
    svtkAbstractInterpolatedVelocityField* func = nullptr;
    int maxCellSize = 0;
    if (this->CheckInputs(func, &maxCellSize) != SVTK_OK)
    {
      svtkDebugMacro("No appropriate inputs have been found. Can not execute.");
      if (func)
      {
        func->Delete();
      }
      seeds->Delete();
      integrationDirections->Delete();
      seedIds->Delete();
      this->InputData->UnRegister(this);
      return 1;
    }

    if (svtkOverlappingAMR::SafeDownCast(this->InputData))
    {
      svtkOverlappingAMR* amr = svtkOverlappingAMR::SafeDownCast(this->InputData);
      amr->GenerateParentChildInformation();
    }

    svtkCompositeDataIterator* iter = this->InputData->NewIterator();
    svtkSmartPointer<svtkCompositeDataIterator> iterP(iter);
    iter->Delete();

    iterP->GoToFirstItem();
    svtkDataSet* input0 = nullptr;
    if (!iterP->IsDoneWithTraversal() && !input0)
    {
      input0 = svtkDataSet::SafeDownCast(iterP->GetCurrentDataObject());
      iterP->GoToNextItem();
    }
    int vecType(0);
    svtkDataArray* vectors = this->GetInputArrayToProcess(0, input0, vecType);
    if (vectors)
    {
      const char* vecName = vectors->GetName();
      double propagation = 0;
      svtkIdType numSteps = 0;
      double integrationTime = 0;
      this->Integrate(input0->GetPointData(), output, seeds, seedIds, integrationDirections,
        lastPoint, func, maxCellSize, vecType, vecName, propagation, numSteps, integrationTime);
    }
    func->Delete();
    seeds->Delete();
  }

  integrationDirections->Delete();
  seedIds->Delete();

  this->InputData->UnRegister(this);
  return 1;
}

//---------------------------------------------------------------------------
int svtkStreamTracer::CheckInputs(svtkAbstractInterpolatedVelocityField*& func, int* maxCellSize)
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
    // turn on the following segment, in place of the above line, if an
    // interpolator equipped with a cell locator is dedired as the default
    //
    // func = svtkCellLocatorInterpolatedVelocityField::New();
    // svtkSmartPointer< svtkStaticCellLocator > locator =
    // svtkSmartPointer< svtkStaticCellLocator >::New();
    // svtkCellLocatorInterpolatedVelocityField::SafeDownCast( func )
    //   ->SetCellLocatorPrototype( locator );
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

  // Check if the data attributes match, warn if not
  svtkPointData* pd0 = input0->GetPointData();
  int numPdArrays = pd0->GetNumberOfArrays();
  this->HasMatchingPointAttributes = true;
  for (iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    svtkDataSet* data = svtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    svtkPointData* pd = data->GetPointData();
    if (pd->GetNumberOfArrays() != numPdArrays)
    {
      this->HasMatchingPointAttributes = false;
    }
    for (int i = 0; i < numPdArrays; i++)
    {
      if (!pd->GetArray(pd0->GetArrayName(i)) || !pd0->GetArray(pd->GetArrayName(i)))
      {
        this->HasMatchingPointAttributes = false;
      }
    }
  }
  return SVTK_OK;
}

//---------------------------------------------------------------------------
void svtkStreamTracer::Integrate(svtkPointData* input0Data, svtkPolyData* output,
  svtkDataArray* seedSource, svtkIdList* seedIds, svtkIntArray* integrationDirections,
  double lastPoint[3], svtkAbstractInterpolatedVelocityField* func, int maxCellSize, int vecType,
  const char* vecName, double& inPropagation, svtkIdType& inNumSteps, double& inIntegrationTime)
{
  svtkIdType numLines = seedIds->GetNumberOfIds();
  double propagation = inPropagation;
  svtkIdType numSteps = inNumSteps;
  double integrationTime = inIntegrationTime;

  // Useful pointers
  svtkDataSetAttributes* outputPD = output->GetPointData();
  svtkDataSetAttributes* outputCD = output->GetCellData();
  svtkPointData* inputPD;
  svtkDataSet* input;
  svtkDataArray* inVectors;

  int direction = 1;

  if (this->GetIntegrator() == nullptr)
  {
    svtkErrorMacro("No integrator is specified.");
    return;
  }

  double* weights = nullptr;
  if (maxCellSize > 0)
  {
    weights = new double[maxCellSize];
  }

  // Used in GetCell()
  svtkGenericCell* cell = svtkGenericCell::New();

  // Create a new integrator, the type is the same as Integrator
  svtkInitialValueProblemSolver* integrator = this->GetIntegrator()->NewInstance();
  integrator->SetFunctionSet(func);

  // Check Surface option
  svtkInterpolatedVelocityField* surfaceFunc = nullptr;
  if (this->SurfaceStreamlines == true)
  {
    surfaceFunc = svtkInterpolatedVelocityField::SafeDownCast(func);
    if (surfaceFunc == nullptr)
    {
      svtkWarningMacro(<< "Surface Streamlines works only with Point Locator "
                         "Interpolated Velocity Field, setting it off");
      this->SetSurfaceStreamlines(false);
    }
    else
    {
      surfaceFunc->SetForceSurfaceTangentVector(true);
      surfaceFunc->SetSurfaceDataset(true);
    }
  }

  // Since we do not know what the total number of points
  // will be, we do not allocate any. This is important for
  // cases where a lot of streamers are used at once. If we
  // were to allocate any points here, potentially, we can
  // waste a lot of memory if a lot of streamers are used.
  // Always insert the first point
  svtkPoints* outputPoints = svtkPoints::New();
  svtkCellArray* outputLines = svtkCellArray::New();

  // We will keep track of integration time in this array
  svtkDoubleArray* time = svtkDoubleArray::New();
  time->SetName("IntegrationTime");

  // This array explains why the integration stopped
  svtkIntArray* retVals = svtkIntArray::New();
  retVals->SetName("ReasonForTermination");

  svtkIntArray* sids = svtkIntArray::New();
  sids->SetName("SeedIds");

  svtkSmartPointer<svtkDoubleArray> velocityVectors;
  if (vecType != svtkDataObject::POINT)
  {
    velocityVectors = svtkSmartPointer<svtkDoubleArray>::New();
    velocityVectors->SetName(vecName);
    velocityVectors->SetNumberOfComponents(3);
  }
  svtkDoubleArray* cellVectors = nullptr;
  svtkDoubleArray* vorticity = nullptr;
  svtkDoubleArray* rotation = nullptr;
  svtkDoubleArray* angularVel = nullptr;
  if (this->ComputeVorticity)
  {
    cellVectors = svtkDoubleArray::New();
    cellVectors->SetNumberOfComponents(3);
    cellVectors->Allocate(3 * SVTK_CELL_SIZE);

    vorticity = svtkDoubleArray::New();
    vorticity->SetName("Vorticity");
    vorticity->SetNumberOfComponents(3);

    rotation = svtkDoubleArray::New();
    rotation->SetName("Rotation");

    angularVel = svtkDoubleArray::New();
    angularVel->SetName("AngularVelocity");
  }

  // We will interpolate all point attributes of the input on each point of
  // the output (unless they are turned off). Note that we are using only
  // the first input, if there are more than one, the attributes have to match.
  //
  // Note: We have to use a specific value (safe to employ the maximum number
  //       of steps) as the size of the initial memory allocation here. The
  //       use of the default argument might incur a crash problem (due to
  //       "insufficient memory") in the parallel mode. This is the case when
  //       a streamline intensely shuttles between two processes in an exactly
  //       interleaving fashion --- only one point is produced on each process
  //       (and actually two points, after point duplication, are saved to a
  //       svtkPolyData in svtkDistributedStreamTracer::NoBlockProcessTask) and
  //       as a consequence a large number of such small svtkPolyData objects
  //       are needed to represent a streamline, consuming up the memory before
  //       the intermediate memory is timely released.
  outputPD->InterpolateAllocate(input0Data, this->MaximumNumberOfSteps);

  svtkIdType numPtsTotal = 0;
  double velocity[3];

  int shouldAbort = 0;

  for (int currentLine = 0; currentLine < numLines; currentLine++)
  {
    double progress = static_cast<double>(currentLine) / numLines;
    this->UpdateProgress(progress);

    switch (integrationDirections->GetValue(currentLine))
    {
      case FORWARD:
        direction = 1;
        break;
      case BACKWARD:
        direction = -1;
        break;
    }

    // temporary variables used in the integration
    double point1[3], point2[3], pcoords[3], vort[3], omega;
    svtkIdType index, numPts = 0;

    // Clear the last cell to avoid starting a search from
    // the last point in the streamline
    func->ClearLastCellId();

    // Initial point
    seedSource->GetTuple(seedIds->GetId(currentLine), point1);
    memcpy(point2, point1, 3 * sizeof(double));
    if (!func->FunctionValues(point1, velocity))
    {
      continue;
    }

    if (propagation >= this->MaximumPropagation || numSteps > this->MaximumNumberOfSteps)
    {
      continue;
    }

    numPts++;
    numPtsTotal++;
    svtkIdType nextPoint = outputPoints->InsertNextPoint(point1);
    double lastInsertedPoint[3];
    outputPoints->GetPoint(nextPoint, lastInsertedPoint);
    time->InsertNextValue(integrationTime);

    // We will always pass an arc-length step size to the integrator.
    // If the user specifies a step size in cell length unit, we will
    // have to convert it to arc length.
    IntervalInformation stepSize; // either positive or negative
    stepSize.Unit = LENGTH_UNIT;
    stepSize.Interval = 0;
    IntervalInformation aStep; // always positive
    aStep.Unit = LENGTH_UNIT;
    double step, minStep = 0, maxStep = 0;
    double stepTaken;
    double speed;
    double cellLength;
    int retVal = OUT_OF_LENGTH, tmp;

    // Make sure we use the dataset found by the svtkAbstractInterpolatedVelocityField
    input = func->GetLastDataSet();
    inputPD = input->GetPointData();
    inVectors = input->GetAttributesAsFieldData(vecType)->GetArray(vecName);
    // Convert intervals to arc-length unit
    input->GetCell(func->GetLastCellId(), cell);
    cellLength = sqrt(static_cast<double>(cell->GetLength2()));
    speed = svtkMath::Norm(velocity);
    // Never call conversion methods if speed == 0
    if (speed != 0.0)
    {
      this->ConvertIntervals(stepSize.Interval, minStep, maxStep, direction, cellLength);
    }

    // Interpolate all point attributes on first point
    func->GetLastWeights(weights);
    InterpolatePoint(
      outputPD, inputPD, nextPoint, cell->PointIds, weights, this->HasMatchingPointAttributes);
    // handle both point and cell velocity attributes.
    svtkDataArray* outputVelocityVectors = outputPD->GetArray(vecName);
    if (vecType != svtkDataObject::POINT)
    {
      velocityVectors->InsertNextTuple(velocity);
      outputVelocityVectors = velocityVectors;
    }

    // Compute vorticity if required
    // This can be used later for streamribbon generation.
    if (this->ComputeVorticity)
    {
      if (vecType == svtkDataObject::POINT)
      {
        inVectors->GetTuples(cell->PointIds, cellVectors);
        func->GetLastLocalCoordinates(pcoords);
        svtkStreamTracer::CalculateVorticity(cell, pcoords, cellVectors, vort);
      }
      else
      {
        vort[0] = 0;
        vort[1] = 0;
        vort[2] = 0;
      }
      vorticity->InsertNextTuple(vort);
      // rotation
      // local rotation = vorticity . unit tangent ( i.e. velocity/speed )
      if (speed != 0.0)
      {
        omega = svtkMath::Dot(vort, velocity);
        omega /= speed;
        omega *= this->RotationScale;
      }
      else
      {
        omega = 0.0;
      }
      angularVel->InsertNextValue(omega);
      rotation->InsertNextValue(0.0);
    }

    double error = 0;

    // Integrate until the maximum propagation length is reached,
    // maximum number of steps is reached or until a boundary is encountered.
    // Begin Integration
    while (propagation < this->MaximumPropagation)
    {

      if (numSteps > this->MaximumNumberOfSteps)
      {
        retVal = OUT_OF_STEPS;
        break;
      }

      bool endIntegration = false;
      for (std::size_t i = 0; i < this->CustomTerminationCallback.size(); ++i)
      {
        if (this->CustomTerminationCallback[i](
              this->CustomTerminationClientData[i], outputPoints, outputVelocityVectors, direction))
        {
          retVal = this->CustomReasonForTermination[i];
          endIntegration = true;
          break;
        }
      }
      if (endIntegration)
      {
        break;
      }

      if (numSteps++ % 1000 == 1)
      {
        progress = (currentLine + propagation / this->MaximumPropagation) / numLines;
        this->UpdateProgress(progress);

        if (this->GetAbortExecute())
        {
          shouldAbort = 1;
          break;
        }
      }

      // Never call conversion methods if speed == 0
      if ((speed == 0) || (speed <= this->TerminalSpeed))
      {
        retVal = STAGNATION;
        break;
      }

      // If, with the next step, propagation will be larger than
      // max, reduce it so that it is (approximately) equal to max.
      aStep.Interval = fabs(stepSize.Interval);

      if ((propagation + aStep.Interval) > this->MaximumPropagation)
      {
        aStep.Interval = this->MaximumPropagation - propagation;
        if (stepSize.Interval >= 0)
        {
          stepSize.Interval = this->ConvertToLength(aStep, cellLength);
        }
        else
        {
          stepSize.Interval = this->ConvertToLength(aStep, cellLength) * (-1.0);
        }
        maxStep = stepSize.Interval;
      }
      this->LastUsedStepSize = stepSize.Interval;

      // Calculate the next step using the integrator provided
      // Break if the next point is out of bounds.
      func->SetNormalizeVector(true);
      tmp = integrator->ComputeNextStep(point1, point2, 0, stepSize.Interval, stepTaken, minStep,
        maxStep, this->MaximumError, error);
      func->SetNormalizeVector(false);
      if (tmp != 0)
      {
        retVal = tmp;
        memcpy(lastPoint, point2, 3 * sizeof(double));
        break;
      }

      // This is the next starting point
      if (this->SurfaceStreamlines && surfaceFunc != nullptr)
      {
        if (surfaceFunc->SnapPointOnCell(point2, point1) != 1)
        {
          retVal = OUT_OF_DOMAIN;
          memcpy(lastPoint, point2, 3 * sizeof(double));
          break;
        }
      }
      else
      {
        for (int i = 0; i < 3; i++)
        {
          point1[i] = point2[i];
        }
      }

      // Interpolate the velocity at the next point
      if (!func->FunctionValues(point2, velocity))
      {
        retVal = OUT_OF_DOMAIN;
        memcpy(lastPoint, point2, 3 * sizeof(double));
        break;
      }

      // It is not enough to use the starting point for stagnation calculation
      // Use average speed to check if it is below stagnation threshold
      double speed2 = svtkMath::Norm(velocity);
      if ((speed + speed2) / 2 <= this->TerminalSpeed)
      {
        retVal = STAGNATION;
        break;
      }

      integrationTime += stepTaken / speed;
      // Calculate propagation (using the same units as MaximumPropagation
      propagation += fabs(stepSize.Interval);

      // Make sure we use the dataset found by the svtkAbstractInterpolatedVelocityField
      input = func->GetLastDataSet();
      inputPD = input->GetPointData();
      inVectors = input->GetAttributesAsFieldData(vecType)->GetArray(vecName);

      // Calculate cell length and speed to be used in unit conversions
      input->GetCell(func->GetLastCellId(), cell);
      cellLength = sqrt(static_cast<double>(cell->GetLength2()));
      speed = speed2;

      // Check if conversion to float will produce a point in same place
      float convertedPoint[3];
      for (int i = 0; i < 3; i++)
      {
        convertedPoint[i] = point1[i];
      }
      if (lastInsertedPoint[0] != convertedPoint[0] || lastInsertedPoint[1] != convertedPoint[1] ||
        lastInsertedPoint[2] != convertedPoint[2])
      {
        // Point is valid. Insert it.
        numPts++;
        numPtsTotal++;
        nextPoint = outputPoints->InsertNextPoint(point1);
        outputPoints->GetPoint(nextPoint, lastInsertedPoint);
        time->InsertNextValue(integrationTime);

        // Interpolate all point attributes on current point
        func->GetLastWeights(weights);
        InterpolatePoint(
          outputPD, inputPD, nextPoint, cell->PointIds, weights, this->HasMatchingPointAttributes);

        if (vecType != svtkDataObject::POINT)
        {
          velocityVectors->InsertNextTuple(velocity);
        }
        // Compute vorticity if required
        // This can be used later for streamribbon generation.
        if (this->ComputeVorticity)
        {
          if (vecType == svtkDataObject::POINT)
          {
            inVectors->GetTuples(cell->PointIds, cellVectors);
            func->GetLastLocalCoordinates(pcoords);
            svtkStreamTracer::CalculateVorticity(cell, pcoords, cellVectors, vort);
          }
          else
          {
            vort[0] = 0;
            vort[1] = 0;
            vort[2] = 0;
          }
          vorticity->InsertNextTuple(vort);
          // rotation
          // angular velocity = vorticity . unit tangent ( i.e. velocity/speed )
          // rotation = sum ( angular velocity * stepSize )
          omega = svtkMath::Dot(vort, velocity);
          omega /= speed;
          omega *= this->RotationScale;
          index = angularVel->InsertNextValue(omega);
          rotation->InsertNextValue(rotation->GetValue(index - 1) +
            (angularVel->GetValue(index - 1) + omega) / 2 *
              (integrationTime - time->GetValue(index - 1)));
        }
      }

      // Never call conversion methods if speed == 0
      if ((speed == 0) || (speed <= this->TerminalSpeed))
      {
        retVal = STAGNATION;
        break;
      }

      // Convert all intervals to arc length
      this->ConvertIntervals(step, minStep, maxStep, direction, cellLength);

      // If the solver is adaptive and the next step size (stepSize.Interval)
      // that the solver wants to use is smaller than minStep or larger
      // than maxStep, re-adjust it. This has to be done every step
      // because minStep and maxStep can change depending on the cell
      // size (unless it is specified in arc-length unit)
      if (integrator->IsAdaptive())
      {
        if (fabs(stepSize.Interval) < fabs(minStep))
        {
          stepSize.Interval = fabs(minStep) * stepSize.Interval / fabs(stepSize.Interval);
        }
        else if (fabs(stepSize.Interval) > fabs(maxStep))
        {
          stepSize.Interval = fabs(maxStep) * stepSize.Interval / fabs(stepSize.Interval);
        }
      }
      else
      {
        stepSize.Interval = step;
      }
    }

    if (shouldAbort)
    {
      break;
    }

    if (numPts > 1)
    {
      outputLines->InsertNextCell(numPts);
      for (int i = numPtsTotal - numPts; i < numPtsTotal; i++)
      {
        outputLines->InsertCellPoint(i);
      }
      retVals->InsertNextValue(retVal);
      sids->InsertNextValue(seedIds->GetId(currentLine));
    }

    // Initialize these to 0 before starting the next line.
    // The values passed in the function call are only used
    // for the first line.
    inPropagation = propagation;
    inNumSteps = numSteps;
    inIntegrationTime = integrationTime;

    propagation = 0;
    numSteps = 0;
    integrationTime = 0;
  }

  if (!shouldAbort)
  {
    // Create the output polyline
    output->SetPoints(outputPoints);
    outputPD->AddArray(time);
    if (vecType != svtkDataObject::POINT)
    {
      outputPD->AddArray(velocityVectors);
    }
    if (vorticity)
    {
      outputPD->AddArray(vorticity);
      outputPD->AddArray(rotation);
      outputPD->AddArray(angularVel);
    }

    svtkIdType numPts = outputPoints->GetNumberOfPoints();
    if (numPts > 1)
    {
      // Assign geometry and attributes
      output->SetLines(outputLines);
      if (this->GenerateNormalsInIntegrate)
      {
        this->GenerateNormals(output, nullptr, vecName);
      }

      outputCD->AddArray(retVals);
      outputCD->AddArray(sids);
    }
  }

  if (vorticity)
  {
    vorticity->Delete();
    rotation->Delete();
    angularVel->Delete();
  }

  if (cellVectors)
  {
    cellVectors->Delete();
  }
  retVals->Delete();
  sids->Delete();

  outputPoints->Delete();
  outputLines->Delete();

  time->Delete();

  integrator->Delete();
  cell->Delete();

  delete[] weights;

  output->Squeeze();
}

//---------------------------------------------------------------------------
void svtkStreamTracer::GenerateNormals(svtkPolyData* output, double* firstNormal, const char* vecName)
{
  // Useful pointers
  svtkDataSetAttributes* outputPD = output->GetPointData();

  svtkPoints* outputPoints = output->GetPoints();
  svtkCellArray* outputLines = output->GetLines();

  svtkDataArray* rotation = outputPD->GetArray("Rotation");

  svtkIdType numPts = outputPoints->GetNumberOfPoints();
  if (numPts > 1)
  {
    if (this->ComputeVorticity)
    {
      svtkPolyLine* lineNormalGenerator = svtkPolyLine::New();
      svtkDoubleArray* normals = svtkDoubleArray::New();
      normals->SetNumberOfComponents(3);
      normals->SetNumberOfTuples(numPts);
      // Make sure the normals are initialized in case
      // GenerateSlidingNormals() fails and returns before
      // creating all normals
      for (svtkIdType idx = 0; idx < numPts; idx++)
      {
        normals->SetTuple3(idx, 1, 0, 0);
      }

      lineNormalGenerator->GenerateSlidingNormals(outputPoints, outputLines, normals, firstNormal);
      lineNormalGenerator->Delete();

      svtkIdType i;
      int j;
      double normal[3], local1[3], local2[3], theta, costheta, sintheta, length;
      double velocity[3];
      normals->SetName("Normals");
      svtkDataArray* newVectors = outputPD->GetVectors(vecName);
      for (i = 0; i < numPts; i++)
      {
        normals->GetTuple(i, normal);
        if (newVectors == nullptr || newVectors->GetNumberOfTuples() != numPts)
        { // This should never happen.
          svtkErrorMacro("Bad velocity array.");
          return;
        }
        newVectors->GetTuple(i, velocity);
        // obtain two unit orthogonal vectors on the plane perpendicular to
        // the streamline
        for (j = 0; j < 3; j++)
        {
          local1[j] = normal[j];
        }
        length = svtkMath::Normalize(local1);
        svtkMath::Cross(local1, velocity, local2);
        svtkMath::Normalize(local2);
        // Rotate the normal with theta
        rotation->GetTuple(i, &theta);
        costheta = cos(theta);
        sintheta = sin(theta);
        for (j = 0; j < 3; j++)
        {
          normal[j] = length * (costheta * local1[j] + sintheta * local2[j]);
        }
        normals->SetTuple(i, normal);
      }
      outputPD->AddArray(normals);
      outputPD->SetActiveAttribute("Normals", svtkDataSetAttributes::VECTORS);
      normals->Delete();
    }
  }
}

//---------------------------------------------------------------------------
// This is used by sub-classes in certain situations. It
// does a lot less (for example, does not compute attributes)
// than Integrate.
double svtkStreamTracer::SimpleIntegrate(
  double seed[3], double lastPoint[3], double stepSize, svtkAbstractInterpolatedVelocityField* func)
{
  svtkIdType numSteps = 0;
  svtkIdType maxSteps = 20;
  double error = 0;
  double stepTaken = 0;
  double point1[3], point2[3];
  double velocity[3];
  double speed;
  int stepResult;

  (void)seed; // Seed is not used

  memcpy(point1, lastPoint, 3 * sizeof(double));

  // Create a new integrator, the type is the same as Integrator
  svtkInitialValueProblemSolver* integrator = this->GetIntegrator()->NewInstance();
  integrator->SetFunctionSet(func);

  while (1)
  {

    if (numSteps++ > maxSteps)
    {
      break;
    }

    // Calculate the next step using the integrator provided
    // Break if the next point is out of bounds.
    func->SetNormalizeVector(true);
    double tmpStepTaken = 0;
    stepResult =
      integrator->ComputeNextStep(point1, point2, 0, stepSize, tmpStepTaken, 0, 0, 0, error);
    stepTaken += tmpStepTaken;
    func->SetNormalizeVector(false);
    if (stepResult != 0)
    {
      memcpy(lastPoint, point2, 3 * sizeof(double));
      break;
    }

    // This is the next starting point
    for (int i = 0; i < 3; i++)
    {
      point1[i] = point2[i];
    }

    // Interpolate the velocity at the next point
    if (!func->FunctionValues(point2, velocity))
    {
      memcpy(lastPoint, point2, 3 * sizeof(double));
      break;
    }

    speed = svtkMath::Norm(velocity);

    // Never call conversion methods if speed == 0
    if ((speed == 0) || (speed <= this->TerminalSpeed))
    {
      break;
    }

    memcpy(point1, point2, 3 * sizeof(double));
    // End Integration
  }

  integrator->Delete();
  return stepTaken;
}

//---------------------------------------------------------------------------
int svtkStreamTracer::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataObject");
  }
  else if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return 1;
}

//---------------------------------------------------------------------------
void svtkStreamTracer::AddCustomTerminationCallback(
  CustomTerminationCallbackType callback, void* clientdata, int reasonForTermination)
{
  this->CustomTerminationCallback.push_back(callback);
  this->CustomTerminationClientData.push_back(clientdata);
  this->CustomReasonForTermination.push_back(reasonForTermination);
  this->Modified();
}

//---------------------------------------------------------------------------
void svtkStreamTracer::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Start position: " << this->StartPosition[0] << " " << this->StartPosition[1]
     << " " << this->StartPosition[2] << endl;
  os << indent << "Terminal speed: " << this->TerminalSpeed << endl;

  os << indent << "Maximum propagation: " << this->MaximumPropagation << " unit: length." << endl;

  os << indent << "Integration step unit: "
     << ((this->IntegrationStepUnit == LENGTH_UNIT) ? "length." : "cell length.") << endl;

  os << indent << "Initial integration step: " << this->InitialIntegrationStep << endl;

  os << indent << "Minimum integration step: " << this->MinimumIntegrationStep << endl;

  os << indent << "Maximum integration step: " << this->MaximumIntegrationStep << endl;

  os << indent << "Integration direction: ";
  switch (this->IntegrationDirection)
  {
    case FORWARD:
      os << "forward.";
      break;
    case BACKWARD:
      os << "backward.";
      break;
    case BOTH:
      os << "both directions.";
      break;
  }
  os << endl;

  os << indent << "Integrator: " << this->Integrator << endl;
  os << indent << "Maximum error: " << this->MaximumError << endl;
  os << indent << "Maximum number of steps: " << this->MaximumNumberOfSteps << endl;
  os << indent << "Vorticity computation: " << (this->ComputeVorticity ? " On" : " Off") << endl;
  os << indent << "Rotation scale: " << this->RotationScale << endl;
}

//---------------------------------------------------------------------------
svtkExecutive* svtkStreamTracer::CreateDefaultExecutive()
{
  return svtkCompositeDataPipeline::New();
}
