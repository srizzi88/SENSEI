/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPPCAStatistics.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2011 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/
#include "svtkToolkits.h"

#include "svtkPPCAStatistics.h"

#include "svtkAbstractArray.h"
#include "svtkCommunicator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiProcessController.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPMultiCorrelativeStatistics.h"
#include "svtkPOrderStatistics.h"
#include "svtkTable.h"

svtkStandardNewMacro(svtkPPCAStatistics);
svtkCxxSetObjectMacro(svtkPPCAStatistics, Controller, svtkMultiProcessController);
//-----------------------------------------------------------------------------
svtkPPCAStatistics::svtkPPCAStatistics()
{
  this->Controller = 0;
  this->SetController(svtkMultiProcessController::GetGlobalController());
}

//-----------------------------------------------------------------------------
svtkPPCAStatistics::~svtkPPCAStatistics()
{
  this->SetController(0);
}

//-----------------------------------------------------------------------------
void svtkPPCAStatistics::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}

// ----------------------------------------------------------------------
void svtkPPCAStatistics::Learn(
  svtkTable* inData, svtkTable* inParameters, svtkMultiBlockDataSet* outMeta)
{
  if (!outMeta)
  {
    return;
  }

  // First calculate correlative statistics on local data set
  this->Superclass::Learn(inData, inParameters, outMeta);

  // Get a hold of the (sparse) covariance matrix
  svtkTable* sparseCov = svtkTable::SafeDownCast(outMeta->GetBlock(0));
  if (!sparseCov)
  {
    return;
  }

  if (!this->MedianAbsoluteDeviation)
  {
    svtkPMultiCorrelativeStatistics::GatherStatistics(this->Controller, sparseCov);
  }
}

// ----------------------------------------------------------------------
void svtkPPCAStatistics::Test(svtkTable* inData, svtkMultiBlockDataSet* inMeta, svtkTable* outMeta)
{
  if (this->Controller->GetNumberOfProcesses() > 1)
  {
    svtkWarningMacro("Parallel PCA: Hypothesis testing not implemented for more than 1 process.");
    return;
  }

  this->Superclass::Test(inData, inMeta, outMeta);
}

// ----------------------------------------------------------------------
svtkOrderStatistics* svtkPPCAStatistics::CreateOrderStatisticsInstance()
{
  return svtkPOrderStatistics::New();
}
