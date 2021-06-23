/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLagrangianBasicIntegrationModel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkLagrangianBasicIntegrationModel.h"

#include "svtkBilinearQuadIntersection.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkDataObjectTypes.h"
#include "svtkDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkFieldData.h"
#include "svtkGenericCell.h"
#include "svtkIntArray.h"
#include "svtkLagrangianParticle.h"
#include "svtkLagrangianParticleTracker.h"
#include "svtkLagrangianThreadedData.h"
#include "svtkLongLongArray.h"
#include "svtkMath.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkQuad.h"
#include "svtkSetGet.h"
#include "svtkSmartPointer.h"
#include "svtkStaticCellLocator.h"
#include "svtkStringArray.h"
#include "svtkUnsignedCharArray.h"
#include "svtkVector.h"

#include <cassert>
#include <mutex>
#include <set>
#include <sstream>
#include <vector>

#define USER_SURFACE_TYPE 100 // Minimal value for user defined surface type

//----------------------------------------------------------------------------
typedef std::vector<svtkSmartPointer<svtkAbstractCellLocator> > LocatorsTypeBase;
class svtkLocatorsType : public LocatorsTypeBase
{
};

typedef std::vector<svtkSmartPointer<svtkDataSet> > DataSetsTypeBase;
class svtkDataSetsType : public DataSetsTypeBase
{
};

typedef std::pair<unsigned int, svtkSmartPointer<svtkDataSet> > SurfaceItem;
typedef std::vector<SurfaceItem> SurfaceTypeBase;
class svtkSurfaceType : public SurfaceTypeBase
{
};

typedef std::pair<unsigned int, double> PassThroughItem;
typedef std::set<PassThroughItem> PassThroughSetType;

//----------------------------------------------------------------------------
svtkLagrangianBasicIntegrationModel::svtkLagrangianBasicIntegrationModel()
  : Locator(nullptr)
  , Tolerance(1.0e-8)
  , NonPlanarQuadSupport(false)
  , UseInitialIntegrationTime(false)
  , Tracker(nullptr)
{
  SurfaceArrayDescription surfaceTypeDescription;
  surfaceTypeDescription.nComp = 1;
  surfaceTypeDescription.type = SVTK_INT;
  surfaceTypeDescription.enumValues.push_back(std::make_pair(SURFACE_TYPE_MODEL, "ModelDefined"));
  surfaceTypeDescription.enumValues.push_back(std::make_pair(SURFACE_TYPE_TERM, "Terminate"));
  surfaceTypeDescription.enumValues.push_back(std::make_pair(SURFACE_TYPE_BOUNCE, "Bounce"));
  surfaceTypeDescription.enumValues.push_back(std::make_pair(SURFACE_TYPE_BREAK, "BreakUp"));
  surfaceTypeDescription.enumValues.push_back(std::make_pair(SURFACE_TYPE_PASS, "PassThrough"));
  this->SurfaceArrayDescriptions["SurfaceType"] = surfaceTypeDescription;

  this->SeedArrayNames->InsertNextValue("ParticleInitialVelocity");
  this->SeedArrayComps->InsertNextValue(3);
  this->SeedArrayTypes->InsertNextValue(SVTK_DOUBLE);
  this->SeedArrayNames->InsertNextValue("ParticleInitialIntegrationTime");
  this->SeedArrayComps->InsertNextValue(1);
  this->SeedArrayTypes->InsertNextValue(SVTK_DOUBLE);

  this->Locators = new svtkLocatorsType;
  this->DataSets = new svtkDataSetsType;
  this->Surfaces = new svtkSurfaceType;
  this->SurfaceLocators = new svtkLocatorsType;

  // Using a svtkStaticCellLocator by default
  svtkNew<svtkStaticCellLocator> locator;
  this->SetLocator(locator);
  this->LocatorsBuilt = false;
}

//----------------------------------------------------------------------------
svtkLagrangianBasicIntegrationModel::~svtkLagrangianBasicIntegrationModel()
{
  this->ClearDataSets();
  this->ClearDataSets(true);
  this->SetLocator(nullptr);
  delete this->Locators;
  delete this->DataSets;
  delete this->Surfaces;
  delete this->SurfaceLocators;
}

//----------------------------------------------------------------------------
void svtkLagrangianBasicIntegrationModel::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->Locator)
  {
    os << indent << "Locator: " << endl;
    this->Locator->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Locator: " << this->Locator << endl;
  }
  os << indent << "Tolerance: " << this->Tolerance << endl;
}

//----------------------------------------------------------------------------
void svtkLagrangianBasicIntegrationModel::SetTracker(svtkLagrangianParticleTracker* tracker)
{
  this->Tracker = tracker;
}

//----------------------------------------------------------------------------
void svtkLagrangianBasicIntegrationModel::AddDataSet(
  svtkDataSet* dataset, bool surface, unsigned int surfaceFlatIndex)
{
  // Sanity check
  if (!dataset || dataset->GetNumberOfPoints() == 0 || dataset->GetNumberOfCells() == 0)
  {
    svtkErrorMacro(<< "Dataset is null or empty");
    return;
  }

  if (!this->Locator)
  {
    svtkErrorMacro(<< "Locator is null");
    return;
  }

  // There seems to be some kind of problem with the garbage collector
  // and the referencing of datasets and locators.
  // In order to avoid leaks we shallow copy the dataset.
  // This could be removed once this problem is fixed.
  svtkSmartPointer<svtkDataObject> dob;
  dob.TakeReference(svtkDataObjectTypes::NewDataObject(dataset->GetDataObjectType()));
  svtkDataSet* datasetCpy = svtkDataSet::SafeDownCast(dob);
  datasetCpy->ShallowCopy(dataset);

  // insert the dataset into DataSet vector
  if (surface)
  {
    this->Surfaces->push_back(std::make_pair(surfaceFlatIndex, datasetCpy));
  }
  else
  {
    this->DataSets->push_back(datasetCpy);
  }

  // insert a locator into Locators vector, non-null only for svtkPointSet
  svtkSmartPointer<svtkAbstractCellLocator> locator = nullptr;
  if (dataset->IsA("svtkPointSet"))
  {
    if (surface)
    {
      locator.TakeReference(svtkStaticCellLocator::New());
    }
    else
    {
      locator.TakeReference(this->Locator->NewInstance());
    }

    locator->SetDataSet(datasetCpy);
    locator->CacheCellBoundsOn();
    locator->AutomaticOn();
    locator->BuildLocator();
  }
  else
  {
    // for non-svtkPointSet svtkDataSet, we are using their internal locator
    // It is required to do a findCell call before the threaded code
    // so the locator is built first.
    double x[3];
    dataset->GetPoint(0, x);

    svtkNew<svtkGenericCell> cell;
    dataset->GetCell(0, cell);

    int subId;
    double pcoords[3];
    std::vector<double> weights(dataset->GetMaxCellSize());
    dataset->FindCell(x, nullptr, cell, 0, 0, subId, pcoords, weights.data());
  }

  // Add locator
  if (surface)
  {
    this->SurfaceLocators->push_back(locator);
  }
  else
  {
    this->Locators->push_back(locator);

    int size = dataset->GetMaxCellSize();
    if (size > static_cast<int>(this->SharedWeights.size()))
    {
      this->SharedWeights.resize(size);
    }
  }
}

//----------------------------------------------------------------------------
void svtkLagrangianBasicIntegrationModel::ClearDataSets(bool surface)
{
  if (surface)
  {
    this->Surfaces->clear();
    this->SurfaceLocators->clear();
  }
  else
  {
    this->DataSets->clear();
    this->Locators->clear();
    this->SharedWeights.clear();
  }
}

//----------------------------------------------------------------------------
int svtkLagrangianBasicIntegrationModel::FunctionValues(double* x, double* f, void* userData)
{
  // Sanity check
  if (this->DataSets->empty())
  {
    svtkErrorMacro(<< "Please add a dataset to the integration model before integrating.");
    return 0;
  }
  svtkLagrangianParticle* particle = static_cast<svtkLagrangianParticle*>(userData);
  if (!particle)
  {
    svtkErrorMacro(<< "Could not recover svtkLagrangianParticle");
    return 0;
  }
  svtkAbstractCellLocator* loc;
  svtkDataSet* ds;
  svtkIdType cellId;
  double* weights = particle->GetLastWeights();
  if (this->FindInLocators(x, particle, ds, cellId, loc, weights))
  {
    // Evaluate integration model velocity field with the found cell
    return this->FunctionValues(particle, ds, cellId, weights, x, f);
  }

  // Can't evaluate
  return 0;
}

//---------------------------------------------------------------------------
void svtkLagrangianBasicIntegrationModel::SetLocator(svtkAbstractCellLocator* locator)
{
  if (this->Locator != locator)
  {
    svtkAbstractCellLocator* temp = this->Locator;
    this->Locator = locator;
    if (this->Locator)
    {
      this->Locator->Register(this);
    }
    if (temp)
    {
      temp->UnRegister(this);
    }
    this->Modified();
    this->LocatorsBuilt = false;
  }
}

//---------------------------------------------------------------------------
svtkLagrangianParticle* svtkLagrangianBasicIntegrationModel::ComputeSurfaceInteraction(
  svtkLagrangianParticle* particle, std::queue<svtkLagrangianParticle*>& particles,
  unsigned int& surfaceFlatIndex, PassThroughParticlesType& passThroughParticles)
{
  svtkDataSet* surface = nullptr;
  double interFactor = 1.0;
  svtkIdType cellId = -1;
  int surfaceType = -1;
  PassThroughSetType passThroughInterSet;
  bool perforation;
  do
  {
    passThroughInterSet.clear();
    perforation = false;
    for (size_t iDs = 0; iDs < this->Surfaces->size(); iDs++)
    {
      svtkAbstractCellLocator* loc = (*this->SurfaceLocators)[iDs];
      svtkDataSet* tmpSurface = (*this->Surfaces)[iDs].second;
      svtkGenericCell* cell = particle->GetThreadedData()->GenericCell;
      svtkIdList* cellList = particle->GetThreadedData()->IdList;
      cellList->Reset();
      loc->FindCellsAlongLine(
        particle->GetPosition(), particle->GetNextPosition(), this->Tolerance, cellList);
      for (svtkIdType i = 0; i < cellList->GetNumberOfIds(); i++)
      {
        double tmpFactor;
        double tmpPoint[3];
        svtkIdType tmpCellId = cellList->GetId(i);
        tmpSurface->GetCell(tmpCellId, cell);
        if (this->IntersectWithLine(particle, cell->GetRepresentativeCell(),
              particle->GetPosition(), particle->GetNextPosition(), this->Tolerance, tmpFactor,
              tmpPoint) == 0)
        {
          // FindCellAlongsLines sometimes get false positives
          continue;
        }
        if (tmpFactor < interFactor)
        {
          // Recover surface type for this cell
          double surfaceTypeDbl;

          // "SurfaceType" is at index 2
          int surfaceIndex = 2;

          svtkIdType surfaceTupleId = tmpCellId;

          // When using field data surface type, tuple index is 0
          int ret = this->GetFlowOrSurfaceDataFieldAssociation(surfaceIndex);
          if (ret == -1)
          {
            svtkErrorMacro(<< "Surface Type is not correctly set in surface dataset");
            return nullptr;
          }
          if (ret == svtkDataObject::FIELD_ASSOCIATION_NONE)
          {
            surfaceTupleId = 0;
          }
          if (!this->GetFlowOrSurfaceData(
                particle, surfaceIndex, tmpSurface, surfaceTupleId, nullptr, &surfaceTypeDbl))
          {
            svtkErrorMacro(
              << "Surface Type is not set in surface dataset or"
                 " have incorrect number of components, cannot use surface interaction");
            return nullptr;
          }
          int tmpSurfaceType = static_cast<int>(surfaceTypeDbl);
          if (tmpSurfaceType == svtkLagrangianBasicIntegrationModel::SURFACE_TYPE_PASS)
          {
            // Pass Through Surface, store for later
            passThroughInterSet.insert(std::make_pair((*this->Surfaces)[iDs].first, tmpFactor));
          }
          else
          {
            if (tmpSurface == particle->GetLastSurfaceDataSet() &&
              tmpCellId == particle->GetLastSurfaceCellId())
            {
              perforation = this->CheckSurfacePerforation(particle, tmpSurface, tmpCellId);
              if (perforation)
              {
                break;
              }
              continue;
            }

            // Interacting surface
            interFactor = tmpFactor;
            surface = tmpSurface;
            surfaceFlatIndex = (*this->Surfaces)[iDs].first;
            surfaceType = tmpSurfaceType;
            cellId = tmpCellId;
          }
        }
      }
    }
  } while (perforation);

  PassThroughSetType::iterator it;
  for (it = passThroughInterSet.begin(); it != passThroughInterSet.end(); ++it)
  {
    PassThroughItem item = *it;
    // As one cas see in the test above, if a pass through surface intersects at the exact
    // same location than the point computed using the intersection factor,
    // we do not store the intersection.
    // pass through are considered non prioritary, and do not intersects
    // when at the exact the same place as the main intersection
    if (item.second < interFactor)
    {
      svtkLagrangianParticle* clone = particle->CloneParticle();
      clone->SetInteraction(svtkLagrangianParticle::SURFACE_INTERACTION_PASS);
      this->InterpolateNextParticleVariables(clone, item.second);
      passThroughParticles.push(std::make_pair(item.first, clone));
    }
  }

  // Store surface cache (even nullptr one)
  particle->SetLastSurfaceCell(surface, cellId);

  bool recordInteraction = false;
  svtkLagrangianParticle* interactionParticle = nullptr;
  if (cellId != -1)
  {
    // There is an actual interaction
    // Position next point onto surface
    this->InterpolateNextParticleVariables(particle, interFactor, true);
    interactionParticle = particle->CloneParticle();
    switch (surfaceType)
    {
      case svtkLagrangianBasicIntegrationModel::SURFACE_TYPE_TERM:
        recordInteraction = this->TerminateParticle(particle);
        break;
      case svtkLagrangianBasicIntegrationModel::SURFACE_TYPE_BOUNCE:
        recordInteraction = this->BounceParticle(particle, surface, cellId);
        break;
      case svtkLagrangianBasicIntegrationModel::SURFACE_TYPE_BREAK:
        recordInteraction = this->BreakParticle(particle, surface, cellId, particles);
        break;
      case svtkLagrangianBasicIntegrationModel::SURFACE_TYPE_PASS:
        svtkErrorMacro(<< "Something went wrong with pass-through surface, "
                         "next results will be invalid.");
        return nullptr;
      default:
        if (surfaceType != SURFACE_TYPE_MODEL && surfaceType < USER_SURFACE_TYPE)
        {
          svtkWarningMacro("Please do not use user defined surface type under "
            << USER_SURFACE_TYPE
            << " as they may be used in the future by the Lagrangian Particle Tracker");
        }
        recordInteraction =
          this->InteractWithSurface(surfaceType, particle, surface, cellId, particles);
        break;
    }
    interactionParticle->SetInteraction(particle->GetInteraction());
  }
  if (!recordInteraction)
  {
    delete interactionParticle;
    interactionParticle = nullptr;
  }
  return interactionParticle;
}

//----------------------------------------------------------------------------
bool svtkLagrangianBasicIntegrationModel::TerminateParticle(svtkLagrangianParticle* particle)
{
  particle->SetTermination(svtkLagrangianParticle::PARTICLE_TERMINATION_SURF_TERMINATED);
  particle->SetInteraction(svtkLagrangianParticle::SURFACE_INTERACTION_TERMINATED);
  return true;
}

//----------------------------------------------------------------------------
bool svtkLagrangianBasicIntegrationModel::BounceParticle(
  svtkLagrangianParticle* particle, svtkDataSet* surface, svtkIdType cellId)
{
  particle->SetInteraction(svtkLagrangianParticle::SURFACE_INTERACTION_BOUNCE);

  // Recover surface normal
  // Surface should have been computed already
  assert(surface->GetCellData()->GetNormals() != nullptr);
  double normal[3];
  surface->GetCellData()->GetNormals()->GetTuple(cellId, normal);

  // Change velocity for bouncing and set interaction point
  double* nextVel = particle->GetNextVelocity();
  double dot = svtkMath::Dot(normal, nextVel);
  for (int i = 0; i < 3; i++)
  {
    nextVel[i] = nextVel[i] - 2 * dot * normal[i];
  }
  return true;
}

//----------------------------------------------------------------------------
bool svtkLagrangianBasicIntegrationModel::BreakParticle(svtkLagrangianParticle* particle,
  svtkDataSet* surface, svtkIdType cellId, std::queue<svtkLagrangianParticle*>& particles)
{
  // Terminate particle
  particle->SetTermination(svtkLagrangianParticle::PARTICLE_TERMINATION_SURF_BREAK);
  particle->SetInteraction(svtkLagrangianParticle::SURFACE_INTERACTION_BREAK);

  // Recover surface normal
  // Surface should have been computed already
  assert(surface->GetCellData()->GetNormals() != nullptr);
  double normal[3];
  surface->GetCellData()->GetNormals()->GetTuple(cellId, normal);

  // Create new particles
  svtkLagrangianParticle* particle1 = particle->NewParticle(this->Tracker->GetNewParticleId());
  svtkLagrangianParticle* particle2 = particle->NewParticle(this->Tracker->GetNewParticleId());

  // Compute bounce for each new particle
  double* nextVel = particle->GetNextVelocity();
  double* part1Vel = particle1->GetVelocity();
  double* part2Vel = particle2->GetVelocity();
  double dot = svtkMath::Dot(normal, nextVel);
  double cross[3];
  svtkMath::Cross(normal, nextVel, cross);
  double bounceNorm = svtkMath::Norm(nextVel);

  for (int i = 0; i < 3; i++)
  {
    part1Vel[i] = nextVel[i] - 2 * dot * normal[i] + cross[i];
    part2Vel[i] = nextVel[i] - 2 * dot * normal[i] - cross[i];
  }
  double part1Norm = svtkMath::Norm(part1Vel);
  double part2Norm = svtkMath::Norm(part2Vel);
  for (int i = 0; i < 3; i++)
  {
    if (part1Norm != 0.0)
    {
      part1Vel[i] = part1Vel[i] / part1Norm * bounceNorm;
    }
    if (part2Norm != 0.0)
    {
      part2Vel[i] = part2Vel[i] / part2Norm * bounceNorm;
    }
  }

  // push new particle in queue
  // Mutex Locked Area
  std::lock_guard<std::mutex> guard(this->ParticleQueueMutex);
  particles.push(particle1);
  particles.push(particle2);
  return true;
}

//----------------------------------------------------------------------------
bool svtkLagrangianBasicIntegrationModel::InteractWithSurface(int svtkNotUsed(surfaceType),
  svtkLagrangianParticle* particle, svtkDataSet* svtkNotUsed(surface), svtkIdType svtkNotUsed(cellId),
  std::queue<svtkLagrangianParticle*>& svtkNotUsed(particles))
{
  return this->TerminateParticle(particle);
}

//----------------------------------------------------------------------------
bool svtkLagrangianBasicIntegrationModel::IntersectWithLine(svtkLagrangianParticle* particle,
  svtkCell* cell, double p1[3], double p2[3], double tol, double& t, double x[3])
{
  // Non planar quad support
  if (this->NonPlanarQuadSupport)
  {
    svtkQuad* quad = svtkQuad::SafeDownCast(cell);
    if (quad)
    {
      if (p1[0] == p2[0] && p1[1] == p2[1] && p1[2] == p2[2])
      {
        // the 2 points are the same, no intersection
        return false;
      }

      // create 4 points and fill the bqi
      svtkPoints* points = quad->GetPoints();
      svtkBilinearQuadIntersection* bqi = particle->GetThreadedData()->BilinearQuadIntersection;
      points->GetPoint(0, bqi->GetP00Data());
      points->GetPoint(3, bqi->GetP01Data());
      points->GetPoint(1, bqi->GetP10Data());
      points->GetPoint(2, bqi->GetP11Data());

      // Create the ray
      svtkVector3d r(p1[0], p1[1], p1[2]);                         // origin of the ray
      svtkVector3d q(p2[0] - p1[0], p2[1] - p1[1], p2[2] - p1[2]); // a ray direction

      // the original t before q is normalised
      double tOrig = q.Norm();
      q.Normalize();

      svtkVector3d uv;                     // variables returned
      if (bqi->RayIntersection(r, q, uv)) // run intersection test
      {
        // we have an intersection
        t = uv.GetZ() / tOrig;
        if (t >= 0.0 && t <= 1.0)
        {
          // Recover intersection between p1 and p2
          svtkVector3d intersec = bqi->ComputeCartesianCoordinates(uv.GetX(), uv.GetY());
          x[0] = intersec.GetX();
          x[1] = intersec.GetY();
          x[2] = intersec.GetZ();
          return true;
        }
        else
        {
          // intersection outside of p1p2
          return false;
        }
      }
      else
      {
        // no intersection
        return false;
      }
    }
  }

  // Standard cell intersection
  double pcoords[3];
  int subId;
  int ret = cell->IntersectWithLine(p1, p2, tol, t, x, pcoords, subId);
  if (ret != 0)
  {
    return true;
  }
  else
  {
    return false;
  }
}

//----------------------------------------------------------------------------
void svtkLagrangianBasicIntegrationModel::InterpolateNextParticleVariables(
  svtkLagrangianParticle* particle, double interpolationFactor, bool forceInside)
{
  if (forceInside)
  {
    // Reducing interpolationFactor to ensure we stay in domain
    double magnitude = particle->GetPositionVectorMagnitude();
    interpolationFactor *= (magnitude - this->Tolerance / interpolationFactor) / magnitude;
  }

  double* current = particle->GetEquationVariables();
  double* next = particle->GetNextEquationVariables();
  for (int i = 0; i < particle->GetNumberOfVariables(); i++)
  {
    next[i] = current[i] + (next[i] - current[i]) * interpolationFactor;
  }
  double& stepTime = particle->GetStepTimeRef();
  stepTime *= interpolationFactor;
}

//----------------------------------------------------------------------------
bool svtkLagrangianBasicIntegrationModel::CheckSurfacePerforation(
  svtkLagrangianParticle* particle, svtkDataSet* surface, svtkIdType cellId)
{
  // Recover surface normal
  // Surface should have been computed already
  assert(surface->GetCellData()->GetNormals() != nullptr);
  double normal[3];
  surface->GetCellData()->GetNormals()->GetTuple(cellId, normal);

  // Recover particle vector
  double prevToCurr[3];
  double currToNext[3];
  for (int i = 0; i < 3; i++)
  {
    prevToCurr[i] = particle->GetPosition()[i] - particle->GetPrevPosition()[i];
    currToNext[i] = particle->GetNextPosition()[i] - particle->GetPosition()[i];
  }

  // Check directions
  double dot = svtkMath::Dot(normal, currToNext);
  double prevDot = svtkMath::Dot(normal, prevToCurr);
  double* nextVel = particle->GetNextVelocity();
  double velDot = svtkMath::Dot(normal, nextVel);
  if (dot == 0 || prevDot == 0 || prevDot * dot > 0)
  {
    // vector does not project on the same directions, perforation !
    for (int i = 0; i < 3; i++)
    {
      // Simple perforation management via symmetry
      currToNext[i] = currToNext[i] - 2 * dot * normal[i];
      particle->GetNextPosition()[i] = particle->GetPosition()[i] + currToNext[i];
      nextVel[i] = nextVel[i] - 2 * velDot * normal[i];
    }
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------
void svtkLagrangianBasicIntegrationModel::SetInputArrayToProcess(
  int idx, int port, int connection, int fieldAssociation, const char* name)
{
  // Store the array metadata
  svtkLagrangianBasicIntegrationModel::ArrayVal vals;
  vals.val[0] = port;
  vals.val[1] = connection;
  vals.val[2] = fieldAssociation;
  ArrayMapVal array = ArrayMapVal(vals, name);
  this->InputArrays[idx] = array;
  this->Modified();
}

//----------------------------------------------------------------------------
bool svtkLagrangianBasicIntegrationModel::FindInLocators(double* x, svtkLagrangianParticle* particle)
{
  svtkIdType cellId;
  svtkDataSet* dataset;
  return this->FindInLocators(x, particle, dataset, cellId);
}

//----------------------------------------------------------------------------
bool svtkLagrangianBasicIntegrationModel::FindInLocators(
  double* x, svtkLagrangianParticle* particle, svtkDataSet*& dataset, svtkIdType& cellId)
{
  svtkAbstractCellLocator* loc;
  double* weights = this->SharedWeights.data();
  return this->FindInLocators(x, particle, dataset, cellId, loc, weights);
}

//----------------------------------------------------------------------------
bool svtkLagrangianBasicIntegrationModel::FindInLocators(double* x, svtkLagrangianParticle* particle,
  svtkDataSet*& dataset, svtkIdType& cellId, svtkAbstractCellLocator*& loc, double*& weights)
{
  // Sanity check
  if (this->DataSets->empty())
  {
    return false;
  }

  svtkGenericCell* cell = particle->GetThreadedData()->GenericCell;

  // Try the provided cache
  dataset = particle->GetLastDataSet();
  loc = particle->GetLastLocator();
  cellId = particle->GetLastCellId();
  double* lastPosition = particle->GetLastCellPosition();
  if (dataset)
  {
    // Check the last cell
    if (cellId != -1)
    {
      // Check if previous call was the same
      if (lastPosition[0] == x[0] && lastPosition[1] == x[1] && lastPosition[2] == x[2])
      {
        return true;
      }

      // If not, check if new position is in the same cell
      double pcoords[3];
      int subId;
      double dist2;
      dataset->GetCell(cellId, cell);
      if (cell->EvaluatePosition(x, nullptr, subId, pcoords, dist2, weights) == 1)
      {
        return true;
      }
    }

    // Not in provided cell cache, try the whole dataset
    cellId = this->FindInLocator(dataset, loc, x, cell, weights);
    if (cellId != -1)
    {
      particle->SetLastCell(loc, dataset, cellId, x);
      return true;
    }
  }

  // No cache or Cache miss, try other datasets
  svtkDataSet* lastDataSet = dataset;
  for (size_t iDs = 0; iDs < this->DataSets->size(); iDs++)
  {
    loc = (*this->Locators)[iDs];
    dataset = (*this->DataSets)[iDs];
    if (dataset != lastDataSet)
    {
      cellId = this->FindInLocator(dataset, loc, x, cell, weights);
      if (cellId != -1)
      {
        // Store the found cell for caching purpose
        particle->SetLastCell(loc, dataset, cellId, x);
        return true;
      }
    }
  }
  return false;
}

//----------------------------------------------------------------------------
svtkIdType svtkLagrangianBasicIntegrationModel::FindInLocator(
  svtkDataSet* ds, svtkAbstractCellLocator* loc, double* x, svtkGenericCell* cell, double* weights)
{
  double pcoords[3];
  svtkIdType cellId;
  if (loc)
  {
    // Use locator to find the cell containing x
    cellId = loc->FindCell(x, this->Tolerance, cell, pcoords, weights);
  }
  else
  {
    // No locator, ds is svtkImageData or svtkRectilinearGrid,
    // which does not require any cellToUse when calling FindCell.
    int subId;
    cellId = ds->FindCell(x, nullptr, cell, 0, this->Tolerance, subId, pcoords, weights);
  }

  // Ignore Ghost cells
  if (cellId != -1 && ds->GetCellGhostArray() &&
    ds->GetCellGhostArray()->GetValue(cellId) & svtkDataSetAttributes::DUPLICATECELL)
  {
    return -1;
  }
  return cellId;
}

//----------------------------------------------------------------------------
svtkAbstractArray* svtkLagrangianBasicIntegrationModel::GetSeedArray(
  int idx, svtkLagrangianParticle* particle)
{
  return this->GetSeedArray(idx, particle->GetSeedData());
}

//----------------------------------------------------------------------------
svtkAbstractArray* svtkLagrangianBasicIntegrationModel::GetSeedArray(int idx, svtkPointData* pointData)
{
  // Check the provided index
  if (this->InputArrays.count(idx) == 0)
  {
    svtkErrorMacro(<< "No arrays at index:" << idx);
    return nullptr;
  }

  ArrayMapVal arrayIndexes = this->InputArrays[idx];

  // Check port, should be 1 for Source
  if (arrayIndexes.first.val[0] != 1)
  {
    svtkErrorMacro(<< "This input array at idx " << idx << " named " << arrayIndexes.second
                  << " is not a particle data array");
    return nullptr;
  }

  // Check connection, should be 0, no multiple connection
  if (arrayIndexes.first.val[1] != 0)
  {
    svtkErrorMacro(<< "This filter does not support multiple connections by port");
    return nullptr;
  }

  // Check field association
  switch (arrayIndexes.first.val[2])
  {
    case svtkDataObject::FIELD_ASSOCIATION_POINTS:
    {
      // Recover array
      svtkAbstractArray* array = pointData->GetAbstractArray(arrayIndexes.second.c_str());
      if (!array)
      {
        svtkErrorMacro(<< "This input array at idx " << idx << " named " << arrayIndexes.second
                      << " cannot be found, please check arrays.");
      }
      return array;
    }
    default:
      svtkErrorMacro(<< "Only FIELD_ASSOCIATION_POINTS are supported in particle data input");
  }
  return nullptr;
}

//----------------------------------------------------------------------------
int svtkLagrangianBasicIntegrationModel::GetFlowOrSurfaceDataNumberOfComponents(
  int idx, svtkDataSet* dataSet)
{
  // Check index
  if (this->InputArrays.count(idx) == 0)
  {
    svtkErrorMacro(<< "No arrays at index:" << idx);
    return -1;
  }

  ArrayMapVal arrayIndexes = this->InputArrays[idx];

  // Check port, should be 0 for Input or 2 for Surface
  if (arrayIndexes.first.val[0] != 0 && arrayIndexes.first.val[0] != 2)
  {
    svtkErrorMacro(<< "This input array at idx " << idx << " named " << arrayIndexes.second
                  << " is not a flow or surface data array");
    return -1;
  }

  // Check connection, should be 0, no multiple connection supported
  if (arrayIndexes.first.val[1] != 0)
  {
    svtkErrorMacro(<< "This filter does not support multiple connections by port");
    return -1;
  }

  // Check dataset is present
  if (!dataSet)
  {
    svtkErrorMacro(<< "Please provide a dataSet when calling this method "
                     "for input arrays coming from the flow or surface");
    return -1;
  }

  // Check fieldAssociation
  switch (arrayIndexes.first.val[2])
  {
    // Point needs interpolation
    case svtkDataObject::FIELD_ASSOCIATION_POINTS:
    {
      svtkDataArray* array = dataSet->GetPointData()->GetArray(arrayIndexes.second.c_str());
      if (!array)
      {
        svtkErrorMacro(<< "This input array at idx " << idx << " named " << arrayIndexes.second
                      << " cannot be found, please check arrays.");
        return -1;
      }
      return array->GetNumberOfComponents();
    }
    case svtkDataObject::FIELD_ASSOCIATION_CELLS:
    {
      svtkDataArray* array = dataSet->GetCellData()->GetArray(arrayIndexes.second.c_str());
      if (!array)
      {
        svtkErrorMacro(<< "This input array at idx " << idx << " named " << arrayIndexes.second
                      << " cannot be found, please check arrays.");
        return -1;
      }
      return array->GetNumberOfComponents();
    }
    case svtkDataObject::FIELD_ASSOCIATION_NONE:
    {
      svtkDataArray* array = dataSet->GetFieldData()->GetArray(arrayIndexes.second.c_str());
      if (!array)
      {
        svtkErrorMacro(<< "This input array at idx " << idx << " named " << arrayIndexes.second
                      << " cannot be found, please check arrays.");
        return false;
      }
      return array->GetNumberOfComponents();
    }
    default:
      svtkErrorMacro(<< "Only FIELD_ASSOCIATION_POINTS and FIELD_ASSOCIATION_CELLS "
                    << "are supported in this method");
  }
  return -1;
}

//----------------------------------------------------------------------------
bool svtkLagrangianBasicIntegrationModel::GetFlowOrSurfaceData(svtkLagrangianParticle* particle,
  int idx, svtkDataSet* dataSet, svtkIdType tupleId, double* weights, double* data)
{
  // Check index
  if (this->InputArrays.count(idx) == 0)
  {
    svtkErrorMacro(<< "No arrays at index:" << idx);
    return false;
  }

  ArrayMapVal arrayIndexes = this->InputArrays[idx];

  // Check port, should be 0 for Input or 2 for Surface
  if (arrayIndexes.first.val[0] != 0 && arrayIndexes.first.val[0] != 2)
  {
    svtkErrorMacro(<< "This input array at idx " << idx << " named " << arrayIndexes.second
                  << " is not a flow or surface data array");
    return false;
  }

  // Check connection, should be 0, no multiple connection supported
  if (arrayIndexes.first.val[1] != 0)
  {
    svtkErrorMacro(<< "This filter does not support multiple connections by port");
    return false;
  }

  // Check dataset is present
  if (!dataSet)
  {
    svtkErrorMacro(<< "Please provide a dataSet when calling this method "
                     "for input arrays coming from the flow or surface");
    return false;
  }

  // Check fieldAssociation
  switch (arrayIndexes.first.val[2])
  {
    // Point needs interpolation
    case svtkDataObject::FIELD_ASSOCIATION_POINTS:
    {
      if (!weights)
      {
        svtkErrorMacro(<< "This input array at idx " << idx << " named " << arrayIndexes.second
                      << " is a PointData, yet no weights have been provided");
        return false;
      }
      svtkDataArray* array = dataSet->GetPointData()->GetArray(arrayIndexes.second.c_str());
      if (!array)
      {
        svtkErrorMacro(<< "This input array at idx " << idx << " named " << arrayIndexes.second
                      << " cannot be found, please check arrays.");
        return false;
      }
      if (tupleId >= dataSet->GetNumberOfCells())
      {
        svtkErrorMacro(<< "This input array at idx " << idx << " named " << arrayIndexes.second
                      << " does not contain cellId :" << tupleId << " . Please check arrays.");
        return false;
      }

      // Manual interpolation of data at particle location
      svtkIdList* idList = particle->GetThreadedData()->IdList;
      dataSet->GetCellPoints(tupleId, idList);
      for (int j = 0; j < array->GetNumberOfComponents(); j++)
      {
        data[j] = 0;
        for (int i = 0; i < idList->GetNumberOfIds(); i++)
        {
          data[j] += weights[i] * array->GetComponent(idList->GetId(i), j);
        }
      }
      return true;
    }
    case svtkDataObject::FIELD_ASSOCIATION_CELLS:
    {
      if (tupleId >= dataSet->GetNumberOfCells())
      {
        svtkErrorMacro(<< "This input array at idx " << idx << " named " << arrayIndexes.second
                      << " does not contain cellId :" << tupleId << " . Please check arrays.");
        return false;
      }
      svtkDataArray* array = dataSet->GetCellData()->GetArray(arrayIndexes.second.c_str());
      if (!array)
      {
        svtkErrorMacro(<< "This input array at idx " << idx << " named " << arrayIndexes.second
                      << " cannot be found, please check arrays.");
        return false;
      }
      array->GetTuple(tupleId, data);
      return true;
    }
    case svtkDataObject::FIELD_ASSOCIATION_NONE:
    {
      svtkDataArray* array = dataSet->GetFieldData()->GetArray(arrayIndexes.second.c_str());
      if (!array || tupleId >= array->GetNumberOfTuples())
      {
        svtkErrorMacro(<< "This input array at idx " << idx << " named " << arrayIndexes.second
                      << " cannot be found in FieldData or does not contain"
                         "tuple index: "
                      << tupleId << " , please check arrays.");
        return false;
      }
      array->GetTuple(tupleId, data);
      return true;
    }
    default:
      svtkErrorMacro(<< "Only FIELD_ASSOCIATION_POINTS and FIELD_ASSOCIATION_CELLS "
                    << "are supported in this method");
  }
  return false;
}

//----------------------------------------------------------------------------
int svtkLagrangianBasicIntegrationModel::GetFlowOrSurfaceDataFieldAssociation(int idx)
{
  // Check index
  if (this->InputArrays.count(idx) == 0)
  {
    svtkErrorMacro(<< "No arrays at index:" << idx);
    return -1;
  }

  ArrayMapVal arrayIndexes = this->InputArrays[idx];

  // Check port, should be 0 for Input
  if (arrayIndexes.first.val[0] != 0 && arrayIndexes.first.val[0] != 2)
  {
    svtkErrorMacro(<< "This input array at idx " << idx << " named " << arrayIndexes.second
                  << " is not a flow or surface data array");
    return -1;
  }

  // Check connection, should be 0, no multiple connection supported
  if (arrayIndexes.first.val[1] != 0)
  {
    svtkErrorMacro(<< "This filter does not support multiple connections by port");
    return -1;
  }

  return arrayIndexes.first.val[2];
}

//---------------------------------------------------------------------------
svtkStringArray* svtkLagrangianBasicIntegrationModel::GetSeedArrayNames()
{
  return this->SeedArrayNames;
}

//---------------------------------------------------------------------------
svtkIntArray* svtkLagrangianBasicIntegrationModel::GetSeedArrayComps()
{
  return this->SeedArrayComps;
}

//---------------------------------------------------------------------------
svtkIntArray* svtkLagrangianBasicIntegrationModel::GetSeedArrayTypes()
{
  return this->SeedArrayTypes;
}

//---------------------------------------------------------------------------
svtkStringArray* svtkLagrangianBasicIntegrationModel::GetSurfaceArrayNames()
{
  this->SurfaceArrayNames->SetNumberOfValues(0);
  for (auto it = this->SurfaceArrayDescriptions.begin(); it != this->SurfaceArrayDescriptions.end();
       ++it)
  {
    this->SurfaceArrayNames->InsertNextValue(it->first.c_str());
  }
  return this->SurfaceArrayNames;
}

//---------------------------------------------------------------------------
svtkIntArray* svtkLagrangianBasicIntegrationModel::GetSurfaceArrayComps()
{
  this->SurfaceArrayComps->SetNumberOfValues(0);
  std::map<std::string, SurfaceArrayDescription>::const_iterator it;
  for (it = this->SurfaceArrayDescriptions.begin(); it != this->SurfaceArrayDescriptions.end();
       ++it)
  {
    this->SurfaceArrayComps->InsertNextValue(it->second.nComp);
  }
  return this->SurfaceArrayComps;
}

//---------------------------------------------------------------------------
int svtkLagrangianBasicIntegrationModel::GetWeightsSize()
{
  return static_cast<int>(this->SharedWeights.size());
}

//---------------------------------------------------------------------------
svtkStringArray* svtkLagrangianBasicIntegrationModel::GetSurfaceArrayEnumValues()
{
  this->SurfaceArrayEnumValues->SetNumberOfValues(0);
  std::map<std::string, SurfaceArrayDescription>::const_iterator it;
  for (it = this->SurfaceArrayDescriptions.begin(); it != this->SurfaceArrayDescriptions.end();
       ++it)
  {
    this->SurfaceArrayEnumValues->InsertVariantValue(
      this->SurfaceArrayEnumValues->GetNumberOfValues(), it->second.enumValues.size());
    for (size_t i = 0; i < it->second.enumValues.size(); i++)
    {
      this->SurfaceArrayEnumValues->InsertVariantValue(
        this->SurfaceArrayEnumValues->GetNumberOfValues(), it->second.enumValues[i].first);
      this->SurfaceArrayEnumValues->InsertNextValue(it->second.enumValues[i].second.c_str());
    }
  }
  return this->SurfaceArrayEnumValues;
}

//---------------------------------------------------------------------------
svtkDoubleArray* svtkLagrangianBasicIntegrationModel::GetSurfaceArrayDefaultValues()
{
  this->SurfaceArrayDefaultValues->SetNumberOfValues(0);
  std::map<std::string, SurfaceArrayDescription>::const_iterator it;
  for (it = this->SurfaceArrayDescriptions.begin(); it != this->SurfaceArrayDescriptions.end();
       ++it)
  {
    std::vector<double> defaultValues(it->second.nComp);
    for (size_t iDs = 0; iDs < this->Surfaces->size(); iDs++)
    {
      this->ComputeSurfaceDefaultValues(
        it->first.c_str(), (*this->Surfaces)[iDs].second, it->second.nComp, defaultValues.data());
      this->SurfaceArrayDefaultValues->InsertNextTuple(defaultValues.data());
    }
  }
  return this->SurfaceArrayDefaultValues;
}

//---------------------------------------------------------------------------
svtkIntArray* svtkLagrangianBasicIntegrationModel::GetSurfaceArrayTypes()
{
  this->SurfaceArrayTypes->SetNumberOfValues(0);
  std::map<std::string, SurfaceArrayDescription>::const_iterator it;
  for (it = this->SurfaceArrayDescriptions.begin(); it != this->SurfaceArrayDescriptions.end();
       ++it)
  {
    this->SurfaceArrayTypes->InsertNextValue(it->second.type);
  }
  return this->SurfaceArrayTypes;
}

//---------------------------------------------------------------------------
bool svtkLagrangianBasicIntegrationModel::ManualIntegration(
  svtkInitialValueProblemSolver* svtkNotUsed(integrator), double* svtkNotUsed(xcur),
  double* svtkNotUsed(xnext), double svtkNotUsed(t), double& svtkNotUsed(delT),
  double& svtkNotUsed(delTActual), double svtkNotUsed(minStep), double svtkNotUsed(maxStep),
  double svtkNotUsed(maxError), double svtkNotUsed(cellLength), double& svtkNotUsed(error),
  int& svtkNotUsed(integrationResult), svtkLagrangianParticle* svtkNotUsed(particle))
{
  return false;
}

//---------------------------------------------------------------------------
void svtkLagrangianBasicIntegrationModel::ComputeSurfaceDefaultValues(
  const char* arrayName, svtkDataSet* svtkNotUsed(dataset), int nComponents, double* defaultValues)
{
  double defVal =
    (strcmp(arrayName, "SurfaceType") == 0) ? static_cast<double>(SURFACE_TYPE_TERM) : 0.0;
  std::fill(defaultValues, defaultValues + nComponents, defVal);
}

//---------------------------------------------------------------------------
void svtkLagrangianBasicIntegrationModel::InitializeParticleData(
  svtkFieldData* particleData, int maxTuple)
{
  svtkNew<svtkIntArray> particleStepNumArray;
  particleStepNumArray->SetName("StepNumber");
  particleStepNumArray->SetNumberOfComponents(1);
  particleStepNumArray->Allocate(maxTuple);
  particleData->AddArray(particleStepNumArray);

  svtkNew<svtkDoubleArray> particleVelArray;
  particleVelArray->SetName("ParticleVelocity");
  particleVelArray->SetNumberOfComponents(3);
  particleVelArray->Allocate(maxTuple * 3);
  particleData->AddArray(particleVelArray);

  svtkNew<svtkDoubleArray> particleIntegrationTimeArray;
  particleIntegrationTimeArray->SetName("IntegrationTime");
  particleIntegrationTimeArray->SetNumberOfComponents(1);
  particleIntegrationTimeArray->Allocate(maxTuple);
  particleData->AddArray(particleIntegrationTimeArray);
}

//---------------------------------------------------------------------------
void svtkLagrangianBasicIntegrationModel::InitializePathData(svtkFieldData* data)
{
  svtkNew<svtkLongLongArray> particleIdArray;
  particleIdArray->SetName("Id");
  particleIdArray->SetNumberOfComponents(1);
  data->AddArray(particleIdArray);

  svtkNew<svtkLongLongArray> particleParentIdArray;
  particleParentIdArray->SetName("ParentId");
  particleParentIdArray->SetNumberOfComponents(1);
  data->AddArray(particleParentIdArray);

  svtkNew<svtkLongLongArray> particleSeedIdArray;
  particleSeedIdArray->SetName("SeedId");
  particleSeedIdArray->SetNumberOfComponents(1);
  data->AddArray(particleSeedIdArray);

  svtkNew<svtkIntArray> particleTerminationArray;
  particleTerminationArray->SetName("Termination");
  particleTerminationArray->SetNumberOfComponents(1);
  data->AddArray(particleTerminationArray);
}

//---------------------------------------------------------------------------
void svtkLagrangianBasicIntegrationModel::InitializeInteractionData(svtkFieldData* data)
{
  svtkNew<svtkIntArray> interactionArray;
  interactionArray->SetName("Interaction");
  interactionArray->SetNumberOfComponents(1);
  data->AddArray(interactionArray);
}

//---------------------------------------------------------------------------
void svtkLagrangianBasicIntegrationModel::InsertParticleSeedData(
  svtkLagrangianParticle* particle, svtkFieldData* data)
{
  // Check for max number of tuples in arrays
  svtkIdType maxTuples = 0;
  for (int i = 0; i < data->GetNumberOfArrays(); i++)
  {
    maxTuples = std::max(data->GetArray(i)->GetNumberOfTuples(), maxTuples);
  }

  // Copy seed data in not yet written array only
  // ie not yet at maxTuple
  svtkPointData* seedData = particle->GetSeedData();
  for (int i = 0; i < seedData->GetNumberOfArrays(); i++)
  {
    const char* name = seedData->GetArrayName(i);
    svtkDataArray* arr = data->GetArray(name);
    if (arr->GetNumberOfTuples() < maxTuples)
    {
      arr->InsertNextTuple(particle->GetSeedArrayTupleIndex(), seedData->GetArray(i));
    }
  }
}

//---------------------------------------------------------------------------
void svtkLagrangianBasicIntegrationModel::InsertPathData(
  svtkLagrangianParticle* particle, svtkFieldData* data)
{
  svtkLongLongArray::SafeDownCast(data->GetArray("Id"))->InsertNextValue(particle->GetId());
  svtkLongLongArray::SafeDownCast(data->GetArray("ParentId"))
    ->InsertNextValue(particle->GetParentId());
  svtkLongLongArray::SafeDownCast(data->GetArray("SeedId"))->InsertNextValue(particle->GetSeedId());
  svtkIntArray::SafeDownCast(data->GetArray("Termination"))
    ->InsertNextValue(particle->GetTermination());
}

//---------------------------------------------------------------------------
void svtkLagrangianBasicIntegrationModel::InsertInteractionData(
  svtkLagrangianParticle* particle, svtkFieldData* data)
{
  svtkIntArray::SafeDownCast(data->GetArray("Interaction"))
    ->InsertNextValue(particle->GetInteraction());
}

//---------------------------------------------------------------------------
void svtkLagrangianBasicIntegrationModel::InsertParticleData(
  svtkLagrangianParticle* particle, svtkFieldData* data, int stepEnum)
{
  switch (stepEnum)
  {
    case svtkLagrangianBasicIntegrationModel::VARIABLE_STEP_PREV:
      svtkIntArray::SafeDownCast(data->GetArray("StepNumber"))
        ->InsertNextValue(particle->GetNumberOfSteps() - 1);
      data->GetArray("ParticleVelocity")->InsertNextTuple(particle->GetPrevVelocity());
      data->GetArray("IntegrationTime")->InsertNextTuple1(particle->GetPrevIntegrationTime());
      break;
    case svtkLagrangianBasicIntegrationModel::VARIABLE_STEP_CURRENT:
      svtkIntArray::SafeDownCast(data->GetArray("StepNumber"))
        ->InsertNextValue(particle->GetNumberOfSteps());
      data->GetArray("ParticleVelocity")->InsertNextTuple(particle->GetVelocity());
      data->GetArray("IntegrationTime")->InsertNextTuple1(particle->GetIntegrationTime());
      break;
    case svtkLagrangianBasicIntegrationModel::VARIABLE_STEP_NEXT:
      svtkIntArray::SafeDownCast(data->GetArray("StepNumber"))
        ->InsertNextValue(particle->GetNumberOfSteps() + 1);
      data->GetArray("ParticleVelocity")->InsertNextTuple(particle->GetNextVelocity());
      data->GetArray("IntegrationTime")
        ->InsertNextTuple1(particle->GetIntegrationTime() + particle->GetStepTimeRef());
      break;
    default:
      break;
  }
}
