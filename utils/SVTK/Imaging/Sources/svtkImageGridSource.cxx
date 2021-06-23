/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageGridSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageGridSource.h"

#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include <cmath>

svtkStandardNewMacro(svtkImageGridSource);

//----------------------------------------------------------------------------
svtkImageGridSource::svtkImageGridSource()
{
  this->DataExtent[0] = 0;
  this->DataExtent[1] = 255;
  this->DataExtent[2] = 0;
  this->DataExtent[3] = 255;
  this->DataExtent[4] = 0;
  this->DataExtent[5] = 0;

  this->GridSpacing[0] = 10;
  this->GridSpacing[1] = 10;
  this->GridSpacing[2] = 0;

  this->GridOrigin[0] = 0;
  this->GridOrigin[1] = 0;
  this->GridOrigin[2] = 0;

  this->DataScalarType = SVTK_FLOAT;

  this->DataOrigin[0] = this->DataOrigin[1] = this->DataOrigin[2] = 0.0;
  this->DataSpacing[0] = this->DataSpacing[1] = this->DataSpacing[2] = 1.0;

  this->LineValue = 1.0;
  this->FillValue = 0.0;
  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
int svtkImageGridSource::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  outInfo->Set(svtkDataObject::SPACING(), this->DataSpacing, 3);
  outInfo->Set(svtkDataObject::ORIGIN(), this->DataOrigin, 3);
  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), this->DataExtent, 6);
  svtkDataObject::SetPointDataActiveScalarInfo(outInfo, this->DataScalarType, 1);
  return 1;
}

//----------------------------------------------------------------------------
template <class T>
void svtkImageGridSourceExecute(
  svtkImageGridSource* self, svtkImageData* data, T* outPtr, int outExt[6], int id)
{
  int idxX, idxY, idxZ;
  int xval, yval, zval;
  svtkIdType outIncX, outIncY, outIncZ;
  unsigned long count = 0;
  unsigned long target;

  int gridSpacing[3], gridOrigin[3];
  self->GetGridSpacing(gridSpacing);
  self->GetGridOrigin(gridOrigin);

  T fillValue = T(self->GetFillValue());
  T lineValue = T(self->GetLineValue());

  // Get increments to march through data
  data->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  target =
    static_cast<unsigned long>((outExt[5] - outExt[4] + 1) * (outExt[3] - outExt[2] + 1) / 50.0);
  target++;

  // Loop through output pixel
  for (idxZ = outExt[4]; idxZ <= outExt[5]; idxZ++)
  {
    if (gridSpacing[2])
    {
      zval = (idxZ % gridSpacing[2] == gridOrigin[2]);
    }
    else
    {
      zval = 0;
    }
    for (idxY = outExt[2]; !self->GetAbortExecute() && idxY <= outExt[3]; idxY++)
    {
      if (gridSpacing[1])
      {
        yval = (idxY % gridSpacing[1] == gridOrigin[1]);
      }
      else
      {
        yval = 0;
      }
      if (id == 0)
      {
        if (!(count % target))
        {
          self->UpdateProgress(count / (50.0 * target));
        }
        count++;
      }

      if (gridSpacing[0])
      {
        for (idxX = outExt[0]; idxX <= outExt[1]; idxX++)
        {
          xval = (idxX % gridSpacing[0] == gridOrigin[0]);

          // not very efficient, but it gets the job done
          *outPtr++ = ((zval | yval | xval) ? lineValue : fillValue);
        }
      }
      else
      {
        for (idxX = outExt[0]; idxX <= outExt[1]; idxX++)
        {
          *outPtr++ = ((zval | yval) ? lineValue : fillValue);
        }
      }
      outPtr += outIncY;
    }
    outPtr += outIncZ;
  }
}

//----------------------------------------------------------------------------
void svtkImageGridSource::ExecuteDataWithInformation(svtkDataObject* output, svtkInformation* outInfo)
{
  svtkImageData* data = this->AllocateOutputData(output, outInfo);
  int* outExt = data->GetExtent();
  void* outPtr = data->GetScalarPointerForExtent(outExt);

  // Call the correct templated function for the output
  switch (this->GetDataScalarType())
  {
    svtkTemplateMacro(
      svtkImageGridSourceExecute(this, data, static_cast<SVTK_TT*>(outPtr), outExt, 0));
    default:
      svtkErrorMacro(<< "Execute: Unknown data type");
  }
}

//----------------------------------------------------------------------------
void svtkImageGridSource::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "GridSpacing: (" << this->GridSpacing[0] << ", " << this->GridSpacing[1] << ", "
     << this->GridSpacing[2] << ")\n";
  os << indent << "GridOrigin: (" << this->GridOrigin[0] << ", " << this->GridOrigin[1] << ", "
     << this->GridOrigin[2] << ")\n";
  os << indent << "LineValue: " << this->LineValue << "\n";
  os << indent << "FillValue: " << this->FillValue << "\n";
  os << indent << "DataScalarType: " << svtkImageScalarTypeNameMacro(this->DataScalarType) << "\n";
  os << indent << "DataExtent: (" << this->DataExtent[0] << ", " << this->DataExtent[1] << ", "
     << this->DataExtent[2] << ", " << this->DataExtent[3] << ", " << this->DataExtent[4] << ", "
     << this->DataExtent[5] << ")\n";
  os << indent << "DataSpacing: (" << this->DataSpacing[0] << ", " << this->DataSpacing[1] << ", "
     << this->DataSpacing[2] << ")\n";
  os << indent << "DataOrigin: (" << this->DataOrigin[0] << ", " << this->DataOrigin[1] << ", "
     << this->DataOrigin[2] << ")\n";
}
