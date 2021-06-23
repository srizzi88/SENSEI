/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLocationSelector.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkLocationSelector.h"

#include "svtkDataSetAttributes.h"
#include "svtkGenericCell.h"
#include "svtkInformation.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPoints.h"
#include "svtkSMPTools.h"
#include "svtkSelectionNode.h"
#include "svtkSignedCharArray.h"
#include "svtkStaticPointLocator.h"
#include "svtkUnstructuredGrid.h"

#include <cassert>

class svtkLocationSelector::svtkInternals
{
public:
  svtkInternals(svtkDataArray* selList)
    : SelectionList(selList)
  {
  }

  virtual ~svtkInternals() {}
  virtual bool Execute(svtkDataSet* dataset, svtkSignedCharArray* insidednessArray) = 0;

protected:
  svtkSmartPointer<svtkDataArray> SelectionList;
};

class svtkLocationSelector::svtkInternalsForPoints : public svtkLocationSelector::svtkInternals
{
public:
  svtkInternalsForPoints(svtkDataArray* selList, double searchRadius)
    : svtkInternals(selList)
    , SearchRadius(searchRadius)
  {
  }

  bool Execute(svtkDataSet* dataset, svtkSignedCharArray* insidednessArray) override
  {
    const svtkIdType numPoints = dataset->GetNumberOfPoints();
    if (numPoints <= 0)
    {
      return false;
    }

    svtkSmartPointer<svtkStaticPointLocator> locator;

    if (dataset->IsA("svtkPointSet"))
    {
      locator = svtkSmartPointer<svtkStaticPointLocator>::New();
      locator->SetDataSet(dataset);
      locator->Update();
    }

    std::fill_n(insidednessArray->GetPointer(0), numPoints, static_cast<char>(0));
    const double radius = this->SearchRadius;

    // Find points closest to each point in the locations of interest.
    svtkIdType numLocations = this->SelectionList->GetNumberOfTuples();
    for (svtkIdType locationId = 0; locationId < numLocations; ++locationId)
    {
      double location[3], dist2;
      this->SelectionList->GetTuple(locationId, location);

      svtkIdType ptId = -1;
      if (locator)
      {
        ptId = locator->FindClosestPointWithinRadius(radius, location, dist2);
      }
      else
      {
        ptId = dataset->FindPoint(location);
        if (ptId >= 0)
        {
          double* x = dataset->GetPoint(ptId);
          double distance = svtkMath::Distance2BetweenPoints(x, location);
          if (distance > radius * radius)
          {
            ptId = -1;
          }
        }
      }

      if (ptId >= 0)
      {
        insidednessArray->SetTypedComponent(ptId, 0, 1);
      }
    }

    insidednessArray->Modified();
    return true;
  }

protected:
  double SearchRadius;
};

class svtkLocationSelector::svtkInternalsForCells : public svtkLocationSelector::svtkInternals
{
public:
  svtkInternalsForCells(svtkDataArray* selList)
    : svtkInternals(selList)
  {
  }

  bool Execute(svtkDataSet* dataset, svtkSignedCharArray* insidednessArray) override
  {
    const auto numLocations = this->SelectionList->GetNumberOfTuples();
    const auto numCells = insidednessArray->GetNumberOfTuples();
    std::fill_n(insidednessArray->GetPointer(0), numCells, static_cast<char>(0));
    std::vector<double> weights(dataset->GetMaxCellSize(), 0.0);
    svtkNew<svtkGenericCell> cell;
    for (svtkIdType cc = 0; cc < numLocations; ++cc)
    {
      double coords[3];
      this->SelectionList->GetTuple(cc, coords);
      int subId;

      double pcoords[3];
      auto cid = dataset->FindCell(coords, nullptr, cell, 0, 0.0, subId, pcoords, &weights[0]);
      if (cid >= 0 && cid < numCells)
      {
        insidednessArray->SetValue(cid, 1);
      }
    }
    insidednessArray->Modified();
    return true;
  }
};

svtkStandardNewMacro(svtkLocationSelector);
//----------------------------------------------------------------------------
svtkLocationSelector::svtkLocationSelector()
  : Internals(nullptr)
{
}

//----------------------------------------------------------------------------
svtkLocationSelector::~svtkLocationSelector() {}

//----------------------------------------------------------------------------
void svtkLocationSelector::Initialize(svtkSelectionNode* node)
{
  this->Superclass::Initialize(node);

  this->Internals.reset();

  auto selectionList = svtkDataArray::SafeDownCast(node->GetSelectionList());
  if (!selectionList || selectionList->GetNumberOfTuples() == 0)
  {
    // empty selection list, nothing to do.
    return;
  }

  if (selectionList->GetNumberOfComponents() != 3)
  {
    svtkErrorMacro("Only 3-d locations are current supported.");
    return;
  }

  if (node->GetContentType() != svtkSelectionNode::LOCATIONS)
  {
    svtkErrorMacro("svtkLocationSelector only supported svtkSelectionNode::LOCATIONS. `"
      << node->GetContentType() << "` is not supported.");
    return;
  }

  const int fieldType = node->GetFieldType();
  const int assoc = svtkSelectionNode::ConvertSelectionFieldToAttributeType(fieldType);

  double radius = node->GetProperties()->Has(svtkSelectionNode::EPSILON())
    ? node->GetProperties()->Get(svtkSelectionNode::EPSILON())
    : 0.0;
  switch (assoc)
  {
    case svtkDataObject::FIELD_ASSOCIATION_POINTS:
      this->Internals.reset(new svtkInternalsForPoints(selectionList, radius));
      break;

    case svtkDataObject::FIELD_ASSOCIATION_CELLS:
      this->Internals.reset(new svtkInternalsForCells(selectionList));
      break;

    default:
      svtkErrorMacro(
        "svtkLocationSelector does not support requested field type `" << fieldType << "`.");
      break;
  }
}

//----------------------------------------------------------------------------
void svtkLocationSelector::Finalize()
{
  this->Internals.reset();
}

//----------------------------------------------------------------------------
bool svtkLocationSelector::ComputeSelectedElements(
  svtkDataObject* input, svtkSignedCharArray* insidednessArray)
{
  assert(input != nullptr && insidednessArray != nullptr);
  svtkDataSet* ds = svtkDataSet::SafeDownCast(input);
  return (this->Internals != nullptr && ds != nullptr)
    ? this->Internals->Execute(ds, insidednessArray)
    : false;
}

//----------------------------------------------------------------------------
void svtkLocationSelector::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
