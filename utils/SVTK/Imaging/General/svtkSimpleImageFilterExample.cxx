/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSimpleImageFilterExample.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSimpleImageFilterExample.h"

#include "svtkImageData.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkSimpleImageFilterExample);

// The switch statement in Execute will call this method with
// the appropriate input type (IT). Note that this example assumes
// that the output data type is the same as the input data type.
// This is not always the case.
template <class IT>
void svtkSimpleImageFilterExampleExecute(
  svtkImageData* input, svtkImageData* output, IT* inPtr, IT* outPtr)
{
  int dims[3];
  input->GetDimensions(dims);
  if (input->GetScalarType() != output->GetScalarType())
  {
    svtkGenericWarningMacro(<< "Execute: input ScalarType, " << input->GetScalarType()
                           << ", must match out ScalarType " << output->GetScalarType());
    return;
  }

  int size = dims[0] * dims[1] * dims[2];

  for (int i = 0; i < size; i++)
  {
    outPtr[i] = inPtr[i];
  }
}

void svtkSimpleImageFilterExample::SimpleExecute(svtkImageData* input, svtkImageData* output)
{

  void* inPtr = input->GetScalarPointer();
  void* outPtr = output->GetScalarPointer();

  switch (output->GetScalarType())
  {
    // This is simply a #define for a big case list. It handles all
    // data types SVTK supports.
    svtkTemplateMacro(svtkSimpleImageFilterExampleExecute(
      input, output, static_cast<SVTK_TT*>(inPtr), static_cast<SVTK_TT*>(outPtr)));
    default:
      svtkGenericWarningMacro("Execute: Unknown input ScalarType");
      return;
  }
}
