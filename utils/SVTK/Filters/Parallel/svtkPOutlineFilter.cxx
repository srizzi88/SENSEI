/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPOutlineFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPOutlineFilter.h"

#include "svtkCompositeDataSet.h"
#include "svtkDataSet.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkOutlineSource.h"
#include "svtkPOutlineFilterInternals.h"
#include "svtkPolyData.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkPOutlineFilter);
svtkCxxSetObjectMacro(svtkPOutlineFilter, Controller, svtkMultiProcessController);

svtkPOutlineFilter::svtkPOutlineFilter()
{
  this->Controller = nullptr;
  this->SetController(svtkMultiProcessController::GetGlobalController());

  this->OutlineSource = svtkOutlineSource::New();
}

svtkPOutlineFilter::~svtkPOutlineFilter()
{
  this->SetController(nullptr);
  if (this->OutlineSource != nullptr)
  {
    this->OutlineSource->Delete();
    this->OutlineSource = nullptr;
  }
}

int svtkPOutlineFilter::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkPOutlineFilterInternals internals;
  internals.SetIsCornerSource(false);
  internals.SetController(this->Controller);

  return internals.RequestData(request, inputVector, outputVector);
}

int svtkPOutlineFilter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkCompositeDataSet");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGraph");
  return 1;
}

void svtkPOutlineFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}
