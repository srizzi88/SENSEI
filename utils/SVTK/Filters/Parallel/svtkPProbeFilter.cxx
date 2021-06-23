/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPProbeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPProbeFilter.h"

#include "svtkCellData.h"
#include "svtkCharArray.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkPProbeFilter);

svtkCxxSetObjectMacro(svtkPProbeFilter, Controller, svtkMultiProcessController);

//----------------------------------------------------------------------------
svtkPProbeFilter::svtkPProbeFilter()
{
  this->Controller = nullptr;
  this->SetController(svtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
svtkPProbeFilter::~svtkPProbeFilter()
{
  this->SetController(nullptr);
}

//----------------------------------------------------------------------------
int svtkPProbeFilter::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  if (!this->Superclass::RequestData(request, inputVector, outputVector))
  {
    return 0;
  }

  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkDataSet* output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  int procid = 0;
  int numProcs = 1;
  if (this->Controller)
  {
    procid = this->Controller->GetLocalProcessId();
    numProcs = this->Controller->GetNumberOfProcesses();
  }

  svtkIdType numPoints = this->GetValidPoints()->GetNumberOfTuples();
  if (procid)
  {
    // Satellite node
    this->Controller->Send(&numPoints, 1, 0, PROBE_COMMUNICATION_TAG);
    if (numPoints > 0)
    {
      this->Controller->Send(output, 0, PROBE_COMMUNICATION_TAG);
    }
    output->ReleaseData();
  }
  else if (numProcs > 1)
  {
    svtkIdType numRemoteValidPoints = 0;
    svtkDataSet* remoteProbeOutput = output->NewInstance();
    svtkPointData* remotePointData;
    svtkPointData* pointData = output->GetPointData();
    svtkIdType i;
    svtkIdType k;
    svtkIdType pointId;
    for (i = 1; i < numProcs; i++)
    {
      this->Controller->Receive(&numRemoteValidPoints, 1, i, PROBE_COMMUNICATION_TAG);
      if (numRemoteValidPoints > 0)
      {
        this->Controller->Receive(remoteProbeOutput, i, PROBE_COMMUNICATION_TAG);

        remotePointData = remoteProbeOutput->GetPointData();

        svtkCharArray* maskArray =
          svtkArrayDownCast<svtkCharArray>(remotePointData->GetArray(this->ValidPointMaskArrayName));

        // Iterate over all point data in the output gathered from the remove
        // and copy array values from all the pointIds which have the mask array
        // bit set to 1.
        svtkIdType numRemotePoints = remoteProbeOutput->GetNumberOfPoints();
        if (output->GetNumberOfCells() != remoteProbeOutput->GetNumberOfCells() ||
          output->GetNumberOfPoints() != remoteProbeOutput->GetNumberOfPoints())
        {
          svtkErrorMacro("svtkPProbeFilter assumes the whole geometry dataset "
                        "(which determines positions to probe) is available "
                        "on all nodes, however nodes 0 is different than node "
            << i);
        }
        else if (maskArray)
        {
          for (pointId = 0; pointId < numRemotePoints; ++pointId)
          {
            if (maskArray->GetValue(pointId) == 1)
            {
              for (k = 0; k < pointData->GetNumberOfArrays(); ++k)
              {
                svtkAbstractArray* oaa = pointData->GetArray(k);
                svtkAbstractArray* raa = remotePointData->GetArray(oaa->GetName());
                if (raa != nullptr)
                {
                  oaa->SetTuple(pointId, pointId, raa);
                }
              }
            }
          }
        }
      }
    }
    remoteProbeOutput->Delete();
  }

  return 1;
}

#include "svtkInformationIntegerVectorKey.h"
//----------------------------------------------------------------------------
int svtkPProbeFilter::RequestUpdateExtent(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);

  // inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
  // inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
  // inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
  //            0);
  // If structured data, we want the whole extent. This is necessary because
  // the pipeline will copy the update extent from the output to all inputs.
  sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
    sourceInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);
  // Then we want the same as output pieces.
  sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
    outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
    outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
    outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));

  return 1;
}

//----------------------------------------------------------------------------
int svtkPProbeFilter::FillInputPortInformation(int port, svtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }

  if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataObject");
  }
  return 1;
}

//----------------------------------------------------------------------------
void svtkPProbeFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller " << this->Controller << endl;
}
