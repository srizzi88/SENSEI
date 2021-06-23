/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGenericOutlineFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkGenericOutlineFilter.h"

#include "svtkGenericDataSet.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkOutlineSource.h"
#include "svtkPolyData.h"

svtkStandardNewMacro(svtkGenericOutlineFilter);

//-----------------------------------------------------------------------------
svtkGenericOutlineFilter::svtkGenericOutlineFilter()
{
  this->OutlineSource = svtkOutlineSource::New();
}

//-----------------------------------------------------------------------------
svtkGenericOutlineFilter::~svtkGenericOutlineFilter()
{
  this->OutlineSource->Delete();
}

//-----------------------------------------------------------------------------
int svtkGenericOutlineFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkGenericDataSet* input =
    svtkGenericDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  //  svtkPolyData *output = this->GetOutput();

  svtkDebugMacro(<< "Creating dataset outline");

  //
  // Let OutlineSource do all the work
  //

  this->OutlineSource->SetBounds(input->GetBounds());
  this->OutlineSource->Update();

  output->CopyStructure(this->OutlineSource->GetOutput());
  return 1;
}

//-----------------------------------------------------------------------------
int svtkGenericOutlineFilter::RequestInformation(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  //  svtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  //  svtkInformation *outInfo = outputVector->GetInformationObject(0);

  svtkDebugMacro(<< "Creating dataset outline");

  //
  // Let OutlineSource do all the work
  //

  int result = this->Superclass::RequestInformation(request, inputVector, outputVector);

  this->OutlineSource->UpdateInformation();

  return result;
}

//----------------------------------------------------------------------------
int svtkGenericOutlineFilter::FillInputPortInformation(int port, svtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGenericDataSet");
  return 1;
}
