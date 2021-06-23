/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLagrangianParticleTracker.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkLagrangianParticleTracker.h"

#include "svtkAppendPolyData.h"
#include "svtkBilinearQuadIntersection.h"
#include "svtkBoundingBox.h"
#include "svtkCellData.h"
#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataSet.h"
#include "svtkDataObjectTreeIterator.h"
#include "svtkDataSet.h"
#include "svtkDataSetSurfaceFilter.h"
#include "svtkDoubleArray.h"
#include "svtkExecutive.h"
#include "svtkGenericCell.h"
#include "svtkIdList.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkLagrangianMatidaIntegrationModel.h"
#include "svtkLagrangianParticle.h"
#include "svtkLagrangianThreadedData.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolyDataNormals.h"
#include "svtkPolyLine.h"
#include "svtkPolygon.h"
#include "svtkRungeKutta2.h"
#include "svtkSMPThreadLocalObject.h"
#include "svtkSMPTools.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include <algorithm>
#include <limits>
#include <sstream>

svtkObjectFactoryNewMacro(svtkLagrangianParticleTracker);
svtkCxxSetObjectMacro(
  svtkLagrangianParticleTracker, IntegrationModel, svtkLagrangianBasicIntegrationModel);
svtkCxxSetObjectMacro(svtkLagrangianParticleTracker, Integrator, svtkInitialValueProblemSolver);

struct IntegratingFunctor
{
  svtkLagrangianParticleTracker* Tracker;
  std::vector<svtkLagrangianParticle*>& ParticlesVec;
  std::queue<svtkLagrangianParticle*>& ParticlesQueue;
  svtkPolyData* ParticlePathsOutput;
  svtkDataObject* Surfaces;
  svtkDataObject* InteractionOutput;
  svtkSMPThreadLocal<svtkLagrangianThreadedData*> LocalData;
  bool Serial = false;

  IntegratingFunctor(svtkLagrangianParticleTracker* tracker,
    std::vector<svtkLagrangianParticle*>& particlesVec,
    std::queue<svtkLagrangianParticle*>& particlesQueue, svtkPolyData* particlePathsOutput,
    svtkDataObject* surfaces, svtkDataObject* interactionOutput, bool serial)
    : Tracker(tracker)
    , ParticlesVec(particlesVec)
    , ParticlesQueue(particlesQueue)
    , ParticlePathsOutput(particlePathsOutput)
    , Surfaces(surfaces)
    , InteractionOutput(interactionOutput)
    , Serial(serial)
  {
  }

  void Initialize()
  {
    // Create a local threaded data
    svtkLagrangianThreadedData* localData = new svtkLagrangianThreadedData;
    this->LocalData.Local() = localData;

    // Create a local non-threadsafe integrator with a threadsafe integration model
    localData->Integrator = this->Tracker->Integrator->NewInstance();
    localData->Integrator->SetFunctionSet(this->Tracker->IntegrationModel);

    // Initialize a local idList
    localData->IdList->Allocate(10);

    // Create a local bilinear quad intersection
    localData->BilinearQuadIntersection = new svtkBilinearQuadIntersection;

    if (this->Tracker->GenerateParticlePathsOutput)
    {
      svtkPolyData* localParticlePathsOutput = localData->ParticlePathsOutput;
      // Initialize a local particle path output
      this->Tracker->InitializePathsOutput(this->Tracker->SeedData,
        static_cast<svtkIdType>(this->LocalData.size()), localParticlePathsOutput);
    }

    if (this->Surfaces)
    {
      // Create and initialize a local interaction output
      localData->InteractionOutput = this->InteractionOutput->NewInstance();
      this->Tracker->InitializeInteractionOutput(
        this->Tracker->SeedData, this->Surfaces, localData->InteractionOutput);
    }

    // Let the model initialize the user data if needed
    this->Tracker->IntegrationModel->InitializeThreadedData(localData);
  }

  void operator()(svtkIdType partId, svtkIdType endPartId)
  {
    for (svtkIdType id = partId; id < endPartId; id++)
    {
      svtkLagrangianParticle* particle = this->ParticlesVec[id];
      svtkLagrangianThreadedData* localData = this->LocalData.Local();

      // Set threaded data on the particle
      particle->SetThreadedData(localData);

      // Create polyLine output cell
      svtkNew<svtkPolyLine> particlePath;

      // Integrate
      this->Tracker->Integrate(localData->Integrator, particle, this->ParticlesQueue,
        localData->ParticlePathsOutput, particlePath, localData->InteractionOutput);

      this->Tracker->IntegratedParticleCounter += this->Tracker->IntegratedParticleCounterIncrement;

      this->Tracker->IntegrationModel->ParticleAboutToBeDeleted(particle);
      delete particle;

      // Special case to show progress in serial
      if (this->Serial)
      {
        double progress = static_cast<double>(this->Tracker->IntegratedParticleCounter) /
          this->Tracker->ParticleCounter;
        this->Tracker->UpdateProgress(progress);
      }
    }
    if (!this->Serial)
    {
      // In multithread, protect the progress event with a mutex
      std::lock_guard<std::mutex> guard(this->Tracker->ProgressMutex);
      double progress = static_cast<double>(this->Tracker->IntegratedParticleCounter) /
        this->Tracker->ParticleCounter;
      this->Tracker->UpdateProgress(progress);
    }
  }

  void Reduce()
  {
    // Particle Path reduction
    if (this->Tracker->GenerateParticlePathsOutput)
    {
      svtkNew<svtkAppendPolyData> append;
      append->AddInputData(this->ParticlePathsOutput);
      for (auto data : this->LocalData)
      {
        append->AddInputData(data->ParticlePathsOutput);
      }
      append->Update();
      this->ParticlePathsOutput->ShallowCopy(append->GetOutput());
    }

    if (this->Surfaces)
    {
      // Interaction Reduction
      svtkCompositeDataSet* hdInteractionOutput =
        svtkCompositeDataSet::SafeDownCast(this->InteractionOutput);
      svtkPolyData* pdInteractionOutput = svtkPolyData::SafeDownCast(this->InteractionOutput);
      if (hdInteractionOutput)
      {
        svtkCompositeDataSet* hdSurfaces = svtkCompositeDataSet::SafeDownCast(this->Surfaces);
        svtkSmartPointer<svtkCompositeDataIterator> iter;
        iter.TakeReference(hdSurfaces->NewIterator());
        for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
        {
          svtkNew<svtkAppendPolyData> append;
          svtkPolyData* initialPD = svtkPolyData::SafeDownCast(hdInteractionOutput->GetDataSet(iter));
          if (initialPD)
          {
            append->AddInputData(initialPD);
          }
          for (auto data : this->LocalData)
          {
            append->AddInputData(svtkPolyData::SafeDownCast(
              svtkCompositeDataSet::SafeDownCast(data->InteractionOutput)->GetDataSet(iter)));
          }
          append->Update();
          hdInteractionOutput->SetDataSet(iter, append->GetOutput());
        }
        for (auto data : this->LocalData)
        {
          data->InteractionOutput->Delete();
        }
      }
      else
      {
        svtkNew<svtkAppendPolyData> append;
        append->AddInputData(pdInteractionOutput);
        for (auto data : this->LocalData)
        {
          auto interOut = data->InteractionOutput;
          svtkPolyData* pd = svtkPolyData::SafeDownCast(interOut);
          append->AddInputData(pd);
          interOut->Delete();
        }
        append->Update();
        pdInteractionOutput->ShallowCopy(append->GetOutput());
      }
    }

    // Other threaded Data Reduction
    for (auto data : this->LocalData)
    {
      data->Integrator->Delete();
      delete data->BilinearQuadIntersection;
      this->Tracker->IntegrationModel->FinalizeThreadedData(data);
      delete data;
    }
  }
};

//---------------------------------------------------------------------------
svtkLagrangianParticleTracker::svtkLagrangianParticleTracker()
  : IntegrationModel(svtkLagrangianMatidaIntegrationModel::New())
  , Integrator(svtkRungeKutta2::New())
  , CellLengthComputationMode(STEP_LAST_CELL_LENGTH)
  , StepFactor(1.0)
  , StepFactorMin(0.5)
  , StepFactorMax(1.5)
  , MaximumNumberOfSteps(100)
  , MaximumIntegrationTime(-1.0)
  , AdaptiveStepReintegration(false)
  , GeneratePolyVertexInteractionOutput(false)
  , ParticleCounter(0)
  , IntegratedParticleCounter(0)
  , IntegratedParticleCounterIncrement(1)
  , MinimumVelocityMagnitude(0.001)
  , MinimumReductionFactor(1.1)
  , FlowCache(nullptr)
  , FlowTime(0)
  , SurfacesCache(nullptr)
  , SurfacesTime(0)
{
  this->SetNumberOfInputPorts(3);
  this->SetNumberOfOutputPorts(2);
}

//---------------------------------------------------------------------------
svtkLagrangianParticleTracker::~svtkLagrangianParticleTracker()
{
  this->SetIntegrator(nullptr);
  this->SetIntegrationModel(nullptr);
}

//---------------------------------------------------------------------------
void svtkLagrangianParticleTracker::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->IntegrationModel)
  {
    os << indent << "IntegrationModel: " << endl;
    this->IntegrationModel->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "IntegrationModel: " << this->IntegrationModel << endl;
  }
  if (this->Integrator)
  {
    os << indent << "Integrator: " << endl;
    this->Integrator->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Integrator: " << this->Integrator << endl;
  }
  os << indent << "CellLengthComputationMode: " << this->CellLengthComputationMode << endl;
  os << indent << "StepFactor: " << this->StepFactor << endl;
  os << indent << "StepFactorMin: " << this->StepFactorMin << endl;
  os << indent << "StepFactorMax: " << this->StepFactorMax << endl;
  os << indent << "MaximumNumberOfSteps: " << this->MaximumNumberOfSteps << endl;
  os << indent << "MaximumIntegrationTime: " << this->MaximumIntegrationTime << endl;
  os << indent << "AdaptiveStepReintegration: " << this->AdaptiveStepReintegration << endl;
  os << indent << "GenerateParticlePathsOutput: " << this->GenerateParticlePathsOutput << endl;
  os << indent << "MinimumVelocityMagnitude: " << this->MinimumVelocityMagnitude << endl;
  os << indent << "MinimumReductionFactor: " << this->MinimumReductionFactor << endl;
  os << indent << "ParticleCounter: " << this->ParticleCounter << endl;
  os << indent << "IntegratedParticleCounter: " << this->IntegratedParticleCounter << endl;
}

//---------------------------------------------------------------------------
void svtkLagrangianParticleTracker::SetSourceConnection(svtkAlgorithmOutput* algInput)
{
  this->SetInputConnection(1, algInput);
}

//---------------------------------------------------------------------------
void svtkLagrangianParticleTracker::SetSourceData(svtkDataObject* source)
{
  this->SetInputData(1, source);
}

//---------------------------------------------------------------------------
svtkDataObject* svtkLagrangianParticleTracker::GetSource()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }
  return svtkDataObject::SafeDownCast(this->GetExecutive()->GetInputData(1, 0));
}

//---------------------------------------------------------------------------
void svtkLagrangianParticleTracker::SetSurfaceConnection(svtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(2, algOutput);
}

//---------------------------------------------------------------------------
void svtkLagrangianParticleTracker::SetSurfaceData(svtkDataObject* surface)
{
  this->SetInputData(2, surface);
}

//---------------------------------------------------------------------------
svtkDataObject* svtkLagrangianParticleTracker::GetSurface()
{
  if (this->GetNumberOfInputConnections(2) < 1)
  {
    return nullptr;
  }
  return this->GetExecutive()->GetInputData(2, 0);
}

//---------------------------------------------------------------------------
int svtkLagrangianParticleTracker::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 2)
  {
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return this->Superclass::FillInputPortInformation(port, info);
}

//----------------------------------------------------------------------------
int svtkLagrangianParticleTracker::FillOutputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkPolyData");
  }
  return this->Superclass::FillOutputPortInformation(port, info);
}

//----------------------------------------------------------------------------
int svtkLagrangianParticleTracker::RequestDataObject(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Create particle path output
  svtkInformation* info = outputVector->GetInformationObject(0);
  svtkNew<svtkPolyData> particlePathsOutput;
  info->Set(svtkDataObject::DATA_OBJECT(), particlePathsOutput);

  // Create a surface interaction output
  // First check for composite
  svtkInformation* inInfo = inputVector[2]->GetInformationObject(0);
  info = outputVector->GetInformationObject(1);
  if (inInfo)
  {
    svtkDataObject* input = svtkDataObject::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
    if (input)
    {
      svtkCompositeDataSet* hdInput = svtkCompositeDataSet::SafeDownCast(input);
      if (hdInput)
      {
        svtkDataObject* interactionOutput = input->NewInstance();
        info->Set(svtkDataObject::DATA_OBJECT(), interactionOutput);
        interactionOutput->Delete();
        return 1;
      }
    }
  }
  // In any other case, create a polydata
  svtkNew<svtkPolyData> interactionOutput;
  info->Set(svtkDataObject::DATA_OBJECT(), interactionOutput);
  return 1;
}

//---------------------------------------------------------------------------
int svtkLagrangianParticleTracker::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  std::queue<svtkLagrangianParticle*> particlesQueue;

  if (!this->IntegrationModel)
  {
    svtkErrorMacro(<< "Integration Model is nullptr, cannot integrate");
    return 0;
  }
  this->IntegrationModel->SetTracker(this);

  // Initialize flow
  svtkDataObject* flow = svtkDataObject::GetData(inputVector[0]);
  svtkBoundingBox bounds;
  if (!this->InitializeFlow(flow, &bounds))
  {
    svtkErrorMacro(<< "Could not initialize flow, aborting.");
    return false;
  }

  // Initialize surfaces
  svtkInformation* surfacesInInfo = inputVector[2]->GetInformationObject(0);
  svtkDataObject* surfaces = nullptr;
  if (surfacesInInfo)
  {
    surfaces = surfacesInInfo->Get(svtkDataObject::DATA_OBJECT());
    if (this->UpdateSurfaceCacheIfNeeded(surfaces))
    {
      this->InitializeSurface(surfaces);
    }
  }

  // Recover seeds
  svtkDataObject* seeds = svtkDataObject::GetData(inputVector[1]);
  if (!seeds)
  {
    svtkErrorMacro(<< "Cannot recover seeds, aborting.");
    return 0;
  }

  // Check seed dataset type
  svtkCompositeDataSet* hdInput = svtkCompositeDataSet::SafeDownCast(seeds);
  svtkDataSet* actualSeeds = svtkDataSet::SafeDownCast(seeds);
  if (hdInput)
  {
    // Composite data
    svtkSmartPointer<svtkCompositeDataIterator> iter;
    iter.TakeReference(hdInput->NewIterator());
    bool leafFound = false;
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      svtkDataSet* ds = svtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (ds)
      {
        // We show the warning only when the input contains more than one leaf
        if (leafFound)
        {
          svtkWarningMacro("Only the first block of seeds have been used to "
                          "generate seeds, other blocks are ignored");
          break;
        }
        actualSeeds = ds;
        leafFound = true;
      }
    }
  }

  if (!actualSeeds)
  {
    svtkErrorMacro(<< "This filter cannot handle input of type: "
                  << (seeds ? seeds->GetClassName() : "(none)"));
    return 0;
  }
  this->SeedData = actualSeeds->GetPointData();

  // Initialize Particles from the seeds
  if (!this->InitializeParticles(&bounds, actualSeeds, particlesQueue, this->SeedData))
  {
    svtkErrorMacro(<< "Could not initialize particles, aborting.");
    return false;
  }

  // Initialize outputs
  svtkPolyData* particlePathsOutput = nullptr;
  if (this->GenerateParticlePathsOutput)
  {
    particlePathsOutput = svtkPolyData::GetData(outputVector);
    if (!particlePathsOutput)
    {
      svtkErrorMacro(<< "Cannot find a svtkMultiPiece particle paths output. aborting");
      return 0;
    }
    this->InitializePathsOutput(this->SeedData, 0, particlePathsOutput);
  }

  svtkDataObject* interactionOutput = nullptr;
  if (surfaces)
  {
    svtkInformation* interactionOutInfo = outputVector->GetInformationObject(1);
    interactionOutput = interactionOutInfo->Get(svtkPolyData::DATA_OBJECT());
    if (!interactionOutput)
    {
      svtkErrorMacro(<< "Cannot find a svtkMultiBlock interaction output. aborting");
      return 0;
    }
    svtkCompositeDataSet* hdInteractionOutput = svtkCompositeDataSet::SafeDownCast(interactionOutput);
    if (hdInteractionOutput)
    {
      hdInteractionOutput->CopyStructure(svtkCompositeDataSet::SafeDownCast(surfaces));
    }
    this->InitializeInteractionOutput(this->SeedData, surfaces, interactionOutput);
  }

  // Let model a chance to change the particles or compute things
  // before integration.
  this->IntegrationModel->PreIntegrate(particlesQueue);

  std::vector<svtkLagrangianParticle*> particlesVec;
  while (!this->GetAbortExecute())
  {
    // Check for particle feed
    this->GetParticleFeed(particlesQueue);
    if (particlesQueue.empty())
    {
      break;
    }

    // Move the current particle queue into a SMP usable vector
    particlesVec.clear();
    particlesVec.resize(particlesQueue.size());
    for (auto& particlePtr : particlesVec)
    {
      // Recover particle
      svtkLagrangianParticle* particleTmp = particlesQueue.front();
      particlesQueue.pop();
      particlePtr = particleTmp;
    }

    // Integrate all available particles
    IntegratingFunctor functor(this, particlesVec, particlesQueue, particlePathsOutput, surfaces,
      interactionOutput, svtkSMPTools::GetEstimatedNumberOfThreads() == 1);
    svtkSMPTools::For(0, static_cast<svtkIdType>(particlesVec.size()), functor);
  }

  // Abort if necessary
  if (this->GetAbortExecute())
  {
    // delete all remaining particle
    while (!particlesQueue.empty())
    {
      svtkLagrangianParticle* particle = particlesQueue.front();
      particlesQueue.pop();
      this->IntegrationModel->ParticleAboutToBeDeleted(particle);
      delete particle;
    }
  }
  // Finalize outputs
  else if (!this->FinalizeOutputs(particlePathsOutput, interactionOutput))
  {
    svtkErrorMacro(<< "Cannot Finalize outputs");
    return 0;
  }
  return 1;
}

//---------------------------------------------------------------------------
svtkMTimeType svtkLagrangianParticleTracker::GetMTime()
{
  // Take integrator and integration model MTime into account
  return std::max(this->Superclass::GetMTime(),
    std::max(this->IntegrationModel ? this->IntegrationModel->GetMTime() : 0,
      this->Integrator ? this->Integrator->GetMTime() : 0));
}

//---------------------------------------------------------------------------
svtkIdType svtkLagrangianParticleTracker::GetNewParticleId()
{
  return this->ParticleCounter++;
}

//---------------------------------------------------------------------------
bool svtkLagrangianParticleTracker::InitializePathsOutput(
  svtkPointData* seedData, svtkIdType numberOfSeeds, svtkPolyData*& particlePathsOutput)
{

  svtkNew<svtkPoints> particlePathsPoints;
  svtkNew<svtkCellArray> particlePaths;
  svtkNew<svtkCellArray> particleVerts;
  particlePathsOutput->SetPoints(particlePathsPoints);
  particlePathsOutput->SetLines(particlePaths);
  particlePathsOutput->SetVerts(particleVerts);

  // Prepare particle paths output point data
  svtkCellData* particlePathsCellData = particlePathsOutput->GetCellData();
  particlePathsCellData->CopyStructure(seedData);
  this->IntegrationModel->InitializePathData(particlePathsCellData);

  // Initialize Particle Paths Point Data
  svtkPointData* particlePathsPointData = particlePathsOutput->GetPointData();
  this->IntegrationModel->InitializeParticleData(particlePathsPointData, numberOfSeeds);

  return true;
}

//---------------------------------------------------------------------------
bool svtkLagrangianParticleTracker::InitializeInteractionOutput(
  svtkPointData* seedData, svtkDataObject* surfaces, svtkDataObject*& interactionOutput)
{
  // Check surfaces dataset type
  svtkCompositeDataSet* hdInput = svtkCompositeDataSet::SafeDownCast(surfaces);
  svtkDataSet* dsInput = svtkDataSet::SafeDownCast(surfaces);
  if (hdInput)
  {
    svtkCompositeDataSet* hdInteractionOutput = svtkCompositeDataSet::SafeDownCast(interactionOutput);
    hdInteractionOutput->CopyStructure(hdInput);
    svtkSmartPointer<svtkCompositeDataIterator> iter;
    iter.TakeReference(hdInput->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      svtkNew<svtkPolyData> pd;
      svtkNew<svtkCellArray> cells;
      svtkNew<svtkPoints> points;
      pd->SetPoints(points);
      pd->GetPointData()->CopyStructure(seedData);
      this->IntegrationModel->InitializePathData(pd->GetPointData());
      this->IntegrationModel->InitializeInteractionData(pd->GetPointData());
      this->IntegrationModel->InitializeParticleData(pd->GetPointData());
      hdInteractionOutput->SetDataSet(iter, pd);
    }
  }
  else if (dsInput)
  {
    svtkPolyData* pdInteractionOutput = svtkPolyData::SafeDownCast(interactionOutput);
    svtkNew<svtkPoints> points;
    svtkNew<svtkCellArray> cells;
    pdInteractionOutput->SetPoints(points);
    pdInteractionOutput->GetPointData()->CopyStructure(seedData);
    this->IntegrationModel->InitializePathData(pdInteractionOutput->GetPointData());
    this->IntegrationModel->InitializeInteractionData(pdInteractionOutput->GetPointData());
    this->IntegrationModel->InitializeParticleData(pdInteractionOutput->GetPointData());
  }
  return true;
}

//---------------------------------------------------------------------------
bool svtkLagrangianParticleTracker::FinalizeOutputs(
  svtkPolyData* particlePathsOutput, svtkDataObject* interactionOutput)
{
  if (this->GenerateParticlePathsOutput)
  {
    if (!particlePathsOutput)
    {
      svtkErrorMacro("Could not recover a output path polydata, something went wrong");
      return false;
    }

    // Recover structures
    svtkPointData* particlePathsPointData = particlePathsOutput->GetPointData();
    svtkPoints* particlePathsPoints = particlePathsOutput->GetPoints();

    // Squeeze and resize point data
    for (int i = 0; i < particlePathsPointData->GetNumberOfArrays(); i++)
    {
      svtkDataArray* array = particlePathsPointData->GetArray(i);
      array->Resize(particlePathsPoints->GetNumberOfPoints());
      array->Squeeze();
    }
  }

  // Insert interaction poly-vertex cell
  if (interactionOutput)
  {
    svtkCompositeDataSet* hdInteractionOutput = svtkCompositeDataSet::SafeDownCast(interactionOutput);
    svtkPolyData* pdInteractionOutput = svtkPolyData::SafeDownCast(interactionOutput);
    if (hdInteractionOutput)
    {
      svtkNew<svtkDataObjectTreeIterator> iter;
      iter->SetDataSet(hdInteractionOutput);
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        svtkPolyData* pdBlock = svtkPolyData::SafeDownCast(hdInteractionOutput->GetDataSet(iter));
        if (!pdBlock)
        {
          svtkErrorMacro(<< "Cannot recover interaction output, something went wrong");
          return false;
        }
        if (this->GeneratePolyVertexInteractionOutput)
        {
          this->InsertPolyVertexCell(pdBlock);
        }
        else
        {
          this->InsertVertexCells(pdBlock);
        }
      }
    }
    else
    {
      if (this->GeneratePolyVertexInteractionOutput)
      {
        this->InsertPolyVertexCell(pdInteractionOutput);
      }
      else
      {
        this->InsertVertexCells(pdInteractionOutput);
      }
    }
  }

  // Enable model post processing
  this->IntegrationModel->FinalizeOutputs(particlePathsOutput, interactionOutput);
  return true;
}

//---------------------------------------------------------------------------
void svtkLagrangianParticleTracker::InsertPolyVertexCell(svtkPolyData* polydata)
{
  // Insert a vertex cell for each point
  svtkIdType nPoint = polydata->GetNumberOfPoints();
  if (nPoint > 0)
  {
    svtkNew<svtkCellArray> polyVertex;
    polyVertex->AllocateEstimate(1, nPoint);
    polyVertex->InsertNextCell(nPoint);
    for (svtkIdType i = 0; i < nPoint; i++)
    {
      polyVertex->InsertCellPoint(i);
    }
    polydata->SetVerts(polyVertex);
  }
}

//---------------------------------------------------------------------------
void svtkLagrangianParticleTracker::InsertVertexCells(svtkPolyData* polydata)
{
  // Insert a vertex cell for each point
  svtkIdType nPoint = polydata->GetNumberOfPoints();
  if (nPoint > 0)
  {
    svtkNew<svtkCellArray> polyVertex;
    polyVertex->AllocateEstimate(1, nPoint);
    for (svtkIdType i = 0; i < nPoint; i++)
    {
      polyVertex->InsertNextCell(1);
      polyVertex->InsertCellPoint(i);
    }
    polydata->SetVerts(polyVertex);
  }
}

//---------------------------------------------------------------------------
bool svtkLagrangianParticleTracker::InitializeFlow(svtkDataObject* input, svtkBoundingBox* bounds)
{
  // Check for updated cache
  if (input == this->FlowCache && input->GetMTime() <= this->FlowTime &&
    this->IntegrationModel->GetLocatorsBuilt())
  {
    bounds->Reset();
    bounds->AddBox(this->FlowBoundsCache);
    return true;
  }

  // No Cache, do the initialization
  // Clear previously setup flow
  this->IntegrationModel->ClearDataSets();

  // Check flow dataset type
  svtkCompositeDataSet* hdInput = svtkCompositeDataSet::SafeDownCast(input);
  svtkDataSet* dsInput = svtkDataSet::SafeDownCast(input);
  if (hdInput)
  {
    // Composite data
    svtkSmartPointer<svtkCompositeDataIterator> iter;
    iter.TakeReference(hdInput->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      svtkDataSet* ds = svtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (ds)
      {
        // Add each leaf to the integration model
        this->IntegrationModel->AddDataSet(ds);
        ds->ComputeBounds();
        bounds->AddBounds(ds->GetBounds());
      }
    }
  }
  else if (dsInput)
  {
    // Add dataset to integration model
    this->IntegrationModel->AddDataSet(dsInput);
    dsInput->ComputeBounds();
    bounds->AddBounds(dsInput->GetBounds());
  }
  else
  {
    svtkErrorMacro(<< "This filter cannot handle input of type: "
                  << (input ? input->GetClassName() : "(none)"));
    return false;
  }
  this->IntegrationModel->SetLocatorsBuilt(true);
  this->FlowCache = input;
  this->FlowTime = input->GetMTime();
  this->FlowBoundsCache.Reset();
  this->FlowBoundsCache.AddBox(*bounds);
  return true;
}

//---------------------------------------------------------------------------
bool svtkLagrangianParticleTracker::UpdateSurfaceCacheIfNeeded(svtkDataObject*& surfaces)
{
  if (surfaces != this->SurfacesCache || surfaces->GetMTime() > this->SurfacesTime)
  {
    this->SurfacesCache = surfaces;
    this->SurfacesTime = surfaces->GetMTime();
    return true;
  }
  return false;
}

//---------------------------------------------------------------------------
void svtkLagrangianParticleTracker::InitializeSurface(svtkDataObject*& surfaces)
{
  // Clear previously setup surfaces
  this->IntegrationModel->ClearDataSets(/*surface*/ true);

  // Check surfaces dataset type
  svtkCompositeDataSet* hdInput = svtkCompositeDataSet::SafeDownCast(surfaces);
  svtkDataSet* dsInput = svtkDataSet::SafeDownCast(surfaces);

  if (hdInput)
  {
    // Composite data
    svtkSmartPointer<svtkCompositeDataIterator> iter;
    iter.TakeReference(hdInput->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      svtkDataSet* ds = svtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (ds)
      {
        svtkPolyData* pd = svtkPolyData::SafeDownCast(iter->GetCurrentDataObject());
        svtkNew<svtkDataSetSurfaceFilter> surfaceFilter;
        if (!pd)
        {
          surfaceFilter->SetInputData(ds);
          surfaceFilter->Update();
          pd = surfaceFilter->GetOutput();
        }

        // Add each leaf to the integration model surfaces
        // Compute normals if non-present
        svtkNew<svtkPolyDataNormals> normals;
        if (!pd->GetCellData()->GetNormals())
        {
          normals->ComputePointNormalsOff();
          normals->ComputeCellNormalsOn();
          normals->SetInputData(pd);
          normals->Update();
          pd = normals->GetOutput();
        }
        if (pd->GetNumberOfCells() > 0)
        {
          this->IntegrationModel->AddDataSet(pd, /*surface*/ true, iter->GetCurrentFlatIndex());
        }
      }
    }
  }
  else if (dsInput)
  {
    svtkPolyData* pd = svtkPolyData::SafeDownCast(dsInput);
    svtkNew<svtkDataSetSurfaceFilter> surfaceFilter;
    if (!pd)
    {
      surfaceFilter->SetInputData(dsInput);
      surfaceFilter->Update();
      pd = surfaceFilter->GetOutput();
    }

    // Add surface to integration model
    // Compute normals if non-present
    svtkNew<svtkPolyDataNormals> normals;
    if (!pd->GetCellData()->GetNormals())
    {
      normals->ComputePointNormalsOff();
      normals->ComputeCellNormalsOn();
      normals->SetInputData(pd);
      normals->Update();
      pd = normals->GetOutput();
    }
    if (pd->GetNumberOfCells() > 0)
    {
      this->IntegrationModel->AddDataSet(pd, /*surface*/ true);
    }
  }
}

//---------------------------------------------------------------------------
bool svtkLagrangianParticleTracker::InitializeParticles(const svtkBoundingBox* bounds,
  svtkDataSet* seeds, std::queue<svtkLagrangianParticle*>& particles, svtkPointData* seedData)
{
  // Sanity check
  if (!seeds)
  {
    svtkErrorMacro(<< "Cannot generate Particles without seeds");
    return false;
  }

  // Recover data
  svtkDataArray* initialVelocities = nullptr;
  svtkDataArray* initialIntegrationTimes = nullptr;
  if (seeds->GetNumberOfPoints() > 0)
  {
    // Recover initial velocities, index 0
    initialVelocities =
      svtkDataArray::SafeDownCast(this->IntegrationModel->GetSeedArray(0, seedData));
    if (!initialVelocities)
    {
      svtkErrorMacro(<< "initialVelocity is not set in particle data, "
                       "unable to initialize particles!");
      return false;
    }

    // Recover initial integration time if any, index 1
    if (this->IntegrationModel->GetUseInitialIntegrationTime())
    {
      initialIntegrationTimes =
        svtkDataArray::SafeDownCast(this->IntegrationModel->GetSeedArray(1, seedData));
      if (!initialVelocities)
      {
        svtkWarningMacro("initialIntegrationTimes is not set in particle data, "
                        "initial integration time set to zero!");
      }
    }
  }

  // Create one particle for each point
  int nVar = this->IntegrationModel->GetNumberOfIndependentVariables();
  this->GenerateParticles(
    bounds, seeds, initialVelocities, initialIntegrationTimes, seedData, nVar, particles);
  return true;
}

//---------------------------------------------------------------------------
void svtkLagrangianParticleTracker::GenerateParticles(const svtkBoundingBox* svtkNotUsed(bounds),
  svtkDataSet* seeds, svtkDataArray* initialVelocities, svtkDataArray* initialIntegrationTimes,
  svtkPointData* seedData, int nVar, std::queue<svtkLagrangianParticle*>& particles)
{
  // Create and set a dummy particle so FindInLocators can use caching.
  std::unique_ptr<svtkLagrangianThreadedData> dummyData(new svtkLagrangianThreadedData);
  svtkLagrangianParticle dummyParticle(
    0, 0, 0, 0, 0, nullptr, this->IntegrationModel->GetWeightsSize(), 0);
  dummyParticle.SetThreadedData(dummyData.get());

  this->ParticleCounter = 0;
  this->IntegratedParticleCounter = 0;

  for (svtkIdType i = 0; i < seeds->GetNumberOfPoints(); i++)
  {
    double position[3];
    seeds->GetPoint(i, position);
    double initialIntegrationTime =
      initialIntegrationTimes ? initialIntegrationTimes->GetTuple1(i) : 0;
    svtkIdType particleId = this->GetNewParticleId();
    svtkLagrangianParticle* particle = new svtkLagrangianParticle(nVar, particleId, particleId, i,
      initialIntegrationTime, seedData, this->IntegrationModel->GetWeightsSize(),
      this->IntegrationModel->GetNumberOfTrackedUserData());
    memcpy(particle->GetPosition(), position, 3 * sizeof(double));
    initialVelocities->GetTuple(i, particle->GetVelocity());
    this->IntegrationModel->InitializeParticle(particle);
    if (this->IntegrationModel->FindInLocators(particle->GetPosition(), &dummyParticle))
    {
      particles.push(particle);
    }
    else
    {
      this->IntegrationModel->ParticleAboutToBeDeleted(particle);
      delete particle;
    }
  }
}

//---------------------------------------------------------------------------
void svtkLagrangianParticleTracker::GetParticleFeed(
  std::queue<svtkLagrangianParticle*>& svtkNotUsed(particleQueue))
{
}

//---------------------------------------------------------------------------
int svtkLagrangianParticleTracker::Integrate(svtkInitialValueProblemSolver* integrator,
  svtkLagrangianParticle* particle, std::queue<svtkLagrangianParticle*>& particlesQueue,
  svtkPolyData* particlePathsOutput, svtkPolyLine* particlePath, svtkDataObject* interactionOutput)
{
  // Sanity check
  if (!particle)
  {
    svtkErrorMacro(<< "Cannot integrate nullptr particle");
    return -1;
  }

  // Integrate until MaximumNumberOfSteps or MaximumIntegrationTime is reached or special case stops
  int integrationRes = 0;
  double stepFactor = this->StepFactor;
  double reintegrationFactor = 1;
  double& stepTimeActual = particle->GetStepTimeRef();
  while (particle->GetTermination() == svtkLagrangianParticle::PARTICLE_TERMINATION_NOT_TERMINATED)
  {
    // Compute step
    double velocityMagnitude = reintegrationFactor *
      std::max(this->MinimumVelocityMagnitude, svtkMath::Norm(particle->GetVelocity()));
    double cellLength = this->ComputeCellLength(particle);

    double stepLength = stepFactor * cellLength;
    double stepLengthMin = this->StepFactorMin * cellLength;
    double stepLengthMax = this->StepFactorMax * cellLength;
    double stepTime = stepLength / (reintegrationFactor * velocityMagnitude);
    double stepTimeMin = stepLengthMin / (reintegrationFactor * velocityMagnitude);
    double stepTimeMax = stepLengthMax / (reintegrationFactor * velocityMagnitude);

    // Integrate one step
    if (!this->ComputeNextStep(integrator, particle->GetEquationVariables(),
          particle->GetNextEquationVariables(), particle->GetIntegrationTime(), stepTime,
          stepTimeActual, stepTimeMin, stepTimeMax, cellLength, integrationRes, particle))
    {
      svtkErrorMacro(<< "Integration Error");
      break;
    }

    bool stagnating = std::abs(particle->GetPosition()[0] - particle->GetNextPosition()[0]) <
        std::numeric_limits<double>::epsilon() &&
      std::abs(particle->GetPosition()[1] - particle->GetNextPosition()[1]) <
        std::numeric_limits<double>::epsilon() &&
      std::abs(particle->GetPosition()[2] - particle->GetNextPosition()[2]) <
        std::numeric_limits<double>::epsilon();

    // Only stagnating OUT_OF_DOMAIN are actually out of domain
    bool outOfDomain = integrationRes == svtkInitialValueProblemSolver::OUT_OF_DOMAIN && stagnating;

    // Simpler Adaptive Step Reintegration code
    if (this->AdaptiveStepReintegration &&
      this->IntegrationModel->CheckAdaptiveStepReintegration(particle))
    {
      double stepLengthCurr2 =
        svtkMath::Distance2BetweenPoints(particle->GetPosition(), particle->GetNextPosition());
      double stepLengthMax2 = stepLengthMax * stepLengthMax;
      if (stepLengthCurr2 > stepLengthMax2)
      {
        reintegrationFactor *= 2;
        continue;
      }
      reintegrationFactor = 1;
    }

    if (outOfDomain)
    {
      // Stop integration
      particle->SetTermination(svtkLagrangianParticle::PARTICLE_TERMINATION_OUT_OF_DOMAIN);
      break;
    }

    // We care only about non-stagnating particle
    if (!stagnating)
    {
      // Surface interaction
      svtkLagrangianBasicIntegrationModel::PassThroughParticlesType passThroughParticles;
      unsigned int interactedSurfaceFlaxIndex;
      svtkLagrangianParticle* interactionParticle =
        this->IntegrationModel->ComputeSurfaceInteraction(
          particle, particlesQueue, interactedSurfaceFlaxIndex, passThroughParticles);
      if (interactionParticle)
      {
        this->InsertInteractionOutputPoint(
          interactionParticle, interactedSurfaceFlaxIndex, interactionOutput);
        this->IntegrationModel->ParticleAboutToBeDeleted(interactionParticle);
        delete interactionParticle;
        interactionParticle = nullptr;
      }

      // Insert pass through interaction points
      // Note: when going out of domain right after going some pass through
      // surfaces, the pass through interaction point will not be
      // on a particle track, since we do not want to show out of domain particle
      // track. The pass through interaction still has occurred and it is not a bug.
      while (!passThroughParticles.empty())
      {
        svtkLagrangianBasicIntegrationModel::PassThroughParticlesItem item =
          passThroughParticles.front();
        passThroughParticles.pop();

        this->InsertInteractionOutputPoint(item.second, item.first, interactionOutput);

        // the pass through particles needs to be deleted
        this->IntegrationModel->ParticleAboutToBeDeleted(item.second);
        delete item.second;
      }

      // Particle has been correctly integrated and interacted, record it
      // Insert Current particle as an output point

      if (this->GenerateParticlePathsOutput)
      {
        this->InsertPathOutputPoint(particle, particlePathsOutput, particlePath->GetPointIds());
      }

      // Particle has been terminated by surface
      if (particle->GetTermination() != svtkLagrangianParticle::PARTICLE_TERMINATION_NOT_TERMINATED)
      {
        // Insert last particle path point on surface
        particle->MoveToNextPosition();

        if (this->GenerateParticlePathsOutput)
        {
          this->InsertPathOutputPoint(particle, particlePathsOutput, particlePath->GetPointIds());
        }

        // stop integration
        break;
      }
    }

    if (this->IntegrationModel->CheckFreeFlightTermination(particle))
    {
      particle->SetTermination(svtkLagrangianParticle::PARTICLE_TERMINATION_FLIGHT_TERMINATED);
      break;
    }

    // Keep integrating
    particle->MoveToNextPosition();

    // Compute now adaptive step
    if (integrator->IsAdaptive() || this->AdaptiveStepReintegration)
    {
      stepFactor = stepTime * reintegrationFactor * velocityMagnitude / cellLength;
    }
    if (this->MaximumNumberOfSteps > -1 &&
      particle->GetNumberOfSteps() == this->MaximumNumberOfSteps &&
      particle->GetTermination() == svtkLagrangianParticle::PARTICLE_TERMINATION_NOT_TERMINATED)
    {
      particle->SetTermination(svtkLagrangianParticle::PARTICLE_TERMINATION_OUT_OF_STEPS);
    }
    if (this->MaximumIntegrationTime >= 0.0 &&
      particle->GetIntegrationTime() >= this->MaximumIntegrationTime &&
      particle->GetTermination() == svtkLagrangianParticle::PARTICLE_TERMINATION_NOT_TERMINATED)
    {
      particle->SetTermination(svtkLagrangianParticle::PARTICLE_TERMINATION_OUT_OF_TIME);
    }
  }

  if (this->GenerateParticlePathsOutput)
  {
    if (particlePath->GetPointIds()->GetNumberOfIds() == 1)
    {
      particlePath->GetPointIds()->InsertNextId(particlePath->GetPointId(0));
    }

    // Duplicate single point particle paths, to avoid degenerated lines.
    if (particlePath->GetPointIds()->GetNumberOfIds() > 0)
    {

      // Add particle path or vertex to cell array
      particlePathsOutput->GetLines()->InsertNextCell(particlePath);
      this->IntegrationModel->InsertPathData(particle, particlePathsOutput->GetCellData());

      // Insert data from seed data only in not yet written arrays
      this->IntegrationModel->InsertParticleSeedData(particle, particlePathsOutput->GetCellData());
    }
  }

  return integrationRes;
}

//---------------------------------------------------------------------------
void svtkLagrangianParticleTracker::InsertPathOutputPoint(svtkLagrangianParticle* particle,
  svtkPolyData* particlePathsOutput, svtkIdList* particlePathPointId, bool prev)
{
  // Recover structures
  svtkPoints* particlePathsPoints = particlePathsOutput->GetPoints();
  svtkPointData* particlePathsPointData = particlePathsOutput->GetPointData();

  // Store previous position
  svtkIdType pointId = particlePathsPoints->InsertNextPoint(
    prev ? particle->GetPrevPosition() : particle->GetPosition());

  particlePathPointId->InsertNextId(pointId);

  // Insert particle data
  this->IntegrationModel->InsertParticleData(particle, particlePathsPointData,
    prev ? svtkLagrangianBasicIntegrationModel::VARIABLE_STEP_PREV
         : svtkLagrangianBasicIntegrationModel::VARIABLE_STEP_CURRENT);
}

//---------------------------------------------------------------------------
void svtkLagrangianParticleTracker::InsertInteractionOutputPoint(svtkLagrangianParticle* particle,
  unsigned int interactedSurfaceFlatIndex, svtkDataObject* interactionOutput)
{
  // Find the correct output
  svtkCompositeDataSet* hdOutput = svtkCompositeDataSet::SafeDownCast(interactionOutput);
  svtkPolyData* pdOutput = svtkPolyData::SafeDownCast(interactionOutput);
  svtkPolyData* interactionPd = nullptr;
  if (hdOutput)
  {
    svtkSmartPointer<svtkCompositeDataIterator> iter;
    iter.TakeReference(hdOutput->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      if (interactedSurfaceFlatIndex == iter->GetCurrentFlatIndex())
      {
        interactionPd = svtkPolyData::SafeDownCast(hdOutput->GetDataSet(iter));
        break;
      }
    }
  }
  else if (pdOutput)
  {
    interactionPd = pdOutput;
  }

  if (!interactionPd)
  {
    svtkErrorMacro(<< "Something went wrong with interaction output, "
                     "cannot find correct interaction output polydata");
    return;
  }

  // "Next" Point
  svtkPoints* points = interactionPd->GetPoints();
  points->InsertNextPoint(particle->GetNextPosition());

  // Fill up interaction point data
  svtkPointData* pointData = interactionPd->GetPointData();
  this->IntegrationModel->InsertPathData(particle, pointData);
  this->IntegrationModel->InsertInteractionData(particle, pointData);
  this->IntegrationModel->InsertParticleData(
    particle, pointData, svtkLagrangianBasicIntegrationModel::VARIABLE_STEP_NEXT);

  // Finally, Insert data from seed data only on not yet written arrays
  this->IntegrationModel->InsertParticleSeedData(particle, pointData);
}

//---------------------------------------------------------------------------
double svtkLagrangianParticleTracker::ComputeCellLength(svtkLagrangianParticle* particle)
{
  double cellLength = 1.0;
  svtkDataSet* dataset = nullptr;
  svtkGenericCell* cell = particle->GetThreadedData()->GenericCell;
  if (!cell)
  {
    svtkErrorMacro("Could not recover a generic cell for cell length computation");
    return 1.0;
  }
  bool forceLastCell = false;
  if (this->CellLengthComputationMode == STEP_CUR_CELL_LENGTH ||
    this->CellLengthComputationMode == STEP_CUR_CELL_VEL_DIR ||
    this->CellLengthComputationMode == STEP_CUR_CELL_DIV_THEO)
  {
    svtkIdType cellId;
    svtkAbstractCellLocator* loc;
    double* weights = particle->GetLastWeights();
    if (this->IntegrationModel->FindInLocators(
          particle->GetPosition(), particle, dataset, cellId, loc, weights))
    {
      dataset->GetCell(cellId, cell);
    }
    else
    {
      forceLastCell = true;
    }
  }
  if (this->CellLengthComputationMode == STEP_LAST_CELL_LENGTH ||
    this->CellLengthComputationMode == STEP_LAST_CELL_VEL_DIR ||
    this->CellLengthComputationMode == STEP_LAST_CELL_DIV_THEO || forceLastCell)
  {
    dataset = particle->GetLastDataSet();
    if (!dataset)
    {
      return cellLength;
    }
    dataset->GetCell(particle->GetLastCellId(), cell);
    if (!cell)
    {
      return cellLength;
    }
  }

  double* vel = particle->GetVelocity();
  if ((this->CellLengthComputationMode == STEP_CUR_CELL_VEL_DIR ||
        this->CellLengthComputationMode == STEP_LAST_CELL_VEL_DIR) &&
    svtkMath::Norm(vel) > 0.0)
  {
    double velHat[3] = { vel[0], vel[1], vel[2] };
    svtkMath::Normalize(velHat);
    double tmpCellLength = 0.0;
    for (int ne = 0; ne < cell->GetNumberOfEdges(); ++ne)
    {
      double evect[3], x0[3], x1[3];
      svtkCell* edge = cell->GetEdge(ne);
      svtkIdType e0 = edge->GetPointId(0);
      svtkIdType e1 = edge->GetPointId(1);

      dataset->GetPoint(e0, x0);
      dataset->GetPoint(e1, x1);
      svtkMath::Subtract(x0, x1, evect);
      double elength = std::fabs(svtkMath::Dot(evect, velHat));
      tmpCellLength = std::max(tmpCellLength, elength);
    }
    cellLength = tmpCellLength;
  }
  else if ((this->CellLengthComputationMode == STEP_CUR_CELL_DIV_THEO ||
             this->CellLengthComputationMode == STEP_LAST_CELL_DIV_THEO) &&
    svtkMath::Norm(vel) > 0.0)
  {
    double velHat[3] = { vel[0], vel[1], vel[2] };
    svtkMath::Normalize(velHat);
    double xa = 0.0;  // cell cross-sectional area in velHat direction
    double vol = 0.0; // cell volume
    for (int nf = 0; nf < cell->GetNumberOfFaces(); ++nf)
    {
      double norm[3];                         // cell face normal
      double centroid[3] = { 0.0, 0.0, 0.0 }; // cell face centroid
      svtkCell* face = cell->GetFace(nf);
      svtkPoints* pts = face->GetPoints();
      svtkIdType nPoints = pts->GetNumberOfPoints();
      const double area = svtkPolygon::ComputeArea(pts, nPoints, nullptr, norm);
      const double fact = 1.0 / static_cast<double>(nPoints);
      for (int np = 0; np < nPoints; ++np)
      {
        double* x = pts->GetPoint(np);
        for (int nc = 0; nc < 3; ++nc)
        {
          centroid[nc] += x[nc] * fact;
        }
      }
      xa += std::fabs(svtkMath::Dot(norm, velHat) * area) / 2.0; // sum unsigned areas
      vol += svtkMath::Dot(norm, centroid) * area / 3.0;         // using divergence theorem
    }
    // characteristic length is cell volume / cell cross-sectional area in velocity direction
    // Absolute value of volume because of some Fluent cases where all the volumes seem negative
    cellLength = std::fabs(vol) / xa;
  }
  else
  {
    cellLength = std::sqrt(cell->GetLength2());
  }
  return cellLength;
}

//---------------------------------------------------------------------------
bool svtkLagrangianParticleTracker::ComputeNextStep(svtkInitialValueProblemSolver* integrator,
  double* xprev, double* xnext, double t, double& delT, double& delTActual, double minStep,
  double maxStep, double cellLength, int& integrationRes, svtkLagrangianParticle* particle)
{
  // Check for potential manual integration
  double error;
  if (!this->IntegrationModel->ManualIntegration(integrator, xprev, xnext, t, delT, delTActual,
        minStep, maxStep, this->IntegrationModel->GetTolerance(), cellLength, error, integrationRes,
        particle))
  {
    // integrate one step
    integrationRes = integrator->ComputeNextStep(xprev, xnext, t, delT, delTActual, minStep,
      maxStep, this->IntegrationModel->GetTolerance(), error, particle);
  }

  // Check failure cases
  if (integrationRes == svtkInitialValueProblemSolver::NOT_INITIALIZED)
  {
    svtkErrorMacro(<< "Integrator is not initialized. Aborting.");
    return false;
  }
  if (integrationRes == svtkInitialValueProblemSolver::UNEXPECTED_VALUE)
  {
    svtkErrorMacro(<< "Integrator encountered an unexpected value. Dropping particle.");
    return false;
  }
  return true;
}
