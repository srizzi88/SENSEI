/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageWeightedSum.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageWeightedSum.h"

#include "svtkDataSetAttributes.h"
#include "svtkDoubleArray.h"
#include "svtkImageData.h"
#include "svtkImageIterator.h"
#include "svtkImageProgressIterator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkImageWeightedSum);

svtkCxxSetObjectMacro(svtkImageWeightedSum, Weights, svtkDoubleArray);
//----------------------------------------------------------------------------
// Description:
// Constructor sets default values
svtkImageWeightedSum::svtkImageWeightedSum()
{
  this->SetNumberOfInputPorts(1);

  // array of weights: need as many weights as inputs
  this->Weights = svtkDoubleArray::New();
  // By default normalize
  this->NormalizeByWeight = 1;
}

//----------------------------------------------------------------------------
svtkImageWeightedSum::~svtkImageWeightedSum()
{
  this->Weights->Delete();
}

//----------------------------------------------------------------------------
void svtkImageWeightedSum::SetWeight(svtkIdType id, double weight)
{
  // Reallocate if needed and pass the new weight
  this->Weights->InsertValue(id, weight);
}

//----------------------------------------------------------------------------
double svtkImageWeightedSum::CalculateTotalWeight()
{
  double totalWeight = 0.0;

  for (int i = 0; i < this->Weights->GetNumberOfTuples(); ++i)
  {
    totalWeight += this->Weights->GetValue(i);
  }
  return totalWeight;
}

//----------------------------------------------------------------------------
// Description:
// This templated function executes the filter for any type of data.
template <class T>
void svtkImageWeightedSumExecute(svtkImageWeightedSum* self, svtkImageData** inDatas, int numInputs,
  svtkImageData* outData, int outExt[6], int id, T*)
{
  svtkImageIterator<T> inItsFast[256];
  T* inSIFast[256];
  svtkImageProgressIterator<T> outIt(outData, outExt, self, id);

  double* weights = static_cast<svtkDoubleArray*>(self->GetWeights())->GetPointer(0);
  double totalWeight = self->CalculateTotalWeight();
  int normalize = self->GetNormalizeByWeight();
  svtkImageIterator<T>* inIts;
  T** inSI;
  if (numInputs < 256)
  {
    inIts = inItsFast;
    inSI = inSIFast;
  }
  else
  {
    inIts = new svtkImageIterator<T>[numInputs];
    inSI = new T*[numInputs];
  }

  // Loop through all input ImageData to initialize iterators
  for (int i = 0; i < numInputs; ++i)
  {
    inIts[i].Initialize(inDatas[i], outExt);
  }
  // Loop through output pixels
  while (!outIt.IsAtEnd())
  {
    for (int j = 0; j < numInputs; ++j)
    {
      inSI[j] = inIts[j].BeginSpan();
    }
    T* outSI = outIt.BeginSpan();
    T* outSIEnd = outIt.EndSpan();
    // Pixel operation
    while (outSI != outSIEnd)
    {
      double sum = 0.;
      for (int k = 0; k < numInputs; ++k)
      {
        sum += weights[k] * *inSI[k];
      }
      // Divide only if needed and different from 0
      if (normalize && totalWeight != 0.0)
      {
        sum /= totalWeight;
      }
      *outSI = static_cast<T>(sum); // do the cast only at the end
      outSI++;
      for (int l = 0; l < numInputs; ++l)
      {
        inSI[l]++;
      }
    }
    for (int j = 0; j < numInputs; ++j)
    {
      inIts[j].NextSpan();
    }
    outIt.NextSpan();
  }

  if (numInputs >= 256)
  {
    delete[] inIts;
    delete[] inSI;
  }
}

//----------------------------------------------------------------------------
int svtkImageWeightedSum::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  int numInputs = this->GetNumberOfInputConnections(0);
  if (!numInputs)
  {
    return 0;
  }
  int outputType = SVTK_DOUBLE;
  svtkInformation* info = inputVector[0]->GetInformationObject(0);
  svtkInformation* scalarInfo = svtkDataObject::GetActiveFieldInformation(
    info, svtkDataObject::FIELD_ASSOCIATION_POINTS, svtkDataSetAttributes::SCALARS);
  if (scalarInfo)
  {
    outputType = scalarInfo->Get(svtkDataObject::FIELD_ARRAY_TYPE());
  }
  int type;
  for (int whichInput = 1; whichInput < numInputs; whichInput++)
  {
    svtkInformation* inInfo = inputVector[0]->GetInformationObject(whichInput);
    svtkInformation* inScalarInfo = svtkDataObject::GetActiveFieldInformation(
      inInfo, svtkDataObject::FIELD_ASSOCIATION_POINTS, svtkDataSetAttributes::SCALARS);
    if (inScalarInfo)
    {
      type = inScalarInfo->Get(svtkDataObject::FIELD_ARRAY_TYPE());
      // Should we also check weight[whichInput] != 0
      if (type != outputType)
      {
        // Could be more fancy
        outputType = SVTK_DOUBLE;
      }
    }
  }
  svtkDataObject::SetPointDataActiveScalarInfo(outInfo, outputType, 1);
  return 1;
}

//----------------------------------------------------------------------------
// Description:
// This method is passed a input and output data, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the datas data types.
void svtkImageWeightedSum::ThreadedRequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* svtkNotUsed(outputVector),
  svtkImageData*** inData, svtkImageData** outData, int outExt[6], int id)
{
  if (inData[0][0] == nullptr)
  {
    svtkErrorMacro(<< "Input " << 0 << " must be specified.");
    return;
  }

  int numInputs = this->GetNumberOfInputConnections(0);
  int numWeights = this->Weights->GetNumberOfTuples();
  if (numWeights != numInputs)
  {
    if (id == 0)
    {
      svtkErrorMacro("ThreadedRequestData: There are "
        << numInputs << " svtkImageData inputs provided but only " << numWeights
        << " weights provided");
    }
    return;
  }

  int scalarType = inData[0][0]->GetScalarType();
  int numComp = inData[0][0]->GetNumberOfScalarComponents();
  for (int i = 1; i < numInputs; ++i)
  {
    int otherType = inData[0][i]->GetScalarType();
    int otherComp = inData[0][i]->GetNumberOfScalarComponents();
    if (otherType != scalarType || otherComp != numComp)
    {
      if (id == 0)
      {
        svtkErrorMacro("ThreadedRequestData: Input "
          << i << " has " << otherComp << " components of type " << otherType
          << ", but input 0 has " << numComp << " components of type " << scalarType);
      }
      return;
    }
  }

  switch (scalarType)
  {
    svtkTemplateMacro(svtkImageWeightedSumExecute(
      this, inData[0], numInputs, outData[0], outExt, id, static_cast<SVTK_TT*>(nullptr)));
    default:
      if (id == 0)
      {
        svtkErrorMacro(<< "Execute: Unknown ScalarType");
      }
      return;
  }
}

//----------------------------------------------------------------------------
int svtkImageWeightedSum::FillInputPortInformation(int i, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  return this->Superclass::FillInputPortInformation(i, info);
}

//----------------------------------------------------------------------------
void svtkImageWeightedSum::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  // objects
  os << indent << "NormalizeByWeight: " << (this->NormalizeByWeight ? "On" : "Off") << "\n";
  os << indent << "Weights: " << this->Weights << "\n";
  this->Weights->PrintSelf(os, indent.GetNextIndent());
}
