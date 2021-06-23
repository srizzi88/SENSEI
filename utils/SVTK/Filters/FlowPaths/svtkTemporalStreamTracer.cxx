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
#include "svtkMath.h"
#include "svtkMultiBlockDataSet.h"
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
#include <cassert>

#ifdef _WIN32
#undef JB_H5PART_PARTICLE_OUTPUT
#else
//  #define JB_H5PART_PARTICLE_OUTPUT
#endif

#ifdef JB_H5PART_PARTICLE_OUTPUT
// #include "svtkH5PartWriter.h"
#include "svtkXMLParticleWriter.h"
#endif

#include <algorithm>
#include <functional>

using namespace svtkTemporalStreamTracerNamespace;

// The 3D cell with the maximum number of points is SVTK_LAGRANGE_HEXAHEDRON.
// We support up to 6th order hexahedra.
#define SVTK_MAXIMUM_NUMBER_OF_POINTS 216

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//#define JB_DEBUG__
#if defined JB_DEBUG__
#ifdef _WIN32
#define OUTPUTTEXT(a) svtkOutputWindowDisplayText(a);
#else
#endif

#undef svtkDebugMacro
#define svtkDebugMacro(a)                                                                           \
  {                                                                                                \
    svtkOStreamWrapper::EndlType endl;                                                              \
    svtkOStreamWrapper::UseEndl(endl);                                                              \
    svtkOStrStreamWrapper svtkmsg;                                                                   \
    svtkmsg << "P(" << this->UpdatePieceId << "): " a << "\n";                                      \
    OUTPUTTEXT(svtkmsg.str());                                                                      \
    svtkmsg.rdbuf()->freeze(0);                                                                     \
  }

#undef svtkErrorMacro
#define svtkErrorMacro(a) svtkDebugMacro(a)

#endif
//---------------------------------------------------------------------------
svtkStandardNewMacro(svtkTemporalStreamTracer);
svtkCxxSetObjectMacro(svtkTemporalStreamTracer, ParticleWriter, svtkAbstractParticleWriter);
//---------------------------------------------------------------------------
svtkTemporalStreamTracer::svtkTemporalStreamTracer()
{
  this->IntegrationDirection = FORWARD;
  this->TimeStep = 0;
  this->ActualTimeStep = 0;
  this->NumberOfInputTimeSteps = 0;
  this->ForceReinjectionEveryNSteps = 1;
  this->ReinjectionFlag = 0;
  this->ReinjectionCounter = 0;
  this->UpdatePieceId = 0;
  this->UpdateNumPieces = 0;
  this->AllFixedGeometry = 1;
  this->StaticMesh = 1;
  this->StaticSeeds = 1;
  this->ComputeVorticity = 1;
  this->IgnorePipelineTime = 0;
  this->ParticleWriter = nullptr;
  this->ParticleFileName = nullptr;
  this->EnableParticleWriting = false;
  this->UniqueIdCounter = 0;
  this->UniqueIdCounterMPI = 0;
  this->InterpolationCount = 0;
  //
  this->NumberOfParticles = 0;
  this->TimeStepResolution = 1.0;
  this->TerminationTime = 0.0;
  this->TerminationTimeUnit = TERMINATION_STEP_UNIT;
  this->EarliestTime = -1E6;
  // we are not actually using these for now

  this->MaximumPropagation = 1.0;

  this->IntegrationStepUnit = LENGTH_UNIT;
  this->MinimumIntegrationStep = 1.0E-2;
  this->MaximumIntegrationStep = 1.0;
  this->InitialIntegrationStep = 0.5;
  //
  this->Interpolator = svtkSmartPointer<svtkTemporalInterpolatedVelocityField>::New();
  //
  this->SetNumberOfInputPorts(2);

#ifdef JB_H5PART_PARTICLE_OUTPUT
#ifdef _WIN32
  svtkDebugMacro(<< "Setting svtkH5PartWriter");
  svtkH5PartWriter* writer = svtkH5PartWriter::New();
#else
  svtkDebugMacro(<< "Setting svtkXMLParticleWriter");
  svtkXMLParticleWriter* writer = svtkXMLParticleWriter::New();
#endif
  this->SetParticleWriter(writer);
  writer->Delete();
#endif

  this->SetIntegratorType(RUNGE_KUTTA4);
  this->RequestIndex = 0;
  SVTK_LEGACY_BODY(svtkTemporalStreamTracer::svtkTemporalStreamTracer, "SVTK 9.0");
}
//---------------------------------------------------------------------------
svtkTemporalStreamTracer::~svtkTemporalStreamTracer()
{
  this->SetParticleWriter(nullptr);
  delete[] this->ParticleFileName;
  this->ParticleFileName = nullptr;
}
//----------------------------------------------------------------------------
int svtkTemporalStreamTracer::FillInputPortInformation(int port, svtkInformation* info)
{
  // port 0 must be a temporal collection of any type
  // the executive should put a temporal collection in when
  // we request multiple time steps.
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataObject");
    info->Set(svtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
    //    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  else if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
    info->Set(svtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
    //    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return 1;
}
//----------------------------------------------------------------------------
void svtkTemporalStreamTracer::AddSourceConnection(svtkAlgorithmOutput* input)
{
  this->AddInputConnection(1, input);
}
//----------------------------------------------------------------------------
void svtkTemporalStreamTracer::RemoveAllSources()
{
  this->SetInputConnection(1, nullptr);
}
//----------------------------------------------------------------------------
svtkTypeBool svtkTemporalStreamTracer::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    return this->RequestInformation(request, inputVector, outputVector);
  }
  if (request->Has(svtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
  }
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->RequestData(request, inputVector, outputVector);
  }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}
//----------------------------------------------------------------------------
int svtkTemporalStreamTracer::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (inInfo->Has(svtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    this->NumberOfInputTimeSteps = inInfo->Length(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
    svtkDebugMacro(<< "svtkTemporalStreamTracer "
                     "inputVector TIME_STEPS "
                  << this->NumberOfInputTimeSteps);
    //
    // Get list of input time step values
    this->InputTimeValues.resize(this->NumberOfInputTimeSteps);
    inInfo->Get(svtkStreamingDemandDrivenPipeline::TIME_STEPS(), &this->InputTimeValues[0]);
    if (this->NumberOfInputTimeSteps == 1)
    {
      svtkErrorMacro(<< "Not enough input time steps for particle integration");
      return 0;
    }
    //
    // We only output T-1 time steps
    //
    this->OutputTimeValues.resize(this->NumberOfInputTimeSteps - 1);
    this->OutputTimeValues.clear();
    this->OutputTimeValues.insert(this->OutputTimeValues.begin(), this->InputTimeValues.begin() + 1,
      this->InputTimeValues.end());
  }
  else
  {
    svtkErrorMacro(<< "Input information has no TIME_STEPS set");
    return 0;
  }

  outInfo->Set(svtkStreamingDemandDrivenPipeline::TIME_STEPS(), &this->OutputTimeValues[0],
    static_cast<int>(this->OutputTimeValues.size()));

  return 1;
}
//----------------------------------------------------------------------------
class WithinTolerance : public std::binary_function<double, double, bool>
{
public:
  result_type operator()(first_argument_type a, second_argument_type b) const
  {
    bool result = (fabs(a - b) <= (a * 1E-6));
    return (result_type)result;
  }
};
//----------------------------------------------------------------------------
int svtkTemporalStreamTracer::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  int numInputs = inputVector[0]->GetNumberOfInformationObjects();
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  //
  // The output has requested a time value, what times must we ask from our input
  //
  if (this->IgnorePipelineTime ||
    !outInfo->Has(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    double requestedTimeValue;
    //
    // ideally we want the output information to be requesting a time step,
    // but since it isn't we must use the SetTimeStep value as a Time request
    //
    if (this->TimeStep < this->OutputTimeValues.size())
    {
      requestedTimeValue = this->OutputTimeValues[this->TimeStep];
    }
    else
    {
      requestedTimeValue = this->OutputTimeValues.back();
    }
    this->ActualTimeStep = this->TimeStep;

    svtkDebugMacro(<< "SetTimeStep       : requestedTimeValue " << requestedTimeValue
                  << " ActualTimeStep " << this->ActualTimeStep);
    (void)requestedTimeValue; // silence unused variable warning
  }
  else
  {
    //
    // Get the requested time step.
    //
    double requestedTimeValue = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    this->ActualTimeStep =
      std::find_if(this->OutputTimeValues.begin(), this->OutputTimeValues.end(),
        std::bind(WithinTolerance(), std::placeholders::_1, requestedTimeValue)) -
      this->OutputTimeValues.begin();
    if (this->ActualTimeStep >= this->OutputTimeValues.size())
    {
      this->ActualTimeStep = 0;
    }
    svtkDebugMacro(<< "UPDATE_TIME_STEPS : requestedTimeValue " << requestedTimeValue
                  << " ActualTimeStep " << this->ActualTimeStep);
  }

  if (this->ActualTimeStep < this->OutputTimeValues.size())
  {
    for (int i = 0; i < numInputs; i++)
    {
      svtkInformation* inInfo = inputVector[0]->GetInformationObject(i);
      // our output timestep T is timestep T+1 in the source
      // so output inputTimeSteps[T], inputTimeSteps[T+1]
      inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(),
        this->InputTimeValues[this->ActualTimeStep + this->RequestIndex]);
      svtkDebugMacro(<< "requested 1 time values : "
                    << this->InputTimeValues[this->ActualTimeStep + this->RequestIndex]);
    }
  }
  else
  {
    svtkDebugMacro(<< "UPDATE_TIME_STEPS : Error getting requested time step");
    return 0;
  }

  return 1;
}
//---------------------------------------------------------------------------
int svtkTemporalStreamTracer::InitializeInterpolator()
{
  if (!this->InputDataT[0] || !this->InputDataT[1])
  {
    return 0;
  }
  //
  // When Multiblock arrays are processed, some may be empty
  // if the first is empty, we won't find the correct vector name
  // so scan until we get one
  //
  svtkSmartPointer<svtkCompositeDataIterator> iterP;
  iterP.TakeReference(this->InputDataT[0]->NewIterator());
  iterP->GoToFirstItem();
  char* vecname = nullptr;
  while (!iterP->IsDoneWithTraversal())
  {
    svtkDataArray* vectors = this->GetInputArrayToProcess(0, iterP->GetCurrentDataObject());
    if (vectors)
    {
      vecname = vectors->GetName();
      break;
    }
    iterP->GoToNextItem();
  }
  if (!vecname)
  {
    svtkDebugMacro(<< "Couldn't find vector array " << vecname);
    return SVTK_ERROR;
  }

  svtkDebugMacro(<< "Interpolator using array " << vecname);
  this->Interpolator->SelectVectors(vecname);

  this->AllFixedGeometry = 1;

  int numValidInputBlocks[2] = { 0, 0 };
  int numTotalInputBlocks[2] = { 0, 0 };
  this->DataReferenceT[0] = this->DataReferenceT[1] = nullptr;
  for (int T = 0; T < 2; T++)
  {
    this->CachedBounds[T].clear();
    int index = 0;
    // iterate over all blocks of input and cache the bounds information
    // and determine fixed/dynamic mesh status.

    svtkSmartPointer<svtkCompositeDataIterator> anotherIterP;
    anotherIterP.TakeReference(this->InputDataT[T]->NewIterator());
    anotherIterP->GoToFirstItem();
    while (!anotherIterP->IsDoneWithTraversal())
    {
      numTotalInputBlocks[T]++;
      svtkDataSet* inp = svtkDataSet::SafeDownCast(anotherIterP->GetCurrentDataObject());
      if (inp)
      {
        if (inp->GetNumberOfCells() == 0)
        {
          svtkDebugMacro("Skipping an empty dataset");
        }
        else if (!inp->GetPointData()->GetVectors(vecname) && inp->GetNumberOfPoints() > 0)
        {
          svtkDebugMacro("One of the input datasets has no velocity vector.");
        }
        else
        {
          // svtkDebugMacro("pass " << i << " Found dataset with " << inp->GetNumberOfCells() << "
          // cells");
          //
          // store the bounding boxes of each local dataset for faster 'point-in-dataset' testing
          //
          bounds bbox;
          inp->ComputeBounds();
          inp->GetBounds(&bbox.b[0]);
          this->CachedBounds[T].push_back(bbox);
          bool static_dataset = (this->StaticMesh != 0);
          this->AllFixedGeometry = this->AllFixedGeometry && static_dataset;
          // add the dataset to the interpolator
          this->Interpolator->SetDataSetAtTime(
            index++, T, this->CurrentTimeSteps[T], inp, static_dataset);
          if (!this->DataReferenceT[T])
          {
            this->DataReferenceT[T] = inp;
          }
          //
          numValidInputBlocks[T]++;
        }
      }
      anotherIterP->GoToNextItem();
    }
  }
  if (numValidInputBlocks[0] == 0 || numValidInputBlocks[1] == 0)
  {
    svtkDebugMacro("Not enough inputs have been found. Can not execute."
      << numValidInputBlocks[0] << " " << numValidInputBlocks[1]);
    return SVTK_ERROR;
  }
  if (numValidInputBlocks[0] != numValidInputBlocks[1])
  {
    svtkDebugMacro("The number of datasets is different between time steps "
      << numValidInputBlocks[0] << " " << numValidInputBlocks[1]);
    return SVTK_ERROR;
  }
  //
  svtkDebugMacro("Number of Valid input blocks is " << numValidInputBlocks[0] << " from "
                                                   << numTotalInputBlocks[0]);
  svtkDebugMacro("AllFixedGeometry " << this->AllFixedGeometry);

  // force optimizations if StaticMesh is set.
  if (this->StaticMesh)
  {
    svtkDebugMacro("Static Mesh optimizations Forced ON");
    this->AllFixedGeometry = 1;
  }

  //
  return SVTK_OK;
}
int svtkTemporalStreamTracer::SetTemporalInput(svtkDataObject* data, int i)
{
  // if not set, create a multiblock dataset to hold all input blocks
  if (!this->InputDataT[i])
  {
    this->InputDataT[i] = svtkSmartPointer<svtkMultiBlockDataSet>::New();
  }
  // if simple dataset, add to our list, otherwise, add blocks
  svtkDataSet* dsInput = svtkDataSet::SafeDownCast(data);
  svtkMultiBlockDataSet* mbInput = svtkMultiBlockDataSet::SafeDownCast(data);

  if (dsInput)
  {
    svtkSmartPointer<svtkDataSet> copy;
    copy.TakeReference(dsInput->NewInstance());
    copy->ShallowCopy(dsInput);
    this->InputDataT[i]->SetBlock(this->InputDataT[i]->GetNumberOfBlocks(), copy);
  }
  else if (mbInput)
  {
    svtkSmartPointer<svtkCompositeDataIterator> iter;
    iter.TakeReference(mbInput->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      svtkDataSet* ds = svtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (ds)
      {
        svtkSmartPointer<svtkDataSet> copy;
        copy.TakeReference(ds->NewInstance());
        copy->ShallowCopy(ds);
        this->InputDataT[i]->SetBlock(this->InputDataT[i]->GetNumberOfBlocks(), copy);
      }
    }
  }
  else
  {
    svtkDebugMacro(
      "This filter cannot handle inputs of type: " << (data ? data->GetClassName() : "(none)"));
    return 0;
  }

  return 1;
}

//---------------------------------------------------------------------------
bool svtkTemporalStreamTracer::InsideBounds(double point[])
{
  double delta[3] = { 0.0, 0.0, 0.0 };
  for (int t = 0; t < 2; ++t)
  {
    for (unsigned int i = 0; i < (this->CachedBounds[t].size()); ++i)
    {
      if (svtkMath::PointIsWithinBounds(point, &((this->CachedBounds[t])[i].b[0]), delta))
        return true;
    }
  }
  return false;
}
//---------------------------------------------------------------------------
void svtkTemporalStreamTracer::TestParticles(
  ParticleVector& candidates, ParticleVector& passed, int& count)
{
  int i = 0;
  int div = static_cast<int>(candidates.size() / 10);
  if (div == 0)
  {
    div = 1;
  }
  count = 0;
  for (ParticleIterator it = candidates.begin(); it != candidates.end(); ++it, ++i)
  {
    ParticleInformation& info = (*it);
    double* pos = &info.CurrentPosition.x[0];
    // if outside bounds, reject instantly
    if (this->InsideBounds(pos))
    {
      if (info.UniqueParticleId == 602)
      {
        svtkDebugMacro(<< "TestParticles got 602");
      }
      // since this is first test, avoid bad cache tests
      this->Interpolator->ClearCache();
      info.LocationState = this->Interpolator->TestPoint(pos);
      if (info.LocationState == ID_OUTSIDE_ALL /*|| location==ID_OUTSIDE_T0*/)
      {
        // can't really use this particle.
        svtkDebugMacro(<< "TestParticles rejected particle");
      }
      else
      {
        // get the cached ids and datasets from the TestPoint call
        this->Interpolator->GetCachedCellIds(info.CachedCellId, info.CachedDataSetId);
        passed.push_back(info);
        count++;
      }
    }
    if (i % div == 0)
    {
      //      svtkDebugMacro(<< "TestParticles " << i);
    }
  }
}
//---------------------------------------------------------------------------
void svtkTemporalStreamTracer::AssignSeedsToProcessors(svtkDataSet* source, int sourceID, int ptId,
  ParticleVector& LocalSeedPoints, int& LocalAssignedCount)
{
  ParticleVector candidates;
  //
  // take points from the source object and create a particle list
  //
  svtkIdType numSeeds = source->GetNumberOfPoints();
  candidates.resize(numSeeds);
  //
  for (svtkIdType i = 0; i < numSeeds; i++)
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

#ifndef NDEBUG
  int numTested = static_cast<int>(candidates.size());
#endif
  this->TestParticles(candidates, LocalSeedPoints, LocalAssignedCount);
  int TotalAssigned = LocalAssignedCount;
  (void)TotalAssigned;

  // Assign unique identifiers taking into account uneven distribution
  // across processes and seeds which were rejected
  this->AssignUniqueIds(LocalSeedPoints);

#ifndef NDEBUG
  svtkDebugMacro(<< "Tested " << numTested << " LocallyAssigned " << LocalAssignedCount);
  if (this->UpdatePieceId == 0)
  {
    svtkDebugMacro(<< "Total Assigned to all processes " << TotalAssigned);
  }
#endif
}
//---------------------------------------------------------------------------
void svtkTemporalStreamTracer::AssignUniqueIds(
  svtkTemporalStreamTracerNamespace::ParticleVector& LocalSeedPoints)
{
  svtkIdType ParticleCountOffset = 0;
  svtkIdType numParticles = static_cast<svtkIdType>(LocalSeedPoints.size());
  for (svtkIdType i = 0; i < numParticles; i++)
  {
    LocalSeedPoints[i].UniqueParticleId = this->UniqueIdCounter + ParticleCountOffset + i;
  }
  this->UniqueIdCounter += numParticles;
}
//---------------------------------------------------------------------------
void svtkTemporalStreamTracer::TransmitReceiveParticles(ParticleVector&, ParticleVector&, bool) {}

//---------------------------------------------------------------------------
void svtkTemporalStreamTracer::UpdateParticleList(ParticleVector& candidates)
{
  int numSeedsNew = static_cast<int>(candidates.size());
  //
  for (int i = 0; i < numSeedsNew; i++)
  {
    // allocate a new particle on the list and get a reference to it
    this->ParticleHistories.push_back(candidates[i]);
  }
  this->NumberOfParticles = static_cast<int>(ParticleHistories.size());

  svtkDebugMacro(<< "UpdateParticleList completed with " << this->NumberOfParticles << " particles");
}

int svtkTemporalStreamTracer::ProcessInput(svtkInformationVector** inputVector)
{
  assert(this->RequestIndex >= 0 && this->RequestIndex < 2);
  int numInputs = inputVector[0]->GetNumberOfInformationObjects();
  if (numInputs != 1)
  {
    if (numInputs == 0)
    {
      svtkErrorMacro(<< "No input found.");
      return 0;
    }
    svtkWarningMacro(<< "Multiple inputs founds. Use only the first one.");
  }

  // inherited from streamtracer, make sure it is null
  this->InputData = nullptr;
  this->InputDataT[this->RequestIndex] = nullptr;

  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (inInfo)
  {
    svtkDataObject* input = inInfo->Get(svtkDataObject::DATA_OBJECT());
    SetTemporalInput(input, this->RequestIndex);
    //
    // Get the timestep information for this instant
    //
    std::vector<double> timesteps;
    if (inInfo->Has(svtkDataObject::DATA_TIME_STEP()))
    {
      timesteps.resize(1);
      timesteps[0] = inInfo->Get(svtkDataObject::DATA_TIME_STEP());
    }
    else
    {
      svtkErrorMacro(<< "No time step info");
      return 1;
    }
    this->CurrentTimeSteps[this->RequestIndex] = timesteps[0] * this->TimeStepResolution;
  }
  return 1;
}

int svtkTemporalStreamTracer::GenerateOutput(
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)

{

  //
  // Parallel/Piece information
  //
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  this->UpdatePieceId = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  this->UpdateNumPieces = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  //
  // How many Seed point sources are connected?
  // Copy the sources into a vector for later use
  //

  int numSources = inputVector[1]->GetNumberOfInformationObjects();
  std::vector<svtkDataSet*> SeedSources;
  for (int idx = 0; idx < numSources; ++idx)
  {
    svtkDataObject* dobj = nullptr;
    svtkInformation* inInfo = inputVector[1]->GetInformationObject(idx);
    if (inInfo)
    {
      dobj = inInfo->Get(svtkDataObject::DATA_OBJECT());
      SeedSources.push_back(svtkDataSet::SafeDownCast(dobj));
    }
  }

  if (this->IntegrationDirection != FORWARD)
  {
    svtkErrorMacro(<< "We can only handle forward time particle tracking at the moment");
    return 1;
  }

  //
  // Add the datasets to an interpolator object
  //
  if (this->InitializeInterpolator() != SVTK_OK)
  {
    if (this->InputDataT[0])
      this->InputDataT[0] = nullptr;
    if (this->InputDataT[1])
      this->InputDataT[1] = nullptr;
    svtkErrorMacro(<< "InitializeInterpolator failed");
    return 1;
  }

  //
  // Setup some variables
  //
  svtkSmartPointer<svtkInitialValueProblemSolver> integrator;
  integrator.TakeReference(this->GetIntegrator()->NewInstance());
  integrator->SetFunctionSet(this->Interpolator);

  //
  // Make sure the Particle Positions are initialized with Seed particles
  //
  this->ReinjectionFlag = 0;
  if (this->ForceReinjectionEveryNSteps > 0)
  {
    if ((this->ActualTimeStep % this->ForceReinjectionEveryNSteps) == 0)
    {
      this->ReinjectionFlag = 1;
    }
  }
  //
  // If T=0 reset everything to allow us to setup stuff then start an animation
  // with a clean slate
  //
  if (this->ActualTimeStep == 0) // XXX: what if I start from some other time?
  {
    this->LocalSeeds.clear();
    this->ParticleHistories.clear();
    this->EarliestTime = -1E6;
    this->ReinjectionFlag = 1;
    this->ReinjectionCounter = 0;
    this->UniqueIdCounter = 0;
    this->UniqueIdCounterMPI = 0;
  }
  else if (this->CurrentTimeSteps[0] < this->EarliestTime)
  {
    //
    // We don't want to go back in time, so just reuse whatever we have
    //
    svtkDebugMacro("skipping particle tracking because we have seen this timestep before");
    outInfo->Set(svtkDataObject::DATA_TIME_STEP(), this->OutputTimeValues[this->ActualTimeStep]);
    if (this->InputDataT[0])
      this->InputDataT[0] = nullptr;
    if (this->InputDataT[1])
      this->InputDataT[1] = nullptr;
    return 1;
  }
  this->EarliestTime = (this->CurrentTimeSteps[0] > this->EarliestTime) ? this->CurrentTimeSteps[0]
                                                                        : this->EarliestTime;
  //
  //
  //
  for (unsigned int i = 0; i < SeedSources.size(); i++)
  {
    if (SeedSources[i]->GetMTime() > this->ParticleInjectionTime)
    {
      //    this->ReinjectionFlag = 1;
    }
  }

  //
  // Lists for seed particles
  //
  ParticleVector candidates;
  ParticleVector received;
  //

  if (this->ReinjectionFlag)
  {
    int seedPointId = 0;
    if (this->StaticSeeds && this->AllFixedGeometry && this->LocalSeeds.empty())
    {
      for (unsigned int i = 0; i < SeedSources.size(); i++)
      {
        this->AssignSeedsToProcessors(SeedSources[i], i, 0, this->LocalSeeds, seedPointId);
      }
    }
    else
    {
      // wipe the list and reclassify for each injection
      this->LocalSeeds.clear();
      for (unsigned int i = 0; i < SeedSources.size(); i++)
      {
        this->AssignSeedsToProcessors(SeedSources[i], i, 0, this->LocalSeeds, seedPointId);
      }
    }
    this->ParticleInjectionTime.Modified();

    // Now update our main list with the ones we are keeping
    svtkDebugMacro(<< "Reinjection about to update candidates (" << this->LocalSeeds.size()
                  << " particles)");
    this->UpdateParticleList(this->LocalSeeds);
    this->ReinjectionCounter += 1;
  }

  //
  // setup all our output arrays
  //
  svtkDebugMacro(<< "About to allocate point arrays ");
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
  this->ParticleAge = svtkSmartPointer<svtkFloatArray>::New();
  this->ParticleIds = svtkSmartPointer<svtkIntArray>::New();
  this->ParticleSourceIds = svtkSmartPointer<svtkCharArray>::New();
  this->InjectedPointIds = svtkSmartPointer<svtkIntArray>::New();
  this->InjectedStepIds = svtkSmartPointer<svtkIntArray>::New();
  this->ErrorCodeArray = svtkSmartPointer<svtkIntArray>::New();
  this->ParticleVorticity = svtkSmartPointer<svtkFloatArray>::New();
  this->ParticleRotation = svtkSmartPointer<svtkFloatArray>::New();
  this->ParticleAngularVel = svtkSmartPointer<svtkFloatArray>::New();
  this->cellVectors = svtkSmartPointer<svtkDoubleArray>::New();
  this->ParticleCells = svtkSmartPointer<svtkCellArray>::New();
  this->OutputCoordinates = svtkSmartPointer<svtkPoints>::New();
  this->OutputPointData = output->GetPointData();
  this->OutputPointData->Initialize();
  this->InterpolationCount = 0;
  svtkDebugMacro(<< "About to Interpolate allocate space");
  this->OutputPointData->InterpolateAllocate(this->DataReferenceT[1]->GetPointData());
  //
  this->ParticleAge->SetName("ParticleAge");
  this->ParticleIds->SetName("ParticleId");
  this->ParticleSourceIds->SetName("ParticleSourceId");
  this->InjectedPointIds->SetName("InjectedPointId");
  this->InjectedStepIds->SetName("InjectionStepId");
  this->ErrorCodeArray->SetName("ErrorCode");

  if (this->ComputeVorticity)
  {
    this->cellVectors->SetNumberOfComponents(3);
    this->cellVectors->Allocate(3 * SVTK_CELL_SIZE);
    this->ParticleVorticity->SetName("Vorticity");
    this->ParticleRotation->SetName("Rotation");
    this->ParticleAngularVel->SetName("AngularVelocity");
  }

  output->SetPoints(this->OutputCoordinates);
  output->SetVerts(this->ParticleCells);
  svtkDebugMacro(<< "Finished allocating point arrays ");

  //
  // Perform 2 passes
  // Pass 0 : Integration of particles created by a source in this process
  // or received at start from a source in another process.
  //
  // Pass 1 : Particles that were sent in mid integration from another process
  // are added in and their integration continued here. In actual fact, the process
  // should be repeated until all particles are finished, but the chances of
  // a particle stepping inside and out again through a single domain
  // in one time step are small (hopefully!)

  svtkDebugMacro(<< "Clear MPI send list ");
  this->MPISendList.clear();

#ifndef NDEBUG
  int Number = static_cast<int>(this->ParticleHistories.size());
#endif
  ParticleListIterator it_first = this->ParticleHistories.begin();
  ParticleListIterator it_last = this->ParticleHistories.end();
  ParticleListIterator it_next;
#define PASSES 2
  for (int pass = 0; pass < PASSES; pass++)
  {
    svtkDebugMacro(<< "Begin Pass " << pass << " with " << Number << " Particles");
    for (ParticleListIterator it = it_first; it != it_last;)
    {
      // Keep the 'next' iterator handy because if a particle is terminated
      // or leaves the domain, the 'current' iterator will be deleted.
      it_next = it;
      it_next++;
      //
      // Shall we terminate this particle
      //
      double interval = (this->CurrentTimeSteps[1] - this->CurrentTimeSteps[0]);
      bool terminated = false;
      if (this->TerminationTime > 0)
      {
        if (this->TerminationTimeUnit == TERMINATION_TIME_UNIT &&
          (it->age + interval) > this->TerminationTime)
        {
          terminated = true;
        }
        else if (this->TerminationTimeUnit == TERMINATION_STEP_UNIT &&
          (it->TimeStepAge + 1) > this->TerminationTime)
        {
          terminated = true;
        }
      }
      if (terminated)
      {
        this->ParticleHistories.erase(it);
      }
      else
      {
        this->IntegrateParticle(
          it, this->CurrentTimeSteps[0], this->CurrentTimeSteps[1], integrator);
      }
      //
      if (this->GetAbortExecute())
      {
        break;
      }
      it = it_next;
    }
    // Particles might have been deleted during the first pass as they move
    // out of domain or age. Before adding any new particles that are sent
    // to us, we must know the starting point ready for the second pass
    bool list_valid = (!this->ParticleHistories.empty());
    if (list_valid)
    {
      // point to one before the end
      it_first = --this->ParticleHistories.end();
    }
    // Send and receive any particles which exited/entered the domain
    if (this->UpdateNumPieces > 1 && pass < (PASSES - 1))
    {
      // the Particle lists will grow if any are received
      // so we must be very careful with our iterators
      svtkDebugMacro(<< "End of Pass " << pass << " with " << this->ParticleHistories.size() << " "
                    << " about to Transmit/Receive " << this->MPISendList.size());
      this->TransmitReceiveParticles(this->MPISendList, received, true);
      // don't want the ones that we sent away
      this->MPISendList.clear();
      int assigned;
      // classify all the ones we received
      if (!received.empty())
      {
        this->TestParticles(received, candidates, assigned);
        svtkDebugMacro(<< "received " << received.size() << " : assigned locally " << assigned);
        received.clear();
      }
      // Now update our main list with the ones we are keeping
      this->UpdateParticleList(candidates);
      // free up unwanted memory
#ifndef NDEBUG
      Number = static_cast<int>(candidates.size());
#endif
      candidates.clear();
    }
    it_last = this->ParticleHistories.end();
    if (list_valid)
    {
      // increment to point to first new entry
      it_first++;
    }
    else
    {
      it_first = this->ParticleHistories.begin();
    }
  }
  if (!this->MPISendList.empty())
  {
    // If a particle went out of domain on the second pass, it should be sent
    // can it really pass right through a domain in one step?
    // what about grazing the edge of rotating zone?
    svtkDebugMacro(<< "MPISendList not empty " << this->MPISendList.size());
    this->MPISendList.clear();
  }

  //
  // We must only add these scalar arrays at the end because the
  // existing scalars on the input get interpolated during iteration
  // over the particles
  //
  this->OutputPointData->AddArray(this->ParticleIds);
  this->OutputPointData->AddArray(this->ParticleSourceIds);
  this->OutputPointData->AddArray(this->InjectedPointIds);
  this->OutputPointData->AddArray(this->InjectedStepIds);
  this->OutputPointData->AddArray(this->ErrorCodeArray);
  this->OutputPointData->AddArray(this->ParticleAge);
  if (this->ComputeVorticity)
  {
    this->OutputPointData->AddArray(this->ParticleVorticity);
    this->OutputPointData->AddArray(this->ParticleRotation);
    this->OutputPointData->AddArray(this->ParticleAngularVel);
  }

  if (this->InterpolationCount != this->OutputCoordinates->GetNumberOfPoints())
  {
    svtkErrorMacro(<< "Mismatch in point array/data counts");
  }
  //
  outInfo->Set(svtkDataObject::DATA_TIME_STEP(), this->OutputTimeValues[this->ActualTimeStep]);

  // save some locator building, by re-using them as time progresses
  this->Interpolator->AdvanceOneTimeStep();

  //
  // Let go of inputs
  //
  if (this->InputDataT[0])
    this->InputDataT[0] = nullptr;
  if (this->InputDataT[1])
    this->InputDataT[1] = nullptr;

  //
  // Write Particles out if necessary
  //
  // NB. We don't want our writer to trigger any updates,
  // so shallow copy the output
  if (this->ParticleWriter && this->EnableParticleWriting)
  {
    svtkSmartPointer<svtkPolyData> polys = svtkSmartPointer<svtkPolyData>::New();
    polys->ShallowCopy(output);
    int N = polys->GetNumberOfPoints();
    (void)N;
    this->ParticleWriter->SetFileName(this->ParticleFileName);
    this->ParticleWriter->SetTimeStep(this->ActualTimeStep);
    this->ParticleWriter->SetTimeValue(this->CurrentTimeSteps[1]);
    this->ParticleWriter->SetInputData(polys);
    this->ParticleWriter->Write();
    this->ParticleWriter->CloseFile();
    this->ParticleWriter->SetInputData(nullptr);
    svtkDebugMacro(<< "Written " << N);
  }
  return 1;
}

int svtkTemporalStreamTracer::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  //
  // Inputs information
  //
  bool result(true);
  if (RequestIndex < 2)
  {
    result = (this->ProcessInput(inputVector) == 1);
    if (result && RequestIndex == 1)
    {
      this->GenerateOutput(inputVector, outputVector);
    }
  }

  this->RequestIndex++;
  if (result && this->RequestIndex < 2)
  {
    request->Set(svtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
  }
  else
  {
    request->Remove(svtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
    this->RequestIndex = 0;
  }

  //  this->Interpolator->ShowCacheResults();
  //  svtkErrorMacro(<<"RequestData done");
  return 1;
}
//---------------------------------------------------------------------------
void svtkTemporalStreamTracer::IntegrateParticle(ParticleListIterator& it, double currenttime,
  double targettime, svtkInitialValueProblemSolver* integrator)
{
  double epsilon = (targettime - currenttime) / 100.0;
  double velocity[3], point1[4], point2[4] = { 0.0, 0.0, 0.0, 0.0 };
  double minStep = 0, maxStep = 0;
  double stepWanted, stepTaken = 0.0;
  substeps = 0;

  ParticleInformation& info = (*it);
  // Get the Initial point {x,y,z,t}
  memcpy(point1, &info.CurrentPosition, sizeof(Position));

  if (point1[3] < (currenttime - epsilon) || point1[3] > (targettime + epsilon))
  {
    svtkDebugMacro(<< "Bad particle time : expected (" << this->CurrentTimeSteps[0] << "-"
                  << this->CurrentTimeSteps[1] << ") got " << point1[3]);
  }

  IntervalInformation delT;
  delT.Unit = LENGTH_UNIT;
  delT.Interval = (targettime - currenttime) * this->InitialIntegrationStep;
  epsilon = delT.Interval * 1E-3;

  //
  // begin interpolation between available time values, if the particle has
  // a cached cell ID and dataset - try to use it,
  //
  this->Interpolator->SetCachedCellIds(info.CachedCellId, info.CachedDataSetId);

  bool particle_good = true;
  info.ErrorCode = 0;
  while (point1[3] < (targettime - epsilon))
  {
    //
    // Here beginneth the real work
    //
    double error = 0;

    // If, with the next step, propagation will be larger than
    // max, reduce it so that it is (approximately) equal to max.
    stepWanted = delT.Interval;
    if ((point1[3] + stepWanted) > targettime)
    {
      stepWanted = targettime - point1[3];
      maxStep = stepWanted;
    }
    this->LastUsedStepSize = stepWanted;

    // Calculate the next step using the integrator provided.
    // If the next point is out of bounds, send it to another process
    if (integrator->ComputeNextStep(point1, point2, point1[3], stepWanted, stepTaken, minStep,
          maxStep, this->MaximumError, error) != 0)
    {
      // if the particle is sent, remove it from the list
      info.ErrorCode = 1;
      if (this->SendParticleToAnotherProcess(info, point1, this->LastUsedStepSize))
      {
        this->ParticleHistories.erase(it);
        particle_good = false;
        break;
      }
      else
      {
        // particle was not sent, retry saved it, so copy info back
        substeps++;
        memcpy(point1, &info.CurrentPosition, sizeof(Position));
      }
    }
    else // success, increment position/time
    {
      substeps++;

      // increment the particle time
      point2[3] = point1[3] + stepTaken;
      info.age += stepTaken;

      // Point is valid. Insert it.
      memcpy(&info.CurrentPosition, point2, sizeof(Position));
      memcpy(point1, point2, sizeof(Position));
    }

    // If the solver is adaptive and the next time step (delT.Interval)
    // that the solver wants to use is smaller than minStep or larger
    // than maxStep, re-adjust it. This has to be done every step
    // because minStep and maxStep can change depending on the Cell
    // size (unless it is specified in time units)
    if (integrator->IsAdaptive())
    {
      // code removed. Put it back when this is stable
    }
  }
  if (particle_good)
  {
    // The integration succeeded, but check the computed final position
    // is actually inside the domain (the intermediate steps taken inside
    // the integrator were ok, but the final step may just pass out)
    // if it moves out, we can't interpolate scalars, so we must send it away
    info.LocationState = this->Interpolator->TestPoint(info.CurrentPosition.x);
    if (info.LocationState == ID_OUTSIDE_ALL)
    {
      info.ErrorCode = 2;
      // if the particle is sent, remove it from the list
      if (this->SendParticleToAnotherProcess(info, point1, this->LastUsedStepSize))
      {
        this->ParticleHistories.erase(it);
        particle_good = false;
      }
    }
  }

  //
  // Has this particle stagnated
  //
  if (particle_good)
  {
    this->Interpolator->GetLastGoodVelocity(velocity);
    info.speed = svtkMath::Norm(velocity);
    if (it->speed <= this->TerminalSpeed)
    {
      this->ParticleHistories.erase(it);
      particle_good = false;
    }
  }

  //
  // We got this far without error :
  // Insert the point into the output
  // Create any new scalars and interpolate existing ones
  // Cache cell ids and datasets
  //
  if (particle_good)
  {
    //
    // store the last Cell Ids and dataset indices for next time particle is updated
    //
    this->Interpolator->GetCachedCellIds(info.CachedCellId, info.CachedDataSetId);
    //
    info.TimeStepAge += 1;
    //
    // Now generate the output geometry and scalars
    //
    double* coord = &info.CurrentPosition.x[0];
    svtkIdType tempId = this->OutputCoordinates->InsertNextPoint(coord);
    // create the cell
    this->ParticleCells->InsertNextCell(1, &tempId);
    // set the easy scalars for this particle
    this->ParticleIds->InsertNextValue(info.UniqueParticleId);
    this->ParticleSourceIds->InsertNextValue(info.SourceID);
    this->InjectedPointIds->InsertNextValue(info.InjectedPointId);
    this->InjectedStepIds->InsertNextValue(info.InjectedStepId);
    this->ErrorCodeArray->InsertNextValue(info.ErrorCode);
    this->ParticleAge->InsertNextValue(info.age);
    //
    // Interpolate all existing point attributes
    // In principle we always integrate the particle until it reaches Time2
    // - so we don't need to do any interpolation of the scalars
    // between T0 and T1, just fetch the values
    // of the spatially interpolated scalars from T1.
    //
    if (info.LocationState == ID_OUTSIDE_T1)
    {
      this->Interpolator->InterpolatePoint(0, this->OutputPointData, tempId);
    }
    else
    {
      this->Interpolator->InterpolatePoint(1, this->OutputPointData, tempId);
    }
    this->InterpolationCount++;
    //
    // Compute vorticity
    //
    if (this->ComputeVorticity)
    {
      svtkGenericCell* cell;
      double pcoords[3], vorticity[3], weights[SVTK_MAXIMUM_NUMBER_OF_POINTS];
      double rotation, omega;
      // have to use T0 if particle is out at T1, otherwise use T1
      if (info.LocationState == ID_OUTSIDE_T1)
      {
        this->Interpolator->GetVorticityData(0, pcoords, weights, cell, this->cellVectors);
      }
      else
      {
        this->Interpolator->GetVorticityData(1, pcoords, weights, cell, this->cellVectors);
      }
      svtkStreamTracer::CalculateVorticity(cell, pcoords, cellVectors, vorticity);
      this->ParticleVorticity->InsertNextTuple(vorticity);
      // local rotation = vorticity . unit tangent ( i.e. velocity/speed )
      if (info.speed != 0.0)
      {
        omega = svtkMath::Dot(vorticity, velocity);
        omega /= info.speed;
        omega *= this->RotationScale;
      }
      else
      {
        omega = 0.0;
      }
      svtkIdType index = this->ParticleAngularVel->InsertNextValue(omega);
      if (index > 0)
      {
        rotation =
          info.rotation + (info.angularVel + omega) / 2 * (info.CurrentPosition.x[3] - info.time);
      }
      else
      {
        rotation = 0.0;
      }
      this->ParticleRotation->InsertNextValue(rotation);
      info.rotation = rotation;
      info.angularVel = omega;
      info.time = info.CurrentPosition.x[3];
    }
  }
  else
    this->Interpolator->ClearCache();

  double eps = (this->CurrentTimeSteps[1] - this->CurrentTimeSteps[0]) / 100;
  if (point1[3] < (this->CurrentTimeSteps[0] - eps) ||
    point1[3] > (this->CurrentTimeSteps[1] + eps))
  {
    svtkDebugMacro(<< "Unexpected time ending IntegrateParticle - expected ("
                  << this->CurrentTimeSteps[0] << "-" << this->CurrentTimeSteps[1] << ") got "
                  << point1[3]);
  }
}
//---------------------------------------------------------------------------
bool svtkTemporalStreamTracer::RetryWithPush(
  ParticleInformation& info, double velocity[3], double delT)
{
  // try adding a one increment push to the particle to get over a rotating/moving boundary
  for (int v = 0; v < 3; v++)
    info.CurrentPosition.x[v] += velocity[v] * delT;
  info.CurrentPosition.x[3] += delT;
  info.LocationState = this->Interpolator->TestPoint(info.CurrentPosition.x);
  if (info.LocationState != ID_OUTSIDE_ALL)
  {
    // a push helped the particle get back into a dataset,
    info.age += delT;
    info.ErrorCode = 6;
    return 1;
  }
  return 0;
}
//---------------------------------------------------------------------------
bool svtkTemporalStreamTracer::SendParticleToAnotherProcess(
  ParticleInformation& info, double point1[4], double delT)
{
  //  return 1;
  double velocity[3];
  this->Interpolator->ClearCache();
  if (info.UniqueParticleId == 3)
  {
    svtkDebugMacro(<< "3 is about to be sent");
  }
  info.LocationState = this->Interpolator->TestPoint(point1);
  if (info.LocationState == ID_OUTSIDE_ALL)
  {
    // something is wrong, the particle has left the building completely
    // we can't get the last good velocity as it won't be valid
    // send the particle 'as is' and hope it lands in another process
    if (substeps > 0)
    {
      this->Interpolator->GetLastGoodVelocity(velocity);
    }
    else
    {
      velocity[0] = velocity[1] = velocity[2] = 0.0;
    }
    info.ErrorCode = 3;
  }
  else if (info.LocationState == ID_OUTSIDE_T0)
  {
    // the particle left the volume but can be tested at T2, so use the velocity at T2
    this->Interpolator->GetLastGoodVelocity(velocity);
    info.ErrorCode = 4;
  }
  else if (info.LocationState == ID_OUTSIDE_T1)
  {
    // the particle left the volume but can be tested at T1, so use the velocity at T1
    this->Interpolator->GetLastGoodVelocity(velocity);
    info.ErrorCode = 5;
  }
  else
  {
    // The test returned INSIDE_ALL, so test failed near start of integration,
    this->Interpolator->GetLastGoodVelocity(velocity);
  }
  if (this->RetryWithPush(info, velocity, delT))
    return 0;
  this->AddParticleToMPISendList(info);
  return 1;
}
//---------------------------------------------------------------------------
void svtkTemporalStreamTracer::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "TimeStepResolution: " << this->TimeStepResolution << endl;
  os << indent << "ParticleWriter: " << this->ParticleWriter << endl;
  os << indent << "ParticleFileName: " << (this->ParticleFileName ? this->ParticleFileName : "None")
     << endl;
  os << indent << "TimeStep: " << this->TimeStep << endl;
  os << indent << "ForceReinjectionEveryNSteps: " << this->ForceReinjectionEveryNSteps << endl;
  os << indent << "EnableParticleWriting: " << this->EnableParticleWriting << endl;
  os << indent << "IgnorePipelineTime: " << this->IgnorePipelineTime << endl;
  os << indent << "StaticMesh: " << this->StaticMesh << endl;
  os << indent << "TerminationTime: " << this->TerminationTime << endl;
  os << indent << "TerminationTimeUnit: " << this->TerminationTimeUnit << endl;
  os << indent << "StaticSeeds: " << this->StaticSeeds << endl;
}
//---------------------------------------------------------------------------
bool svtkTemporalStreamTracer::ComputeDomainExitLocation(
  double pos[4], double p2[4], double intersection[4], svtkGenericCell* cell)
{
  double t, pcoords[3];
  int subId;
  if (cell->IntersectWithLine(pos, p2, 1E-3, t, intersection, pcoords, subId) == 0)
  {
    svtkDebugMacro(<< "No cell/domain exit was found");
    return 0;
  }
  else
  {
    // We found an intersection on the edge of the cell.
    // Shift it by a small amount to ensure that it crosses over the edge
    // into the adjoining cell.
    for (int i = 0; i < 3; i++)
      intersection[i] = pos[i] + (t + 0.01) * (p2[i] - pos[i]);
    // intersection stored, compute T for intersection
    intersection[3] = pos[3] + (t + 0.01) * (p2[3] - pos[3]);
    return 1;
  }
}
//---------------------------------------------------------------------------
void svtkTemporalStreamTracer::AddParticleToMPISendList(ParticleInformation& info)
{
  double eps = (this->CurrentTimeSteps[1] - this->CurrentTimeSteps[0]) / 100;
  if (info.CurrentPosition.x[3] < (this->CurrentTimeSteps[0] - eps) ||
    info.CurrentPosition.x[3] > (this->CurrentTimeSteps[1] + eps))
  {
    svtkDebugMacro(<< "Unexpected time value in MPISendList - expected ("
                  << this->CurrentTimeSteps[0] << "-" << this->CurrentTimeSteps[1] << ") got "
                  << info.CurrentPosition.x[3]);
  }
}
//---------------------------------------------------------------------------
/*
  // Now try to compute the trajectory exit location from the cell on the edge
  if (this->Interpolator->GetLastValidCellId(0)!=-1) {
    svtkDataSet *hitdata = this->Interpolator->GetLastDataSet(0);
    hitdata->GetCell(this->Interpolator->GetLastValidCellId(0), this->GenericCell);
    double intersection[4];
    if (this->ComputeDomainExitLocation(point1, point2, intersection, this->GenericCell))
    {
      svtkDebugMacro(<< "SendParticleToAnotherProcess : Sending Particle " << particleId << " Time "
  << intersection[3]); this->AddParticleToMPISendList(intersection, velocity); svtkIdType nextPoint =
  ParticleCoordinates->InsertNextPoint(point2);
      this->ParticleHistories[particleId].push_back(nextPoint);
      this->LiveParticleIds[particleId] = 0;
      return 1;
    }
    else {
      svtkDebugMacro(<< "Domain-Exit aborted : Domain Intersection failed" << particleId );
      this->LiveParticleIds[particleId] = 0;
      return 0;
    }
  }
  else {
    svtkDebugMacro(<< "Domain-Exit aborted : Couldn't copy cell from earlier test" << particleId );
    this->LiveParticleIds[particleId] = 0;
    return 0;
  }
*/
