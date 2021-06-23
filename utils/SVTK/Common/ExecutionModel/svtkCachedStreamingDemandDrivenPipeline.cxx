/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCachedStreamingDemandDrivenPipeline.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCachedStreamingDemandDrivenPipeline.h"

#include "svtkInformationIntegerKey.h"
#include "svtkInformationIntegerVectorKey.h"
#include "svtkObjectFactory.h"

#include "svtkAlgorithm.h"
#include "svtkAlgorithmOutput.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkPointData.h"

svtkStandardNewMacro(svtkCachedStreamingDemandDrivenPipeline);

//----------------------------------------------------------------------------
svtkCachedStreamingDemandDrivenPipeline ::svtkCachedStreamingDemandDrivenPipeline()
{
  this->CacheSize = 0;
  this->Data = nullptr;
  this->Times = nullptr;

  this->SetCacheSize(10);
}

//----------------------------------------------------------------------------
svtkCachedStreamingDemandDrivenPipeline ::~svtkCachedStreamingDemandDrivenPipeline()
{
  this->SetCacheSize(0);
}

//----------------------------------------------------------------------------
void svtkCachedStreamingDemandDrivenPipeline::SetCacheSize(int size)
{
  int idx;

  if (size == this->CacheSize)
  {
    return;
  }

  this->Modified();

  // free the old data
  for (idx = 0; idx < this->CacheSize; ++idx)
  {
    if (this->Data[idx])
    {
      this->Data[idx]->Delete();
      this->Data[idx] = nullptr;
    }
  }
  delete[] this->Data;
  this->Data = nullptr;
  delete[] this->Times;
  this->Times = nullptr;

  this->CacheSize = size;
  if (size == 0)
  {
    return;
  }

  this->Data = new svtkDataObject*[size];
  this->Times = new svtkMTimeType[size];

  for (idx = 0; idx < size; ++idx)
  {
    this->Data[idx] = nullptr;
    this->Times[idx] = 0;
  }
}

//----------------------------------------------------------------------------
void svtkCachedStreamingDemandDrivenPipeline ::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CacheSize: " << this->CacheSize << "\n";
}

//----------------------------------------------------------------------------
int svtkCachedStreamingDemandDrivenPipeline ::NeedToExecuteData(
  int outputPort, svtkInformationVector** inInfoVec, svtkInformationVector* outInfoVec)
{
  // If no port is specified, check all ports.  This behavior is
  // implemented by the superclass.
  if (outputPort < 0)
  {
    return this->Superclass::NeedToExecuteData(outputPort, inInfoVec, outInfoVec);
  }

  // Does the superclass want to execute? We must skip our direct superclass
  // because it looks at update extents but does not know about the cache
  if (this->svtkDemandDrivenPipeline::NeedToExecuteData(outputPort, inInfoVec, outInfoVec))
  {
    return 1;
  }

  // Has the algorithm asked to be executed again?
  if (this->ContinueExecuting)
  {
    return 1;
  }

  // First look through the cached data to see if it is still valid.
  int i;
  svtkMTimeType pmt = this->GetPipelineMTime();
  for (i = 0; i < this->CacheSize; ++i)
  {
    if (this->Data[i] && this->Times[i] < pmt)
    {
      this->Data[i]->Delete();
      this->Data[i] = nullptr;
      this->Times[i] = 0;
    }
  }

  // We need to check the requested update extent.  Get the output
  // port information and data information.  We do not need to check
  // existence of values because it has already been verified by
  // VerifyOutputInformation.
  svtkInformation* outInfo = outInfoVec->GetInformationObject(outputPort);
  svtkDataObject* dataObject = outInfo->Get(svtkDataObject::DATA_OBJECT());
  svtkInformation* dataInfo = dataObject->GetInformation();
  if (dataInfo->Get(svtkDataObject::DATA_EXTENT_TYPE()) == SVTK_PIECES_EXTENT)
  {
    int updatePiece = outInfo->Get(UPDATE_PIECE_NUMBER());
    int updateNumberOfPieces = outInfo->Get(UPDATE_NUMBER_OF_PIECES());
    int updateGhostLevel = outInfo->Get(UPDATE_NUMBER_OF_GHOST_LEVELS());

    // check to see if any data in the cache fits this request
    for (i = 0; i < this->CacheSize; ++i)
    {
      if (this->Data[i])
      {
        dataInfo = this->Data[i]->GetInformation();

        // Check the unstructured extent.  If we do not have the requested
        // piece, we need to execute.
        int dataPiece = dataInfo->Get(svtkDataObject::DATA_PIECE_NUMBER());
        int dataNumberOfPieces = dataInfo->Get(svtkDataObject::DATA_NUMBER_OF_PIECES());
        int dataGhostLevel = dataInfo->Get(svtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS());
        if (dataInfo->Get(svtkDataObject::DATA_EXTENT_TYPE()) == SVTK_PIECES_EXTENT &&
          dataPiece == updatePiece && dataNumberOfPieces == updateNumberOfPieces &&
          dataGhostLevel == updateGhostLevel)
        {
          // we have a matching data we must copy it to our output, but for
          // now we don't support polydata
          return 1;
        }
      }
    }
  }
  else if (dataInfo->Get(svtkDataObject::DATA_EXTENT_TYPE()) == SVTK_3D_EXTENT)
  {
    // Check the structured extent.  If the update extent is outside
    // of the extent and not empty, we need to execute.
    int dataExtent[6];
    int updateExtent[6];
    outInfo->Get(UPDATE_EXTENT(), updateExtent);

    // check to see if any data in the cache fits this request
    for (i = 0; i < this->CacheSize; ++i)
    {
      if (this->Data[i])
      {
        dataInfo = this->Data[i]->GetInformation();
        dataInfo->Get(svtkDataObject::DATA_EXTENT(), dataExtent);
        if (dataInfo->Get(svtkDataObject::DATA_EXTENT_TYPE()) == SVTK_3D_EXTENT &&
          !(updateExtent[0] < dataExtent[0] || updateExtent[1] > dataExtent[1] ||
            updateExtent[2] < dataExtent[2] || updateExtent[3] > dataExtent[3] ||
            updateExtent[4] < dataExtent[4] || updateExtent[5] > dataExtent[5]) &&
          (updateExtent[0] <= updateExtent[1] && updateExtent[2] <= updateExtent[3] &&
            updateExtent[4] <= updateExtent[5]))
        {
          // we have a match
          // Pass this data to output.
          svtkImageData* id = svtkImageData::SafeDownCast(dataObject);
          svtkImageData* id2 = svtkImageData::SafeDownCast(this->Data[i]);
          if (id && id2)
          {
            id->SetExtent(dataExtent);
            id->GetPointData()->PassData(id2->GetPointData());
            // not sure if we need this
            dataObject->DataHasBeenGenerated();
            return 0;
          }
        }
      }
    }
  }

  // We do need to execute
  return 1;
}

//----------------------------------------------------------------------------
int svtkCachedStreamingDemandDrivenPipeline ::ExecuteData(
  svtkInformation* request, svtkInformationVector** inInfoVec, svtkInformationVector* outInfoVec)
{
  // only works for one in one out algorithms
  if (request->Get(FROM_OUTPUT_PORT()) != 0)
  {
    svtkErrorMacro("svtkCachedStreamingDemandDrivenPipeline can only be used for algorithms with one "
                  "output and one input");
    return 0;
  }

  // first do the usual thing
  int result = this->Superclass::ExecuteData(request, inInfoVec, outInfoVec);

  // then save the newly generated data
  svtkMTimeType bestTime = SVTK_INT_MAX;
  int bestIdx = 0;

  // Save the image in cache.
  // Find a spot to put the data.
  for (int i = 0; i < this->CacheSize; ++i)
  {
    if (this->Data[i] == nullptr)
    {
      bestIdx = i;
      break;
    }
    if (this->Times[i] < bestTime)
    {
      bestIdx = i;
      bestTime = this->Times[i];
    }
  }

  svtkInformation* outInfo = outInfoVec->GetInformationObject(0);
  svtkDataObject* dataObject = outInfo->Get(svtkDataObject::DATA_OBJECT());
  if (this->Data[bestIdx] == nullptr)
  {
    this->Data[bestIdx] = dataObject->NewInstance();
  }
  this->Data[bestIdx]->ReleaseData();

  svtkImageData* id = svtkImageData::SafeDownCast(dataObject);
  if (id)
  {
    svtkInformation* inInfo = inInfoVec[0]->GetInformationObject(0);
    svtkImageData* input = svtkImageData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
    id->SetExtent(input->GetExtent());
    id->GetPointData()->PassData(input->GetPointData());
    id->DataHasBeenGenerated();
  }

  svtkImageData* id2 = svtkImageData::SafeDownCast(this->Data[bestIdx]);
  if (id && id2)
  {
    id2->SetExtent(id->GetExtent());
    id2->GetPointData()->SetScalars(id->GetPointData()->GetScalars());
  }

  this->Times[bestIdx] = dataObject->GetUpdateTime();

  return result;
}
