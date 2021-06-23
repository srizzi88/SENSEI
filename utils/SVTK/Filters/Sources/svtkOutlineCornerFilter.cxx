/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOutlineCornerFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOutlineCornerFilter.h"

#include "svtkDataSet.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkOutlineCornerSource.h"
#include "svtkPolyData.h"

svtkStandardNewMacro(svtkOutlineCornerFilter);

svtkOutlineCornerFilter::svtkOutlineCornerFilter()
{
  this->CornerFactor = 0.2;
  this->OutlineCornerSource = svtkOutlineCornerSource::New();
}

svtkOutlineCornerFilter::~svtkOutlineCornerFilter()
{
  if (this->OutlineCornerSource != nullptr)
  {
    this->OutlineCornerSource->Delete();
    this->OutlineCornerSource = nullptr;
  }
}

int svtkOutlineCornerFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkDebugMacro(<< "Creating dataset outline");

  //
  // Let OutlineCornerSource do all the work
  //
  this->OutlineCornerSource->SetBounds(input->GetBounds());
  this->OutlineCornerSource->SetCornerFactor(this->GetCornerFactor());
  this->OutlineCornerSource->Update();

  output->CopyStructure(this->OutlineCornerSource->GetOutput());

  return 1;
}

int svtkOutlineCornerFilter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}

void svtkOutlineCornerFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CornerFactor: " << this->CornerFactor << "\n";
}
