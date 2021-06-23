/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageThreshold.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageThreshold.h"

#include "svtkDataSetAttributes.h"
#include "svtkImageData.h"
#include "svtkImageProgressIterator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkImageThreshold);

//----------------------------------------------------------------------------
// Constructor sets default values
svtkImageThreshold::svtkImageThreshold()
{
  this->UpperThreshold = SVTK_FLOAT_MAX;
  this->LowerThreshold = -SVTK_FLOAT_MAX;
  this->ReplaceIn = 0;
  this->InValue = 0.0;
  this->ReplaceOut = 0;
  this->OutValue = 0.0;

  this->OutputScalarType = -1; // invalid; output same as input
}

//----------------------------------------------------------------------------
void svtkImageThreshold::SetInValue(double val)
{
  if (val != this->InValue || this->ReplaceIn != 1)
  {
    this->InValue = val;
    this->ReplaceIn = 1;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkImageThreshold::SetOutValue(double val)
{
  if (val != this->OutValue || this->ReplaceOut != 1)
  {
    this->OutValue = val;
    this->ReplaceOut = 1;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
// The values greater than or equal to the value match.
void svtkImageThreshold::ThresholdByUpper(double thresh)
{
  if (this->LowerThreshold != thresh || this->UpperThreshold < SVTK_FLOAT_MAX)
  {
    this->LowerThreshold = thresh;
    this->UpperThreshold = SVTK_FLOAT_MAX;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
// The values less than or equal to the value match.
void svtkImageThreshold::ThresholdByLower(double thresh)
{
  if (this->UpperThreshold != thresh || this->LowerThreshold > -SVTK_FLOAT_MAX)
  {
    this->UpperThreshold = thresh;
    this->LowerThreshold = -SVTK_FLOAT_MAX;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
// The values in a range (inclusive) match
void svtkImageThreshold::ThresholdBetween(double lower, double upper)
{
  if (this->LowerThreshold != lower || this->UpperThreshold != upper)
  {
    this->LowerThreshold = lower;
    this->UpperThreshold = upper;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int svtkImageThreshold::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  if (this->OutputScalarType == -1)
  {
    svtkInformation* inScalarInfo = svtkDataObject::GetActiveFieldInformation(
      inInfo, svtkDataObject::FIELD_ASSOCIATION_POINTS, svtkDataSetAttributes::SCALARS);
    if (!inScalarInfo)
    {
      svtkErrorMacro("Missing scalar field on input information!");
      return 0;
    }
    svtkDataObject::SetPointDataActiveScalarInfo(
      outInfo, inScalarInfo->Get(svtkDataObject::FIELD_ARRAY_TYPE()), -1);
  }
  else
  {
    svtkDataObject::SetPointDataActiveScalarInfo(outInfo, this->OutputScalarType, -1);
  }
  return 1;
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class IT, class OT>
void svtkImageThresholdExecute(svtkImageThreshold* self, svtkImageData* inData, svtkImageData* outData,
  int outExt[6], int id, IT*, OT*)
{
  svtkImageIterator<IT> inIt(inData, outExt);
  svtkImageProgressIterator<OT> outIt(outData, outExt, self, id);
  IT lowerThreshold;
  IT upperThreshold;
  int replaceIn = self->GetReplaceIn();
  OT inValue;
  int replaceOut = self->GetReplaceOut();
  OT outValue;
  IT temp;

  // Make sure the thresholds are valid for the input scalar range
  if (static_cast<double>(self->GetLowerThreshold()) < inData->GetScalarTypeMin())
  {
    lowerThreshold = static_cast<IT>(inData->GetScalarTypeMin());
  }
  else
  {
    if (static_cast<double>(self->GetLowerThreshold()) > inData->GetScalarTypeMax())
    {
      lowerThreshold = static_cast<IT>(inData->GetScalarTypeMax());
    }
    else
    {
      lowerThreshold = static_cast<IT>(self->GetLowerThreshold());
    }
  }
  if (static_cast<double>(self->GetUpperThreshold()) > inData->GetScalarTypeMax())
  {
    upperThreshold = static_cast<IT>(inData->GetScalarTypeMax());
  }
  else
  {
    if (static_cast<double>(self->GetUpperThreshold()) < inData->GetScalarTypeMin())
    {
      upperThreshold = static_cast<IT>(inData->GetScalarTypeMin());
    }
    else
    {
      upperThreshold = static_cast<IT>(self->GetUpperThreshold());
    }
  }

  // Make sure the replacement values are within the output scalar range
  if (static_cast<double>(self->GetInValue()) < outData->GetScalarTypeMin())
  {
    inValue = static_cast<OT>(outData->GetScalarTypeMin());
  }
  else
  {
    if (static_cast<double>(self->GetInValue()) > outData->GetScalarTypeMax())
    {
      inValue = static_cast<OT>(outData->GetScalarTypeMax());
    }
    else
    {
      inValue = static_cast<OT>(self->GetInValue());
    }
  }
  if (static_cast<double>(self->GetOutValue()) > outData->GetScalarTypeMax())
  {
    outValue = static_cast<OT>(outData->GetScalarTypeMax());
  }
  else
  {
    if (static_cast<double>(self->GetOutValue()) < outData->GetScalarTypeMin())
    {
      outValue = static_cast<OT>(outData->GetScalarTypeMin());
    }
    else
    {
      outValue = static_cast<OT>(self->GetOutValue());
    }
  }

  // Loop through output pixels
  while (!outIt.IsAtEnd())
  {
    IT* inSI = inIt.BeginSpan();
    OT* outSI = outIt.BeginSpan();
    OT* outSIEnd = outIt.EndSpan();
    while (outSI != outSIEnd)
    {
      // Pixel operation
      temp = (*inSI);
      if (lowerThreshold <= temp && temp <= upperThreshold)
      {
        // match
        if (replaceIn)
        {
          *outSI = inValue;
        }
        else
        {
          *outSI = static_cast<OT>(temp);
        }
      }
      else
      {
        // not match
        if (replaceOut)
        {
          *outSI = outValue;
        }
        else
        {
          *outSI = static_cast<OT>(temp);
        }
      }
      ++inSI;
      ++outSI;
    }
    inIt.NextSpan();
    outIt.NextSpan();
  }
}

//----------------------------------------------------------------------------
template <class T>
void svtkImageThresholdExecute1(
  svtkImageThreshold* self, svtkImageData* inData, svtkImageData* outData, int outExt[6], int id, T*)
{
  switch (outData->GetScalarType())
  {
    svtkTemplateMacro(svtkImageThresholdExecute(
      self, inData, outData, outExt, id, static_cast<T*>(nullptr), static_cast<SVTK_TT*>(nullptr)));
    default:
      svtkGenericWarningMacro("Execute: Unknown input ScalarType");
      return;
  }
}

//----------------------------------------------------------------------------
// This method is passed a input and output data, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the datas data types.
void svtkImageThreshold::ThreadedRequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* svtkNotUsed(outputVector),
  svtkImageData*** inData, svtkImageData** outData, int outExt[6], int id)
{
  switch (inData[0][0]->GetScalarType())
  {
    svtkTemplateMacro(svtkImageThresholdExecute1(
      this, inData[0][0], outData[0], outExt, id, static_cast<SVTK_TT*>(nullptr)));
    default:
      svtkErrorMacro(<< "Execute: Unknown input ScalarType");
      return;
  }
}

//----------------------------------------------------------------------------
void svtkImageThreshold::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "OutputScalarType: " << this->OutputScalarType << "\n";
  os << indent << "InValue: " << this->InValue << "\n";
  os << indent << "OutValue: " << this->OutValue << "\n";
  os << indent << "LowerThreshold: " << this->LowerThreshold << "\n";
  os << indent << "UpperThreshold: " << this->UpperThreshold << "\n";
  os << indent << "ReplaceIn: " << this->ReplaceIn << "\n";
  os << indent << "ReplaceOut: " << this->ReplaceOut << "\n";
}
