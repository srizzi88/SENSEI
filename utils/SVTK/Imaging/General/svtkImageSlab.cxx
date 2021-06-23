/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageSlab.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkImageSlab.h"

#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTypeTraits.h"

#include "svtkTemplateAliasMacro.h"
// turn off 64-bit ints when templating over all types, since
// they cannot be stored in "double" without loss of precision
#undef SVTK_USE_INT64
#define SVTK_USE_INT64 0
#undef SVTK_USE_UINT64
#define SVTK_USE_UINT64 0

#include <cmath>

svtkStandardNewMacro(svtkImageSlab);

//----------------------------------------------------------------------------
svtkImageSlab::svtkImageSlab()
{
  this->Operation = SVTK_IMAGE_SLAB_MEAN;
  this->TrapezoidIntegration = 0;
  this->Orientation = 2;
  this->SliceRange[0] = SVTK_INT_MIN;
  this->SliceRange[1] = SVTK_INT_MAX;
  this->OutputScalarType = 0;
  this->MultiSliceOutput = 0;
}

//----------------------------------------------------------------------------
svtkImageSlab::~svtkImageSlab() = default;

//----------------------------------------------------------------------------
int svtkImageSlab::RequestInformation(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  int extent[6];
  int range[2];
  double origin[3];
  double spacing[3];
  double sliceSpacing;
  int dimIndex;
  int scalarType;

  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);
  inInfo->Get(svtkDataObject::SPACING(), spacing);
  inInfo->Get(svtkDataObject::ORIGIN(), origin);

  // get the direction along which to sum slices
  dimIndex = this->GetOrientation();

  // clamp the range to the whole extent
  this->GetSliceRange(range);
  if (range[0] < extent[2 * dimIndex])
  {
    range[0] = extent[2 * dimIndex];
  }
  if (range[1] > extent[2 * dimIndex + 1])
  {
    range[1] = extent[2 * dimIndex + 1];
  }

  // set new origin to be in the center of the stack of slices
  sliceSpacing = spacing[dimIndex];
  origin[dimIndex] = (origin[dimIndex] + 0.5 * sliceSpacing * (range[0] + range[1]));

  if (this->GetMultiSliceOutput())
  {
    // output extent is input extent, decreased by the slice range
    extent[2 * dimIndex] -= range[0];
    extent[2 * dimIndex + 1] -= range[1];
  }
  else
  {
    // set new extent to single-slice
    extent[2 * dimIndex] = 0;
    extent[2 * dimIndex + 1] = 0;
  }

  // set the output scalar type
  scalarType = this->GetOutputScalarType();

  // set the output information
  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent, 6);
  outInfo->Set(svtkDataObject::SPACING(), spacing, 3);
  outInfo->Set(svtkDataObject::ORIGIN(), origin, 3);

  // if requested, change the type to float or double
  if (scalarType == SVTK_FLOAT || scalarType == SVTK_DOUBLE)
  {
    svtkDataObject::SetPointDataActiveScalarInfo(outInfo, scalarType, -1);
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkImageSlab::RequestUpdateExtent(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  int outExt[6];
  int inExt[6];
  int extent[6];
  int range[2];
  int dimIndex;

  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), outExt);
  inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);

  // initialize input extent to output extent
  inExt[0] = outExt[0];
  inExt[1] = outExt[1];
  inExt[2] = outExt[2];
  inExt[3] = outExt[3];
  inExt[4] = outExt[4];
  inExt[5] = outExt[5];

  // get the direction along which to sum slices
  dimIndex = this->GetOrientation();

  // clamp the range to the whole extent
  this->GetSliceRange(range);
  if (range[0] < extent[2 * dimIndex])
  {
    range[0] = extent[2 * dimIndex];
  }
  if (range[1] > extent[2 * dimIndex + 1])
  {
    range[1] = extent[2 * dimIndex + 1];
  }

  // input range is the output range plus the specified slice range
  inExt[2 * dimIndex] += range[0];
  inExt[2 * dimIndex + 1] += range[1];

  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExt, 6);

  return 1;
}

// anonymous namespace to limit visibility
namespace
{

//----------------------------------------------------------------------------
// rounding functions for each type

template <class T>
void svtkSlabRound(double val, T& rnd)
{
  rnd = static_cast<T>(svtkMath::Floor(val + 0.5));
}

template <>
void svtkSlabRound<svtkTypeUInt32>(double val, svtkTypeUInt32& rnd)
{
  rnd = static_cast<svtkTypeUInt32>(val + 0.5);
}

template <>
void svtkSlabRound<svtkTypeFloat32>(double val, svtkTypeFloat32& rnd)
{
  rnd = val;
}

template <>
void svtkSlabRound<svtkTypeFloat64>(double val, svtkTypeFloat64& rnd)
{
  rnd = val;
}

//----------------------------------------------------------------------------
// clamping functions for each type

template <class T>
void svtkSlabClamp(double val, T& clamp)
{
  double minval = static_cast<double>(svtkTypeTraits<T>::Min());
  double maxval = static_cast<double>(svtkTypeTraits<T>::Max());
  val = (val > minval ? val : minval);
  val = (val < maxval ? val : maxval);
  svtkSlabRound(val, clamp);
}

template <>
void svtkSlabClamp<svtkTypeFloat32>(double val, float& clamp)
{
  clamp = val;
}

template <>
void svtkSlabClamp<svtkTypeFloat64>(double val, double& clamp)
{
  clamp = val;
}

//----------------------------------------------------------------------------
template <class T1, class T2>
void svtkImageSlabExecute(svtkImageSlab* self, svtkImageData* inData, T1* inPtr, svtkImageData* outData,
  T2* outPtr, int outExt[6], int id)
{
  svtkIdType outIncX, outIncY, outIncZ;
  svtkIdType inInc[3];
  int inExt[6];

  // get increments to march through data
  inData->GetExtent(inExt);
  inData->GetIncrements(inInc);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  int numscalars = inData->GetNumberOfScalarComponents();
  int rowlen = (outExt[1] - outExt[0] + 1) * numscalars;

  // get the operation
  int operation = self->GetOperation();
  int trapezoid = self->GetTrapezoidIntegration();

  // get the dimension along which to do the projection
  int dimIndex = self->GetOrientation();
  if (dimIndex < 0)
  {
    dimIndex = 0;
  }
  else if (dimIndex > 2)
  {
    dimIndex = 2;
  }

  // clamp the range to the whole extent
  int range[2];
  self->GetSliceRange(range);
  if (range[0] < inExt[2 * dimIndex])
  {
    range[0] = inExt[2 * dimIndex];
  }
  if (range[1] > inExt[2 * dimIndex + 1])
  {
    range[1] = inExt[2 * dimIndex + 1];
  }
  int numSlices = range[1] - range[0] + 1;

  // trapezoid integration is impossible if only one slice
  if (numSlices <= 1)
  {
    trapezoid = 0;
  }

  // averaging requires double precision summation
  double* rowBuffer = nullptr;
  if (operation == SVTK_IMAGE_SLAB_MEAN || operation == SVTK_IMAGE_SLAB_SUM)
  {
    rowBuffer = new double[rowlen];
  }

  unsigned long count = 0;
  unsigned long target = ((unsigned long)(outExt[3] - outExt[2] + 1) * (outExt[5] - outExt[4] + 1));
  target++;

  // Loop through output pixels
  for (int idZ = outExt[4]; idZ <= outExt[5]; idZ++)
  {
    T1* inPtrY = inPtr;
    for (int idY = outExt[2]; idY <= outExt[3]; idY++)
    {
      if (!id)
      {
        if (!(count % target))
        {
          self->UpdateProgress(count / (1.0 * target));
        }
        count++;
      }

      // ====== code for handling average and sum ======
      if (operation == SVTK_IMAGE_SLAB_MEAN || operation == SVTK_IMAGE_SLAB_SUM)
      {
        T1* inSlicePtr = inPtrY;
        double* rowPtr = rowBuffer;

        // initialize using first row
        T1* inPtrX = inSlicePtr;
        if (trapezoid)
        {
          double f = 0.5;
          for (int j = 0; j < rowlen; j++)
          {
            *rowPtr++ = f * (*inPtrX++);
          }
        }
        else
        {
          for (int j = 0; j < rowlen; j++)
          {
            *rowPtr++ = *inPtrX++;
          }
        }
        inSlicePtr += inInc[dimIndex];

        // perform the summation
        int sumSlices = (trapezoid ? (numSlices - 1) : numSlices);
        for (int sliceIdx = 1; sliceIdx < sumSlices; sliceIdx++)
        {
          inPtrX = inSlicePtr;
          rowPtr = rowBuffer;

          for (int i = 0; i < rowlen; i++)
          {
            *rowPtr++ += *inPtrX++;
          }
          inSlicePtr += inInc[dimIndex];
        }

        if (trapezoid)
        {
          inPtrX = inSlicePtr;
          rowPtr = rowBuffer;

          double f = 0.5;
          for (int i = 0; i < rowlen; i++)
          {
            *rowPtr++ += f * (*inPtrX++);
          }
        }

        rowPtr = rowBuffer;
        if (operation == SVTK_IMAGE_SLAB_MEAN)
        {
          // do the division via multiplication
          double factor = 1.0 / sumSlices;
          for (int k = 0; k < rowlen; k++)
          {
            svtkSlabRound((*rowPtr++) * factor, *outPtr++);
          }
        }
        else // SVTK_IMAGE_SLAB_SUM
        {
          // clamp to limits of numeric type
          for (int k = 0; k < rowlen; k++)
          {
            svtkSlabClamp(*rowPtr++, *outPtr++);
          }
        }
      }

      // ====== code for handling max and min ======
      else
      {
        T1* inSlicePtr = inPtrY;
        T2* outPtrX = outPtr;

        // initialize using first row
        T1* inPtrX = inSlicePtr;
        for (int j = 0; j < rowlen; j++)
        {
          *outPtrX++ = *inPtrX++;
        }
        inSlicePtr += inInc[dimIndex];

        if (operation == SVTK_IMAGE_SLAB_MIN)
        {
          for (int sliceIdx = 1; sliceIdx < numSlices; sliceIdx++)
          {
            inPtrX = inSlicePtr;
            outPtrX = outPtr;

            for (int i = 0; i < rowlen; i++)
            {
              *outPtrX = ((*outPtrX < *inPtrX) ? *outPtrX : *inPtrX);
              inPtrX++;
              outPtrX++;
            }

            inSlicePtr += inInc[dimIndex];
          }
        }
        else // SVTK_IMAGE_SLAB_MAX
        {
          for (int sliceIdx = 1; sliceIdx < numSlices; sliceIdx++)
          {
            inPtrX = inSlicePtr;
            outPtrX = outPtr;

            for (int i = 0; i < rowlen; i++)
            {
              *outPtrX = ((*outPtrX > *inPtrX) ? *outPtrX : *inPtrX);
              inPtrX++;
              outPtrX++;
            }

            inSlicePtr += inInc[dimIndex];
          }
        }

        outPtr += rowlen;
      }

      // ====== end of operation-specific code ======

      outPtr += outIncY;
      inPtrY += inInc[1];
    }

    outPtr += outIncZ;
    inPtr += inInc[2];
  }

  if (operation == SVTK_IMAGE_SLAB_MEAN || operation == SVTK_IMAGE_SLAB_SUM)
  {
    delete[] rowBuffer;
  }
}

} // end of anonymous namespace

//----------------------------------------------------------------------------
void svtkImageSlab::ThreadedRequestData(svtkInformation*, svtkInformationVector** inVector,
  svtkInformationVector*, svtkImageData*** inData, svtkImageData** outData, int outExt[6], int id)
{
  void* inPtr;
  void* outPtr;
  int inExt[6];
  int extent[6];
  int dimIndex;
  int range[2];

  svtkDebugMacro("Execute: inData = " << inData << ", outData = " << outData);

  // get the direction along which to sum slices
  dimIndex = this->GetOrientation();

  // clamp the range to the whole extent
  svtkInformation* inInfo = inVector[0]->GetInformationObject(0);
  inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);
  this->GetSliceRange(range);
  if (range[0] < extent[2 * dimIndex])
  {
    range[0] = extent[2 * dimIndex];
  }
  if (range[1] > extent[2 * dimIndex + 1])
  {
    range[1] = extent[2 * dimIndex + 1];
  }

  // initialize input extent to output extent
  inExt[0] = outExt[0];
  inExt[1] = outExt[1];
  inExt[2] = outExt[2];
  inExt[3] = outExt[3];
  inExt[4] = outExt[4];
  inExt[5] = outExt[5];

  // the adjust for the slice range
  inExt[2 * dimIndex] += range[0];
  inExt[2 * dimIndex + 1] += range[1];

  // now get the pointers for the extents
  inPtr = inData[0][0]->GetScalarPointerForExtent(inExt);
  outPtr = outData[0]->GetScalarPointerForExtent(outExt);

  // get the scalar type
  int outScalarType = outData[0]->GetScalarType();
  int inScalarType = inData[0][0]->GetScalarType();

  // and call the execute method
  if (outScalarType == inScalarType)
  {
    switch (inScalarType)
    {
      svtkTemplateAliasMacro(svtkImageSlabExecute(this, inData[0][0], static_cast<SVTK_TT*>(inPtr),
        outData[0], static_cast<SVTK_TT*>(outPtr), outExt, id));
      default:
        svtkErrorMacro("Execute: Unknown ScalarType");
        return;
    }
  }
  else if (outScalarType == SVTK_FLOAT)
  {
    switch (inScalarType)
    {
      svtkTemplateAliasMacro(svtkImageSlabExecute(this, inData[0][0], static_cast<SVTK_TT*>(inPtr),
        outData[0], static_cast<float*>(outPtr), outExt, id));
      default:
        svtkErrorMacro("Execute: Unknown ScalarType");
        return;
    }
  }
  else if (outScalarType == SVTK_DOUBLE)
  {
    switch (inScalarType)
    {
      svtkTemplateAliasMacro(svtkImageSlabExecute(this, inData[0][0], static_cast<SVTK_TT*>(inPtr),
        outData[0], static_cast<double*>(outPtr), outExt, id));
      default:
        svtkErrorMacro("Execute: Unknown ScalarType");
        return;
    }
  }
  else
  {
    svtkErrorMacro("Execute: Unknown ScalarType");
    return;
  }
}

//----------------------------------------------------------------------------
void svtkImageSlab::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Operation: " << this->GetOperationAsString() << "\n";
  os << indent << "TrapezoidIntegration: " << (this->TrapezoidIntegration ? "On\n" : "Off\n");
  os << indent << "Orientation: " << this->GetOrientation() << "\n";
  os << indent << "SliceRange: " << this->GetSliceRange()[0] << " " << this->GetSliceRange()[1]
     << "\n";
  os << indent << "OutputScalarType: " << this->OutputScalarType << "\n";
  os << indent << "MultiSliceOutput: " << (this->MultiSliceOutput ? "On\n" : "Off\n");
}

//----------------------------------------------------------------------------
const char* svtkImageSlab::GetOperationAsString()
{
  switch (this->Operation)
  {
    case SVTK_IMAGE_SLAB_MIN:
      return "Min";
    case SVTK_IMAGE_SLAB_MAX:
      return "Max";
    case SVTK_IMAGE_SLAB_MEAN:
      return "Mean";
    case SVTK_IMAGE_SLAB_SUM:
      return "Sum";
    default:
      return "";
  }
}
