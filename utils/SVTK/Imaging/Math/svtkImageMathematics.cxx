/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageMathematics.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageMathematics.h"

#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include <cmath>

svtkStandardNewMacro(svtkImageMathematics);

//----------------------------------------------------------------------------
svtkImageMathematics::svtkImageMathematics()
{
  this->Operation = SVTK_ADD;
  this->ConstantK = 1.0;
  this->ConstantC = 0.0;
  this->DivideByZeroToC = 0;
  this->SetNumberOfInputPorts(2);
}

//----------------------------------------------------------------------------
// The output extent is the intersection.
int svtkImageMathematics::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* inInfo2 = inputVector[1]->GetInformationObject(0);

  int ext[6], ext2[6], idx;

  inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), ext);

  // two input take intersection
  if (this->Operation == SVTK_ADD || this->Operation == SVTK_SUBTRACT ||
    this->Operation == SVTK_MULTIPLY || this->Operation == SVTK_DIVIDE ||
    this->Operation == SVTK_MIN || this->Operation == SVTK_MAX || this->Operation == SVTK_ATAN2)
  {
    if (!inInfo2)
    {
      svtkErrorMacro(<< "Second input must be specified for this operation.");
      return 1;
    }

    inInfo2->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), ext2);
    for (idx = 0; idx < 3; ++idx)
    {
      if (ext2[idx * 2] > ext[idx * 2])
      {
        ext[idx * 2] = ext2[idx * 2];
      }
      if (ext2[idx * 2 + 1] < ext[idx * 2 + 1])
      {
        ext[idx * 2 + 1] = ext2[idx * 2 + 1];
      }
    }
  }

  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), ext, 6);

  return 1;
}

//----------------------------------------------------------------------------
template <class TValue, class TIvar>
void svtkImageMathematicsClamp(TValue& value, TIvar ivar, svtkImageData* data)
{
  if (ivar < static_cast<TIvar>(data->GetScalarTypeMin()))
  {
    value = static_cast<TValue>(data->GetScalarTypeMin());
  }
  else if (ivar > static_cast<TIvar>(data->GetScalarTypeMax()))
  {
    value = static_cast<TValue>(data->GetScalarTypeMax());
  }
  else
  {
    value = static_cast<TValue>(ivar);
  }
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
// Handles the one input operations
template <class T>
void svtkImageMathematicsExecute1(svtkImageMathematics* self, svtkImageData* in1Data, T* in1Ptr,
  svtkImageData* outData, T* outPtr, int outExt[6], int id)
{
  int idxR, idxY, idxZ;
  int maxY, maxZ;
  svtkIdType inIncX, inIncY, inIncZ;
  svtkIdType outIncX, outIncY, outIncZ;
  int rowLength;
  unsigned long count = 0;
  unsigned long target;
  int op = self->GetOperation();

  // find the region to loop over
  rowLength = (outExt[1] - outExt[0] + 1) * in1Data->GetNumberOfScalarComponents();
  // What a pain.  Maybe I should just make another filter.
  if (op == SVTK_CONJUGATE)
  {
    rowLength = (outExt[1] - outExt[0] + 1);
  }
  maxY = outExt[3] - outExt[2];
  maxZ = outExt[5] - outExt[4];
  target = static_cast<unsigned long>((maxZ + 1) * (maxY + 1) / 50.0);
  target++;

  // Get increments to march through data
  in1Data->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  int divideByZeroToC = self->GetDivideByZeroToC();
  double doubleConstantk = self->GetConstantK();

  // Avoid casts by making constants the same type as input/output
  // Of course they must be clamped to a valid range for the scalar type
  T constantk, constantc;
  svtkImageMathematicsClamp(constantk, self->GetConstantK(), in1Data);
  svtkImageMathematicsClamp(constantc, self->GetConstantC(), in1Data);

  // Loop through output pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
  {
    for (idxY = 0; idxY <= maxY; idxY++)
    {
      if (!id)
      {
        if (!(count % target))
        {
          self->UpdateProgress(count / (50.0 * target));
        }
        count++;
      }
      for (idxR = 0; idxR < rowLength; idxR++)
      {
        // Pixel operation
        switch (op)
        {
          case SVTK_INVERT:
            if (*in1Ptr)
            {
              *outPtr = static_cast<T>(1.0 / *in1Ptr);
            }
            else
            {
              if (divideByZeroToC)
              {
                *outPtr = constantc;
              }
              else
              {
                *outPtr = static_cast<T>(outData->GetScalarTypeMax());
              }
            }
            break;
          case SVTK_SIN:
            *outPtr = static_cast<T>(sin(static_cast<double>(*in1Ptr)));
            break;
          case SVTK_COS:
            *outPtr = static_cast<T>(cos(static_cast<double>(*in1Ptr)));
            break;
          case SVTK_EXP:
            *outPtr = static_cast<T>(exp(static_cast<double>(*in1Ptr)));
            break;
          case SVTK_LOG:
            *outPtr = static_cast<T>(log(static_cast<double>(*in1Ptr)));
            break;
          case SVTK_ABS:
            *outPtr = static_cast<T>(fabs(static_cast<double>(*in1Ptr)));
            break;
          case SVTK_SQR:
            *outPtr = static_cast<T>(*in1Ptr * *in1Ptr);
            break;
          case SVTK_SQRT:
            *outPtr = static_cast<T>(sqrt(static_cast<double>(*in1Ptr)));
            break;
          case SVTK_ATAN:
            *outPtr = static_cast<T>(atan(static_cast<double>(*in1Ptr)));
            break;
          case SVTK_MULTIPLYBYK:
            *outPtr = static_cast<T>(doubleConstantk * static_cast<double>(*in1Ptr));
            break;
          case SVTK_ADDC:
            *outPtr = constantc + *in1Ptr;
            break;
          case SVTK_REPLACECBYK:
            *outPtr = (*in1Ptr == constantc) ? constantk : *in1Ptr;
            break;
          case SVTK_CONJUGATE:
            outPtr[0] = in1Ptr[0];
            outPtr[1] = static_cast<T>(-1.0 * static_cast<double>(in1Ptr[1]));
            // Why bother trying to figure out the continuous increments.
            outPtr++;
            in1Ptr++;
            break;
        }
        outPtr++;
        in1Ptr++;
      }
      outPtr += outIncY;
      in1Ptr += inIncY;
    }
    outPtr += outIncZ;
    in1Ptr += inIncZ;
  }
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
// Handles the two input operations
template <class T>
void svtkImageMathematicsExecute2(svtkImageMathematics* self, svtkImageData* in1Data, T* in1Ptr,
  svtkImageData* in2Data, T* in2Ptr, svtkImageData* outData, T* outPtr, int outExt[6], int id)
{
  int idxR, idxY, idxZ;
  int maxY, maxZ;
  svtkIdType inIncX, inIncY, inIncZ;
  svtkIdType in2IncX, in2IncY, in2IncZ;
  svtkIdType outIncX, outIncY, outIncZ;
  int rowLength;
  unsigned long count = 0;
  unsigned long target;
  int op = self->GetOperation();
  int divideByZeroToC = self->GetDivideByZeroToC();
  double constantc = self->GetConstantC();

  // find the region to loop over
  rowLength = (outExt[1] - outExt[0] + 1) * in1Data->GetNumberOfScalarComponents();
  // What a pain.  Maybe I should just make another filter.
  if (op == SVTK_COMPLEX_MULTIPLY)
  {
    rowLength = (outExt[1] - outExt[0] + 1);
  }

  maxY = outExt[3] - outExt[2];
  maxZ = outExt[5] - outExt[4];
  target = static_cast<unsigned long>((maxZ + 1) * (maxY + 1) / 50.0);
  target++;

  // Get increments to march through data
  in1Data->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  in2Data->GetContinuousIncrements(outExt, in2IncX, in2IncY, in2IncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  // Loop through output pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
  {
    for (idxY = 0; !self->AbortExecute && idxY <= maxY; idxY++)
    {
      if (!id)
      {
        if (!(count % target))
        {
          self->UpdateProgress(count / (50.0 * target));
        }
        count++;
      }
      for (idxR = 0; idxR < rowLength; idxR++)
      {
        // Pixel operation
        switch (op)
        {
          case SVTK_ADD:
            *outPtr = *in1Ptr + *in2Ptr;
            break;
          case SVTK_SUBTRACT:
            *outPtr = *in1Ptr - *in2Ptr;
            break;
          case SVTK_MULTIPLY:
            *outPtr = *in1Ptr * *in2Ptr;
            break;
          case SVTK_DIVIDE:
            if (*in2Ptr)
            {
              *outPtr = *in1Ptr / *in2Ptr;
            }
            else
            {
              if (divideByZeroToC)
              {
                *outPtr = static_cast<T>(constantc);
              }
              else
              {
                // *outPtr = (T)(*in1Ptr / 0.00001);
                *outPtr = static_cast<T>(outData->GetScalarTypeMax());
              }
            }
            break;
          case SVTK_MIN:
            if (*in1Ptr < *in2Ptr)
            {
              *outPtr = *in1Ptr;
            }
            else
            {
              *outPtr = *in2Ptr;
            }
            break;
          case SVTK_MAX:
            if (*in1Ptr > *in2Ptr)
            {
              *outPtr = *in1Ptr;
            }
            else
            {
              *outPtr = *in2Ptr;
            }
            break;
          case SVTK_ATAN2:
            if (*in1Ptr == 0.0 && *in2Ptr == 0.0)
            {
              *outPtr = 0;
            }
            else
            {
              *outPtr =
                static_cast<T>(atan2(static_cast<double>(*in1Ptr), static_cast<double>(*in2Ptr)));
            }
            break;
          case SVTK_COMPLEX_MULTIPLY:
            outPtr[0] = in1Ptr[0] * in2Ptr[0] - in1Ptr[1] * in2Ptr[1];
            outPtr[1] = in1Ptr[1] * in2Ptr[0] + in1Ptr[0] * in2Ptr[1];
            // Why bother trying to figure out the continuous increments.
            outPtr++;
            in1Ptr++;
            in2Ptr++;
            break;
        }
        outPtr++;
        in1Ptr++;
        in2Ptr++;
      }
      outPtr += outIncY;
      in1Ptr += inIncY;
      in2Ptr += in2IncY;
    }
    outPtr += outIncZ;
    in1Ptr += inIncZ;
    in2Ptr += in2IncZ;
  }
}

//----------------------------------------------------------------------------
// This method is passed a input and output datas, and executes the filter
// algorithm to fill the output from the inputs.
// It just executes a switch statement to call the correct function for
// the datas data types.
void svtkImageMathematics::ThreadedRequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* svtkNotUsed(outputVector),
  svtkImageData*** inData, svtkImageData** outData, int outExt[6], int id)
{
  void* inPtr1;
  void* outPtr;

  inPtr1 = inData[0][0]->GetScalarPointerForExtent(outExt);
  outPtr = outData[0]->GetScalarPointerForExtent(outExt);

  if (this->Operation == SVTK_ADD || this->Operation == SVTK_SUBTRACT ||
    this->Operation == SVTK_MULTIPLY || this->Operation == SVTK_DIVIDE ||
    this->Operation == SVTK_MIN || this->Operation == SVTK_MAX || this->Operation == SVTK_ATAN2 ||
    this->Operation == SVTK_COMPLEX_MULTIPLY)
  {
    void* inPtr2;

    if (this->Operation == SVTK_COMPLEX_MULTIPLY)
    {
      if (inData[0][0]->GetNumberOfScalarComponents() != 2 ||
        inData[1][0]->GetNumberOfScalarComponents() != 2)
      {
        svtkErrorMacro("Complex inputs must have two components.");
        return;
      }
    }

    if (!inData[1] || !inData[1][0])
    {
      svtkErrorMacro("ImageMathematics requested to perform a two input operation "
                    "with only one input\n");
      return;
    }

    inPtr2 = inData[1][0]->GetScalarPointerForExtent(outExt);

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
      svtkTemplateMacro(
        svtkImageMathematicsExecute2(this, inData[0][0], static_cast<SVTK_TT*>(inPtr1), inData[1][0],
          static_cast<SVTK_TT*>(inPtr2), outData[0], static_cast<SVTK_TT*>(outPtr), outExt, id));
      default:
        svtkErrorMacro(<< "Execute: Unknown ScalarType");
        return;
    }
  }
  else
  {
    // this filter expects that input is the same type as output.
    if (inData[0][0]->GetScalarType() != outData[0]->GetScalarType())
    {
      svtkErrorMacro(<< "Execute: input ScalarType, " << inData[0][0]->GetScalarType()
                    << ", must match out ScalarType " << outData[0]->GetScalarType());
      return;
    }

    if (this->Operation == SVTK_CONJUGATE)
    {
      if (inData[0][0]->GetNumberOfScalarComponents() != 2)
      {
        svtkErrorMacro("Complex inputs must have two components.");
        return;
      }
    }

    switch (inData[0][0]->GetScalarType())
    {
      svtkTemplateMacro(svtkImageMathematicsExecute1(this, inData[0][0], static_cast<SVTK_TT*>(inPtr1),
        outData[0], static_cast<SVTK_TT*>(outPtr), outExt, id));
      default:
        svtkErrorMacro(<< "Execute: Unknown ScalarType");
        return;
    }
  }
}

//----------------------------------------------------------------------------
int svtkImageMathematics::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageData");
  return 1;
}

//----------------------------------------------------------------------------
void svtkImageMathematics::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Operation: " << this->Operation << "\n";
  os << indent << "ConstantK: " << this->ConstantK << "\n";
  os << indent << "ConstantC: " << this->ConstantC << "\n";
  os << indent << "DivideByZeroToC: " << (this->DivideByZeroToC ? "On" : "Off") << "\n";
}
