/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageLogic.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageLogic.h"

#include "svtkImageData.h"
#include "svtkImageProgressIterator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include <cmath>

svtkStandardNewMacro(svtkImageLogic);

//----------------------------------------------------------------------------
svtkImageLogic::svtkImageLogic()
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);
  this->Operation = SVTK_AND;
  this->OutputTrueValue = 255;
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
// Handles the one input operations
template <class T>
void svtkImageLogicExecute1(
  svtkImageLogic* self, svtkImageData* inData, svtkImageData* outData, int outExt[6], int id, T*)
{
  svtkImageIterator<T> inIt(inData, outExt);
  svtkImageProgressIterator<T> outIt(outData, outExt, self, id);
  T trueValue = static_cast<T>(self->GetOutputTrueValue());
  int op = self->GetOperation();

  // Loop through output pixels
  while (!outIt.IsAtEnd())
  {
    T* inSI = inIt.BeginSpan();
    T* outSI = outIt.BeginSpan();
    T* outSIEnd = outIt.EndSpan();
    // Pixel operation
    switch (op)
    {
      case SVTK_NOT:
        while (outSI != outSIEnd)
        {
          if (!*inSI)
          {
            *outSI = trueValue;
          }
          else
          {
            *outSI = 0;
          }
          outSI++;
          inSI++;
        }
        break;
      case SVTK_NOP:
        while (outSI != outSIEnd)
        {
          if (*inSI)
          {
            *outSI = trueValue;
          }
          else
          {
            *outSI = 0;
          }
          outSI++;
          inSI++;
        }
        break;
    }
    inIt.NextSpan();
    outIt.NextSpan();
  }
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
// Handles the two input operations
template <class T>
void svtkImageLogicExecute2(svtkImageLogic* self, svtkImageData* in1Data, svtkImageData* in2Data,
  svtkImageData* outData, int outExt[6], int id, T*)
{
  svtkImageIterator<T> inIt1(in1Data, outExt);
  svtkImageIterator<T> inIt2(in2Data, outExt);
  svtkImageProgressIterator<T> outIt(outData, outExt, self, id);
  T trueValue = static_cast<T>(self->GetOutputTrueValue());
  int op = self->GetOperation();

  // Loop through output pixels
  while (!outIt.IsAtEnd())
  {
    T* inSI1 = inIt1.BeginSpan();
    T* inSI2 = inIt2.BeginSpan();
    T* outSI = outIt.BeginSpan();
    T* outSIEnd = outIt.EndSpan();
    // Pixel operation
    switch (op)
    {
      case SVTK_AND:
        while (outSI != outSIEnd)
        {
          if (*inSI1 && *inSI2)
          {
            *outSI = trueValue;
          }
          else
          {
            *outSI = 0;
          }
          outSI++;
          inSI1++;
          inSI2++;
        }
        break;
      case SVTK_OR:
        while (outSI != outSIEnd)
        {
          if (*inSI1 || *inSI2)
          {
            *outSI = trueValue;
          }
          else
          {
            *outSI = 0;
          }
          outSI++;
          inSI1++;
          inSI2++;
        }
        break;
      case SVTK_XOR:
        while (outSI != outSIEnd)
        {
          if ((!*inSI1 && *inSI2) || (*inSI1 && !*inSI2))
          {
            *outSI = trueValue;
          }
          else
          {
            *outSI = 0;
          }
          outSI++;
          inSI1++;
          inSI2++;
        }
        break;
      case SVTK_NAND:
        while (outSI != outSIEnd)
        {
          if (!(*inSI1 && *inSI2))
          {
            *outSI = trueValue;
          }
          else
          {
            *outSI = 0;
          }
          outSI++;
          inSI1++;
          inSI2++;
        }
        break;
      case SVTK_NOR:
        while (outSI != outSIEnd)
        {
          if (!(*inSI1 || *inSI2))
          {
            *outSI = trueValue;
          }
          else
          {
            *outSI = 0;
          }
          outSI++;
          inSI1++;
          inSI2++;
        }
        break;
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
void svtkImageLogic::ThreadedRequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* svtkNotUsed(outputVector),
  svtkImageData*** inData, svtkImageData** outData, int outExt[6], int id)
{
  if (inData[0][0] == nullptr)
  {
    svtkErrorMacro(<< "Input " << 0 << " must be specified.");
    return;
  }

  // this filter expects that input is the same type as output.
  if (inData[0][0]->GetScalarType() != outData[0]->GetScalarType())
  {
    svtkErrorMacro(<< "Execute: input ScalarType, " << inData[0][0]->GetScalarType()
                  << ", must match out ScalarType " << outData[0]->GetScalarType());
    return;
  }

  if (this->Operation == SVTK_NOT || this->Operation == SVTK_NOP)
  {
    switch (inData[0][0]->GetScalarType())
    {
      svtkTemplateMacro(svtkImageLogicExecute1(
        this, inData[0][0], outData[0], outExt, id, static_cast<SVTK_TT*>(nullptr)));
      default:
        svtkErrorMacro(<< "Execute: Unknown ScalarType");
        return;
    }
  }
  else
  {
    if (inData[1][0] == nullptr)
    {
      svtkErrorMacro(<< "Input " << 1 << " must be specified.");
      return;
    }

    // this filter expects that inputs that have the same type:
    if (inData[0][0]->GetScalarType() != inData[1][0]->GetScalarType())
    {
      svtkErrorMacro(<< "Execute: input1 ScalarType, " << inData[0][0]->GetScalarType()
                    << ", must match input2 ScalarType " << inData[1][0]->GetScalarType());
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
      svtkTemplateMacro(svtkImageLogicExecute2(
        this, inData[0][0], inData[1][0], outData[0], outExt, id, static_cast<SVTK_TT*>(nullptr)));
      default:
        svtkErrorMacro(<< "Execute: Unknown ScalarType");
        return;
    }
  }
}

//----------------------------------------------------------------------------
int svtkImageLogic::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageData");
  return 1;
}

//----------------------------------------------------------------------------
void svtkImageLogic::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Operation: " << this->Operation << "\n";
  os << indent << "OutputTrueValue: " << this->OutputTrueValue << "\n";
}
