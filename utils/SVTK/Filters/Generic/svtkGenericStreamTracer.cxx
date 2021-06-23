/*=========================================================================

  Program:   Visualization Toolkits
  Module:    svtkGenericStreamTracer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkGenericStreamTracer.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDoubleArray.h"
#include "svtkGenericAdaptorCell.h"
#include "svtkGenericAttribute.h"
#include "svtkGenericAttributeCollection.h"
#include "svtkGenericDataSet.h"
#include "svtkGenericInterpolatedVelocityField.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkPolyLine.h"
#include "svtkRungeKutta2.h"
#include "svtkRungeKutta4.h"
#include "svtkRungeKutta45.h"
#include <cassert>

#include "svtkExecutive.h" // for GetExecutive()
#include "svtkInformation.h"
#include "svtkInformationVector.h"

svtkStandardNewMacro(svtkGenericStreamTracer);
svtkCxxSetObjectMacro(svtkGenericStreamTracer, Integrator, svtkInitialValueProblemSolver);
svtkCxxSetObjectMacro(
  svtkGenericStreamTracer, InterpolatorPrototype, svtkGenericInterpolatedVelocityField);

const double svtkGenericStreamTracer::EPSILON = 1.0E-12;

//-----------------------------------------------------------------------------
svtkGenericStreamTracer::svtkGenericStreamTracer()
{
  this->SetNumberOfInputPorts(2);

  this->Integrator = svtkRungeKutta2::New();
  this->IntegrationDirection = FORWARD;
  for (int i = 0; i < 3; i++)
  {
    this->StartPosition[i] = 0.0;
  }
  this->MaximumPropagation.Unit = LENGTH_UNIT;
  this->MaximumPropagation.Interval = 1.0;

  this->MinimumIntegrationStep.Unit = CELL_LENGTH_UNIT;
  this->MinimumIntegrationStep.Interval = 1.0E-2;

  this->MaximumIntegrationStep.Unit = CELL_LENGTH_UNIT;
  this->MaximumIntegrationStep.Interval = 1.0;

  this->InitialIntegrationStep.Unit = CELL_LENGTH_UNIT;
  this->InitialIntegrationStep.Interval = 0.5;

  this->MaximumError = 1.0e-6;

  this->MaximumNumberOfSteps = 2000;

  this->TerminalSpeed = EPSILON;

  this->ComputeVorticity = 1;
  this->RotationScale = 1.0;

  this->InputVectorsSelection = nullptr;

  this->LastUsedTimeStep = 0.0;

  this->GenerateNormalsInIntegrate = 1;

  this->InterpolatorPrototype = nullptr;
}

//-----------------------------------------------------------------------------
svtkGenericStreamTracer::~svtkGenericStreamTracer()
{
  this->SetIntegrator(nullptr);
  this->SetInputVectorsSelection(nullptr);
  this->SetInterpolatorPrototype(nullptr);
}

//-----------------------------------------------------------------------------
void svtkGenericStreamTracer::SetSourceData(svtkDataSet* source)
{
  this->SetInputDataInternal(1, source);
}

//-----------------------------------------------------------------------------
svtkDataSet* svtkGenericStreamTracer::GetSource()
{
  if (this->GetNumberOfInputConnections(1) < 1) // because the port is optional
  {
    return nullptr;
  }
  return static_cast<svtkDataSet*>(this->GetExecutive()->GetInputData(1, 0));
}

//-----------------------------------------------------------------------------
void svtkGenericStreamTracer::AddInputData(svtkGenericDataSet* input)
{
  this->Superclass::AddInputData(input);
}

//----------------------------------------------------------------------------
int svtkGenericStreamTracer ::FillInputPortInformation(int port, svtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  else
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGenericDataSet");
  }
  return 1;
}

//-----------------------------------------------------------------------------
int svtkGenericStreamTracer::GetIntegratorType()
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

//-----------------------------------------------------------------------------
void svtkGenericStreamTracer::SetIntegratorType(int type)
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

//-----------------------------------------------------------------------------
void svtkGenericStreamTracer::SetIntervalInformation(
  int unit, svtkGenericStreamTracer::IntervalInformation& currentValues)
{
  if (unit == currentValues.Unit)
  {
    return;
  }

  if ((unit < TIME_UNIT) || (unit > CELL_LENGTH_UNIT))
  {
    svtkWarningMacro("Unrecognized unit. Using TIME_UNIT instead.");
    currentValues.Unit = TIME_UNIT;
  }
  else
  {
    currentValues.Unit = unit;
  }

  this->Modified();
}

//-----------------------------------------------------------------------------
void svtkGenericStreamTracer::SetIntervalInformation(
  int unit, double interval, svtkGenericStreamTracer::IntervalInformation& currentValues)
{
  if ((unit == currentValues.Unit) && (interval == currentValues.Interval))
  {
    return;
  }

  this->SetIntervalInformation(unit, currentValues);

  currentValues.Interval = interval;
  this->Modified();
}

//-----------------------------------------------------------------------------
void svtkGenericStreamTracer::SetMaximumPropagation(int unit, double max)
{
  this->SetIntervalInformation(unit, max, this->MaximumPropagation);
}
//-----------------------------------------------------------------------------
void svtkGenericStreamTracer::SetMaximumPropagation(double max)
{
  if (max == this->MaximumPropagation.Interval)
  {
    return;
  }
  this->MaximumPropagation.Interval = max;
  this->Modified();
}
//-----------------------------------------------------------------------------
void svtkGenericStreamTracer::SetMaximumPropagationUnit(int unit)
{
  this->SetIntervalInformation(unit, this->MaximumPropagation);
}
//-----------------------------------------------------------------------------
int svtkGenericStreamTracer::GetMaximumPropagationUnit()
{
  return this->MaximumPropagation.Unit;
}
//-----------------------------------------------------------------------------
double svtkGenericStreamTracer::GetMaximumPropagation()
{
  return this->MaximumPropagation.Interval;
}

//-----------------------------------------------------------------------------
void svtkGenericStreamTracer::SetMinimumIntegrationStep(int unit, double step)
{
  this->SetIntervalInformation(unit, step, this->MinimumIntegrationStep);
}
//-----------------------------------------------------------------------------
void svtkGenericStreamTracer::SetMinimumIntegrationStepUnit(int unit)
{
  this->SetIntervalInformation(unit, this->MinimumIntegrationStep);
}
//-----------------------------------------------------------------------------
void svtkGenericStreamTracer::SetMinimumIntegrationStep(double step)
{
  if (step == this->MinimumIntegrationStep.Interval)
  {
    return;
  }
  this->MinimumIntegrationStep.Interval = step;
  this->Modified();
}
//-----------------------------------------------------------------------------
int svtkGenericStreamTracer::GetMinimumIntegrationStepUnit()
{
  return this->MinimumIntegrationStep.Unit;
}
//-----------------------------------------------------------------------------
double svtkGenericStreamTracer::GetMinimumIntegrationStep()
{
  return this->MinimumIntegrationStep.Interval;
}

//-----------------------------------------------------------------------------
void svtkGenericStreamTracer::SetMaximumIntegrationStep(int unit, double step)
{
  this->SetIntervalInformation(unit, step, this->MaximumIntegrationStep);
}
//-----------------------------------------------------------------------------
void svtkGenericStreamTracer::SetMaximumIntegrationStepUnit(int unit)
{
  this->SetIntervalInformation(unit, this->MaximumIntegrationStep);
}
//-----------------------------------------------------------------------------
void svtkGenericStreamTracer::SetMaximumIntegrationStep(double step)
{
  if (step == this->MaximumIntegrationStep.Interval)
  {
    return;
  }
  this->MaximumIntegrationStep.Interval = step;
  this->Modified();
}
//-----------------------------------------------------------------------------
int svtkGenericStreamTracer::GetMaximumIntegrationStepUnit()
{
  return this->MaximumIntegrationStep.Unit;
}
//-----------------------------------------------------------------------------
double svtkGenericStreamTracer::GetMaximumIntegrationStep()
{
  return this->MaximumIntegrationStep.Interval;
}

//-----------------------------------------------------------------------------
void svtkGenericStreamTracer::SetInitialIntegrationStep(int unit, double step)
{
  this->SetIntervalInformation(unit, step, this->InitialIntegrationStep);
}
//-----------------------------------------------------------------------------
void svtkGenericStreamTracer::SetInitialIntegrationStepUnit(int unit)
{
  this->SetIntervalInformation(unit, this->InitialIntegrationStep);
}
//-----------------------------------------------------------------------------
void svtkGenericStreamTracer::SetInitialIntegrationStep(double step)
{
  if (step == this->InitialIntegrationStep.Interval)
  {
    return;
  }
  this->InitialIntegrationStep.Interval = step;
  this->Modified();
}
//-----------------------------------------------------------------------------
int svtkGenericStreamTracer::GetInitialIntegrationStepUnit()
{
  return this->InitialIntegrationStep.Unit;
}
//-----------------------------------------------------------------------------
double svtkGenericStreamTracer::GetInitialIntegrationStep()
{
  return this->InitialIntegrationStep.Interval;
}

//-----------------------------------------------------------------------------
double svtkGenericStreamTracer::ConvertToTime(
  svtkGenericStreamTracer::IntervalInformation& interval, double cellLength, double speed)
{
  double retVal = 0.0;
  switch (interval.Unit)
  {
    case TIME_UNIT:
      retVal = interval.Interval;
      break;
    case LENGTH_UNIT:
      retVal = interval.Interval / speed;
      break;
    case CELL_LENGTH_UNIT:
      retVal = interval.Interval * cellLength / speed;
      break;
  }
  return retVal;
}

//-----------------------------------------------------------------------------
double svtkGenericStreamTracer::ConvertToLength(
  svtkGenericStreamTracer::IntervalInformation& interval, double cellLength, double speed)
{
  double retVal = 0.0;
  switch (interval.Unit)
  {
    case TIME_UNIT:
      retVal = interval.Interval * speed;
      break;
    case LENGTH_UNIT:
      retVal = interval.Interval;
      break;
    case CELL_LENGTH_UNIT:
      retVal = interval.Interval * cellLength;
      break;
  }
  return retVal;
}

//-----------------------------------------------------------------------------
double svtkGenericStreamTracer::ConvertToCellLength(
  svtkGenericStreamTracer::IntervalInformation& interval, double cellLength, double speed)
{
  double retVal = 0.0;
  switch (interval.Unit)
  {
    case TIME_UNIT:
      retVal = (interval.Interval * speed) / cellLength;
      break;
    case LENGTH_UNIT:
      retVal = interval.Interval / cellLength;
      break;
    case CELL_LENGTH_UNIT:
      retVal = interval.Interval;
      break;
  }
  return retVal;
}

//-----------------------------------------------------------------------------
double svtkGenericStreamTracer::ConvertToUnit(
  svtkGenericStreamTracer::IntervalInformation& interval, int unit, double cellLength, double speed)
{
  double retVal = 0.0;
  switch (unit)
  {
    case TIME_UNIT:
      retVal = ConvertToTime(interval, cellLength, speed);
      break;
    case LENGTH_UNIT:
      retVal = ConvertToLength(interval, cellLength, speed);
      break;
    case CELL_LENGTH_UNIT:
      retVal = ConvertToCellLength(interval, cellLength, speed);
      break;
  }
  return retVal;
}

//-----------------------------------------------------------------------------
void svtkGenericStreamTracer::ConvertIntervals(
  double& step, double& minStep, double& maxStep, int direction, double cellLength, double speed)
{
  step = direction * this->ConvertToTime(this->InitialIntegrationStep, cellLength, speed);
  if (this->MinimumIntegrationStep.Interval <= 0.0)
  {
    minStep = step;
  }
  else
  {
    minStep = this->ConvertToTime(this->MinimumIntegrationStep, cellLength, speed);
  }
  if (this->MaximumIntegrationStep.Interval <= 0.0)
  {
    maxStep = step;
  }
  else
  {
    maxStep = this->ConvertToTime(this->MaximumIntegrationStep, cellLength, speed);
  }
}

//-----------------------------------------------------------------------------
void svtkGenericStreamTracer::CalculateVorticity(svtkGenericAdaptorCell* cell, double pcoords[3],
  svtkGenericAttribute* attribute, double vorticity[3])
{
  assert("pre: attribute_exists" && attribute != nullptr);
  assert("pre: point_centered_attribute" && attribute->GetCentering() == svtkPointCentered);
  assert("pre: vector_attribute" && attribute->GetType() == svtkDataSetAttributes::VECTORS);

  double derivs[9];
  cell->Derivatives(0, pcoords, attribute, derivs);

  vorticity[0] = derivs[7] - derivs[5];
  vorticity[1] = derivs[2] - derivs[6];
  vorticity[2] = derivs[3] - derivs[1];
}

//-----------------------------------------------------------------------------
void svtkGenericStreamTracer::InitializeSeeds(
  svtkDataArray*& seeds, svtkIdList*& seedIds, svtkIntArray*& integrationDirections)
{
  svtkDataSet* source = this->GetSource();
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

//-----------------------------------------------------------------------------
int svtkGenericStreamTracer::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkGenericDataSet* input =
    svtkGenericDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkDataArray* seeds = nullptr;
  svtkIdList* seedIds = nullptr;
  svtkIntArray* integrationDirections = nullptr;
  this->InitializeSeeds(seeds, seedIds, integrationDirections);

  if (seeds)
  {
    double lastPoint[3];
    svtkGenericInterpolatedVelocityField* func;
    if (this->CheckInputs(func, inputVector) != SVTK_OK)
    {
      svtkDebugMacro("No appropriate inputs have been found. Can not execute.");
      func->Delete();
      seeds->Delete();
      integrationDirections->Delete();
      seedIds->Delete();
      return 1;
    }
    this->Integrate(input, output, seeds, seedIds, integrationDirections, lastPoint, func);
    func->Delete();
    seeds->Delete();
  }

  integrationDirections->Delete();
  seedIds->Delete();
  return 1;
}

//-----------------------------------------------------------------------------
int svtkGenericStreamTracer::CheckInputs(
  svtkGenericInterpolatedVelocityField*& func, svtkInformationVector** inputVector)
{
  // Set the function set to be integrated
  if (!this->InterpolatorPrototype)
  {
    func = svtkGenericInterpolatedVelocityField::New();
  }
  else
  {
    func = this->InterpolatorPrototype->NewInstance();
    func->CopyParameters(this->InterpolatorPrototype);
  }
  func->SelectVectors(this->InputVectorsSelection);

  // Add all the inputs ( except source, of course ) which
  // have the appropriate vectors and compute the maximum
  // cell size.
  int numInputs = 0;
  int numInputConnections = this->GetNumberOfInputConnections(0);
  for (int i = 0; i < numInputConnections; i++)
  {
    svtkInformation* info = inputVector[0]->GetInformationObject(i);
    svtkGenericDataSet* inp = nullptr;

    if (info != nullptr)
    {
      inp = svtkGenericDataSet::SafeDownCast(info->Get(svtkDataObject::DATA_OBJECT()));
    }
    if (inp != nullptr)
    {
      int attrib;
      int attributeFound;
      if (this->InputVectorsSelection != nullptr)
      {
        attrib = inp->GetAttributes()->FindAttribute(this->InputVectorsSelection);

        attributeFound = attrib >= 0;
        if (attributeFound)
        {
          attributeFound = (inp->GetAttributes()->GetAttribute(attrib)->GetType() ==
                             svtkDataSetAttributes::VECTORS) &&
            (inp->GetAttributes()->GetAttribute(attrib)->GetCentering() == svtkPointCentered);
        }
      }
      else
      {
        // Find the first attribute, point centered and with vector type.
        attrib = 0;
        attributeFound = 0;
        int c = inp->GetAttributes()->GetNumberOfAttributes();
        while (attrib < c && !attributeFound)
        {
          attributeFound = (inp->GetAttributes()->GetAttribute(attrib)->GetType() ==
                             svtkDataSetAttributes::VECTORS) &&
            (inp->GetAttributes()->GetAttribute(attrib)->GetCentering() == svtkPointCentered);
          ++attrib;
        }
        if (attributeFound)
        {
          --attrib;
          this->SetInputVectorsSelection(inp->GetAttributes()->GetAttribute(attrib)->GetName());
        }
      }
      if (!attributeFound)
      {
        svtkDebugMacro("Input " << i << "does not contain a velocity vector.");
        continue;
      }
      func->AddDataSet(inp);
      numInputs++;
    }
  }
  if (numInputs == 0)
  {
    svtkDebugMacro("No appropriate inputs have been found. Can not execute.");
    return SVTK_ERROR;
  }
  return SVTK_OK;
}

//-----------------------------------------------------------------------------
void svtkGenericStreamTracer::Integrate(svtkGenericDataSet* input0, svtkPolyData* output,
  svtkDataArray* seedSource, svtkIdList* seedIds, svtkIntArray* integrationDirections,
  double lastPoint[3], svtkGenericInterpolatedVelocityField* func)
{
  int i;
  svtkIdType numLines = seedIds->GetNumberOfIds();

  // Useful pointers
  svtkDataSetAttributes* outputPD = output->GetPointData();
  svtkDataSetAttributes* outputCD = output->GetCellData();
  svtkGenericDataSet* input;
  svtkGenericAttribute* inVectors;

  int direction = 1;

  if (this->GetIntegrator() == nullptr)
  {
    svtkErrorMacro("No integrator is specified.");
    return;
  }

  // Used in GetCell()
  //  svtkGenericCell* cell = svtkGenericCell::New();
  svtkGenericAdaptorCell* cell = nullptr;

  // Create a new integrator, the type is the same as Integrator
  svtkInitialValueProblemSolver* integrator = this->GetIntegrator()->NewInstance();
  integrator->SetFunctionSet(func);

  // Since we do not know what the total number of points
  // will be, we do not allocate any. This is important for
  // cases where a lot of streamers are used at once. If we
  // were to allocate any points here, potentially, we can
  // waste a lot of memory if a lot of streamers are used.
  // Always insert the first point
  svtkPoints* outputPoints = svtkPoints::New();
  svtkCellArray* outputLines = svtkCellArray::New();

  // We will keep track of time in this array
  svtkDoubleArray* time = svtkDoubleArray::New();
  time->SetName("IntegrationTime");

  // This array explains why the integration stopped
  svtkIntArray* retVals = svtkIntArray::New();
  retVals->SetName("ReasonForTermination");

  svtkDoubleArray* vorticity = nullptr;
  svtkDoubleArray* rotation = nullptr;
  svtkDoubleArray* angularVel = nullptr;
  if (this->ComputeVorticity)
  {
    vorticity = svtkDoubleArray::New();
    vorticity->SetName("Vorticity");
    vorticity->SetNumberOfComponents(3);

    rotation = svtkDoubleArray::New();
    rotation->SetName("Rotation");

    angularVel = svtkDoubleArray::New();
    angularVel->SetName("AngularVelocity");
  }

  // We will interpolate all point attributes of the input on
  // each point of the output (unless they are turned off)
  // Note that we are using only the first input, if there are more
  // than one, the attributes have to match.

  // prepare the output attributes
  svtkGenericAttributeCollection* attributes = input0->GetAttributes();
  svtkGenericAttribute* attribute;
  svtkDataArray* attributeArray;

  int c = attributes->GetNumberOfAttributes();
  int attributeType;

  // Only point centered attributes will be interpolated.
  // Cell centered attributes are not ignored and not copied in output:
  // is a missing part in svtkStreamTracer? Need to ask to the Berk.
  for (i = 0; i < c; ++i)
  {
    attribute = attributes->GetAttribute(i);
    attributeType = attribute->GetType();
    if (attribute->GetCentering() == svtkPointCentered)
    {
      attributeArray = svtkDataArray::CreateDataArray(attribute->GetComponentType());
      attributeArray->SetNumberOfComponents(attribute->GetNumberOfComponents());
      attributeArray->SetName(attribute->GetName());
      outputPD->AddArray(attributeArray);
      attributeArray->Delete();

      if (outputPD->GetAttribute(attributeType) == nullptr)
      {
        outputPD->SetActiveAttribute(outputPD->GetNumberOfArrays() - 1, attributeType);
      }
    }
  }
  double* values =
    new double[outputPD->GetNumberOfComponents()]; // point centered attributes at some point.

  // Note:  It is an overestimation to have the estimate the same number of
  // output points and input points.  We sill have to squeeze at end.

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
    func->ClearLastCell();

    // Initial point
    seedSource->GetTuple(seedIds->GetId(currentLine), point1);
    memcpy(point2, point1, 3 * sizeof(double));
    if (!func->FunctionValues(point1, velocity))
    {
      continue;
    }

    numPts++;
    numPtsTotal++;
    svtkIdType nextPoint = outputPoints->InsertNextPoint(point1);
    time->InsertNextValue(0.0);

    // We will always pass a time step to the integrator.
    // If the user specifies a step size with another unit, we will
    // have to convert it to time.
    IntervalInformation delT;
    delT.Unit = TIME_UNIT;
    delT.Interval = 0;
    IntervalInformation aStep;
    aStep.Unit = this->MaximumPropagation.Unit;
    double propagation = 0.0, step, minStep = 0, maxStep = 0;
    double stepTaken, accumTime = 0;
    double speed;
    double cellLength;
    int retVal = OUT_OF_TIME, tmp;

    // Make sure we use the dataset found
    // by the svtkGenericInterpolatedVelocityField
    input = func->GetLastDataSet();

    inVectors = input->GetAttributes()->GetAttribute(
      input->GetAttributes()->FindAttribute(this->InputVectorsSelection));

    // Convert intervals to time unit
    cell = func->GetLastCell();
    cellLength = sqrt(static_cast<double>(cell->GetLength2()));
    speed = svtkMath::Norm(velocity);

    // Never call conversion methods if speed == 0
    if (speed != 0.0)
    {
      this->ConvertIntervals(delT.Interval, minStep, maxStep, direction, cellLength, speed);
    }

    // Interpolate all point attributes on first point
    func->GetLastLocalCoordinates(pcoords);
    cell->InterpolateTuple(input->GetAttributes(), pcoords, values);

    double* p = values;
    svtkDataArray* dataArray;
    c = outputPD->GetNumberOfArrays();
    int j;
    for (j = 0; j < c; ++j)
    {
      dataArray = outputPD->GetArray(j);
      dataArray->InsertTuple(nextPoint, p);
      p += dataArray->GetNumberOfComponents();
    }

    // Compute vorticity if required
    // This can be used later for streamribbon generation.
    if (this->ComputeVorticity)
    {
      // Here, we're assuming a linear cell by only taken values at
      // corner points. there should be a subdivision step here instead.
      // What is the criterium to stop the subdivision?
      // Note: the original svtkStreamTracer is taking cell points, it means
      // that for the quadratic cell, the standard stream tracer is more
      // accurate than this one!
      svtkGenericStreamTracer::CalculateVorticity(cell, pcoords, inVectors, vort);

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

    svtkIdType numSteps = 0;
    double error = 0;
    // Integrate until the maximum propagation length is reached,
    // maximum number of steps is reached or until a boundary is encountered.
    // Begin Integration
    while (propagation < this->MaximumPropagation.Interval)
    {

      if (numSteps > this->MaximumNumberOfSteps)
      {
        retVal = OUT_OF_STEPS;
        break;
      }

      if (numSteps++ % 1000 == 1)
      {
        progress = (currentLine + propagation / this->MaximumPropagation.Interval) / numLines;
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
      aStep.Interval =
        fabs(this->ConvertToUnit(delT, this->MaximumPropagation.Unit, cellLength, speed));
      if ((propagation + aStep.Interval) > this->MaximumPropagation.Interval)
      {
        aStep.Interval = this->MaximumPropagation.Interval - propagation;
        if (delT.Interval >= 0)
        {
          delT.Interval = this->ConvertToTime(aStep, cellLength, speed);
        }
        else
        {
          delT.Interval = -1.0 * this->ConvertToTime(aStep, cellLength, speed);
        }
        maxStep = delT.Interval;
      }
      this->LastUsedTimeStep = delT.Interval;

      // Calculate the next step using the integrator provided
      // Break if the next point is out of bounds.
      if ((tmp = integrator->ComputeNextStep(point1, point2, 0, delT.Interval, stepTaken, minStep,
             maxStep, this->MaximumError, error)) != 0)
      {
        retVal = tmp;
        memcpy(lastPoint, point2, 3 * sizeof(double));
        break;
      }

      accumTime += stepTaken;
      // Calculate propagation (using the same units as MaximumPropagation
      propagation +=
        fabs(this->ConvertToUnit(delT, this->MaximumPropagation.Unit, cellLength, speed));

      // This is the next starting point
      for (i = 0; i < 3; i++)
      {
        point1[i] = point2[i];
      }

      // Interpolate the velocity at the next point
      if (!func->FunctionValues(point2, velocity))
      {
        retVal = OUT_OF_DOMAIN;
        memcpy(lastPoint, point2, 3 * sizeof(double));
        break;
      }

      // Make sure we use the dataset found by the svtkInterpolatedVelocityField
      input = func->GetLastDataSet();

      inVectors = input->GetAttributes()->GetAttribute(
        input->GetAttributes()->FindAttribute(this->InputVectorsSelection));

      // Point is valid. Insert it.
      numPts++;
      numPtsTotal++;
      nextPoint = outputPoints->InsertNextPoint(point1);
      time->InsertNextValue(accumTime);

      // Calculate cell length and speed to be used in unit conversions
      cell = func->GetLastCell();
      cellLength = sqrt(static_cast<double>(cell->GetLength2()));

      speed = svtkMath::Norm(velocity);

      // Interpolate all point attributes on current point
      func->GetLastLocalCoordinates(pcoords);
      cell->InterpolateTuple(input->GetAttributes(), pcoords, values);

      p = values;
      c = outputPD->GetNumberOfArrays();
      for (j = 0; j < c; ++j)
      {
        dataArray = outputPD->GetArray(j);
        dataArray->InsertTuple(nextPoint, p);
        p += dataArray->GetNumberOfComponents();
      }

      // Compute vorticity if required
      // This can be used later for streamribbon generation.
      if (this->ComputeVorticity)
      {
        svtkGenericStreamTracer::CalculateVorticity(cell, pcoords, inVectors, vort);

        vorticity->InsertNextTuple(vort);
        // rotation
        // angular velocity = vorticity . unit tangent ( i.e. velocity/speed )
        // rotation = sum ( angular velocity * delT )
        omega = svtkMath::Dot(vort, velocity);
        omega /= speed;
        omega *= this->RotationScale;
        index = angularVel->InsertNextValue(omega);
        rotation->InsertNextValue(rotation->GetValue(index - 1) +
          (angularVel->GetValue(index - 1) + omega) / 2 * (accumTime - time->GetValue(index - 1)));
      }

      // Never call conversion methods if speed == 0
      if ((speed == 0) || (speed <= this->TerminalSpeed))
      {
        retVal = STAGNATION;
        break;
      }

      // Convert all intervals to time
      this->ConvertIntervals(step, minStep, maxStep, direction, cellLength, speed);

      // If the solver is adaptive and the next time step (delT.Interval)
      // that the solver wants to use is smaller than minStep or larger
      // than maxStep, re-adjust it. This has to be done every step
      // because minStep and maxStep can change depending on the cell
      // size (unless it is specified in time units)
      if (integrator->IsAdaptive())
      {
        if (fabs(delT.Interval) < fabs(minStep))
        {
          delT.Interval = fabs(minStep) * delT.Interval / fabs(delT.Interval);
        }
        else if (fabs(delT.Interval) > fabs(maxStep))
        {
          delT.Interval = fabs(maxStep) * delT.Interval / fabs(delT.Interval);
        }
      }
      else
      {
        delT.Interval = step;
      }

      // End Integration
    }

    if (shouldAbort)
    {
      break;
    }

    if (numPts > 1)
    {
      outputLines->InsertNextCell(numPts);
      for (i = numPtsTotal - numPts; i < numPtsTotal; i++)
      {
        outputLines->InsertCellPoint(i);
      }
      retVals->InsertNextValue(retVal);
    }
  }

  if (!shouldAbort)
  {
    // Create the output polyline
    output->SetPoints(outputPoints);
    outputPD->AddArray(time);
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
        this->GenerateNormals(output, nullptr);
      }

      outputCD->AddArray(retVals);
    }
  }

  if (vorticity)
  {
    vorticity->Delete();
    rotation->Delete();
    angularVel->Delete();
  }

  retVals->Delete();

  outputPoints->Delete();
  outputLines->Delete();

  time->Delete();

  integrator->Delete();

  delete[] values;

  output->Squeeze();
}

//-----------------------------------------------------------------------------
void svtkGenericStreamTracer::GenerateNormals(svtkPolyData* output, double* firstNormal)
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

      lineNormalGenerator->GenerateSlidingNormals(outputPoints, outputLines, normals, firstNormal);
      lineNormalGenerator->Delete();

      int i, j;
      double normal[3], local1[3], local2[3], theta, costheta, sintheta, length;
      double velocity[3];
      normals->SetName("Normals");
      svtkDataArray* newVectors = outputPD->GetVectors(this->InputVectorsSelection);
      for (i = 0; i < numPts; i++)
      {
        normals->GetTuple(i, normal);
        if (newVectors == nullptr)
        { // This should never happen.
          svtkErrorMacro("Could not find output array.");
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

//-----------------------------------------------------------------------------
// This is used by sub-classes in certain situations. It
// does a lot less (for example, does not compute attributes)
// than Integrate.
void svtkGenericStreamTracer::SimpleIntegrate(
  double seed[3], double lastPoint[3], double delt, svtkGenericInterpolatedVelocityField* func)
{
  svtkIdType numSteps = 0;
  svtkIdType maxSteps = 20;
  double error = 0;
  double stepTaken;
  double point1[3], point2[3];
  double velocity[3];
  double speed;

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
    if (integrator->ComputeNextStep(point1, point2, 0, delt, stepTaken, 0, 0, 0, error) != 0)
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
}

//-----------------------------------------------------------------------------
void svtkGenericStreamTracer::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Start position: " << this->StartPosition[0] << " " << this->StartPosition[1]
     << " " << this->StartPosition[2] << endl;
  os << indent << "Terminal speed: " << this->TerminalSpeed << endl;
  os << indent << "Maximum propagation: " << this->MaximumPropagation.Interval << " unit: ";
  switch (this->MaximumPropagation.Unit)
  {
    case TIME_UNIT:
      os << "time.";
      break;
    case LENGTH_UNIT:
      os << "length.";
      break;
    case CELL_LENGTH_UNIT:
      os << "cell length.";
      break;
  }
  os << endl;

  os << indent << "Min. integration step: " << this->MinimumIntegrationStep.Interval << " unit: ";
  switch (this->MinimumIntegrationStep.Unit)
  {
    case TIME_UNIT:
      os << "time.";
      break;
    case LENGTH_UNIT:
      os << "length.";
      break;
    case CELL_LENGTH_UNIT:
      os << "cell length.";
      break;
  }
  os << endl;

  os << indent << "Max. integration step: " << this->MaximumIntegrationStep.Interval << " unit: ";
  switch (this->MaximumIntegrationStep.Unit)
  {
    case TIME_UNIT:
      os << "time.";
      break;
    case LENGTH_UNIT:
      os << "length.";
      break;
    case CELL_LENGTH_UNIT:
      os << "cell length.";
      break;
  }
  os << endl;

  os << indent << "Initial integration step: " << this->InitialIntegrationStep.Interval
     << " unit: ";
  switch (this->InitialIntegrationStep.Unit)
  {
    case TIME_UNIT:
      os << "time.";
      break;
    case LENGTH_UNIT:
      os << "length.";
      break;
    case CELL_LENGTH_UNIT:
      os << "cell length.";
      break;
  }
  os << endl;

  os << indent << "Integration direction: ";
  switch (this->IntegrationDirection)
  {
    case FORWARD:
      os << "forward.";
      break;
    case BACKWARD:
      os << "backward.";
      break;
  }
  os << endl;

  os << indent << "Integrator: " << this->Integrator << endl;
  os << indent << "Maximum error: " << this->MaximumError << endl;
  os << indent << "Max. number of steps: " << this->MaximumNumberOfSteps << endl;
  os << indent << "Vorticity computation: " << (this->ComputeVorticity ? " On" : " Off") << endl;
  os << indent << "Rotation scale: " << this->RotationScale << endl;

  if (this->InputVectorsSelection)
  {
    os << indent << "InputVectorsSelection: " << this->InputVectorsSelection;
  }
}
//-----------------------------------------------------------------------------
void svtkGenericStreamTracer::SetSourceConnection(svtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}
