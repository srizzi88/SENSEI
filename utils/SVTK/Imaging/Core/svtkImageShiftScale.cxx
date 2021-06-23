/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageShiftScale.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageShiftScale.h"

#include "svtkImageData.h"
#include "svtkImageProgressIterator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkImageShiftScale);

//----------------------------------------------------------------------------
svtkImageShiftScale::svtkImageShiftScale()
{
  this->Shift = 0.0;
  this->Scale = 1.0;
  this->OutputScalarType = -1;
  this->ClampOverflow = 0;
}

//----------------------------------------------------------------------------
svtkImageShiftScale::~svtkImageShiftScale() = default;

//----------------------------------------------------------------------------
void svtkImageShiftScale::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Shift: " << this->Shift << "\n";
  os << indent << "Scale: " << this->Scale << "\n";
  os << indent << "Output Scalar Type: " << this->OutputScalarType << "\n";
  os << indent << "ClampOverflow: " << (this->ClampOverflow ? "On" : "Off") << "\n";
}

//----------------------------------------------------------------------------
int svtkImageShiftScale::RequestInformation(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector)
{
  // Set the image scalar type for the output.
  if (this->OutputScalarType != -1)
  {
    svtkInformation* outInfo = outputVector->GetInformationObject(0);
    svtkDataObject::SetPointDataActiveScalarInfo(outInfo, this->OutputScalarType, -1);
  }
  return 1;
}

//----------------------------------------------------------------------------
// This function template implements the filter for any type of data.
// The last two arguments help the svtkTemplateMacro calls below
// instantiate the proper input and output types.
template <class IT, class OT>
void svtkImageShiftScaleExecute(svtkImageShiftScale* self, svtkImageData* inData,
  svtkImageData* outData, int outExt[6], int id, IT*, OT*)
{
  // Create iterators for the input and output extents assigned to
  // this thread.
  svtkImageIterator<IT> inIt(inData, outExt);
  svtkImageProgressIterator<OT> outIt(outData, outExt, self, id);

  // Get the shift and scale parameters values.
  double shift = self->GetShift();
  double scale = self->GetScale();

  // Clamp pixel values within the range of the output type.
  double typeMin = outData->GetScalarTypeMin();
  double typeMax = outData->GetScalarTypeMax();
  int clamp = self->GetClampOverflow();

  // Loop through output pixels.
  while (!outIt.IsAtEnd())
  {
    IT* inSI = inIt.BeginSpan();
    OT* outSI = outIt.BeginSpan();
    OT* outSIEnd = outIt.EndSpan();
    if (clamp)
    {
      while (outSI != outSIEnd)
      {
        // Pixel operation
        double val = (static_cast<double>(*inSI) + shift) * scale;
        if (val > typeMax)
        {
          val = typeMax;
        }
        if (val < typeMin)
        {
          val = typeMin;
        }
        *outSI = static_cast<OT>(val);
        ++outSI;
        ++inSI;
      }
    }
    else
    {
      while (outSI != outSIEnd)
      {
        // Pixel operation
        double val = (static_cast<double>(*inSI) + shift) * scale;

        // NB: without clamping, this cast may result in undefined behavior!
        *outSI = static_cast<OT>(val);
        ++outSI;
        ++inSI;
      }
    }
    inIt.NextSpan();
    outIt.NextSpan();
  }
}

//----------------------------------------------------------------------------
template <class T>
void svtkImageShiftScaleExecute1(
  svtkImageShiftScale* self, svtkImageData* inData, svtkImageData* outData, int outExt[6], int id, T*)
{
  switch (outData->GetScalarType())
  {
    svtkTemplateMacro(svtkImageShiftScaleExecute(
      self, inData, outData, outExt, id, static_cast<T*>(nullptr), static_cast<SVTK_TT*>(nullptr)));
    default:
      svtkErrorWithObjectMacro(self, "ThreadedRequestData: Unknown output ScalarType");
      return;
  }
}

//----------------------------------------------------------------------------
// This method is passed a input and output data, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the datas data types.
void svtkImageShiftScale::ThreadedRequestData(svtkInformation*, svtkInformationVector**,
  svtkInformationVector*, svtkImageData*** inData, svtkImageData** outData, int outExt[6],
  int threadId)
{
  svtkImageData* input = inData[0][0];
  svtkImageData* output = outData[0];
  switch (input->GetScalarType())
  {
    svtkTemplateMacro(svtkImageShiftScaleExecute1(
      this, input, output, outExt, threadId, static_cast<SVTK_TT*>(nullptr)));
    default:
      svtkErrorMacro("ThreadedRequestData: Unknown input ScalarType");
      return;
  }
}
