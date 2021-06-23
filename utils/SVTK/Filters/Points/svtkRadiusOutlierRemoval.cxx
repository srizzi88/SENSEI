/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRadiusOutlierRemoval.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkRadiusOutlierRemoval.h"

#include "svtkAbstractPointLocator.h"
#include "svtkIdList.h"
#include "svtkObjectFactory.h"
#include "svtkPointSet.h"
#include "svtkPoints.h"
#include "svtkSMPThreadLocalObject.h"
#include "svtkSMPTools.h"
#include "svtkStaticPointLocator.h"

svtkStandardNewMacro(svtkRadiusOutlierRemoval);
svtkCxxSetObjectMacro(svtkRadiusOutlierRemoval, Locator, svtkAbstractPointLocator);

//----------------------------------------------------------------------------
// Helper classes to support efficient computing, and threaded execution.
namespace
{

//----------------------------------------------------------------------------
// The threaded core of the algorithm (first pass)
template <typename T>
struct RemoveOutliers
{
  const T* Points;
  svtkAbstractPointLocator* Locator;
  double Radius;
  int NumNeighbors;
  svtkIdType* PointMap;

  // Don't want to allocate working arrays on every thread invocation. Thread local
  // storage lots of new/delete.
  svtkSMPThreadLocalObject<svtkIdList> PIds;

  RemoveOutliers(T* points, svtkAbstractPointLocator* loc, double radius, int numNei, svtkIdType* map)
    : Points(points)
    , Locator(loc)
    , Radius(radius)
    , NumNeighbors(numNei)
    , PointMap(map)
  {
  }

  // Just allocate a little bit of memory to get started.
  void Initialize()
  {
    svtkIdList*& pIds = this->PIds.Local();
    pIds->Allocate(128); // allocate some memory
  }

  void operator()(svtkIdType ptId, svtkIdType endPtId)
  {
    const T* p = this->Points + 3 * ptId;
    svtkIdType* map = this->PointMap + ptId;
    double x[3];
    svtkIdList*& pIds = this->PIds.Local();

    for (; ptId < endPtId; ++ptId)
    {
      x[0] = static_cast<double>(*p++);
      x[1] = static_cast<double>(*p++);
      x[2] = static_cast<double>(*p++);

      this->Locator->FindPointsWithinRadius(this->Radius, x, pIds);
      svtkIdType numPts = pIds->GetNumberOfIds();

      // Keep in mind that The FindPoints method will always return at
      // least one point (itself).
      *map++ = (numPts > this->NumNeighbors ? 1 : -1);
    }
  }

  void Reduce() {}

  static void Execute(svtkRadiusOutlierRemoval* self, svtkIdType numPts, T* points, svtkIdType* map)
  {
    RemoveOutliers remove(
      points, self->GetLocator(), self->GetRadius(), self->GetNumberOfNeighbors(), map);
    svtkSMPTools::For(0, numPts, remove);
  }

}; // RemoveOutliers

} // anonymous namespace

//================= Begin class proper =======================================
//----------------------------------------------------------------------------
svtkRadiusOutlierRemoval::svtkRadiusOutlierRemoval()
{
  this->Radius = 1.0;
  this->NumberOfNeighbors = 2;
  this->Locator = svtkStaticPointLocator::New();
}

//----------------------------------------------------------------------------
svtkRadiusOutlierRemoval::~svtkRadiusOutlierRemoval()
{
  this->SetLocator(nullptr);
}

//----------------------------------------------------------------------------
// Traverse all the input points to see how many neighbors each point has
// within a specified radius, and populate the map which indicates how points
// are to be copied to the output.
int svtkRadiusOutlierRemoval::FilterPoints(svtkPointSet* input)
{
  // Perform the point removal
  // Start by building the locator
  if (!this->Locator)
  {
    svtkErrorMacro(<< "Point locator required\n");
    return 0;
  }
  this->Locator->SetDataSet(input);
  this->Locator->BuildLocator();

  // Determine which points, if any, should be removed. We create a map
  // to keep track. The bulk of the algorithmic work is done in this pass.
  svtkIdType numPts = input->GetNumberOfPoints();
  void* inPtr = input->GetPoints()->GetVoidPointer(0);
  switch (input->GetPoints()->GetDataType())
  {
    svtkTemplateMacro(RemoveOutliers<SVTK_TT>::Execute(this, numPts, (SVTK_TT*)inPtr, this->PointMap));
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkRadiusOutlierRemoval::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Number of Neighbors: " << this->NumberOfNeighbors << "\n";
  os << indent << "Locator: " << this->Locator << "\n";
}
