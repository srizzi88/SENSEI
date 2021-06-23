/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageNormalize.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageNormalize.h"

#include "svtkImageData.h"
#include "svtkImageProgressIterator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include <cmath>

svtkStandardNewMacro(svtkImageNormalize);

//----------------------------------------------------------------------------
svtkImageNormalize::svtkImageNormalize()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
int svtkImageNormalize::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkDataObject::SetPointDataActiveScalarInfo(outInfo, SVTK_FLOAT, -1);
  return 1;
}

//----------------------------------------------------------------------------
// This execute method handles boundaries.
// it handles boundaries. Pixels are just replicated to get values
// out of extent.
template <class T>
void svtkImageNormalizeExecute(
  svtkImageNormalize* self, svtkImageData* inData, svtkImageData* outData, int outExt[6], int id, T*)
{
  svtkImageIterator<T> inIt(inData, outExt);
  svtkImageProgressIterator<float> outIt(outData, outExt, self, id);
  int idxC, maxC;
  float sum;
  T* inVect;

  // find the region to loop over
  maxC = inData->GetNumberOfScalarComponents();

  // Loop through output pixels
  while (!outIt.IsAtEnd())
  {
    T* inSI = inIt.BeginSpan();
    float* outSI = outIt.BeginSpan();
    float* outSIEnd = outIt.EndSpan();
    while (outSI != outSIEnd)
    {
      // save the start of the vector
      inVect = inSI;

      // compute the magnitude.
      sum = 0.0;
      for (idxC = 0; idxC < maxC; idxC++)
      {
        sum += static_cast<float>(*inSI) * static_cast<float>(*inSI);
        inSI++;
      }
      if (sum > 0.0)
      {
        sum = 1.0 / sqrt(sum);
      }

      // now divide to normalize.
      for (idxC = 0; idxC < maxC; idxC++)
      {
        *outSI = static_cast<float>(*inVect) * sum;
        inVect++;
        outSI++;
      }
    }
    inIt.NextSpan();
    outIt.NextSpan();
  }
}

//----------------------------------------------------------------------------
// This method contains a switch statement that calls the correct
// templated function for the input data type.  The output data
// must match input type.  This method does handle boundary conditions.
void svtkImageNormalize::ThreadedExecute(
  svtkImageData* inData, svtkImageData* outData, int outExt[6], int id)
{
  svtkDebugMacro(<< "Execute: inData = " << inData << ", outData = " << outData);

  // this filter expects that input is the same type as output.
  if (outData->GetScalarType() != SVTK_FLOAT)
  {
    svtkErrorMacro(<< "Execute: output ScalarType, " << outData->GetScalarType()
                  << ", must be float");
    return;
  }

  switch (inData->GetScalarType())
  {
    svtkTemplateMacro(
      svtkImageNormalizeExecute(this, inData, outData, outExt, id, static_cast<SVTK_TT*>(nullptr)));
    default:
      svtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
  }
}
