/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageMaskBits.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageMaskBits.h"

#include "svtkImageData.h"
#include "svtkImageProgressIterator.h"
#include "svtkObjectFactory.h"

#include <cmath>

svtkStandardNewMacro(svtkImageMaskBits);

svtkImageMaskBits::svtkImageMaskBits()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
  this->Operation = SVTK_AND;
  this->Masks[0] = 0xffffffff;
  this->Masks[1] = 0xffffffff;
  this->Masks[2] = 0xffffffff;
  this->Masks[3] = 0xffffffff;
}

//----------------------------------------------------------------------------
// This execute method handles boundaries.
// it handles boundaries. Pixels are just replicated to get values
// out of extent.
template <class T>
void svtkImageMaskBitsExecute(
  svtkImageMaskBits* self, svtkImageData* inData, svtkImageData* outData, int outExt[6], int id, T*)
{
  svtkImageIterator<T> inIt(inData, outExt);
  svtkImageProgressIterator<T> outIt(outData, outExt, self, id);
  int idxC, maxC;
  unsigned int* masks;
  int operation;

  // find the region to loop over
  maxC = inData->GetNumberOfScalarComponents();
  masks = self->GetMasks();
  operation = self->GetOperation();

  // Loop through output pixels
  while (!outIt.IsAtEnd())
  {
    T* inSI = inIt.BeginSpan();
    T* outSI = outIt.BeginSpan();
    T* outSIEnd = outIt.EndSpan();
    switch (operation)
    {
      case SVTK_AND:
        while (outSI != outSIEnd)
        {
          for (idxC = 0; idxC < maxC; idxC++)
          {
            // Pixel operation
            *outSI++ = *inSI++ & static_cast<T>(masks[idxC]);
          }
        }
        break;
      case SVTK_OR:
        while (outSI != outSIEnd)
        {
          for (idxC = 0; idxC < maxC; idxC++)
          {
            // Pixel operation
            *outSI++ = *inSI++ | static_cast<T>(masks[idxC]);
          }
        }
        break;
      case SVTK_XOR:
        while (outSI != outSIEnd)
        {
          for (idxC = 0; idxC < maxC; idxC++)
          {
            // Pixel operation
            *outSI++ = *inSI++ ^ static_cast<T>(masks[idxC]);
          }
        }
        break;
      case SVTK_NAND:
        while (outSI != outSIEnd)
        {
          for (idxC = 0; idxC < maxC; idxC++)
          {
            // Pixel operation
            *outSI++ = ~(*inSI++ & static_cast<T>(masks[idxC]));
          }
        }
        break;
      case SVTK_NOR:
        while (outSI != outSIEnd)
        {
          for (idxC = 0; idxC < maxC; idxC++)
          {
            // Pixel operation
            *outSI++ = ~(*inSI++ | static_cast<T>(masks[idxC]));
          }
        }
        break;
    }
    inIt.NextSpan();
    outIt.NextSpan();
  }
}

//----------------------------------------------------------------------------
// This method contains a switch statement that calls the correct
// templated function for the input data type.  The output data
// must match input type.  This method does handle boundary conditions.
void svtkImageMaskBits::ThreadedExecute(
  svtkImageData* inData, svtkImageData* outData, int outExt[6], int id)
{
  // this filter expects that input is the same type as output.
  if (inData->GetScalarType() != outData->GetScalarType())
  {
    svtkErrorMacro(<< "Execute: input ScalarType, " << inData->GetScalarType()
                  << ", must match out ScalarType " << outData->GetScalarType());
    return;
  }

  switch (inData->GetScalarType())
  {
    case SVTK_INT:
      svtkImageMaskBitsExecute(this, inData, outData, outExt, id, static_cast<int*>(nullptr));
      break;
    case SVTK_UNSIGNED_INT:
      svtkImageMaskBitsExecute(
        this, inData, outData, outExt, id, static_cast<unsigned int*>(nullptr));
      break;
    case SVTK_LONG:
      svtkImageMaskBitsExecute(this, inData, outData, outExt, id, static_cast<long*>(nullptr));
      break;
    case SVTK_UNSIGNED_LONG:
      svtkImageMaskBitsExecute(
        this, inData, outData, outExt, id, static_cast<unsigned long*>(nullptr));
      break;
    case SVTK_SHORT:
      svtkImageMaskBitsExecute(this, inData, outData, outExt, id, static_cast<short*>(nullptr));
      break;
    case SVTK_UNSIGNED_SHORT:
      svtkImageMaskBitsExecute(
        this, inData, outData, outExt, id, static_cast<unsigned short*>(nullptr));
      break;
    case SVTK_CHAR:
      svtkImageMaskBitsExecute(this, inData, outData, outExt, id, static_cast<char*>(nullptr));
      break;
    case SVTK_UNSIGNED_CHAR:
      svtkImageMaskBitsExecute(
        this, inData, outData, outExt, id, static_cast<unsigned char*>(nullptr));
      break;
    default:
      svtkErrorMacro(<< "Execute: ScalarType can only be [unsigned] char, [unsigned] short, "
                    << "[unsigned] int, or [unsigned] long.");
      return;
  }
}

void svtkImageMaskBits::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Operation: " << this->Operation << "\n";
  os << indent << "Masks: (" << this->Masks[0] << ", " << this->Masks[1] << ", " << this->Masks[2]
     << ", " << this->Masks[3] << ")" << endl;
}
