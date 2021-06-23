/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCountVertices.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkCountVertices.h"

#include "svtkCellData.h"
#include "svtkCellIterator.h"
#include "svtkDataSet.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkCountVertices);

//------------------------------------------------------------------------------
void svtkCountVertices::PrintSelf(std::ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent
     << "OutputArrayName: " << (this->OutputArrayName ? this->OutputArrayName : "(nullptr)")
     << "\n";
}

//------------------------------------------------------------------------------
svtkCountVertices::svtkCountVertices()
  : OutputArrayName(nullptr)
{
  this->SetOutputArrayName("Vertex Count");
}

//------------------------------------------------------------------------------
svtkCountVertices::~svtkCountVertices()
{
  this->SetOutputArrayName(nullptr);
}

//------------------------------------------------------------------------------
int svtkCountVertices::RequestData(
  svtkInformation*, svtkInformationVector** inInfoVec, svtkInformationVector* outInfoVec)
{
  // get the info objects
  svtkInformation* inInfo = inInfoVec[0]->GetInformationObject(0);
  svtkInformation* outInfo = outInfoVec->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataSet* output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  assert(input && output);

  output->ShallowCopy(input);

  svtkNew<svtkIdTypeArray> vertCount;
  vertCount->Allocate(input->GetNumberOfCells());
  vertCount->SetName(this->OutputArrayName);
  output->GetCellData()->AddArray(vertCount);

  svtkCellIterator* it = input->NewCellIterator();
  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextCell())
  {
    vertCount->InsertNextValue(it->GetNumberOfPoints());
  }
  it->Delete();

  return 1;
}

//------------------------------------------------------------------------------
int svtkCountVertices::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int svtkCountVertices::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}
