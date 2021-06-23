/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPiecewiseFunctionShiftScale.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPiecewiseFunctionShiftScale.h"

#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPiecewiseFunction.h"

svtkStandardNewMacro(svtkPiecewiseFunctionShiftScale);

svtkPiecewiseFunctionShiftScale::svtkPiecewiseFunctionShiftScale()
{
  this->PositionShift = 0.0;
  this->PositionScale = 1.0;
  this->ValueShift = 0.0;
  this->ValueScale = 1.0;
}

svtkPiecewiseFunctionShiftScale::~svtkPiecewiseFunctionShiftScale() = default;

int svtkPiecewiseFunctionShiftScale::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkPiecewiseFunction* input =
    svtkPiecewiseFunction::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPiecewiseFunction* output =
    svtkPiecewiseFunction::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  double* inFunction = input->GetDataPointer();
  int numInValues = input->GetSize();

  output->RemoveAllPoints();

  int i;

  for (i = 0; i < numInValues; i++)
  {
    output->AddPoint((inFunction[2 * i] + this->PositionShift) * this->PositionScale,
      (inFunction[2 * i + 1] + this->ValueShift) * this->ValueScale);
  }

  return 1;
}

void svtkPiecewiseFunctionShiftScale::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "PositionShift: " << this->PositionShift << "\n";
  os << indent << "PositionScale: " << this->PositionScale << "\n";
  os << indent << "ValueShift: " << this->ValueShift << "\n";
  os << indent << "ValueScale: " << this->ValueScale << "\n";
}
