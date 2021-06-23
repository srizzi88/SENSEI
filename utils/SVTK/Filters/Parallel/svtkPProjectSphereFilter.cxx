/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPProjectSphereFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPProjectSphereFilter.h"

#include "svtkCommunicator.h"
#include "svtkIdList.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"

#include <map>

svtkStandardNewMacro(svtkPProjectSphereFilter);

//-----------------------------------------------------------------------------
svtkPProjectSphereFilter::svtkPProjectSphereFilter() = default;

//-----------------------------------------------------------------------------
svtkPProjectSphereFilter::~svtkPProjectSphereFilter() = default;

//-----------------------------------------------------------------------------
void svtkPProjectSphereFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void svtkPProjectSphereFilter::ComputePointsClosestToCenterLine(
  double minDist2ToCenterLine, svtkIdList* polePointIds)
{
  if (svtkMultiProcessController* controller = svtkMultiProcessController::GetGlobalController())
  {
    if (controller->GetNumberOfProcesses() > 1)
    {
      double tmp = minDist2ToCenterLine;
      double minDist2ToCenterLineGlobal = 0;
      controller->AllReduce(&tmp, &minDist2ToCenterLineGlobal, 1, svtkCommunicator::MAX_OP);
      if (tmp < minDist2ToCenterLineGlobal)
      {
        polePointIds->Reset();
      }
    }
  }
}

//-----------------------------------------------------------------------------
double svtkPProjectSphereFilter::GetZTranslation(svtkPointSet* input)
{
  double localMax = this->Superclass::GetZTranslation(input);
  double globalMax = localMax;
  if (svtkMultiProcessController* controller = svtkMultiProcessController::GetGlobalController())
  {
    if (controller->GetNumberOfProcesses() > 1)
    {
      controller->AllReduce(&localMax, &globalMax, 1, svtkCommunicator::MAX_OP);
    }
  }

  return globalMax;
}
