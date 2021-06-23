/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCountFaces.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkCountFaces.h"

#include "svtkCellData.h"
#include "svtkCellIterator.h"
#include "svtkDataSet.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkCountFaces);

//------------------------------------------------------------------------------
void svtkCountFaces::PrintSelf(std::ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent
     << "OutputArrayName: " << (this->OutputArrayName ? this->OutputArrayName : "(nullptr)")
     << "\n";
}

//------------------------------------------------------------------------------
svtkCountFaces::svtkCountFaces()
  : OutputArrayName(nullptr)
{
  this->SetOutputArrayName("Face Count");
}

//------------------------------------------------------------------------------
svtkCountFaces::~svtkCountFaces()
{
  this->SetOutputArrayName(nullptr);
}

//------------------------------------------------------------------------------
int svtkCountFaces::RequestData(
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

  svtkNew<svtkIdTypeArray> faceCount;
  faceCount->Allocate(input->GetNumberOfCells());
  faceCount->SetName(this->OutputArrayName);
  output->GetCellData()->AddArray(faceCount);

  svtkCellIterator* it = input->NewCellIterator();
  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextCell())
  {
    faceCount->InsertNextValue(it->GetNumberOfFaces());
  }
  it->Delete();

  return 1;
}

//------------------------------------------------------------------------------
int svtkCountFaces::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int svtkCountFaces::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}
