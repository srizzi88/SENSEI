/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageAccumulate.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageAccumulate.h"

#include "svtkImageData.h"
#include "svtkImageStencilData.h"
#include "svtkImageStencilIterator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include <cmath>

svtkStandardNewMacro(svtkImageAccumulate);

//----------------------------------------------------------------------------
// Constructor sets default values
svtkImageAccumulate::svtkImageAccumulate()
{
  for (int idx = 0; idx < 3; ++idx)
  {
    this->ComponentSpacing[idx] = 1.0;
    this->ComponentOrigin[idx] = 0.0;
    this->ComponentExtent[idx * 2] = 0;
    this->ComponentExtent[idx * 2 + 1] = 0;
  }
  this->ComponentExtent[1] = 255;

  this->ReverseStencil = 0;

  this->Min[0] = this->Min[1] = this->Min[2] = 0.0;
  this->Max[0] = this->Max[1] = this->Max[2] = 0.0;
  this->Mean[0] = this->Mean[1] = this->Mean[2] = 0.0;
  this->StandardDeviation[0] = this->StandardDeviation[1] = this->StandardDeviation[2] = 0.0;
  this->VoxelCount = 0;
  this->IgnoreZero = 0;

  // we have the image input and the optional stencil input
  this->SetNumberOfInputPorts(2);
}

//----------------------------------------------------------------------------
svtkImageAccumulate::~svtkImageAccumulate() = default;

//----------------------------------------------------------------------------
void svtkImageAccumulate::SetComponentExtent(int extent[6])
{
  int idx, modified = 0;

  for (idx = 0; idx < 6; ++idx)
  {
    if (this->ComponentExtent[idx] != extent[idx])
    {
      this->ComponentExtent[idx] = extent[idx];
      modified = 1;
    }
  }
  if (modified)
  {
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkImageAccumulate::SetComponentExtent(
  int minX, int maxX, int minY, int maxY, int minZ, int maxZ)
{
  int extent[6];

  extent[0] = minX;
  extent[1] = maxX;
  extent[2] = minY;
  extent[3] = maxY;
  extent[4] = minZ;
  extent[5] = maxZ;
  this->SetComponentExtent(extent);
}

//----------------------------------------------------------------------------
void svtkImageAccumulate::GetComponentExtent(int extent[6])
{
  for (int idx = 0; idx < 6; ++idx)
  {
    extent[idx] = this->ComponentExtent[idx];
  }
}

//----------------------------------------------------------------------------
void svtkImageAccumulate::SetStencilData(svtkImageStencilData* stencil)
{
  this->SetInputData(1, stencil);
}

//----------------------------------------------------------------------------
svtkImageStencilData* svtkImageAccumulate::GetStencil()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }
  return svtkImageStencilData::SafeDownCast(this->GetExecutive()->GetInputData(1, 0));
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
int svtkImageAccumulateExecute(svtkImageAccumulate* self, svtkImageData* inData, T*,
  svtkImageData* outData, svtkIdType* outPtr, double min[3], double max[3], double mean[3],
  double standardDeviation[3], svtkIdType* voxelCount, int* updateExtent)
{
  // variables used to compute statistics (filter handles max 3 components)
  double sum[3];
  sum[0] = sum[1] = sum[2] = 0.0;
  double sumSqr[3];
  sumSqr[0] = sumSqr[1] = sumSqr[2] = 0.0;
  min[0] = min[1] = min[2] = SVTK_DOUBLE_MAX;
  max[0] = max[1] = max[2] = SVTK_DOUBLE_MIN;
  standardDeviation[0] = standardDeviation[1] = standardDeviation[2] = 0.0;
  *voxelCount = 0;

  // input's number of components is used as output dimensionality
  int numC = inData->GetNumberOfScalarComponents();
  if (numC > 3)
  {
    return 0;
  }

  // get information for output data
  int outExtent[6];
  outData->GetExtent(outExtent);
  svtkIdType outIncs[3];
  outData->GetIncrements(outIncs);
  double origin[3];
  outData->GetOrigin(origin);
  double spacing[3];
  outData->GetSpacing(spacing);

  // zero count in every bin
  svtkIdType size = 1;
  size *= (outExtent[1] - outExtent[0] + 1);
  size *= (outExtent[3] - outExtent[2] + 1);
  size *= (outExtent[5] - outExtent[4] + 1);
  for (svtkIdType j = 0; j < size; j++)
  {
    outPtr[j] = 0;
  }

  svtkImageStencilData* stencil = self->GetStencil();
  bool reverseStencil = (self->GetReverseStencil() != 0);
  bool ignoreZero = (self->GetIgnoreZero() != 0);

  svtkImageStencilIterator<T> inIter(inData, stencil, updateExtent, self);

  while (!inIter.IsAtEnd())
  {
    if (inIter.IsInStencil() ^ reverseStencil)
    {
      T* inPtr = inIter.BeginSpan();
      T* spanEndPtr = inIter.EndSpan();

      while (inPtr != spanEndPtr)
      {
        // find the bin for this pixel.
        bool outOfBounds = false;
        svtkIdType* outPtrC = outPtr;
        for (int idxC = 0; idxC < numC; ++idxC)
        {
          double v = static_cast<double>(*inPtr++);
          if (!ignoreZero || v != 0)
          {
            // gather statistics
            sum[idxC] += v;
            sumSqr[idxC] += v * v;
            if (v > max[idxC])
            {
              max[idxC] = v;
            }
            if (v < min[idxC])
            {
              min[idxC] = v;
            }
            (*voxelCount)++;
          }

          // compute the index
          int outIdx = svtkMath::Floor((v - origin[idxC]) / spacing[idxC]);

          // verify that it is in range
          if (outIdx >= outExtent[idxC * 2] && outIdx <= outExtent[idxC * 2 + 1])
          {
            outPtrC += (outIdx - outExtent[idxC * 2]) * outIncs[idxC];
          }
          else
          {
            outOfBounds = true;
          }
        }

        // increment the bin
        if (!outOfBounds)
        {
          ++(*outPtrC);
        }
      }
    }

    inIter.NextSpan();
  }

  // initialize the statistics
  mean[0] = 0;
  mean[1] = 0;
  mean[2] = 0;

  standardDeviation[0] = 0;
  standardDeviation[1] = 0;
  standardDeviation[2] = 0;

  if (*voxelCount != 0) // avoid the div0
  {
    double n = static_cast<double>(*voxelCount);
    mean[0] = sum[0] / n;
    mean[1] = sum[1] / n;
    mean[2] = sum[2] / n;

    if (*voxelCount - 1 != 0) // avoid the div0
    {
      double m = static_cast<double>(*voxelCount - 1);
      standardDeviation[0] = sqrt((sumSqr[0] - mean[0] * mean[0] * n) / m);
      standardDeviation[1] = sqrt((sumSqr[1] - mean[1] * mean[1] * n) / m);
      standardDeviation[2] = sqrt((sumSqr[2] - mean[2] * mean[2] * n) / m);
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
// This method is passed a input and output Data, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the Datas data types.
int svtkImageAccumulate::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  void* inPtr;
  void* outPtr;

  // get the input
  svtkInformation* in1Info = inputVector[0]->GetInformationObject(0);
  svtkImageData* inData = svtkImageData::SafeDownCast(in1Info->Get(svtkDataObject::DATA_OBJECT()));
  int* uExt = in1Info->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());

  // get the output
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkImageData* outData = svtkImageData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkDebugMacro(<< "Executing image accumulate");

  // We need to allocate our own scalars since we are overriding
  // the superclasses "Execute()" method.
  outData->SetExtent(outInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
  outData->AllocateScalars(outInfo);

  svtkDataArray* inArray = this->GetInputArrayToProcess(0, inputVector);
  inPtr = inData->GetArrayPointerForExtent(inArray, uExt);
  outPtr = outData->GetScalarPointer();

  // Components turned into x, y and z
  if (inData->GetNumberOfScalarComponents() > 3)
  {
    svtkErrorMacro("This filter can handle up to 3 components");
    return 0;
  }

  // this filter expects that output is type int.
  if (outData->GetScalarType() != SVTK_ID_TYPE)
  {
    svtkErrorMacro(<< "Execute: out ScalarType " << outData->GetScalarType()
                  << " must be svtkIdType\n");
    return 0;
  }

  int retVal = 0;
  switch (inData->GetScalarType())
  {
    svtkTemplateMacro(retVal = svtkImageAccumulateExecute(this, inData, static_cast<SVTK_TT*>(inPtr),
                       outData, static_cast<svtkIdType*>(outPtr), this->Min, this->Max, this->Mean,
                       this->StandardDeviation, &this->VoxelCount, uExt));
    default:
      svtkErrorMacro(<< "Execute: Unknown ScalarType");
      return 0;
  }

  return retVal;
}

//----------------------------------------------------------------------------
int svtkImageAccumulate::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), this->ComponentExtent, 6);
  outInfo->Set(svtkDataObject::ORIGIN(), this->ComponentOrigin, 3);
  outInfo->Set(svtkDataObject::SPACING(), this->ComponentSpacing, 3);

  svtkDataObject::SetPointDataActiveScalarInfo(outInfo, SVTK_ID_TYPE, 1);
  return 1;
}

//----------------------------------------------------------------------------
// Get ALL of the input.
int svtkImageAccumulate::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* stencilInfo = nullptr;
  if (inputVector[1]->GetNumberOfInformationObjects() > 0)
  {
    stencilInfo = inputVector[1]->GetInformationObject(0);
  }

  // Use the whole extent of the first input as the update extent for
  // both inputs.  This way the stencil will be the same size as the
  // input.
  int extent[6] = { 0, -1, 0, -1, 0, -1 };
  inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), extent, 6);
  if (stencilInfo)
  {
    stencilInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), extent, 6);
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkImageAccumulate::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageStencilData");
    // the stencil input is optional
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  else
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageData");
  }
  return 1;
}

//----------------------------------------------------------------------------
void svtkImageAccumulate::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Mean: (" << this->Mean[0] << ", " << this->Mean[1] << ", " << this->Mean[2]
     << ")\n";
  os << indent << "Min: (" << this->Min[0] << ", " << this->Min[1] << ", " << this->Min[2] << ")\n";
  os << indent << "Max: (" << this->Max[0] << ", " << this->Max[1] << ", " << this->Max[2] << ")\n";
  os << indent << "StandardDeviation: (" << this->StandardDeviation[0] << ", "
     << this->StandardDeviation[1] << ", " << this->StandardDeviation[2] << ")\n";
  os << indent << "VoxelCount: " << this->VoxelCount << "\n";
  os << indent << "Stencil: " << this->GetStencil() << "\n";
  os << indent << "ReverseStencil: " << (this->ReverseStencil ? "On\n" : "Off\n");
  os << indent << "IgnoreZero: " << (this->IgnoreZero ? "On" : "Off") << "\n";

  os << indent << "ComponentOrigin: ( " << this->ComponentOrigin[0] << ", "
     << this->ComponentOrigin[1] << ", " << this->ComponentOrigin[2] << " )\n";

  os << indent << "ComponentSpacing: ( " << this->ComponentSpacing[0] << ", "
     << this->ComponentSpacing[1] << ", " << this->ComponentSpacing[2] << " )\n";

  os << indent << "ComponentExtent: ( " << this->ComponentExtent[0] << ","
     << this->ComponentExtent[1] << " " << this->ComponentExtent[2] << ","
     << this->ComponentExtent[3] << " " << this->ComponentExtent[4] << ","
     << this->ComponentExtent[5] << " }\n";
}
