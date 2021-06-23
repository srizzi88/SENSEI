/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageDotProduct.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageDotProduct.h"

#include "svtkImageData.h"
#include "svtkImageProgressIterator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkImageDotProduct);

//----------------------------------------------------------------------------
svtkImageDotProduct::svtkImageDotProduct()
{
  this->SetNumberOfInputPorts(2);
}

//----------------------------------------------------------------------------
// Colapse the first axis
int svtkImageDotProduct::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  svtkDataObject::SetPointDataActiveScalarInfo(outputVector->GetInformationObject(0), -1, 1);
  return 1;
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
// Handles the two input operations
template <class T>
void svtkImageDotProductExecute(svtkImageDotProduct* self, svtkImageData* in1Data,
  svtkImageData* in2Data, svtkImageData* outData, int outExt[6], int id, T*)
{
  svtkImageIterator<T> inIt1(in1Data, outExt);
  svtkImageIterator<T> inIt2(in2Data, outExt);
  svtkImageProgressIterator<T> outIt(outData, outExt, self, id);
  float dot;

  // find the region to loop over
  int maxC = in1Data->GetNumberOfScalarComponents();
  int idxC;

  // Loop through output pixels
  while (!outIt.IsAtEnd())
  {
    T* inSI1 = inIt1.BeginSpan();
    T* inSI2 = inIt2.BeginSpan();
    T* outSI = outIt.BeginSpan();
    T* outSIEnd = outIt.EndSpan();
    while (outSI != outSIEnd)
    {
      // now process the components
      dot = 0.0;
      for (idxC = 0; idxC < maxC; idxC++)
      {
        dot += static_cast<float>(*inSI1 * *inSI2);
        ++inSI1;
        ++inSI2;
      }
      *outSI = static_cast<T>(dot);
      ++outSI;
    }
    inIt1.NextSpan();
    inIt2.NextSpan();
    outIt.NextSpan();
  }
}

//----------------------------------------------------------------------------
// This method is passed a input and output regions, and executes the filter
// algorithm to fill the output from the inputs.
// It just executes a switch statement to call the correct function for
// the regions data types.
void svtkImageDotProduct::ThreadedRequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* svtkNotUsed(outputVector),
  svtkImageData*** inData, svtkImageData** outData, int outExt[6], int id)
{
  // this filter expects that input is the same type as output.
  if (inData[0][0]->GetScalarType() != outData[0]->GetScalarType())
  {
    svtkErrorMacro(<< "Execute: input1 ScalarType, " << inData[0][0]->GetScalarType()
                  << ", must match output ScalarType " << outData[0]->GetScalarType());
    return;
  }

  if (inData[1][0]->GetScalarType() != outData[0]->GetScalarType())
  {
    svtkErrorMacro(<< "Execute: input2 ScalarType, " << inData[1][0]->GetScalarType()
                  << ", must match output ScalarType " << outData[0]->GetScalarType());
    return;
  }

  // this filter expects that inputs that have the same number of components
  if (inData[0][0]->GetNumberOfScalarComponents() != inData[1][0]->GetNumberOfScalarComponents())
  {
    svtkErrorMacro(<< "Execute: input1 NumberOfScalarComponents, "
                  << inData[0][0]->GetNumberOfScalarComponents()
                  << ", must match out input2 NumberOfScalarComponents "
                  << inData[1][0]->GetNumberOfScalarComponents());
    return;
  }

  switch (inData[0][0]->GetScalarType())
  {
    svtkTemplateMacro(svtkImageDotProductExecute(
      this, inData[0][0], inData[1][0], outData[0], outExt, id, static_cast<SVTK_TT*>(nullptr)));
    default:
      svtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
  }
}
