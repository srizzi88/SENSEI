// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTableFFT.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "svtkTableFFT.h"

#include "svtkDataArray.h"
#include "svtkImageData.h"
#include "svtkImageFFT.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkTable.h"

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

#include <cstring>

#include <svtksys/SystemTools.hxx>
using namespace svtksys;

//=============================================================================
svtkStandardNewMacro(svtkTableFFT);

//-----------------------------------------------------------------------------
svtkTableFFT::svtkTableFFT() = default;

svtkTableFFT::~svtkTableFFT() = default;

void svtkTableFFT::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
int svtkTableFFT::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkTable* input = svtkTable::GetData(inputVector[0]);
  svtkTable* output = svtkTable::GetData(outputVector);

  if (!input || !output)
  {
    svtkWarningMacro(<< "No input or output.");
    return 0;
  }

  svtkIdType numColumns = input->GetNumberOfColumns();
  for (svtkIdType col = 0; col < numColumns; col++)
  {
    this->UpdateProgress((double)col / numColumns);

    svtkDataArray* array = svtkArrayDownCast<svtkDataArray>(input->GetColumn(col));
    if (!array)
      continue;
    if (array->GetNumberOfComponents() != 1)
      continue;
    if (array->GetName())
    {
      if (SystemTools::Strucmp(array->GetName(), "time") == 0)
        continue;
      if (strcmp(array->GetName(), "svtkValidPointMask") == 0)
      {
        output->AddColumn(array);
        continue;
      }
    }
    if (array->IsA("svtkIdTypeArray"))
      continue;

    svtkSmartPointer<svtkDataArray> frequencies = this->DoFFT(array);
    frequencies->SetName(array->GetName());
    output->AddColumn(frequencies);
  }

  return 1;
}

//-----------------------------------------------------------------------------
svtkSmartPointer<svtkDataArray> svtkTableFFT::DoFFT(svtkDataArray* input)
{
  // Build an image data containing the input data.
  SVTK_CREATE(svtkImageData, imgInput);
  imgInput->SetDimensions(input->GetNumberOfTuples(), 1, 1);
  imgInput->SetScalarType(input->GetDataType(), input->GetInformation());
  imgInput->GetPointData()->SetScalars(input);

  // Compute the FFT
  SVTK_CREATE(svtkImageFFT, fft);
  fft->SetInputData(imgInput);
  fft->Update();

  // Return the result
  return svtkSmartPointer<svtkDataArray>(fft->GetOutput()->GetPointData()->GetScalars());
}
