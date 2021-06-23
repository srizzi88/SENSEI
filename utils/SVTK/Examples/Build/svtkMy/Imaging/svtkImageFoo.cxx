/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageFoo.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageFoo.h"

#include "svtkBar.h"
#include "svtkImageData.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"

//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkImageFoo);

//----------------------------------------------------------------------------
svtkImageFoo::svtkImageFoo()
{
  this->Foo = 0.0;
  this->OutputScalarType = -1;
  this->Bar = svtkBar::New();
}

//----------------------------------------------------------------------------
svtkImageFoo::~svtkImageFoo()
{
  if (this->Bar)
  {
    this->Bar->Delete();
    this->Bar = nullptr;
  }
}

//----------------------------------------------------------------------------
void svtkImageFoo::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Foo: " << this->Foo << "\n";
  os << indent << "Output Scalar Type: " << this->OutputScalarType << "\n";
}

//----------------------------------------------------------------------------
int svtkImageFoo::RequestInformation(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector)
{
  // Set the scalar type we will produce in the output information for
  // the first output port.
  if (this->OutputScalarType != -1)
  {
    svtkInformation* outInfo = outputVector->GetInformationObject(0);
    svtkDataObject::SetPointDataActiveScalarInfo(outInfo, this->OutputScalarType, -1);
  }
  return 1;
}

//----------------------------------------------------------------------------
// This function template implements the filter for any combination of
// input and output data type.
template <class IT, class OT>
void svtkImageFooExecute(svtkImageFoo* self, svtkImageData* inData, IT* inPtr, svtkImageData* outData,
  OT* outPtr, int outExt[6], int id)
{
  float foo = self->GetFoo();

  int idxR, idxY, idxZ;
  int maxY, maxZ;
  svtkIdType inIncX, inIncY, inIncZ;
  svtkIdType outIncX, outIncY, outIncZ;
  int rowLength;

  unsigned long count = 0;
  unsigned long target;

  // find the region to loop over

  rowLength = (outExt[1] - outExt[0] + 1) * inData->GetNumberOfScalarComponents();
  maxY = outExt[3] - outExt[2];
  maxZ = outExt[5] - outExt[4];
  target = (unsigned long)((maxZ + 1) * (maxY + 1) / 50.0);
  target++;

  // Get increments to march through data

  inData->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
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
        // Pixel operation. Add foo. Dumber would be impossible.
        *outPtr = (OT)((float)(*inPtr) + foo);
        outPtr++;
        inPtr++;
      }
      outPtr += outIncY;
      inPtr += inIncY;
    }
    outPtr += outIncZ;
    inPtr += inIncZ;
  }
}

//----------------------------------------------------------------------------
// This function template is instantiated for each input data type and
// forwards the call to the above function template for each output
// data type.
template <class T>
void svtkImageFooExecute1(
  svtkImageFoo* self, svtkImageData* inData, T* inPtr, svtkImageData* outData, int outExt[6], int id)
{
  void* outPtr = outData->GetScalarPointerForExtent(outExt);
  int outType = outData->GetScalarType();
  switch (outType)
  {
    svtkTemplateMacro(
      svtkImageFooExecute(self, inData, inPtr, outData, static_cast<SVTK_TT*>(outPtr), outExt, id));
    default:
      svtkErrorWithObjectMacro(self, "Unknown output scalar type " << outType);
      return;
  }
}

//----------------------------------------------------------------------------
// This method is passed an input and output data, and executes the
// filter algorithm to fill the output from the input.  It just
// executes a switch statement to call the correct function for the
// datas data types.
void svtkImageFoo::ThreadedRequestData(svtkInformation*, svtkInformationVector**,
  svtkInformationVector*, svtkImageData*** inData, svtkImageData** outData, int outExt[6], int id)
{
  void* inPtr = inData[0][0]->GetScalarPointerForExtent(outExt);
  int inType = inData[0][0]->GetScalarType();
  switch (inType)
  {
    svtkTemplateMacro(
      svtkImageFooExecute1(this, inData[0][0], static_cast<SVTK_TT*>(inPtr), outData[0], outExt, id));
    default:
      svtkErrorMacro("Unknown input scalar type " << inType);
      return;
  }
}
