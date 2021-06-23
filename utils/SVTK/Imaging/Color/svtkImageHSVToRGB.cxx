/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageHSVToRGB.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageHSVToRGB.h"

#include "svtkImageData.h"
#include "svtkImageProgressIterator.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkImageHSVToRGB);

//----------------------------------------------------------------------------
svtkImageHSVToRGB::svtkImageHSVToRGB()
{
  this->Maximum = 255.0;
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
void svtkImageHSVToRGBExecute(
  svtkImageHSVToRGB* self, svtkImageData* inData, svtkImageData* outData, int outExt[6], int id, T*)
{
  svtkImageIterator<T> inIt(inData, outExt);
  svtkImageProgressIterator<T> outIt(outData, outExt, self, id);
  double R, G, B, H, S, V;
  double max = self->GetMaximum();
  int idxC;

  // find the region to loop over
  int maxC = inData->GetNumberOfScalarComponents() - 1;

  // Loop through output pixels
  while (!outIt.IsAtEnd())
  {
    T* inSI = inIt.BeginSpan();
    T* outSI = outIt.BeginSpan();
    T* outSIEnd = outIt.EndSpan();
    while (outSI != outSIEnd)
    {
      // Pixel operation
      H = static_cast<double>(*inSI) / max;
      ++inSI;
      S = static_cast<double>(*inSI) / max;
      ++inSI;
      V = static_cast<double>(*inSI) / max;
      ++inSI;

      svtkMath::HSVToRGB(H, S, V, &R, &G, &B);

      R *= max;
      G *= max;
      B *= max;

      if (R > max)
      {
        R = max;
      }
      if (G > max)
      {
        G = max;
      }
      if (B > max)
      {
        B = max;
      }

      // assign output.
      *outSI = static_cast<T>(R);
      ++outSI;
      *outSI = static_cast<T>(G);
      ++outSI;
      *outSI = static_cast<T>(B);
      ++outSI;

      for (idxC = 3; idxC <= maxC; idxC++)
      {
        *outSI++ = *inSI++;
      }
    }
    inIt.NextSpan();
    outIt.NextSpan();
  }
}

//----------------------------------------------------------------------------
void svtkImageHSVToRGB::ThreadedExecute(
  svtkImageData* inData, svtkImageData* outData, int outExt[6], int id)
{
  svtkDebugMacro(<< "Execute: inData = " << inData << ", outData = " << outData);

  // this filter expects that input is the same type as output.
  if (inData->GetScalarType() != outData->GetScalarType())
  {
    svtkErrorMacro(<< "Execute: input ScalarType, " << inData->GetScalarType()
                  << ", must match out ScalarType " << outData->GetScalarType());
    return;
  }

  // need three components for input and output
  if (inData->GetNumberOfScalarComponents() < 3)
  {
    svtkErrorMacro("Input has too few components");
    return;
  }
  if (outData->GetNumberOfScalarComponents() < 3)
  {
    svtkErrorMacro("Output has too few components");
    return;
  }

  switch (inData->GetScalarType())
  {
    svtkTemplateMacro(
      svtkImageHSVToRGBExecute(this, inData, outData, outExt, id, static_cast<SVTK_TT*>(nullptr)));
    default:
      svtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
  }
}

void svtkImageHSVToRGB::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Maximum: " << this->Maximum << "\n";
}
