/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkStreamingDemandDrivenPipeline.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkStreamingDemandDrivenPipeline.h"

#include "svtkAlgorithm.h"
#include "svtkAlgorithmOutput.h"
#include "svtkDataObject.h"
#include "svtkDataSet.h"
#include "svtkDataSetAttributes.h"
#include "svtkExtentTranslator.h"
#include "svtkInformation.h"
#include "svtkInformationDoubleKey.h"
#include "svtkInformationDoubleVectorKey.h"
#include "svtkInformationIdTypeKey.h"
#include "svtkInformationInformationVectorKey.h"
#include "svtkInformationIntegerKey.h"
#include "svtkInformationIntegerRequestKey.h"
#include "svtkInformationIntegerVectorKey.h"
#include "svtkInformationIterator.h"
#include "svtkInformationObjectBaseKey.h"
#include "svtkInformationRequestKey.h"
#include "svtkInformationStringKey.h"
#include "svtkInformationUnsignedLongKey.h"
#include "svtkInformationVector.h"
#include "svtkLogger.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"

svtkStandardNewMacro(svtkStreamingDemandDrivenPipeline);

svtkInformationKeyMacro(svtkStreamingDemandDrivenPipeline, CONTINUE_EXECUTING, Integer);
svtkInformationKeyMacro(svtkStreamingDemandDrivenPipeline, EXACT_EXTENT, Integer);
svtkInformationKeyMacro(svtkStreamingDemandDrivenPipeline, REQUEST_UPDATE_EXTENT, Request);
svtkInformationKeyMacro(svtkStreamingDemandDrivenPipeline, REQUEST_UPDATE_TIME, Request);
svtkInformationKeyMacro(
  svtkStreamingDemandDrivenPipeline, REQUEST_TIME_DEPENDENT_INFORMATION, Request);
svtkInformationKeyMacro(svtkStreamingDemandDrivenPipeline, UPDATE_EXTENT_INITIALIZED, Integer);
svtkInformationKeyMacro(svtkStreamingDemandDrivenPipeline, UPDATE_PIECE_NUMBER, Integer);
svtkInformationKeyMacro(svtkStreamingDemandDrivenPipeline, UPDATE_NUMBER_OF_PIECES, Integer);
svtkInformationKeyMacro(svtkStreamingDemandDrivenPipeline, UPDATE_NUMBER_OF_GHOST_LEVELS, Integer);
svtkInformationKeyRestrictedMacro(svtkStreamingDemandDrivenPipeline, WHOLE_EXTENT, IntegerVector, 6);
svtkInformationKeyRestrictedMacro(svtkStreamingDemandDrivenPipeline, UPDATE_EXTENT, IntegerVector, 6);
svtkInformationKeyRestrictedMacro(
  svtkStreamingDemandDrivenPipeline, COMBINED_UPDATE_EXTENT, IntegerVector, 6);
svtkInformationKeyMacro(svtkStreamingDemandDrivenPipeline, UNRESTRICTED_UPDATE_EXTENT, Integer);
svtkInformationKeyMacro(svtkStreamingDemandDrivenPipeline, TIME_STEPS, DoubleVector);
svtkInformationKeyMacro(svtkStreamingDemandDrivenPipeline, UPDATE_TIME_STEP, Double);

svtkInformationKeyMacro(svtkStreamingDemandDrivenPipeline, PREVIOUS_UPDATE_TIME_STEP, Double);
svtkInformationKeyMacro(svtkStreamingDemandDrivenPipeline, TIME_RANGE, DoubleVector);

svtkInformationKeyMacro(svtkStreamingDemandDrivenPipeline, BOUNDS, DoubleVector);
svtkInformationKeyMacro(svtkStreamingDemandDrivenPipeline, TIME_DEPENDENT_INFORMATION, Integer);

//----------------------------------------------------------------------------
class svtkStreamingDemandDrivenPipelineToDataObjectFriendship
{
public:
  static void Crop(svtkDataObject* obj, const int* extent) { obj->Crop(extent); }
};

namespace
{
void svtkSDDPSetUpdateExtentToWholeExtent(svtkInformation* info)
{
  typedef svtkStreamingDemandDrivenPipeline svtkSDDP;
  info->Set(svtkSDDP::UPDATE_PIECE_NUMBER(), 0);
  info->Set(svtkSDDP::UPDATE_NUMBER_OF_PIECES(), 1);
  info->Set(svtkSDDP::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
  if (info->Has(svtkSDDP::WHOLE_EXTENT()))
  {
    int extent[6] = { 0, -1, 0, -1, 0, -1 };
    info->Get(svtkSDDP::WHOLE_EXTENT(), extent);
    info->Set(svtkSDDP::UPDATE_EXTENT(), extent, 6);
  }
}
}

//----------------------------------------------------------------------------
svtkStreamingDemandDrivenPipeline::svtkStreamingDemandDrivenPipeline()
{
  this->ContinueExecuting = 0;
  this->UpdateExtentRequest = nullptr;
  this->UpdateTimeRequest = nullptr;
  this->TimeDependentInformationRequest = nullptr;
  this->InformationIterator = svtkInformationIterator::New();

  this->LastPropogateUpdateExtentShortCircuited = 0;
}

//----------------------------------------------------------------------------
svtkStreamingDemandDrivenPipeline::~svtkStreamingDemandDrivenPipeline()
{
  if (this->UpdateExtentRequest)
  {
    this->UpdateExtentRequest->Delete();
  }
  if (this->UpdateTimeRequest)
  {
    this->UpdateTimeRequest->Delete();
  }
  if (this->TimeDependentInformationRequest)
  {
    this->TimeDependentInformationRequest->Delete();
  }
  this->InformationIterator->Delete();
}

//----------------------------------------------------------------------------
void svtkStreamingDemandDrivenPipeline::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
svtkTypeBool svtkStreamingDemandDrivenPipeline ::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inInfoVec, svtkInformationVector* outInfoVec)
{
  // The algorithm should not invoke anything on the executive.
  if (!this->CheckAlgorithm("ProcessRequest", request))
  {
    return 0;
  }

  // Look for specially supported requests.
  if (request->Has(REQUEST_UPDATE_TIME()))
  {
    int result = 1;
    int outputPort = -1;
    if (request->Has(FROM_OUTPUT_PORT()))
    {
      outputPort = request->Get(FROM_OUTPUT_PORT());
    }

    int N2E = this->Superclass::NeedToExecuteData(outputPort, inInfoVec, outInfoVec);
    if (!N2E && outputPort >= 0)
    {
      svtkInformation* outInfo = outInfoVec->GetInformationObject(outputPort);
      svtkDataObject* dataObject = outInfo->Get(svtkDataObject::DATA_OBJECT());
      if (outInfo->Has(TIME_DEPENDENT_INFORMATION()))
      {
        N2E = this->NeedToExecuteBasedOnTime(outInfo, dataObject);
      }
      else
      {
        N2E = 0;
      }
    }
    if (N2E)
    {
      svtkLogF(TRACE, "%s execute-update-time", svtkLogIdentifier(this->Algorithm));
      result = this->CallAlgorithm(request, svtkExecutive::RequestUpstream, inInfoVec, outInfoVec);
      // Propagate the update extent to all inputs.
      if (result)
      {
        result = this->ForwardUpstream(request);
      }
      result = 1;
    }
    return result;
  }

  // Look for specially supported requests.
  if (request->Has(REQUEST_TIME_DEPENDENT_INFORMATION()))
  {
    int result = 1;
    int outputPort = -1;
    if (request->Has(FROM_OUTPUT_PORT()))
    {
      outputPort = request->Get(FROM_OUTPUT_PORT());
    }

    int N2E = this->Superclass::NeedToExecuteData(outputPort, inInfoVec, outInfoVec);
    if (!N2E && outputPort >= 0)
    {
      svtkInformation* outInfo = outInfoVec->GetInformationObject(outputPort);
      svtkDataObject* dataObject = outInfo->Get(svtkDataObject::DATA_OBJECT());
      if (outInfo->Has(TIME_DEPENDENT_INFORMATION()))
      {
        N2E = this->NeedToExecuteBasedOnTime(outInfo, dataObject);
      }
      else
      {
        N2E = 0;
      }
    }
    if (N2E)
    {
      if (!this->ForwardUpstream(request))
      {
        return 0;
      }
      svtkLogF(TRACE, "%s execute-time-dependent-information", svtkLogIdentifier(this->Algorithm));
      result = this->CallAlgorithm(request, svtkExecutive::RequestUpstream, inInfoVec, outInfoVec);
    }
    return result;
  }

  if (request->Has(REQUEST_UPDATE_EXTENT()))
  {
    // Get the output port from which the request was made.
    this->LastPropogateUpdateExtentShortCircuited = 1;
    int outputPort = -1;
    if (request->Has(FROM_OUTPUT_PORT()))
    {
      outputPort = request->Get(FROM_OUTPUT_PORT());
    }

    // Make sure the information on the output port is valid.
    if (!this->VerifyOutputInformation(outputPort, inInfoVec, outInfoVec))
    {
      return 0;
    }

    // Get the output info
    svtkInformation* outInfo = nullptr;
    if (outputPort > -1)
    {
      outInfo = outInfoVec->GetInformationObject(outputPort);
    }

    // Combine the requested extent into COMBINED_UPDATE_EXTENT,
    // but only do so if the UPDATE_EXTENT key exists and if the
    // UPDATE_EXTENT is not an empty extent
    int* updateExtent = nullptr;
    if (outInfo && (updateExtent = outInfo->Get(UPDATE_EXTENT())) != nullptr)
    {
      // Downstream algorithms can set UPDATE_EXTENT_INITIALIZED to
      // REPLACE if they do not want to combine with previous extents
      if (outInfo->Get(UPDATE_EXTENT_INITIALIZED()) != SVTK_UPDATE_EXTENT_REPLACE)
      {
        int* combinedExtent = outInfo->Get(COMBINED_UPDATE_EXTENT());
        if (combinedExtent && combinedExtent[0] <= combinedExtent[1] &&
          combinedExtent[2] <= combinedExtent[3] && combinedExtent[4] <= combinedExtent[5])
        {
          if (updateExtent[0] <= updateExtent[1] && updateExtent[2] <= updateExtent[3] &&
            updateExtent[4] <= updateExtent[5])
          {
            int newExtent[6];
            for (int ii = 0; ii < 6; ii += 2)
            {
              newExtent[ii] = combinedExtent[ii];
              if (updateExtent[ii] < newExtent[ii])
              {
                newExtent[ii] = updateExtent[ii];
              }
              newExtent[ii + 1] = combinedExtent[ii + 1];
              if (updateExtent[ii + 1] > newExtent[ii + 1])
              {
                newExtent[ii + 1] = updateExtent[ii + 1];
              }
            }
            outInfo->Set(COMBINED_UPDATE_EXTENT(), newExtent, 6);
            outInfo->Set(UPDATE_EXTENT(), newExtent, 6);
          }
          else
          {
            outInfo->Set(UPDATE_EXTENT(), combinedExtent, 6);
          }
        }
        else
        {
          outInfo->Set(COMBINED_UPDATE_EXTENT(), updateExtent, 6);
        }
      }
    }

    // If we need to execute, propagate the update extent.
    int result = 1;
    int N2E = this->NeedToExecuteData(outputPort, inInfoVec, outInfoVec);
    if (!N2E && outInfo && this->GetNumberOfInputPorts() &&
      inInfoVec[0]->GetNumberOfInformationObjects() > 0)
    {
      svtkInformation* inInfo = inInfoVec[0]->GetInformationObject(0);
      int outNumberOfPieces = outInfo->Get(UPDATE_NUMBER_OF_PIECES());
      int inNumberOfPieces = inInfo->Get(UPDATE_NUMBER_OF_PIECES());
      if (inNumberOfPieces != outNumberOfPieces)
      {
        N2E = 1;
      }
      else
      {
        if (outNumberOfPieces != 1)
        {
          int outPiece = outInfo->Get(UPDATE_PIECE_NUMBER());
          int inPiece = inInfo->Get(UPDATE_PIECE_NUMBER());
          if (inPiece != outPiece)
          {
            N2E = 1;
          }
        }
      }
    }
    if (N2E)
    {
      // Make sure input types are valid before algorithm does anything.
      if (!this->InputCountIsValid(inInfoVec) || !this->InputTypeIsValid(inInfoVec))
      {
        result = 0;
      }
      else
      {
        // Invoke the request on the algorithm.
        this->LastPropogateUpdateExtentShortCircuited = 0;
        svtkLogF(TRACE, "%s execute-update-extent", svtkLogIdentifier(this->Algorithm));
        result = this->CallAlgorithm(request, svtkExecutive::RequestUpstream, inInfoVec, outInfoVec);

        // Propagate the update extent to all inputs.
        if (result)
        {
          result = this->ForwardUpstream(request);
        }
        result = 1;
      }
    }
    if (!N2E)
    {
      if (outInfo && outInfo->Has(COMBINED_UPDATE_EXTENT()))
      {
        static int emptyExt[6] = { 0, -1, 0, -1, 0, -1 };
        outInfo->Set(COMBINED_UPDATE_EXTENT(), emptyExt, 6);
      }
    }
    return result;
  }

  if (request->Has(REQUEST_DATA()))
  {
    // Let the superclass handle the request first.
    if (this->Superclass::ProcessRequest(request, inInfoVec, outInfoVec))
    {
      for (int i = 0; i < outInfoVec->GetNumberOfInformationObjects(); ++i)
      {
        svtkInformation* info = outInfoVec->GetInformationObject(i);
        // Crop the output if the exact extent flag is set.
        if (info->Has(EXACT_EXTENT()) && info->Get(EXACT_EXTENT()))
        {
          svtkDataObject* data = info->Get(svtkDataObject::DATA_OBJECT());
          svtkStreamingDemandDrivenPipelineToDataObjectFriendship::Crop(
            data, info->Get(UPDATE_EXTENT()));
        }
        // Clear combined update extent, since the update cycle has completed
        if (info->Has(COMBINED_UPDATE_EXTENT()))
        {
          static int emptyExt[6] = { 0, -1, 0, -1, 0, -1 };
          info->Set(COMBINED_UPDATE_EXTENT(), emptyExt, 6);
        }
      }
      return 1;
    }
    return 0;
  }

  // Let the superclass handle other requests.
  return this->Superclass::ProcessRequest(request, inInfoVec, outInfoVec);
}

//----------------------------------------------------------------------------
svtkTypeBool svtkStreamingDemandDrivenPipeline::Update()
{
  return this->Superclass::Update();
}

//----------------------------------------------------------------------------
svtkTypeBool svtkStreamingDemandDrivenPipeline::Update(int port)
{
  return this->Update(port, nullptr);
}

//----------------------------------------------------------------------------
svtkTypeBool svtkStreamingDemandDrivenPipeline::Update(int port, svtkInformationVector* requests)
{
  if (!this->UpdateInformation())
  {
    return 0;
  }
  int numPorts = this->Algorithm->GetNumberOfOutputPorts();
  if (requests)
  {
    svtkInformationVector* outInfoVec = this->GetOutputInformation();
    for (int i = 0; i < numPorts; i++)
    {
      svtkInformation* outInfo = outInfoVec->GetInformationObject(i);
      svtkInformation* req = requests->GetInformationObject(i);
      if (outInfo && req)
      {
        outInfo->Append(req);
      }
    }
  }

  if (port >= -1 && port < numPorts)
  {
    int retval = 1;
    // some streaming filters can request that the pipeline execute multiple
    // times for a single update
    do
    {
      this->PropagateTime(port);
      this->UpdateTimeDependentInformation(port);
      retval = retval && this->PropagateUpdateExtent(port);
      if (retval && !this->LastPropogateUpdateExtentShortCircuited)
      {
        retval = retval && this->UpdateData(port);
      }
    } while (this->ContinueExecuting);
    return retval;
  }
  else
  {
    return 1;
  }
}

//----------------------------------------------------------------------------
svtkTypeBool svtkStreamingDemandDrivenPipeline::UpdateWholeExtent()
{
  this->UpdateInformation();
  // if we have an output then set the UE to WE for it
  if (this->Algorithm->GetNumberOfOutputPorts())
  {
    svtkSDDPSetUpdateExtentToWholeExtent(this->GetOutputInformation()->GetInformationObject(0));
  }
  // otherwise do it for the inputs
  else
  {
    // Loop over all input ports.
    for (int i = 0; i < this->Algorithm->GetNumberOfInputPorts(); ++i)
    {
      // Loop over all connections on this input port.
      int numInConnections = this->Algorithm->GetNumberOfInputConnections(i);
      for (int j = 0; j < numInConnections; j++)
      {
        // Get the pipeline information for this input connection.
        svtkInformation* inInfo = this->GetInputInformation(i, j);
        svtkSDDPSetUpdateExtentToWholeExtent(inInfo);
      }
    }
  }
  return this->Update();
}

//----------------------------------------------------------------------------
int svtkStreamingDemandDrivenPipeline ::ExecuteInformation(
  svtkInformation* request, svtkInformationVector** inInfoVec, svtkInformationVector* outInfoVec)
{
  // Let the superclass make the request to the algorithm.
  if (this->Superclass::ExecuteInformation(request, inInfoVec, outInfoVec))
  {
    for (int i = 0; i < this->Algorithm->GetNumberOfOutputPorts(); ++i)
    {
      svtkInformation* info = outInfoVec->GetInformationObject(i);
      svtkDataObject* data = info->Get(svtkDataObject::DATA_OBJECT());

      if (!data)
      {
        return 0;
      }

      if (data->GetExtentType() == SVTK_3D_EXTENT)
      {
        if (!info->Has(WHOLE_EXTENT()))
        {
          int extent[6] = { 0, -1, 0, -1, 0, -1 };
          info->Set(WHOLE_EXTENT(), extent, 6);
        }
      }

      // Make sure an update request exists.
      // Request all data by default.
      svtkSDDPSetUpdateExtentToWholeExtent(outInfoVec->GetInformationObject(i));
    }
    return 1;
  }
  else
  {
    return 0;
  }
}

//----------------------------------------------------------------------------
void svtkStreamingDemandDrivenPipeline ::CopyDefaultInformation(svtkInformation* request,
  int direction, svtkInformationVector** inInfoVec, svtkInformationVector* outInfoVec)
{
  // Let the superclass copy first.
  this->Superclass::CopyDefaultInformation(request, direction, inInfoVec, outInfoVec);

  if (request->Has(REQUEST_INFORMATION()))
  {
    if (this->GetNumberOfInputPorts() > 0)
    {
      if (svtkInformation* inInfo = inInfoVec[0]->GetInformationObject(0))
      {
        svtkInformation* scalarInfo = svtkDataObject::GetActiveFieldInformation(
          inInfo, svtkDataObject::FIELD_ASSOCIATION_POINTS, svtkDataSetAttributes::SCALARS);
        // Copy information from the first input to all outputs.
        for (int i = 0; i < outInfoVec->GetNumberOfInformationObjects(); ++i)
        {
          svtkInformation* outInfo = outInfoVec->GetInformationObject(i);
          outInfo->CopyEntry(inInfo, WHOLE_EXTENT());
          outInfo->CopyEntry(inInfo, TIME_STEPS());
          outInfo->CopyEntry(inInfo, TIME_RANGE());
          outInfo->CopyEntry(inInfo, svtkDataObject::ORIGIN());
          outInfo->CopyEntry(inInfo, svtkDataObject::SPACING());
          outInfo->CopyEntry(inInfo, TIME_DEPENDENT_INFORMATION());
          if (scalarInfo)
          {
            int scalarType = SVTK_DOUBLE;
            if (scalarInfo->Has(svtkDataObject::FIELD_ARRAY_TYPE()))
            {
              scalarType = scalarInfo->Get(svtkDataObject::FIELD_ARRAY_TYPE());
            }
            int numComp = 1;
            if (scalarInfo->Has(svtkDataObject::FIELD_NUMBER_OF_COMPONENTS()))
            {
              numComp = scalarInfo->Get(svtkDataObject::FIELD_NUMBER_OF_COMPONENTS());
            }
            svtkDataObject::SetPointDataActiveScalarInfo(outInfo, scalarType, numComp);
          }
        }
      }
    }
  }

  if (request->Has(REQUEST_UPDATE_TIME()))
  {
    // Get the output port from which to copy the extent.
    int outputPort = -1;
    if (request->Has(FROM_OUTPUT_PORT()))
    {
      outputPort = request->Get(FROM_OUTPUT_PORT());
    }

    // Setup default information for the inputs.
    if (outInfoVec->GetNumberOfInformationObjects() > 0)
    {
      // Copy information from the output port that made the request.
      // Since VerifyOutputInformation has already been called we know
      // there is output information with a data object.
      svtkInformation* outInfo =
        outInfoVec->GetInformationObject((outputPort >= 0) ? outputPort : 0);

      // Loop over all input ports.
      for (int i = 0; i < this->Algorithm->GetNumberOfInputPorts(); ++i)
      {
        // Loop over all connections on this input port.
        int numInConnections = inInfoVec[i]->GetNumberOfInformationObjects();
        for (int j = 0; j < numInConnections; j++)
        {
          // Get the pipeline information for this input connection.
          svtkInformation* inInfo = inInfoVec[i]->GetInformationObject(j);

          // Copy the time request
          if (outInfo->Has(UPDATE_TIME_STEP()))
          {
            inInfo->CopyEntry(outInfo, UPDATE_TIME_STEP());
          }
        }
      }
    }
  }
  if (request->Has(REQUEST_UPDATE_EXTENT()))
  {
    // Get the output port from which to copy the extent.
    int outputPort = -1;
    if (request->Has(FROM_OUTPUT_PORT()))
    {
      outputPort = request->Get(FROM_OUTPUT_PORT());
    }

    // Initialize input extent to whole extent if it is not
    // already initialized.
    // This may be overwritten by the default code below as
    // well as what that an algorithm may do.
    for (int i = 0; i < this->Algorithm->GetNumberOfInputPorts(); ++i)
    {
      // Loop over all connections on this input port.
      int numInConnections = inInfoVec[i]->GetNumberOfInformationObjects();
      for (int j = 0; j < numInConnections; j++)
      {
        svtkInformation* inInfo = inInfoVec[i]->GetInformationObject(j);
        svtkSDDPSetUpdateExtentToWholeExtent(inInfo);
      }
    }

    // Setup default information for the inputs.
    if (outInfoVec->GetNumberOfInformationObjects() > 0)
    {
      // Copy information from the output port that made the request.
      // Since VerifyOutputInformation has already been called we know
      // there is output information with a data object.
      svtkInformation* outInfo =
        outInfoVec->GetInformationObject((outputPort >= 0) ? outputPort : 0);

      // Loop over all input ports.
      for (int i = 0; i < this->Algorithm->GetNumberOfInputPorts(); ++i)
      {
        // Loop over all connections on this input port.
        int numInConnections = inInfoVec[i]->GetNumberOfInformationObjects();
        for (int j = 0; j < numInConnections; j++)
        {
          // Get the pipeline information for this input connection.
          svtkInformation* inInfo = inInfoVec[i]->GetInformationObject(j);

          // Copy the time request
          if (outInfo->Has(UPDATE_TIME_STEP()))
          {
            inInfo->CopyEntry(outInfo, UPDATE_TIME_STEP());
          }

          // If an algorithm wants an exact extent it must explicitly
          // add it to the request.  We do not want to get the setting
          // from another consumer of the same input.
          inInfo->Remove(EXACT_EXTENT());

          // Get the input data object for this connection.  It should
          // have already been created by the UpdateDataObject pass.
          svtkDataObject* inData = inInfo->Get(svtkDataObject::DATA_OBJECT());
          if (!inData)
          {
            svtkErrorMacro("Cannot copy default update request from output port "
              << outputPort << " on algorithm " << this->Algorithm->GetClassName() << "("
              << this->Algorithm << ") to input connection " << j << " on input port " << i
              << " because there is no data object.");
            continue;
          }

          if (outInfo->Has(UPDATE_EXTENT()))
          {
            inInfo->CopyEntry(outInfo, UPDATE_EXTENT());
          }

          inInfo->CopyEntry(outInfo, UPDATE_PIECE_NUMBER());
          inInfo->CopyEntry(outInfo, UPDATE_NUMBER_OF_PIECES());
          inInfo->CopyEntry(outInfo, UPDATE_NUMBER_OF_GHOST_LEVELS());

          inInfo->CopyEntry(outInfo, UPDATE_EXTENT_INITIALIZED());
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
void svtkStreamingDemandDrivenPipeline ::ResetPipelineInformation(int port, svtkInformation* info)
{
  this->Superclass::ResetPipelineInformation(port, info);
  info->Remove(WHOLE_EXTENT());
  info->Remove(EXACT_EXTENT());
  info->Remove(UPDATE_EXTENT_INITIALIZED());
  info->Remove(UPDATE_EXTENT());
  info->Remove(UPDATE_PIECE_NUMBER());
  info->Remove(UPDATE_NUMBER_OF_PIECES());
  info->Remove(UPDATE_NUMBER_OF_GHOST_LEVELS());
  info->Remove(TIME_STEPS());
  info->Remove(TIME_RANGE());
  info->Remove(UPDATE_TIME_STEP());
  info->Remove(PREVIOUS_UPDATE_TIME_STEP());
  info->Remove(svtkAlgorithm::CAN_HANDLE_PIECE_REQUEST());
  info->Remove(svtkAlgorithm::CAN_PRODUCE_SUB_EXTENT());
}

//----------------------------------------------------------------------------
int svtkStreamingDemandDrivenPipeline::PropagateUpdateExtent(int outputPort)
{
  // The algorithm should not invoke anything on the executive.
  if (!this->CheckAlgorithm("PropagateUpdateExtent", nullptr))
  {
    return 0;
  }

  // Range check.
  if (outputPort < -1 || outputPort >= this->Algorithm->GetNumberOfOutputPorts())
  {
    svtkErrorMacro("PropagateUpdateExtent given output port index "
      << outputPort << " on an algorithm with " << this->Algorithm->GetNumberOfOutputPorts()
      << " output ports.");
    return 0;
  }

  // Setup the request for update extent propagation.
  if (!this->UpdateExtentRequest)
  {
    this->UpdateExtentRequest = svtkInformation::New();
    this->UpdateExtentRequest->Set(REQUEST_UPDATE_EXTENT());
    // The request is forwarded upstream through the pipeline.
    this->UpdateExtentRequest->Set(
      svtkExecutive::FORWARD_DIRECTION(), svtkExecutive::RequestUpstream);
    // Algorithms process this request before it is forwarded.
    this->UpdateExtentRequest->Set(svtkExecutive::ALGORITHM_BEFORE_FORWARD(), 1);
  }

  this->UpdateExtentRequest->Set(FROM_OUTPUT_PORT(), outputPort);

  // Send the request.
  return this->ProcessRequest(
    this->UpdateExtentRequest, this->GetInputInformation(), this->GetOutputInformation());
}

//----------------------------------------------------------------------------
int svtkStreamingDemandDrivenPipeline::PropagateTime(int outputPort)
{
  // The algorithm should not invoke anything on the executive.
  if (!this->CheckAlgorithm("PropagateTime", nullptr))
  {
    return 0;
  }

  // Range check.
  if (outputPort < -1 || outputPort >= this->Algorithm->GetNumberOfOutputPorts())
  {
    svtkErrorMacro("PropagateUpdateTime given output port index "
      << outputPort << " on an algorithm with " << this->Algorithm->GetNumberOfOutputPorts()
      << " output ports.");
    return 0;
  }

  // Setup the request for update extent propagation.
  if (!this->UpdateTimeRequest)
  {
    this->UpdateTimeRequest = svtkInformation::New();
    this->UpdateTimeRequest->Set(REQUEST_UPDATE_TIME());
    // The request is forwarded upstream through the pipeline.
    this->UpdateTimeRequest->Set(svtkExecutive::FORWARD_DIRECTION(), svtkExecutive::RequestUpstream);
    // Algorithms process this request before it is forwarded.
    this->UpdateTimeRequest->Set(svtkExecutive::ALGORITHM_BEFORE_FORWARD(), 1);
  }

  this->UpdateTimeRequest->Set(FROM_OUTPUT_PORT(), outputPort);

  // Send the request.
  return this->ProcessRequest(
    this->UpdateTimeRequest, this->GetInputInformation(), this->GetOutputInformation());
}

//----------------------------------------------------------------------------
int svtkStreamingDemandDrivenPipeline::UpdateTimeDependentInformation(int port)
{
  // The algorithm should not invoke anything on the executive.
  if (!this->CheckAlgorithm("UpdateMetaInformation", nullptr))
  {
    return 0;
  }

  // Setup the request for information.
  if (!this->TimeDependentInformationRequest)
  {
    this->TimeDependentInformationRequest = svtkInformation::New();
    this->TimeDependentInformationRequest->Set(REQUEST_TIME_DEPENDENT_INFORMATION());
    // The request is forwarded upstream through the pipeline.
    this->TimeDependentInformationRequest->Set(
      svtkExecutive::FORWARD_DIRECTION(), svtkExecutive::RequestUpstream);
    // Algorithms process this request after it is forwarded.
    this->TimeDependentInformationRequest->Set(svtkExecutive::ALGORITHM_AFTER_FORWARD(), 1);
  }

  this->TimeDependentInformationRequest->Set(FROM_OUTPUT_PORT(), port);

  // Send the request.
  return this->ProcessRequest(this->TimeDependentInformationRequest, this->GetInputInformation(),
    this->GetOutputInformation());
}

//----------------------------------------------------------------------------
int svtkStreamingDemandDrivenPipeline ::VerifyOutputInformation(
  int outputPort, svtkInformationVector** inInfoVec, svtkInformationVector* outInfoVec)
{
  // If no port is specified, check all ports.
  if (outputPort < 0)
  {
    for (int i = 0; i < this->Algorithm->GetNumberOfOutputPorts(); ++i)
    {
      if (!this->VerifyOutputInformation(i, inInfoVec, outInfoVec))
      {
        return 0;
      }
    }
    return 1;
  }

  // Get the information object to check.
  svtkInformation* outInfo = outInfoVec->GetInformationObject(outputPort);

  // Make sure there is a data object.  It is supposed to be created
  // by the UpdateDataObject step.
  svtkDataObject* dataObject = outInfo->Get(svtkDataObject::DATA_OBJECT());
  if (!dataObject)
  {
    svtkErrorMacro("No data object has been set in the information for "
                  "output port "
      << outputPort << ".");
    return 0;
  }

  // Check extents.
  svtkInformation* dataInfo = dataObject->GetInformation();
  if (dataInfo->Get(svtkDataObject::DATA_EXTENT_TYPE()) == SVTK_PIECES_EXTENT)
  {
    // For an unstructured extent, make sure the update request
    // exists.  We do not need to check if it is valid because
    // out-of-range requests produce empty data.
    if (!outInfo->Has(UPDATE_PIECE_NUMBER()))
    {
      svtkErrorMacro("No update piece number has been set in the "
                    "information for output port "
        << outputPort << " on algorithm " << this->Algorithm->GetClassName() << "("
        << this->Algorithm << ").");
      return 0;
    }
    if (!outInfo->Has(UPDATE_NUMBER_OF_PIECES()))
    {
      svtkErrorMacro("No update number of pieces has been set in the "
                    "information for output port "
        << outputPort << " on algorithm " << this->Algorithm->GetClassName() << "("
        << this->Algorithm << ").");
      return 0;
    }
    if (!outInfo->Has(UPDATE_NUMBER_OF_GHOST_LEVELS()))
    {
      // Use zero ghost levels by default.
      outInfo->Set(UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
    }
  }
  else if (dataInfo->Get(svtkDataObject::DATA_EXTENT_TYPE()) == SVTK_3D_EXTENT)
  {
    // For a structured extent, make sure the update request
    // exists.
    if (!outInfo->Has(WHOLE_EXTENT()))
    {
      svtkErrorMacro("No whole extent has been set in the "
                    "information for output port "
        << outputPort << " on algorithm " << this->Algorithm->GetClassName() << "("
        << this->Algorithm << ").");
      return 0;
    }
    if (!outInfo->Has(UPDATE_EXTENT()))
    {
      svtkErrorMacro("No update extent has been set in the "
                    "information for output port "
        << outputPort << " on algorithm " << this->Algorithm->GetClassName() << "("
        << this->Algorithm << ").");
      return 0;
    }
    // Make sure the update request is inside the whole extent.
    int wholeExtent[6];
    int updateExtent[6];
    outInfo->Get(WHOLE_EXTENT(), wholeExtent);
    outInfo->Get(UPDATE_EXTENT(), updateExtent);
    if ((updateExtent[0] < wholeExtent[0] || updateExtent[1] > wholeExtent[1] ||
          updateExtent[2] < wholeExtent[2] || updateExtent[3] > wholeExtent[3] ||
          updateExtent[4] < wholeExtent[4] || updateExtent[5] > wholeExtent[5]) &&
      (updateExtent[0] <= updateExtent[1] && updateExtent[2] <= updateExtent[3] &&
        updateExtent[4] <= updateExtent[5]))
    {
      if (!outInfo->Has(UNRESTRICTED_UPDATE_EXTENT()))
      {
        // Update extent is outside the whole extent and is not empty.
        svtkErrorMacro("The update extent specified in the "
                      "information for output port "
          << outputPort << " on algorithm " << this->Algorithm->GetClassName() << "("
          << this->Algorithm << ") is " << updateExtent[0] << " " << updateExtent[1] << " "
          << updateExtent[2] << " " << updateExtent[3] << " " << updateExtent[4] << " "
          << updateExtent[5] << ", which is outside the whole extent " << wholeExtent[0] << " "
          << wholeExtent[1] << " " << wholeExtent[2] << " " << wholeExtent[3] << " "
          << wholeExtent[4] << " " << wholeExtent[5] << ".");
        return 0;
      }
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkStreamingDemandDrivenPipeline ::ExecuteDataStart(
  svtkInformation* request, svtkInformationVector** inInfoVec, svtkInformationVector* outInfoVec)
{
  // Preserve the execution continuation flag in the request across
  // iterations of the algorithm.  Perform start operations only if
  // not in an execute continuation.
  if (this->ContinueExecuting)
  {
    request->Set(CONTINUE_EXECUTING(), 1);
  }
  else
  {
    request->Remove(CONTINUE_EXECUTING());
    this->Superclass::ExecuteDataStart(request, inInfoVec, outInfoVec);
  }

  int numInfo = outInfoVec->GetNumberOfInformationObjects();
  for (int i = 0; i < numInfo; ++i)
  {
    svtkInformation* outInfo = outInfoVec->GetInformationObject(i);
    int numPieces = outInfo->Get(UPDATE_NUMBER_OF_PIECES());
    if (numPieces > 1)
    {
      int* uExt = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
      if (uExt)
      {
        // Store the total requested extent in ALL_PIECES_EXTENT.
        // This can be different than DATA_EXTENT if the algorithm
        // produces multiple pieces.
        // NOTE: we store this in outInfo because data info gets
        // wiped during execute. We move this to data info in
        // ExecuteDataEnd.
        outInfo->Set(svtkDataObject::ALL_PIECES_EXTENT(), uExt, 6);
      }

      // If the algorithm is capable of producing sub-extents, use
      // an extent translator to break update extent request into
      // pieces.
      if (outInfo->Has(svtkAlgorithm::CAN_PRODUCE_SUB_EXTENT()))
      {
        int piece = outInfo->Get(UPDATE_PIECE_NUMBER());
        int ghost = outInfo->Get(UPDATE_NUMBER_OF_GHOST_LEVELS());

        int splitMode = svtkExtentTranslator::BLOCK_MODE;
        if (outInfo->Has(svtkExtentTranslator::UPDATE_SPLIT_MODE()))
        {
          splitMode = outInfo->Get(svtkExtentTranslator::UPDATE_SPLIT_MODE());
        }

        svtkExtentTranslator* et = svtkExtentTranslator::New();
        int execExt[6];
        et->PieceToExtentThreadSafe(piece, numPieces, ghost, uExt, execExt, splitMode, 0);
        et->Delete();
        outInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), execExt, 6);
      }
    }
  }
}

//----------------------------------------------------------------------------
void svtkStreamingDemandDrivenPipeline ::ExecuteDataEnd(
  svtkInformation* request, svtkInformationVector** inInfoVec, svtkInformationVector* outInfoVec)
{
  int numInfo = outInfoVec->GetNumberOfInformationObjects();
  for (int i = 0; i < numInfo; ++i)
  {
    svtkInformation* outInfo = outInfoVec->GetInformationObject(i);
    int numPieces = outInfo->Get(UPDATE_NUMBER_OF_PIECES());
    if (numPieces > 1)
    {
      svtkDataObject* dobj = outInfo->Get(svtkDataObject::DATA_OBJECT());

      // See ExecuteDataStart for an explanation of this key and
      // why we move it from outInfo to data info.
      if (outInfo->Has(svtkDataObject::ALL_PIECES_EXTENT()))
      {
        dobj->GetInformation()->Set(
          svtkDataObject::ALL_PIECES_EXTENT(), outInfo->Get(svtkDataObject::ALL_PIECES_EXTENT()), 6);
      }

      if (outInfo->Has(svtkAlgorithm::CAN_PRODUCE_SUB_EXTENT()))
      {
        int ghost = outInfo->Get(UPDATE_NUMBER_OF_GHOST_LEVELS());
        if (ghost > 0)
        {
          svtkDataSet* data = svtkDataSet::SafeDownCast(dobj);
          if (data)
          {
            int* uExt = data->GetInformation()->Get(svtkDataObject::ALL_PIECES_EXTENT());

            int piece = outInfo->Get(UPDATE_PIECE_NUMBER());

            svtkExtentTranslator* et = svtkExtentTranslator::New();
            int zeroExt[6];
            et->PieceToExtentThreadSafe(
              piece, numPieces, 0, uExt, zeroExt, svtkExtentTranslator::BLOCK_MODE, 0);
            et->Delete();

            data->GenerateGhostArray(zeroExt);
          }
        }

        // Restore the full update extent, as the subextent handling will
        // clobber it
        if (outInfo->Has(svtkDataObject::ALL_PIECES_EXTENT()))
        {
          outInfo->Set(UPDATE_EXTENT(), outInfo->Get(svtkDataObject::ALL_PIECES_EXTENT()), 6);
        }
      }
      // Remove ALL_PIECES_EXTENT from outInfo (it was moved to the data obj
      // earlier).
      if (outInfo->Has(svtkDataObject::ALL_PIECES_EXTENT()))
      {
        outInfo->Remove(svtkDataObject::ALL_PIECES_EXTENT());
      }
    }
  }

  // Preserve the execution continuation flag in the request across
  // iterations of the algorithm.  Perform start operations only if
  // not in an execute continuation.
  if (request->Get(CONTINUE_EXECUTING()))
  {
    if (!this->ContinueExecuting)
    {
      this->ContinueExecuting = 1;
      this->Update(request->Get(FROM_OUTPUT_PORT()));
    }
  }
  else
  {
    if (this->ContinueExecuting)
    {
      this->ContinueExecuting = 0;
    }
    this->Superclass::ExecuteDataEnd(request, inInfoVec, outInfoVec);
  }
}

//----------------------------------------------------------------------------
void svtkStreamingDemandDrivenPipeline ::MarkOutputsGenerated(
  svtkInformation* request, svtkInformationVector** inInfoVec, svtkInformationVector* outInfoVec)
{
  // Tell outputs they have been generated.
  this->Superclass::MarkOutputsGenerated(request, inInfoVec, outInfoVec);

  int outputPort = 0;
  if (request->Has(FROM_OUTPUT_PORT()))
  {
    outputPort = request->Get(FROM_OUTPUT_PORT());
    outputPort = (outputPort >= 0 ? outputPort : 0);
  }

  // Get the piece request from the update port (port 0 if none)
  // The defaults are:
  int piece = 0;
  int numPieces = 1;
  int ghostLevel = 0;
  svtkInformation* fromInfo = nullptr;
  if (outputPort < outInfoVec->GetNumberOfInformationObjects())
  {
    fromInfo = outInfoVec->GetInformationObject(outputPort);
    if (fromInfo->Has(UPDATE_PIECE_NUMBER()))
    {
      piece = fromInfo->Get(UPDATE_PIECE_NUMBER());
    }
    if (fromInfo->Has(UPDATE_NUMBER_OF_PIECES()))
    {
      numPieces = fromInfo->Get(UPDATE_NUMBER_OF_PIECES());
    }
    if (fromInfo->Has(UPDATE_NUMBER_OF_GHOST_LEVELS()))
    {
      ghostLevel = fromInfo->Get(UPDATE_NUMBER_OF_GHOST_LEVELS());
    }
  }

  for (int i = 0; i < outInfoVec->GetNumberOfInformationObjects(); ++i)
  {
    svtkInformation* outInfo = outInfoVec->GetInformationObject(i);
    svtkDataObject* data = outInfo->Get(svtkDataObject::DATA_OBJECT());
    // Compute ghost level arrays for generated outputs.
    if (data && !outInfo->Get(DATA_NOT_GENERATED()))
    {
      // Copy the update piece information from the update port to
      // the data piece information of all output ports UNLESS the
      // algorithm already specified it.
      svtkInformation* dataInfo = data->GetInformation();
      if (!dataInfo->Has(svtkDataObject::DATA_PIECE_NUMBER()) ||
        dataInfo->Get(svtkDataObject::DATA_PIECE_NUMBER()) == -1)
      {
        dataInfo->Set(svtkDataObject::DATA_PIECE_NUMBER(), piece);
        dataInfo->Set(svtkDataObject::DATA_NUMBER_OF_PIECES(), numPieces);
        // If the source or filter produced a different number of ghost
        // levels, honor it.
        int dataGhostLevel = 0;
        if (dataInfo->Has(svtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS()))
        {
          dataGhostLevel = dataInfo->Get(svtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS());
        }
        // If the ghost level generated by the algorithm is larger than
        // requested, we keep it. Otherwise, we store the requested one.
        // We do this because there is no point in the algorithm re-executing
        // if the downstream asks for the same level even though the
        // algorithm cannot produce it.
        dataInfo->Set(svtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS(),
          ghostLevel > dataGhostLevel ? ghostLevel : dataGhostLevel);
      }

      // In this block, we make sure that DATA_TIME_STEP() is set if:
      // * There was someone upstream that supports time (TIME_RANGE() key
      //   is present)
      // * Someone downstream requested a timestep (UPDATE_TIME_STEP())
      //
      // A common situation in which the DATA_TIME_STEP() would not be
      // present even if the two conditions above are satisfied is when
      // a filter that is not time-aware is processing a dataset produced
      // by a time-aware source. In this case, DATA_TIME_STEP() should
      // be copied from input to output.
      //
      // Check if the output has DATA_TIME_STEP().
      if (!dataInfo->Has(svtkDataObject::DATA_TIME_STEP()) && outInfo->Has(TIME_RANGE()))
      {
        // It does not.
        // Does the input have it? If yes, copy it.
        svtkDataObject* input = nullptr;
        if (this->GetNumberOfInputPorts() > 0)
        {
          input = this->GetInputData(0, 0);
        }
        if (input && input->GetInformation()->Has(svtkDataObject::DATA_TIME_STEP()))
        {
          dataInfo->CopyEntry(input->GetInformation(), svtkDataObject::DATA_TIME_STEP(), 1);
        }
        // Does the update request have it? If yes, copy it. This
        // should not normally happen.
        else if (fromInfo->Has(UPDATE_TIME_STEP()))
        {
          dataInfo->Set(svtkDataObject::DATA_TIME_STEP(), fromInfo->Get(UPDATE_TIME_STEP()));
        }
      }

      // We are keeping track of the previous time request.
      if (fromInfo->Has(UPDATE_TIME_STEP()))
      {
        outInfo->Set(PREVIOUS_UPDATE_TIME_STEP(), fromInfo->Get(UPDATE_TIME_STEP()));
      }
      else
      {
        outInfo->Remove(PREVIOUS_UPDATE_TIME_STEP());
      }

      // Give the keys an opportunity to store meta-data in
      // the data object about what update request lead to
      // the last execution. This information can later be
      // used to decide whether an execution is necessary.
      svtkSmartPointer<svtkInformationIterator> infoIter =
        svtkSmartPointer<svtkInformationIterator>::New();
      infoIter->SetInformationWeak(outInfo);
      infoIter->InitTraversal();
      while (!infoIter->IsDoneWithTraversal())
      {
        svtkInformationKey* key = infoIter->GetCurrentKey();
        key->StoreMetaData(request, outInfo, dataInfo);
        infoIter->GoToNextItem();
      }
    }
  }
}

//----------------------------------------------------------------------------
int svtkStreamingDemandDrivenPipeline ::NeedToExecuteData(
  int outputPort, svtkInformationVector** inInfoVec, svtkInformationVector* outInfoVec)
{
  // Has the algorithm asked to be executed again?
  if (this->ContinueExecuting)
  {
    return 1;
  }

  // If no port is specified, check all ports.  This behavior is
  // implemented by the superclass.
  if (outputPort < 0)
  {
    return this->Superclass::NeedToExecuteData(outputPort, inInfoVec, outInfoVec);
  }

  svtkInformation* outInfo = outInfoVec->GetInformationObject(outputPort);
  int updateNumberOfPieces = outInfo->Get(UPDATE_NUMBER_OF_PIECES());
  int updatePiece = outInfo->Get(UPDATE_PIECE_NUMBER());

  if (updateNumberOfPieces > 1 && updatePiece > 0)
  {
    // This is a source.
    if (this->Algorithm->GetNumberOfInputPorts() == 0)
    {
      // And cannot handle piece request (i.e. not parallel)
      // and is not a structured source that can produce sub-extents.
      if (!outInfo->Get(svtkAlgorithm::CAN_HANDLE_PIECE_REQUEST()) &&
        !outInfo->Get(svtkAlgorithm::CAN_PRODUCE_SUB_EXTENT()))
      {
        // Then don't execute it.
        return 0;
      }
    }
  }

  // Does the superclass want to execute?
  if (this->Superclass::NeedToExecuteData(outputPort, inInfoVec, outInfoVec))
  {
    return 1;
  }

  // We need to check the requested update extent.  Get the output
  // port information and data information.  We do not need to check
  // existence of values because it has already been verified by
  // VerifyOutputInformation.
  svtkDataObject* dataObject = outInfo->Get(svtkDataObject::DATA_OBJECT());
  svtkInformation* dataInfo = dataObject->GetInformation();

  // Check the unstructured extent.  If we do not have the requested
  // piece, we need to execute.
  int dataNumberOfPieces = dataInfo->Get(svtkDataObject::DATA_NUMBER_OF_PIECES());
  if (dataNumberOfPieces != updateNumberOfPieces)
  {
    return 1;
  }
  int dataGhostLevel = dataInfo->Get(svtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS());
  int updateGhostLevel = outInfo->Get(UPDATE_NUMBER_OF_GHOST_LEVELS());
  if (updateNumberOfPieces > 1 && dataGhostLevel < updateGhostLevel)
  {
    return 1;
  }
  if (dataNumberOfPieces != 1)
  {
    int dataPiece = dataInfo->Get(svtkDataObject::DATA_PIECE_NUMBER());
    if (dataPiece != updatePiece)
    {
      return 1;
    }
  }

  if (outInfo->Has(UPDATE_EXTENT()) && dataInfo->Has(svtkDataObject::DATA_EXTENT_TYPE()) &&
    dataInfo->Get(svtkDataObject::DATA_EXTENT_TYPE()) == SVTK_3D_EXTENT)
  {
    if (!dataInfo->Has(svtkDataObject::DATA_EXTENT()) &&
      !dataInfo->Has(svtkDataObject::ALL_PIECES_EXTENT()))
    {
      return 1;
    }

    // Check the structured extent.  If the update extent is outside
    // of the extent and not empty, we need to execute.
    int updateExtent[6];
    outInfo->Get(UPDATE_EXTENT(), updateExtent);

    int dataExtent[6];
    if (dataInfo->Has(svtkDataObject::ALL_PIECES_EXTENT()))
    {
      dataInfo->Get(svtkDataObject::ALL_PIECES_EXTENT(), dataExtent);
    }
    else
    {
      dataInfo->Get(svtkDataObject::DATA_EXTENT(), dataExtent);
    }

    // if the ue is out side the de
    if ((updateExtent[0] < dataExtent[0] || updateExtent[1] > dataExtent[1] ||
          updateExtent[2] < dataExtent[2] || updateExtent[3] > dataExtent[3] ||
          updateExtent[4] < dataExtent[4] || updateExtent[5] > dataExtent[5]) &&
      // and the ue is set
      (updateExtent[0] <= updateExtent[1] && updateExtent[2] <= updateExtent[3] &&
        updateExtent[4] <= updateExtent[5]))
    {
      return 1;
    }
  }

  if (this->NeedToExecuteBasedOnTime(outInfo, dataObject))
  {
    return 1;
  }

  // Ask the keys if we need to execute. Keys can overwrite
  // NeedToExecute() to make their own decision about whether
  // what they are asking for is different than what is in the
  // data and whether the filter should execute.
  this->InformationIterator->SetInformationWeak(outInfo);

  this->InformationIterator->InitTraversal();
  while (!this->InformationIterator->IsDoneWithTraversal())
  {
    svtkInformationKey* key = this->InformationIterator->GetCurrentKey();
    if (key->NeedToExecute(outInfo, dataInfo))
    {
      return 1;
    }
    this->InformationIterator->GoToNextItem();
  }

  // We do not need to execute.
  return 0;
}

//----------------------------------------------------------------------------
int svtkStreamingDemandDrivenPipeline::NeedToExecuteBasedOnTime(
  svtkInformation* outInfo, svtkDataObject* dataObject)
{
  // If this algorithm does not provide time information and another
  // algorithm upstream did not provide time information, we do not
  // re-execute even if the time request changed.
  if (!outInfo->Has(TIME_RANGE()))
  {
    return 0;
  }

  svtkInformation* dataInfo = dataObject->GetInformation();
  // if we are requesting a particular update time index, check
  // if we have the desired time index.
  if (outInfo->Has(UPDATE_TIME_STEP()))
  {
    if (!dataInfo->Has(svtkDataObject::DATA_TIME_STEP()))
    {
      return 1;
    }

    double ustep = outInfo->Get(UPDATE_TIME_STEP());

    // First check if time request is the same as previous time request.
    // If the previous update request did not correspond to an existing
    // time step and the reader chose a time step with it's own logic, the
    // data time step will be different than the request. If the same time
    // step is requested again, there is no need to re-execute the
    // algorithm.  We know that it does not have this time step.
    if (outInfo->Has(PREVIOUS_UPDATE_TIME_STEP()))
    {
      if (outInfo->Has(UPDATE_TIME_STEP()))
      {
        bool match = true;
        double pstep = outInfo->Get(PREVIOUS_UPDATE_TIME_STEP());
        if (pstep != ustep)
        {
          match = false;
        }
        if (match)
        {
          return 0;
        }
      }
    }

    int hasdsteps = dataInfo->Has(svtkDataObject::DATA_TIME_STEP());
    int hasusteps = dataInfo->Has(UPDATE_TIME_STEP());

    double dstep = dataInfo->Get(svtkDataObject::DATA_TIME_STEP());
    if ((hasdsteps && !hasusteps) || (!hasdsteps && hasusteps))
    {
      return 1;
    }
    if (dstep != ustep)
    {
      return 1;
    }
  }
  return 0;
}

//----------------------------------------------------------------------------
int svtkStreamingDemandDrivenPipeline ::SetWholeExtent(svtkInformation* info, int extent[6])
{
  if (!info)
  {
    svtkGenericWarningMacro("SetWholeExtent on invalid output");
    return 0;
  }
  int modified = 0;
  int oldExtent[6];
  svtkStreamingDemandDrivenPipeline::GetWholeExtent(info, oldExtent);
  if (oldExtent[0] != extent[0] || oldExtent[1] != extent[1] || oldExtent[2] != extent[2] ||
    oldExtent[3] != extent[3] || oldExtent[4] != extent[4] || oldExtent[5] != extent[5])
  {
    modified = 1;
    info->Set(WHOLE_EXTENT(), extent, 6);
  }
  return modified;
}

//----------------------------------------------------------------------------
void svtkStreamingDemandDrivenPipeline ::GetWholeExtent(svtkInformation* info, int extent[6])
{
  static int emptyExtent[6] = { 0, -1, 0, -1, 0, -1 };
  if (!info)
  {
    memcpy(extent, emptyExtent, sizeof(int) * 6);
    return;
  }
  if (!info->Has(WHOLE_EXTENT()))
  {
    info->Set(WHOLE_EXTENT(), emptyExtent, 6);
  }
  info->Get(WHOLE_EXTENT(), extent);
}

//----------------------------------------------------------------------------
int* svtkStreamingDemandDrivenPipeline::GetWholeExtent(svtkInformation* info)
{
  static int emptyExtent[6] = { 0, -1, 0, -1, 0, -1 };
  if (!info)
  {
    return emptyExtent;
  }
  if (!info->Has(WHOLE_EXTENT()))
  {
    info->Set(WHOLE_EXTENT(), emptyExtent, 6);
  }
  return info->Get(WHOLE_EXTENT());
}

// This is here to shut off warning about deprecated calls being
// made from other deprecated functions.
#if defined(__GNUC__) && !defined(__INTEL_COMPILER)
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

//----------------------------------------------------------------------------
void svtkStreamingDemandDrivenPipeline ::GetUpdateExtent(svtkInformation* info, int extent[6])
{
  static int emptyExtent[6] = { 0, -1, 0, -1, 0, -1 };
  if (!info)
  {
    svtkGenericWarningMacro("GetUpdateExtent on invalid output");
    memcpy(extent, emptyExtent, sizeof(int) * 6);
    return;
  }
  if (!info->Has(UPDATE_EXTENT()))
  {
    info->Set(UPDATE_EXTENT(), emptyExtent, 6);
  }
  info->Get(UPDATE_EXTENT(), extent);
}

//----------------------------------------------------------------------------
int* svtkStreamingDemandDrivenPipeline ::GetUpdateExtent(svtkInformation* info)
{
  static int emptyExtent[6] = { 0, -1, 0, -1, 0, -1 };
  if (!info)
  {
    svtkGenericWarningMacro("GetUpdateExtent on invalid output");
    return emptyExtent;
  }
  if (!info->Has(UPDATE_EXTENT()))
  {
    info->Set(UPDATE_EXTENT(), emptyExtent, 6);
  }
  return info->Get(UPDATE_EXTENT());
}

//----------------------------------------------------------------------------
int svtkStreamingDemandDrivenPipeline ::GetUpdatePiece(svtkInformation* info)
{
  if (!info)
  {
    svtkGenericWarningMacro("GetUpdatePiece on invalid output");
    return 0;
  }
  if (!info->Has(UPDATE_PIECE_NUMBER()))
  {
    info->Set(UPDATE_PIECE_NUMBER(), 0);
  }
  return info->Get(UPDATE_PIECE_NUMBER());
}

//----------------------------------------------------------------------------
int svtkStreamingDemandDrivenPipeline ::GetUpdateNumberOfPieces(svtkInformation* info)
{
  if (!info)
  {
    svtkGenericWarningMacro("GetUpdateNumberOfPieces on invalid output");
    return 1;
  }
  if (!info->Has(UPDATE_NUMBER_OF_PIECES()))
  {
    info->Set(UPDATE_NUMBER_OF_PIECES(), 1);
  }
  return info->Get(UPDATE_NUMBER_OF_PIECES());
}

//----------------------------------------------------------------------------
int svtkStreamingDemandDrivenPipeline ::GetUpdateGhostLevel(svtkInformation* info)
{
  if (!info)
  {
    svtkGenericWarningMacro("GetUpdateGhostLevel on invalid output");
    return 0;
  }
  if (!info->Has(UPDATE_NUMBER_OF_GHOST_LEVELS()))
  {
    info->Set(UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
  }
  return info->Get(UPDATE_NUMBER_OF_GHOST_LEVELS());
}

//----------------------------------------------------------------------------
int svtkStreamingDemandDrivenPipeline::SetRequestExactExtent(int port, int flag)
{
  if (!this->OutputPortIndexInRange(port, "set request exact extent flag on"))
  {
    return 0;
  }
  svtkInformation* info = this->GetOutputInformation(port);
  if (this->GetRequestExactExtent(port) != flag)
  {
    info->Set(EXACT_EXTENT(), flag);
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
int svtkStreamingDemandDrivenPipeline::GetRequestExactExtent(int port)
{
  if (!this->OutputPortIndexInRange(port, "get request exact extent flag from"))
  {
    return 0;
  }
  svtkInformation* info = this->GetOutputInformation(port);
  if (!info->Has(EXACT_EXTENT()))
  {
    info->Set(EXACT_EXTENT(), 0);
  }
  return info->Get(EXACT_EXTENT());
}
