/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageRGBToHSI.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageRGBToHSI.h"

#include "svtkImageData.h"
#include "svtkImageProgressIterator.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"

#include <cmath>

svtkStandardNewMacro(svtkImageRGBToHSI);

//----------------------------------------------------------------------------
svtkImageRGBToHSI::svtkImageRGBToHSI()
{
  this->Maximum = 255.0;
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
void svtkImageRGBToHSIExecute(
  svtkImageRGBToHSI* self, svtkImageData* inData, svtkImageData* outData, int outExt[6], int id, T*)
{
  svtkImageIterator<T> inIt(inData, outExt);
  svtkImageProgressIterator<T> outIt(outData, outExt, self, id);
  int idxC, maxC;
  double R, G, B, H, S, I;
  double max = self->GetMaximum();
  double temp;

  // find the region to loop over
  maxC = inData->GetNumberOfScalarComponents() - 1;

  // Loop through output pixels
  while (!outIt.IsAtEnd())
  {
    T* inSI = inIt.BeginSpan();
    T* outSI = outIt.BeginSpan();
    T* outSIEnd = outIt.EndSpan();
    while (outSI != outSIEnd)
    {
      // Pixel operation
      R = static_cast<double>(*inSI);
      inSI++;
      G = static_cast<double>(*inSI);
      inSI++;
      B = static_cast<double>(*inSI);
      inSI++;
      // Saturation
      temp = R;
      if (G < temp)
      {
        temp = G;
      }
      if (B < temp)
      {
        temp = B;
      }
      double sumRGB = R + G + B;
      if (sumRGB == 0.0)
      {
        S = 0.0;
      }
      else
      {
        S = max * (1.0 - (3.0 * temp / sumRGB));
      }

      temp = static_cast<double>(R + G + B);
      // Intensity is easy
      I = temp / 3.0;

      // Hue
      temp = sqrt((R - G) * (R - G) + (R - B) * (G - B));
      if (temp != 0.0)
      {
        temp = acos((0.5 * ((R - G) + (R - B))) / temp);
      }
      if (G >= B)
      {
        H = max * (temp / (2.0 * svtkMath::Pi()));
      }
      else
      {
        H = max * (1.0 - (temp / (2.0 * svtkMath::Pi())));
      }

      // assign output.
      *outSI = static_cast<T>(H);
      outSI++;
      *outSI = static_cast<T>(S);
      outSI++;
      *outSI = static_cast<T>(I);
      outSI++;

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
void svtkImageRGBToHSI::ThreadedExecute(
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
      svtkImageRGBToHSIExecute(this, inData, outData, outExt, id, static_cast<SVTK_TT*>(nullptr)));
    default:
      svtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
  }
}

void svtkImageRGBToHSI::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Maximum: " << this->Maximum << "\n";
}
