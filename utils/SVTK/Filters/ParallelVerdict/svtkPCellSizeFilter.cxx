/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPCellSizeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
#include "svtkPCellSizeFilter.h"

#include "svtkCommunicator.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkPCellSizeFilter);

//-----------------------------------------------------------------------------
svtkPCellSizeFilter::svtkPCellSizeFilter() {}

//-----------------------------------------------------------------------------
svtkPCellSizeFilter::~svtkPCellSizeFilter() {}

//-----------------------------------------------------------------------------
void svtkPCellSizeFilter::ComputeGlobalSum(double sum[4])
{
  svtkMultiProcessController* controller = svtkMultiProcessController::GetGlobalController();
  if (controller->GetNumberOfProcesses() > 1)
  {
    double globalSum[4];
    controller->AllReduce(sum, globalSum, 4, svtkCommunicator::SUM_OP);
    for (int i = 0; i < 4; i++)
    {
      sum[i] = globalSum[i];
    }
  }
}
