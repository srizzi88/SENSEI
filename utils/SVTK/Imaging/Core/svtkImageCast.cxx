/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageCast.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageCast.h"

#include "svtkImageData.h"
#include "svtkImageProgressIterator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkImageCast);

//----------------------------------------------------------------------------
svtkImageCast::svtkImageCast()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
  this->OutputScalarType = SVTK_FLOAT;
  this->ClampOverflow = 0;
}

//----------------------------------------------------------------------------
// Just change the Image type.
int svtkImageCast::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkDataObject::SetPointDataActiveScalarInfo(outInfo, this->OutputScalarType, -1);
  return 1;
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class IT, class OT>
void svtkImageCastExecute(
  svtkImageCast* self, svtkImageData* inData, svtkImageData* outData, int outExt[6], int id, IT*, OT*)
{
  svtkImageIterator<IT> inIt(inData, outExt);
  svtkImageProgressIterator<OT> outIt(outData, outExt, self, id);

  double typeMin, typeMax, val;
  int clamp;

  // for preventing overflow
  typeMin = outData->GetScalarTypeMin();
  typeMax = outData->GetScalarTypeMax();
  clamp = self->GetClampOverflow();

  // Loop through output pixels
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
        val = static_cast<double>(*inSI);
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
        // now process the components
        // NB: without clamping, this cast may result in undefined behavior!
        *outSI = static_cast<OT>(*inSI);
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
void svtkImageCastExecute(
  svtkImageCast* self, svtkImageData* inData, svtkImageData* outData, int outExt[6], int id, T*)
{
  switch (outData->GetScalarType())
  {
    svtkTemplateMacro(svtkImageCastExecute(
      self, inData, outData, outExt, id, static_cast<T*>(nullptr), static_cast<SVTK_TT*>(nullptr)));
    default:
      svtkGenericWarningMacro("Execute: Unknown output ScalarType");
      return;
  }
}

//----------------------------------------------------------------------------
// This method is passed a input and output region, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the regions data types.
void svtkImageCast::ThreadedExecute(
  svtkImageData* inData, svtkImageData* outData, int outExt[6], int id)
{
  switch (inData->GetScalarType())
  {
    svtkTemplateMacro(
      svtkImageCastExecute(this, inData, outData, outExt, id, static_cast<SVTK_TT*>(nullptr)));
    default:
      svtkErrorMacro(<< "Execute: Unknown input ScalarType");
      return;
  }
}

//----------------------------------------------------------------------------
void svtkImageCast::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "OutputScalarType: " << this->OutputScalarType << "\n";
  os << indent << "ClampOverflow: ";
  if (this->ClampOverflow)
  {
    os << "On\n";
  }
  else
  {
    os << "Off\n";
  }
}
