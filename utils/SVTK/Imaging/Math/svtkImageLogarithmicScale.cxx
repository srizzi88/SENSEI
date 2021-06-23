/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageLogarithmicScale.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageLogarithmicScale.h"

#include "svtkImageData.h"
#include "svtkImageProgressIterator.h"
#include "svtkObjectFactory.h"

#include <cmath>

svtkStandardNewMacro(svtkImageLogarithmicScale);

//----------------------------------------------------------------------------
// Constructor sets default values
svtkImageLogarithmicScale::svtkImageLogarithmicScale()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
  this->Constant = 10.0;
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
void svtkImageLogarithmicScaleExecute(svtkImageLogarithmicScale* self, svtkImageData* inData,
  svtkImageData* outData, int outExt[6], int id, T*)
{
  svtkImageIterator<T> inIt(inData, outExt);
  svtkImageProgressIterator<T> outIt(outData, outExt, self, id);
  double c;

  c = self->GetConstant();

  // Loop through output pixels
  while (!outIt.IsAtEnd())
  {
    T* inSI = inIt.BeginSpan();
    T* outSI = outIt.BeginSpan();
    T* outSIEnd = outIt.EndSpan();
    while (outSI != outSIEnd)
    {
      // Pixel operation
      if (*inSI > 0)
      {
        *outSI = static_cast<T>(c * log(static_cast<double>(*inSI) + 1.0));
      }
      else
      {
        *outSI = static_cast<T>(-c * log(1.0 - static_cast<double>(*inSI)));
      }

      outSI++;
      inSI++;
    }
    inIt.NextSpan();
    outIt.NextSpan();
  }
}

//----------------------------------------------------------------------------
// This method is passed a input and output region, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the regions data types.
void svtkImageLogarithmicScale::ThreadedExecute(
  svtkImageData* inData, svtkImageData* outData, int outExt[6], int id)
{
  // this filter expects that input is the same type as output.
  if (inData->GetScalarType() != outData->GetScalarType())
  {
    svtkErrorMacro(<< "Execute: input ScalarType, " << inData->GetScalarType()
                  << ", must match out ScalarType " << outData->GetScalarType());
    return;
  }

  switch (inData->GetScalarType())
  {
    svtkTemplateMacro(svtkImageLogarithmicScaleExecute(
      this, inData, outData, outExt, id, static_cast<SVTK_TT*>(nullptr)));
    default:
      svtkErrorMacro(<< "Execute: Unknown input ScalarType");
      return;
  }
}

void svtkImageLogarithmicScale::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Constant: " << this->Constant << "\n";
}
