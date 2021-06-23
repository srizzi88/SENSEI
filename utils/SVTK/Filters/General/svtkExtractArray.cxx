/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractArray.cxx

-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkExtractArray.h"
#include "svtkArrayData.h"
#include "svtkCommand.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"

///////////////////////////////////////////////////////////////////////////////
// svtkExtractArray

svtkStandardNewMacro(svtkExtractArray);

svtkExtractArray::svtkExtractArray()
  : Index(0)
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

svtkExtractArray::~svtkExtractArray() = default;

void svtkExtractArray::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Index: " << this->Index << endl;
}

int svtkExtractArray::FillInputPortInformation(int port, svtkInformation* info)
{
  switch (port)
  {
    case 0:
      info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkArrayData");
      return 1;
  }

  return 0;
}

int svtkExtractArray::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkArrayData* const input = svtkArrayData::GetData(inputVector[0]);

  if (this->Index < 0 || this->Index >= input->GetNumberOfArrays())
  {
    svtkErrorMacro(<< "Array index " << this->Index << " out-of-range for svtkArrayData containing "
                  << input->GetNumberOfArrays() << " arrays.");
    return 0;
  }

  svtkArrayData* const output = svtkArrayData::GetData(outputVector);
  output->ClearArrays();
  output->AddArray(input->GetArray(this->Index));

  return 1;
}
