/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageEllipsoidSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageEllipsoidSource.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include "svtkImageData.h"

svtkStandardNewMacro(svtkImageEllipsoidSource);

//----------------------------------------------------------------------------
svtkImageEllipsoidSource::svtkImageEllipsoidSource()
{
  this->WholeExtent[0] = 0;
  this->WholeExtent[1] = 255;
  this->WholeExtent[2] = 0;
  this->WholeExtent[3] = 255;
  this->WholeExtent[4] = 0;
  this->WholeExtent[5] = 0;
  this->Center[0] = 128.0;
  this->Center[1] = 128.0;
  this->Center[2] = 0.0;
  this->Radius[0] = 70.0;
  this->Radius[1] = 70.0;
  this->Radius[2] = 70.0;
  this->InValue = 255.0;
  this->OutValue = 0.0;

  this->OutputScalarType = SVTK_UNSIGNED_CHAR;
  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
svtkImageEllipsoidSource::~svtkImageEllipsoidSource() = default;

//----------------------------------------------------------------------------
void svtkImageEllipsoidSource::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Center: (" << this->Center[0] << ", " << this->Center[1] << ", "
     << this->Center[2] << ")\n";

  os << indent << "Radius: (" << this->Radius[0] << ", " << this->Radius[1] << ", "
     << this->Radius[2] << ")\n";

  os << indent << "InValue: " << this->InValue << "\n";
  os << indent << "OutValue: " << this->OutValue << "\n";
  os << indent << "OutputScalarType: " << this->OutputScalarType << "\n";
}
//----------------------------------------------------------------------------
void svtkImageEllipsoidSource::SetWholeExtent(int extent[6])
{
  int idx;

  for (idx = 0; idx < 6; ++idx)
  {
    if (this->WholeExtent[idx] != extent[idx])
    {
      this->WholeExtent[idx] = extent[idx];
      this->Modified();
    }
  }
}

//----------------------------------------------------------------------------
void svtkImageEllipsoidSource::SetWholeExtent(
  int minX, int maxX, int minY, int maxY, int minZ, int maxZ)
{
  int extent[6];

  extent[0] = minX;
  extent[1] = maxX;
  extent[2] = minY;
  extent[3] = maxY;
  extent[4] = minZ;
  extent[5] = maxZ;
  this->SetWholeExtent(extent);
}

//----------------------------------------------------------------------------
void svtkImageEllipsoidSource::GetWholeExtent(int extent[6])
{
  int idx;

  for (idx = 0; idx < 6; ++idx)
  {
    extent[idx] = this->WholeExtent[idx];
  }
}

//----------------------------------------------------------------------------
int svtkImageEllipsoidSource::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  outInfo->Set(svtkDataObject::SPACING(), 1.0, 1.0, 1.0);
  outInfo->Set(svtkDataObject::ORIGIN(), 0.0, 0.0, 0.0);
  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), this->WholeExtent, 6);
  svtkDataObject::SetPointDataActiveScalarInfo(outInfo, this->OutputScalarType, -1);
  return 1;
}

template <class T>
void svtkImageEllipsoidSourceExecute(
  svtkImageEllipsoidSource* self, svtkImageData* data, int ext[6], T* ptr)
{
  int min0, max0;
  int idx0, idx1, idx2;
  svtkIdType inc0, inc1, inc2;
  double s0, s1, s2, temp;
  T outVal, inVal;
  double *center, *radius;
  unsigned long count = 0;
  unsigned long target;

  outVal = static_cast<T>(self->GetOutValue());
  inVal = static_cast<T>(self->GetInValue());
  center = self->GetCenter();
  radius = self->GetRadius();

  min0 = ext[0];
  max0 = ext[1];
  data->GetContinuousIncrements(ext, inc0, inc1, inc2);

  target = static_cast<unsigned long>((ext[5] - ext[4] + 1) * (ext[3] - ext[2] + 1) / 50.0);
  target++;

  for (idx2 = ext[4]; idx2 <= ext[5]; ++idx2)
  {
    // handle divide by zero
    if (radius[2] != 0.0)
    {
      temp = (static_cast<double>(idx2) - center[2]) / radius[2];
    }
    else
    {
      if (static_cast<double>(idx2) - center[2] == 0.0)
      {
        temp = 0.0;
      }
      else
      {
        temp = SVTK_DOUBLE_MAX;
      }
    }

    s2 = temp * temp;
    for (idx1 = ext[2]; !self->AbortExecute && idx1 <= ext[3]; ++idx1)
    {
      if (!(count % target))
      {
        self->UpdateProgress(count / (50.0 * target));
      }
      count++;

      // handle divide by zero
      if (radius[1] != 0.0)
      {
        temp = (static_cast<double>(idx1) - center[1]) / radius[1];
      }
      else
      {
        if (static_cast<double>(idx1) - center[1] == 0.0)
        {
          temp = 0.0;
        }
        else
        {
          temp = SVTK_DOUBLE_MAX;
        }
      }

      s1 = temp * temp;
      for (idx0 = min0; idx0 <= max0; ++idx0)
      {
        // handle divide by zero
        if (radius[0] != 0.0)
        {
          temp = (static_cast<double>(idx0) - center[0]) / radius[0];
        }
        else
        {
          if (static_cast<double>(idx0) - center[0] == 0.0)
          {
            temp = 0.0;
          }
          else
          {
            temp = SVTK_DOUBLE_MAX;
          }
        }

        s0 = temp * temp;
        if (s0 + s1 + s2 > 1.0)
        {
          *ptr = outVal;
        }
        else
        {
          *ptr = inVal;
        }
        ++ptr;
        // inc0 is 0
      }
      ptr += inc1;
    }
    ptr += inc2;
  }
}

//----------------------------------------------------------------------------
int svtkImageEllipsoidSource::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkImageData* data = svtkImageData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
  int extent[6];

  outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), extent);

  data->SetExtent(extent);
  data->AllocateScalars(outInfo);

  void* ptr;
  ptr = data->GetScalarPointerForExtent(extent);

  switch (data->GetScalarType())
  {
    svtkTemplateMacro(svtkImageEllipsoidSourceExecute(this, data, extent, static_cast<SVTK_TT*>(ptr)));
    default:
      svtkErrorMacro("Execute: Unknown output ScalarType");
  }

  return 1;
}
