/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageAppendComponents.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageAppendComponents.h"

#include "svtkAlgorithmOutput.h"
#include "svtkDataSetAttributes.h"
#include "svtkImageData.h"
#include "svtkImageProgressIterator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkImageAppendComponents);

//----------------------------------------------------------------------------
void svtkImageAppendComponents::ReplaceNthInputConnection(int idx, svtkAlgorithmOutput* input)
{
  if (idx < 0 || idx >= this->GetNumberOfInputConnections(0))
  {
    svtkErrorMacro("Attempt to replace connection idx "
      << idx << " of input port " << 0 << ", which has only "
      << this->GetNumberOfInputConnections(0) << " connections.");
    return;
  }

  if (!input || !input->GetProducer())
  {
    svtkErrorMacro("Attempt to replace connection index "
      << idx << " for input port " << 0 << " with "
      << (!input ? "a null input." : "an input with no producer."));
    return;
  }

  this->SetNthInputConnection(0, idx, input);
}

//----------------------------------------------------------------------------
// The default svtkImageAlgorithm semantics are that SetInput() puts
// each input on a different port, we want all the image inputs to
// go on the first port.
void svtkImageAppendComponents::SetInputData(int idx, svtkDataObject* input)
{
  this->SetInputDataInternal(idx, input);
}

//----------------------------------------------------------------------------
svtkDataObject* svtkImageAppendComponents::GetInput(int idx)
{
  if (this->GetNumberOfInputConnections(0) <= idx)
  {
    return nullptr;
  }
  return svtkImageData::SafeDownCast(this->GetExecutive()->GetInputData(0, idx));
}

//----------------------------------------------------------------------------
// This method tells the output it will have more components
int svtkImageAppendComponents::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkInformation* inScalarInfo;

  int idx1, num;
  num = 0;
  for (idx1 = 0; idx1 < this->GetNumberOfInputConnections(0); ++idx1)
  {
    inScalarInfo =
      svtkDataObject::GetActiveFieldInformation(inputVector[0]->GetInformationObject(idx1),
        svtkDataObject::FIELD_ASSOCIATION_POINTS, svtkDataSetAttributes::SCALARS);
    if (inScalarInfo && inScalarInfo->Has(svtkDataObject::FIELD_NUMBER_OF_COMPONENTS()))
    {
      num += inScalarInfo->Get(svtkDataObject::FIELD_NUMBER_OF_COMPONENTS());
    }
  }

  svtkDataObject::SetPointDataActiveScalarInfo(outInfo, -1, num);
  return 1;
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
void svtkImageAppendComponentsExecute(svtkImageAppendComponents* self, svtkImageData* inData,
  svtkImageData* outData, int outComp, int outExt[6], int id, T*)
{
  svtkImageIterator<T> inIt(inData, outExt);
  svtkImageProgressIterator<T> outIt(outData, outExt, self, id);
  int numIn = inData->GetNumberOfScalarComponents();
  int numSkip = outData->GetNumberOfScalarComponents() - numIn;
  int i;

  // Loop through output pixels
  while (!outIt.IsAtEnd())
  {
    T* inSI = inIt.BeginSpan();
    T* outSI = outIt.BeginSpan() + outComp;
    T* outSIEnd = outIt.EndSpan();
    while (outSI < outSIEnd)
    {
      // now process the components
      for (i = 0; i < numIn; ++i)
      {
        *outSI = *inSI;
        ++outSI;
        ++inSI;
      }
      outSI = outSI + numSkip;
    }
    inIt.NextSpan();
    outIt.NextSpan();
  }
}

//----------------------------------------------------------------------------
int svtkImageAppendComponents::FillInputPortInformation(int i, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  return this->Superclass::FillInputPortInformation(i, info);
}

//----------------------------------------------------------------------------
// This method is passed a input and output regions, and executes the filter
// algorithm to fill the output from the inputs.
// It just executes a switch statement to call the correct function for
// the regions data types.
void svtkImageAppendComponents::ThreadedRequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* svtkNotUsed(outputVector),
  svtkImageData*** inData, svtkImageData** outData, int outExt[6], int id)
{
  int idx1, outComp;

  outComp = 0;
  for (idx1 = 0; idx1 < this->GetNumberOfInputConnections(0); ++idx1)
  {
    if (inData[0][idx1] != nullptr)
    {
      // this filter expects that input is the same type as output.
      if (inData[0][idx1]->GetScalarType() != outData[0]->GetScalarType())
      {
        svtkErrorMacro(<< "Execute: input" << idx1 << " ScalarType ("
                      << inData[0][idx1]->GetScalarType() << "), must match output ScalarType ("
                      << outData[0]->GetScalarType() << ")");
        return;
      }
      switch (inData[0][idx1]->GetScalarType())
      {
        svtkTemplateMacro(svtkImageAppendComponentsExecute(
          this, inData[0][idx1], outData[0], outComp, outExt, id, static_cast<SVTK_TT*>(nullptr)));
        default:
          svtkErrorMacro(<< "Execute: Unknown ScalarType");
          return;
      }
      outComp += inData[0][idx1]->GetNumberOfScalarComponents();
    }
  }
}
