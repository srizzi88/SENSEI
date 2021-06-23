/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageEuclideanToPolar.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageEuclideanToPolar.h"

#include "svtkImageData.h"
#include "svtkImageProgressIterator.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"

#include <cmath>

svtkStandardNewMacro(svtkImageEuclideanToPolar);

//----------------------------------------------------------------------------
svtkImageEuclideanToPolar::svtkImageEuclideanToPolar()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
  this->ThetaMaximum = 255.0;
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
void svtkImageEuclideanToPolarExecute(svtkImageEuclideanToPolar* self, svtkImageData* inData,
  svtkImageData* outData, int outExt[6], int id, T*)
{
  svtkImageIterator<T> inIt(inData, outExt);
  svtkImageProgressIterator<T> outIt(outData, outExt, self, id);
  double X, Y, Theta, R;
  double thetaMax = self->GetThetaMaximum();

  // find the region to loop over
  int maxC = inData->GetNumberOfScalarComponents();

  // Loop through output pixels
  while (!outIt.IsAtEnd())
  {
    T* inSI = inIt.BeginSpan();
    T* outSI = outIt.BeginSpan();
    T* outSIEnd = outIt.EndSpan();
    while (outSI != outSIEnd)
    {
      // Pixel operation
      X = static_cast<double>(*inSI);
      Y = static_cast<double>(inSI[1]);

      if ((X == 0.0) && (Y == 0.0))
      {
        Theta = 0.0;
        R = 0.0;
      }
      else
      {
        Theta = atan2(Y, X) * thetaMax / (2.0 * svtkMath::Pi());
        if (Theta < 0.0)
        {
          Theta += thetaMax;
        }
        R = sqrt(X * X + Y * Y);
      }

      *outSI = static_cast<T>(Theta);
      outSI[1] = static_cast<T>(R);
      inSI += maxC;
      outSI += maxC;
    }
    inIt.NextSpan();
    outIt.NextSpan();
  }
}

//----------------------------------------------------------------------------
void svtkImageEuclideanToPolar::ThreadedExecute(
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

  // input must have at least two components
  if (inData->GetNumberOfScalarComponents() < 2)
  {
    svtkErrorMacro(<< "Execute: input does not have at least two components");
    return;
  }

  switch (inData->GetScalarType())
  {
    svtkTemplateMacro(svtkImageEuclideanToPolarExecute(
      this, inData, outData, outExt, id, static_cast<SVTK_TT*>(nullptr)));
    default:
      svtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
  }
}

void svtkImageEuclideanToPolar::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Maximum Angle: " << this->ThetaMaximum << "\n";
}
