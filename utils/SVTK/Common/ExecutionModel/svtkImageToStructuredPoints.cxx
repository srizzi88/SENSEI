/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageToStructuredPoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageToStructuredPoints.h"

#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkFieldData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredPoints.h"

#include <cmath>

svtkStandardNewMacro(svtkImageToStructuredPoints);

//----------------------------------------------------------------------------
svtkImageToStructuredPoints::svtkImageToStructuredPoints()
{
  this->SetNumberOfInputPorts(2);
  this->Translate[0] = this->Translate[1] = this->Translate[2] = 0;
}

//----------------------------------------------------------------------------
svtkImageToStructuredPoints::~svtkImageToStructuredPoints() = default;

//----------------------------------------------------------------------------
void svtkImageToStructuredPoints::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
svtkStructuredPoints* svtkImageToStructuredPoints::GetStructuredPointsOutput()
{
  return svtkStructuredPoints::SafeDownCast(this->GetOutputDataObject(0));
}

//----------------------------------------------------------------------------
void svtkImageToStructuredPoints::SetVectorInputData(svtkImageData* input)
{
  this->SetInputData(1, input);
}

//----------------------------------------------------------------------------
svtkImageData* svtkImageToStructuredPoints::GetVectorInput()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }

  return svtkImageData::SafeDownCast(this->GetExecutive()->GetInputData(1, 0));
}

//----------------------------------------------------------------------------
int svtkImageToStructuredPoints::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* vectorInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  int uExtent[6];
  int* wExtent;

  int idxX, idxY, idxZ;
  int maxX = 0;
  int maxY = 0;
  int maxZ = 0;
  svtkIdType inIncX, inIncY, inIncZ;
  int rowLength;
  unsigned char *inPtr1, *inPtr, *outPtr;
  svtkStructuredPoints* output =
    svtkStructuredPoints::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkImageData* data = svtkImageData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkImageData* vData = nullptr;
  if (vectorInfo)
  {
    vData = svtkImageData::SafeDownCast(vectorInfo->Get(svtkDataObject::DATA_OBJECT()));
  }

  outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), uExtent);
  output->SetExtent(uExtent);

  uExtent[0] += this->Translate[0];
  uExtent[1] += this->Translate[0];
  uExtent[2] += this->Translate[1];
  uExtent[3] += this->Translate[1];
  uExtent[4] += this->Translate[2];
  uExtent[5] += this->Translate[2];

  // if the data extent matches the update extent then just pass the data
  // otherwise we must reformat and copy the data
  if (data)
  {
    wExtent = data->GetExtent();
    if (wExtent[0] == uExtent[0] && wExtent[1] == uExtent[1] && wExtent[2] == uExtent[2] &&
      wExtent[3] == uExtent[3] && wExtent[4] == uExtent[4] && wExtent[5] == uExtent[5])
    {
      if (data->GetPointData())
      {
        output->GetPointData()->PassData(data->GetPointData());
      }
      if (data->GetCellData())
      {
        output->GetCellData()->PassData(data->GetCellData());
      }
      if (data->GetFieldData())
      {
        output->GetFieldData()->ShallowCopy(data->GetFieldData());
      }
    }
    else
    {
      inPtr = static_cast<unsigned char*>(data->GetScalarPointerForExtent(uExtent));
      outPtr = static_cast<unsigned char*>(output->GetScalarPointer());

      // Make sure there are data.
      if (!inPtr || !outPtr)
      {
        output->Initialize();
        return 1;
      }

      // Get increments to march through data
      data->GetIncrements(inIncX, inIncY, inIncZ);

      // find the region to loop over
      rowLength = (uExtent[1] - uExtent[0] + 1) * inIncX * data->GetScalarSize();
      maxX = uExtent[1] - uExtent[0];
      maxY = uExtent[3] - uExtent[2];
      maxZ = uExtent[5] - uExtent[4];
      inIncY *= data->GetScalarSize();
      inIncZ *= data->GetScalarSize();

      // Loop through output pixels
      for (idxZ = 0; idxZ <= maxZ; idxZ++)
      {
        inPtr1 = inPtr + idxZ * inIncZ;
        for (idxY = 0; idxY <= maxY; idxY++)
        {
          memcpy(outPtr, inPtr1, rowLength);
          inPtr1 += inIncY;
          outPtr += rowLength;
        }
      }
    }
  }

  if (vData)
  {
    // if the data extent matches the update extent then just pass the data
    // otherwise we must reformat and copy the data
    wExtent = vData->GetExtent();
    if (wExtent[0] == uExtent[0] && wExtent[1] == uExtent[1] && wExtent[2] == uExtent[2] &&
      wExtent[3] == uExtent[3] && wExtent[4] == uExtent[4] && wExtent[5] == uExtent[5])
    {
      output->GetPointData()->SetVectors(vData->GetPointData()->GetScalars());
    }
    else
    {
      svtkDataArray* fv = svtkDataArray::CreateDataArray(vData->GetScalarType());
      float* inPtr2 = static_cast<float*>(vData->GetScalarPointerForExtent(uExtent));

      // Make sure there are data.
      if (!inPtr2)
      {
        output->Initialize();
        return 1;
      }

      fv->SetNumberOfComponents(3);
      fv->SetNumberOfTuples((maxZ + 1) * (maxY + 1) * (maxX + 1));
      vData->GetContinuousIncrements(uExtent, inIncX, inIncY, inIncZ);
      int numComp = vData->GetNumberOfScalarComponents();
      int idx = 0;

      // Loop through output pixels
      for (idxZ = 0; idxZ <= maxZ; idxZ++)
      {
        for (idxY = 0; idxY <= maxY; idxY++)
        {
          for (idxX = 0; idxX <= maxX; idxX++)
          {
            fv->SetTuple(idx, inPtr2);
            inPtr2 += numComp;
            idx++;
          }
          inPtr2 += inIncY;
        }
        inPtr2 += inIncZ;
      }
      output->GetPointData()->SetVectors(fv);
      fv->Delete();
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
// Copy WholeExtent, Spacing and Origin.
int svtkImageToStructuredPoints::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* vInfo = inputVector[1]->GetInformationObject(0);

  int whole[6], *tmp;
  double *spacing, origin[3];

  svtkInformation* inScalarInfo = svtkDataObject::GetActiveFieldInformation(
    inInfo, svtkDataObject::FIELD_ASSOCIATION_POINTS, svtkDataSetAttributes::SCALARS);
  if (!inScalarInfo)
  {
    svtkErrorMacro("Missing scalar field on input information!");
    return 0;
  }
  svtkDataObject::SetPointDataActiveScalarInfo(outInfo,
    inScalarInfo->Get(svtkDataObject::FIELD_ARRAY_TYPE()),
    inScalarInfo->Get(svtkDataObject::FIELD_NUMBER_OF_COMPONENTS()));

  inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), whole);
  spacing = inInfo->Get(svtkDataObject::SPACING());
  inInfo->Get(svtkDataObject::ORIGIN(), origin);

  // intersections for whole extent
  if (vInfo)
  {
    tmp = vInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
    if (tmp[0] > whole[0])
    {
      whole[0] = tmp[0];
    }
    if (tmp[2] > whole[2])
    {
      whole[2] = tmp[2];
    }
    if (tmp[4] > whole[4])
    {
      whole[4] = tmp[4];
    }
    if (tmp[1] < whole[1])
    {
      whole[1] = tmp[1];
    }
    if (tmp[3] < whole[1])
    {
      whole[3] = tmp[3];
    }
    if (tmp[5] < whole[1])
    {
      whole[5] = tmp[5];
    }
  }

  // slide min extent to 0,0,0 (I Hate this !!!!)
  this->Translate[0] = whole[0];
  this->Translate[1] = whole[2];
  this->Translate[2] = whole[4];

  origin[0] += spacing[0] * whole[0];
  origin[1] += spacing[1] * whole[2];
  origin[2] += spacing[2] * whole[4];
  whole[1] -= whole[0];
  whole[3] -= whole[2];
  whole[5] -= whole[4];
  whole[0] = whole[2] = whole[4] = 0;

  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), whole, 6);
  // Now should Origin and Spacing really be part of information?
  // How about xyx arrays in RectilinearGrid of Points in StructuredGrid?
  outInfo->Set(svtkDataObject::ORIGIN(), origin, 3);
  outInfo->Set(svtkDataObject::SPACING(), spacing, 3);

  return 1;
}

//----------------------------------------------------------------------------
int svtkImageToStructuredPoints::RequestUpdateExtent(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* vInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  int ext[6];

  outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), ext);
  ext[0] += this->Translate[0];
  ext[1] += this->Translate[0];
  ext[2] += this->Translate[1];
  ext[3] += this->Translate[1];
  ext[4] += this->Translate[2];
  ext[5] += this->Translate[2];

  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), ext, 6);

  if (vInfo)
  {
    vInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), ext, 6);
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkImageToStructuredPoints::FillOutputPortInformation(int port, svtkInformation* info)
{
  if (!this->Superclass::FillOutputPortInformation(port, info))
  {
    return 0;
  }
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkStructuredPoints");
  return 1;
}

//----------------------------------------------------------------------------
int svtkImageToStructuredPoints::FillInputPortInformation(int port, svtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return 1;
}
