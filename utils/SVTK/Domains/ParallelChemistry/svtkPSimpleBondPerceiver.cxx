/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: svtkYoungsMaterialInterfaceCEA.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkPSimpleBondPerceiver.h"

#include "svtkDataSetAttributes.h"
#include "svtkDistributedPointCloudFilter.h"
#include "svtkFloatArray.h"
#include "svtkMPIController.h"
#include "svtkMolecule.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkOutEdgeIterator.h"
#include "svtkPeriodicTable.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkUnsignedCharArray.h"

svtkStandardNewMacro(svtkPSimpleBondPerceiver);

//----------------------------------------------------------------------------
static inline bool InBounds(const double* bounds, const double* p)
{
  return p[0] >= bounds[0] && p[0] <= bounds[1] && p[1] >= bounds[2] && p[1] <= bounds[3] &&
    p[2] >= bounds[4] && p[2] <= bounds[5];
}

//----------------------------------------------------------------------------
bool svtkPSimpleBondPerceiver::CreateGhosts(svtkMolecule* molecule)
{
  if (molecule == nullptr)
  {
    return false;
  }

  svtkMPIController* controller =
    svtkMPIController::SafeDownCast(svtkMultiProcessController::GetGlobalController());
  if (controller == nullptr)
  {
    return true;
  }

  double domainBounds[6] = { 0., 0., 0., 0., 0., 0. };
  molecule->GetBounds(domainBounds);

  svtkNew<svtkPeriodicTable> table;
  static float MAX_VDW_RADIUS = table->GetMaxVDWRadius();

  double radius =
    this->IsToleranceAbsolute ? MAX_VDW_RADIUS + this->Tolerance : MAX_VDW_RADIUS * this->Tolerance;
  double outterBounds[6];
  outterBounds[0] = domainBounds[0] - radius;
  outterBounds[2] = domainBounds[2] - radius;
  outterBounds[4] = domainBounds[4] - radius;
  outterBounds[1] = domainBounds[1] + radius;
  outterBounds[3] = domainBounds[3] + radius;
  outterBounds[5] = domainBounds[5] + radius;

  svtkNew<svtkPolyData> inputPoly;
  svtkNew<svtkPoints> points;
  points->DeepCopy(molecule->GetAtomicPositionArray());
  inputPoly->SetPoints(points.Get());
  svtkDataSetAttributes* dataArray = inputPoly->GetPointData();
  dataArray->DeepCopy(molecule->GetVertexData());

  svtkNew<svtkPolyData> outputPoly;
  svtkDistributedPointCloudFilter::GetPointsInsideBounds(
    controller, inputPoly.Get(), outputPoly.Get(), outterBounds);

  molecule->Initialize(outputPoly->GetPoints(), outputPoly->GetPointData());

  molecule->AllocateAtomGhostArray();
  svtkUnsignedCharArray* atomGhostArray = molecule->GetAtomGhostArray();
  atomGhostArray->FillComponent(0, 0);

  molecule->AllocateBondGhostArray();
  svtkUnsignedCharArray* bondGhostArray = molecule->GetBondGhostArray();
  bondGhostArray->FillComponent(0, 0);

  if (!atomGhostArray && !bondGhostArray)
  {
    return false;
  }

  for (svtkIdType i = 0; i < molecule->GetNumberOfAtoms(); i++)
  {
    double p[3];
    molecule->GetPoint(i, p);
    if (!InBounds(domainBounds, p))
    {
      atomGhostArray->SetValue(i, 1);
      svtkNew<svtkOutEdgeIterator> it;
      molecule->GetOutEdges(i, it.Get());
      while (it->HasNext())
      {
        svtkOutEdgeType edge = it->Next();
        bondGhostArray->SetValue(edge.Id, 1);
      }
    }
  }

  return true;
}

//----------------------------------------------------------------------------
void svtkPSimpleBondPerceiver::ComputeBonds(svtkMolecule* molecule)
{
  if (!this->CreateGhosts(molecule))
  {
    svtkWarningMacro(<< "Ghosts were not correctly initialized.");
  }

  this->Superclass::ComputeBonds(molecule);
}
