/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkCompositeDataPipeline.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCompositeDataPipeline.h"

#include "svtkAlgorithm.h"
#include "svtkAlgorithmOutput.h"
#include "svtkCompositeDataIterator.h"
#include "svtkDataObjectTreeIterator.h"
#include "svtkFieldData.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationDoubleKey.h"
#include "svtkInformationExecutivePortKey.h"
#include "svtkInformationExecutivePortVectorKey.h"
#include "svtkInformationIdTypeKey.h"
#include "svtkInformationIntegerKey.h"
#include "svtkInformationIntegerVectorKey.h"
#include "svtkInformationKey.h"
#include "svtkInformationObjectBaseKey.h"
#include "svtkInformationStringKey.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkPartitionedDataSetCollection.h"
#include "svtkPolyData.h"
#include "svtkRectilinearGrid.h"
#include "svtkSmartPointer.h"
#include "svtkStructuredGrid.h"
#include "svtkTrivialProducer.h"
#include "svtkUniformGrid.h"

svtkStandardNewMacro(svtkCompositeDataPipeline);

svtkInformationKeyMacro(svtkCompositeDataPipeline, LOAD_REQUESTED_BLOCKS, Integer);
svtkInformationKeyMacro(svtkCompositeDataPipeline, COMPOSITE_DATA_META_DATA, ObjectBase);
svtkInformationKeyMacro(svtkCompositeDataPipeline, UPDATE_COMPOSITE_INDICES, IntegerVector);
svtkInformationKeyMacro(svtkCompositeDataPipeline, DATA_COMPOSITE_INDICES, IntegerVector);
svtkInformationKeyMacro(svtkCompositeDataPipeline, SUPPRESS_RESET_PI, Integer);
svtkInformationKeyMacro(svtkCompositeDataPipeline, BLOCK_AMOUNT_OF_DETAIL, Double);

//----------------------------------------------------------------------------
svtkCompositeDataPipeline::svtkCompositeDataPipeline()
{
  this->InLocalLoop = 0;
  this->InformationCache = svtkInformation::New();

  this->GenericRequest = svtkInformation::New();

  if (!this->DataObjectRequest)
  {
    this->DataObjectRequest = svtkInformation::New();
  }
  this->DataObjectRequest->Set(svtkDemandDrivenPipeline::REQUEST_DATA_OBJECT());
  // The request is forwarded upstream through the pipeline.
  this->DataObjectRequest->Set(svtkExecutive::FORWARD_DIRECTION(), svtkExecutive::RequestUpstream);
  // Algorithms process this request after it is forwarded.
  this->DataObjectRequest->Set(svtkExecutive::ALGORITHM_AFTER_FORWARD(), 1);

  this->InformationRequest = svtkInformation::New();
  this->InformationRequest->Set(svtkDemandDrivenPipeline::REQUEST_INFORMATION());
  // The request is forwarded upstream through the pipeline.
  this->InformationRequest->Set(svtkExecutive::FORWARD_DIRECTION(), svtkExecutive::RequestUpstream);
  // Algorithms process this request after it is forwarded.
  this->InformationRequest->Set(svtkExecutive::ALGORITHM_AFTER_FORWARD(), 1);

  if (!this->DataRequest)
  {
    this->DataRequest = svtkInformation::New();
  }
  this->DataRequest->Set(REQUEST_DATA());
  // The request is forwarded upstream through the pipeline.
  this->DataRequest->Set(svtkExecutive::FORWARD_DIRECTION(), svtkExecutive::RequestUpstream);
  // Algorithms process this request after it is forwarded.
  this->DataRequest->Set(svtkExecutive::ALGORITHM_AFTER_FORWARD(), 1);
}

//----------------------------------------------------------------------------
svtkCompositeDataPipeline::~svtkCompositeDataPipeline()
{
  this->InformationCache->Delete();

  this->GenericRequest->Delete();
  this->InformationRequest->Delete();
}

//----------------------------------------------------------------------------
int svtkCompositeDataPipeline::ExecuteDataObject(
  svtkInformation* request, svtkInformationVector** inInfoVec, svtkInformationVector* outInfoVec)
{
  svtkDebugMacro(<< "ExecuteDataObject");
  int result = 1;

  // If the input is composite, allow algorithm to handle
  // REQUEST_DATA_OBJECT only if it can handle composite
  // datasets. Otherwise, the algorithm will get a chance to handle
  // REQUEST_DATA_OBJECT when it is being iterated over.
  int compositePort;
  bool shouldIterate = this->ShouldIterateOverInput(inInfoVec, compositePort);
  if (!shouldIterate)
  {
    // Invoke the request on the algorithm.
    result = this->CallAlgorithm(request, svtkExecutive::RequestDownstream, inInfoVec, outInfoVec);
    if (!result)
    {
      return result;
    }
  }

  // Make sure a valid data object exists for all output ports.
  svtkDebugMacro(<< "ExecuteDataObject calling CheckCompositeData");
  result = this->CheckCompositeData(request, inInfoVec, outInfoVec);

  return result;
}

//----------------------------------------------------------------------------
void svtkCompositeDataPipeline::ExecuteDataStart(
  svtkInformation* request, svtkInformationVector** inInfoVec, svtkInformationVector* outInfoVec)
{
  this->Superclass::ExecuteDataStart(request, inInfoVec, outInfoVec);
}

//----------------------------------------------------------------------------
// Handle REQUEST_DATA
int svtkCompositeDataPipeline::ExecuteData(
  svtkInformation* request, svtkInformationVector** inInfoVec, svtkInformationVector* outInfoVec)
{
  svtkDebugMacro(<< "ExecuteData");
  int result = 1;

  int compositePort;
  bool composite = this->ShouldIterateOverInput(inInfoVec, compositePort);

  if (composite)
  {
    if (this->GetNumberOfOutputPorts())
    {
      this->ExecuteSimpleAlgorithm(request, inInfoVec, outInfoVec, compositePort);
    }
    else
    {
      svtkErrorMacro("Can not execute simple algorithm without output ports");
      return 0;
    }
  }
  else
  {
    svtkDebugMacro(<< "  Superclass::ExecuteData");
    result = this->Superclass::ExecuteData(request, inInfoVec, outInfoVec);
  }

  return result;
}

//----------------------------------------------------------------------------
int svtkCompositeDataPipeline::InputTypeIsValid(
  int port, int index, svtkInformationVector** inInfoVec)
{
  if (this->InLocalLoop)
  {
    return this->Superclass::InputTypeIsValid(port, index, inInfoVec);
  }
  if (!inInfoVec[port])
  {
    return 0;
  }

  // If we will be iterating over the input on this port, assume that we
  // can handle any input type. The input type will be checked again during
  // each step of the iteration.
  int compositePort;
  if (this->ShouldIterateOverInput(inInfoVec, compositePort))
  {
    if (compositePort == port)
    {
      return 1;
    }
  }

  // Otherwise, let superclass handle it.
  return this->Superclass::InputTypeIsValid(port, index, inInfoVec);
}

//----------------------------------------------------------------------------
bool svtkCompositeDataPipeline::ShouldIterateOverInput(
  svtkInformationVector** inInfoVec, int& compositePort)
{
  compositePort = -1;
  // Find the first input that has a composite data that does not match
  // the required input type. We assume that that port input has to
  // be iterated over. We also require that this port has only one
  // connection.
  int numInputPorts = this->Algorithm->GetNumberOfInputPorts();
  for (int i = 0; i < numInputPorts; ++i)
  {
    int numInConnections = this->Algorithm->GetNumberOfInputConnections(i);
    // If there is 1 connection
    if (numInConnections == 1)
    {
      svtkInformation* inPortInfo = this->Algorithm->GetInputPortInformation(i);
      if (inPortInfo->Has(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE()) &&
        inPortInfo->Length(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE()) > 0)
      {
        const char* inputType = inPortInfo->Get(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), 0);
        // the filter upstream will iterate

        if (strcmp(inputType, "svtkCompositeDataSet") == 0 ||
          strcmp(inputType, "svtkDataObjectTree") == 0 ||
          strcmp(inputType, "svtkHierarchicalBoxDataSet") == 0 ||
          strcmp(inputType, "svtkOverlappingAMR") == 0 ||
          strcmp(inputType, "svtkNonOverlappingAMR") == 0 ||
          strcmp(inputType, "svtkMultiBlockDataSet") == 0 ||
          strcmp(inputType, "svtkPartitionedDataSetCollection") == 0)
        {
          svtkDebugMacro(<< "ShouldIterateOverInput return 0 (Composite)");
          return false;
        }

        svtkInformation* inInfo = inInfoVec[i]->GetInformationObject(0);
        svtkDataObject* input = inInfo->Get(svtkDataObject::DATA_OBJECT());
        // If input does not match a required input type
        bool foundMatch = false;
        if (input)
        {
          int size = inPortInfo->Length(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
          for (int j = 0; j < size; ++j)
          {
            if (input->IsA(inPortInfo->Get(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), j)))
            {
              foundMatch = true;
            }
          }
        }
        if (input && !foundMatch)
        {
          // If input is composite
          if (svtkCompositeDataSet::SafeDownCast(input))
          {
            // Assume that we have to iterate over input
            compositePort = i;
            svtkDebugMacro(<< "ShouldIterateOverInput returns 1 (input composite)");
            return true;
          }
        }
      }
    }
  }
  svtkDebugMacro(<< "ShouldIterateOverInput returns 0 (default)");
  return false;
}

//----------------------------------------------------------------------------
void svtkCompositeDataPipeline::ExecuteEach(svtkCompositeDataIterator* iter,
  svtkInformationVector** inInfoVec, svtkInformationVector* outInfoVec, int compositePort,
  int connection, svtkInformation* request,
  std::vector<svtkSmartPointer<svtkCompositeDataSet> >& compositeOutputs)
{
  svtkInformation* inInfo = inInfoVec[compositePort]->GetInformationObject(connection);

  svtkIdType num_blocks = 0;
  // a quick iteration to get the total number of blocks to iterate over which
  // is necessary to scale progress events.
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    ++num_blocks;
  }

  const double progress_scale = 1.0 / num_blocks;
  svtkIdType block_index = 0;

  auto algo = this->GetAlgorithm();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem(), ++block_index)
  {
    svtkDataObject* dobj = iter->GetCurrentDataObject();
    if (dobj)
    {
      algo->SetProgressShiftScale(progress_scale * block_index, progress_scale);
      // Note that since VisitOnlyLeaves is ON on the iterator,
      // this method is called only for leaves, hence, we are assured that
      // neither dobj nor outObj are svtkCompositeDataSet subclasses.
      std::vector<svtkDataObject*> outObjs =
        this->ExecuteSimpleAlgorithmForBlock(inInfoVec, outInfoVec, inInfo, request, dobj);
      if (!outObjs.empty())
      {
        for (unsigned port = 0; port < compositeOutputs.size(); ++port)
        {
          if (svtkDataObject* outObj = outObjs[port])
          {
            if (compositeOutputs[port])
            {
              compositeOutputs[port]->SetDataSet(iter, outObj);
            }
            outObj->FastDelete();
          }
        }
      }
    }
  }

  algo->SetProgressShiftScale(0.0, 1.0);
}

//----------------------------------------------------------------------------
// Execute a simple (non-composite-aware) filter multiple times, once per
// block. Collect the result in a composite dataset that is of the same
// structure as the input.
void svtkCompositeDataPipeline::ExecuteSimpleAlgorithm(svtkInformation* request,
  svtkInformationVector** inInfoVec, svtkInformationVector* outInfoVec, int compositePort)
{
  svtkDebugMacro(<< "ExecuteSimpleAlgorithm");

  this->ExecuteDataStart(request, inInfoVec, outInfoVec);

  svtkInformation* outInfo = nullptr;

  if (this->GetNumberOfOutputPorts() > 0)
  {
    outInfo = outInfoVec->GetInformationObject(0);
  }
  if (!outInfo)
  {
    return;
  }

  // Make sure a valid composite data object exists for all output ports.
  this->CheckCompositeData(request, inInfoVec, outInfoVec);

  // if we have no composite inputs
  if (compositePort == -1)
  {
    return;
  }

  // Loop using the first input on the first port.
  // This might not be valid for all cases but it is a decent
  // assumption to start with.
  // TODO: Loop over all inputs
  svtkInformation* inInfo = this->GetInputInformation(compositePort, 0);
  svtkCompositeDataSet* input =
    svtkCompositeDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  bool compositeOutputFound = false;
  std::vector<svtkSmartPointer<svtkCompositeDataSet> > compositeOutputs;
  for (int port = 0; port < outInfoVec->GetNumberOfInformationObjects(); ++port)
  {
    compositeOutputs.push_back(svtkCompositeDataSet::GetData(outInfoVec, port));
    if (compositeOutputs.back())
    {
      compositeOutputFound = true;
    }
  }

  if (input && compositeOutputFound)
  {
    for (int port = 0; port < outInfoVec->GetNumberOfInformationObjects(); ++port)
    {
      if (compositeOutputs[port])
      {
        compositeOutputs[port]->PrepareForNewData();
        compositeOutputs[port]->CopyStructure(input);
        if (input && input->GetFieldData())
        {
          compositeOutputs[port]->GetFieldData()->PassData(input->GetFieldData());
        }
      }
    }

    svtkSmartPointer<svtkInformation> r = svtkSmartPointer<svtkInformation>::New();

    r->Set(FROM_OUTPUT_PORT(), PRODUCER()->GetPort(outInfo));

    // The request is forwarded upstream through the pipeline.
    r->Set(svtkExecutive::FORWARD_DIRECTION(), svtkExecutive::RequestUpstream);

    // Algorithms process this request after it is forwarded.
    r->Set(svtkExecutive::ALGORITHM_AFTER_FORWARD(), 1);

    // Store the information (whole_extent)
    // before looping. Otherwise, executeinformation will cause
    // changes (because we pretend that the max. number of pieces is
    // one to process the whole block)
    this->PushInformation(inInfo);

    svtkDebugMacro(<< "EXECUTING " << this->Algorithm->GetClassName());

    // True when the pipeline is iterating over the current (simple)
    // filter to produce composite output. In this case,
    // ExecuteDataStart() should NOT Initialize() the composite output.
    this->InLocalLoop = 1;

    svtkSmartPointer<svtkCompositeDataIterator> iter;
    iter.TakeReference(input->NewIterator());
    if (svtkPartitionedDataSetCollection::SafeDownCast(input))
    {
      bool iteratePartitions = false;
      svtkInformation* inPortInfo = this->Algorithm->GetInputPortInformation(compositePort);
      if (inPortInfo->Has(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE()) &&
        inPortInfo->Length(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE()) > 0)
      {
        int size = inPortInfo->Length(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
        for (int j = 0; j < size; ++j)
        {
          const char* inputType = inPortInfo->Get(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), j);
          if (strcmp(inputType, "svtkPartitionedDataSet") == 0)
          {
            iteratePartitions = true;
          }
        }
        if (iteratePartitions)
        {
          svtkDataObjectTreeIterator::SafeDownCast(iter)->TraverseSubTreeOff();
          svtkDataObjectTreeIterator::SafeDownCast(iter)->VisitOnlyLeavesOff();
        }
      }
    }

    this->ExecuteEach(iter, inInfoVec, outInfoVec, compositePort, 0, r, compositeOutputs);

    // True when the pipeline is iterating over the current (simple)
    // filter to produce composite output. In this case,
    // ExecuteDataStart() should NOT Initialize() the composite output.
    this->InLocalLoop = 0;
    // Restore the extent information and force it to be
    // copied to the output.
    this->PopInformation(inInfo);
    r->Set(REQUEST_INFORMATION());
    this->CopyDefaultInformation(r, svtkExecutive::RequestDownstream, this->GetInputInformation(),
      this->GetOutputInformation());

    svtkDataObject* curInput = inInfo->Get(svtkDataObject::DATA_OBJECT());
    if (curInput != input)
    {
      inInfo->Remove(svtkDataObject::DATA_OBJECT());
      inInfo->Set(svtkDataObject::DATA_OBJECT(), input);
    }
    for (int port = 0; port < outInfoVec->GetNumberOfInformationObjects(); ++port)
    {
      svtkDataObject* curOutput = svtkDataObject::GetData(outInfoVec, port);
      if (curOutput != compositeOutputs[port].GetPointer())
      {
        outInfoVec->GetInformationObject(port)->Set(
          svtkDataObject::DATA_OBJECT(), compositeOutputs[port]);
      }
    }
  }
  this->ExecuteDataEnd(request, inInfoVec, outInfoVec);
}

//----------------------------------------------------------------------------
std::vector<svtkDataObject*> svtkCompositeDataPipeline::ExecuteSimpleAlgorithmForBlock(
  svtkInformationVector** inInfoVec, svtkInformationVector* outInfoVec, svtkInformation* inInfo,
  svtkInformation* request, svtkDataObject* dobj)
{
  svtkDebugMacro(<< "ExecuteSimpleAlgorithmForBlock");

  std::vector<svtkDataObject*> outputs;

  // if (dobj && dobj->IsA("svtkCompositeDataSet"))
  // {
  //   svtkErrorMacro("ExecuteSimpleAlgorithmForBlock cannot be called "
  //     "for a svtkCompositeDataSet");
  //   return outputs; // return empty vector as error
  // }

  // There must be a bug somewhere. If this Remove()
  // is not called, the following Set() has the effect
  // of removing (!) the key.
  if (inInfo)
  {
    inInfo->Remove(svtkDataObject::DATA_OBJECT());
    inInfo->Set(svtkDataObject::DATA_OBJECT(), dobj);

    svtkTrivialProducer::FillOutputDataInformation(dobj, inInfo);
  }

  request->Set(REQUEST_DATA_OBJECT());
  for (int i = 0; i < outInfoVec->GetNumberOfInformationObjects(); ++i)
  {
    outInfoVec->GetInformationObject(i)->Set(SUPPRESS_RESET_PI(), 1);
  }
  this->Superclass::ExecuteDataObject(request, inInfoVec, outInfoVec);
  for (int i = 0; i < outInfoVec->GetNumberOfInformationObjects(); ++i)
  {
    outInfoVec->GetInformationObject(i)->Remove(SUPPRESS_RESET_PI());
  }
  request->Remove(REQUEST_DATA_OBJECT());

  request->Set(REQUEST_INFORMATION());

  this->Superclass::ExecuteInformation(request, inInfoVec, outInfoVec);
  request->Remove(REQUEST_INFORMATION());

  int storedPiece = -1;
  int storedNumPieces = -1;
  for (int m = 0; m < this->Algorithm->GetNumberOfOutputPorts(); ++m)
  {
    svtkInformation* info = outInfoVec->GetInformationObject(m);
    // Update the whole thing
    if (info->Has(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
    {
      int extent[6] = { 0, -1, 0, -1, 0, -1 };
      info->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);
      info->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), extent, 6);
      storedPiece = info->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
      storedNumPieces = info->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
      info->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
      svtkDebugMacro(<< "UPDATE_PIECE_NUMBER() 0"
                    << " " << info);
      info->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
    }
  }

  request->Set(REQUEST_UPDATE_EXTENT());
  this->CallAlgorithm(request, svtkExecutive::RequestUpstream, inInfoVec, outInfoVec);
  request->Remove(REQUEST_UPDATE_EXTENT());

  request->Set(REQUEST_DATA());
  this->Superclass::ExecuteData(request, inInfoVec, outInfoVec);
  request->Remove(REQUEST_DATA());

  for (int m = 0; m < this->Algorithm->GetNumberOfOutputPorts(); ++m)
  {
    svtkInformation* info = outInfoVec->GetInformationObject(m);
    if (storedPiece != -1)
    {
      info->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), storedNumPieces);
      svtkDebugMacro(<< "UPDATE_PIECE_NUMBER() 0"
                    << " " << info);
      info->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), storedPiece);
    }
  }

  outputs.resize(outInfoVec->GetNumberOfInformationObjects());
  for (unsigned i = 0; i < outputs.size(); ++i)
  {
    svtkDataObject* output = svtkDataObject::GetData(outInfoVec, i);
    if (output)
    {
      svtkDataObject* outputCopy = output->NewInstance();
      outputCopy->ShallowCopy(output);
      outputs[i] = outputCopy;
    }
  }
  return outputs;
}

//----------------------------------------------------------------------------
int svtkCompositeDataPipeline::NeedToExecuteData(
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

  // We need to check the requested update extent.  Get the output
  // port information and data information.  We do not need to check
  // existence of values because it has already been verified by
  // VerifyOutputInformation.
  svtkInformation* outInfo = outInfoVec->GetInformationObject(outputPort);
  svtkDataObject* dataObject = outInfo->Get(svtkDataObject::DATA_OBJECT());

  // If the output is not a composite dataset, let the superclass handle
  // NeedToExecuteData
  if (!svtkCompositeDataSet::SafeDownCast(dataObject))
  {
    return this->Superclass::NeedToExecuteData(outputPort, inInfoVec, outInfoVec);
  }

  // First do the basic checks.
  if (this->svtkDemandDrivenPipeline::NeedToExecuteData(outputPort, inInfoVec, outInfoVec))
  {
    return 1;
  }

  // Now handle composite stuff.

  svtkInformation* dataInfo = dataObject->GetInformation();

  // Check the unstructured extent.  If we do not have the requested
  // piece, we need to execute.
  int updateNumberOfPieces = outInfo->Get(UPDATE_NUMBER_OF_PIECES());
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
    int updatePiece = outInfo->Get(UPDATE_PIECE_NUMBER());
    if (dataPiece != updatePiece)
    {
      return 1;
    }
  }

  if (this->NeedToExecuteBasedOnTime(outInfo, dataObject))
  {
    return 1;
  }

  if (this->NeedToExecuteBasedOnCompositeIndices(outInfo))
  {
    return 1;
  }

  // We do not need to execute.
  return 0;
}

//----------------------------------------------------------------------------
int svtkCompositeDataPipeline::NeedToExecuteBasedOnCompositeIndices(svtkInformation* outInfo)
{
  if (outInfo->Has(UPDATE_COMPOSITE_INDICES()))
  {
    if (!outInfo->Has(DATA_COMPOSITE_INDICES()))
    {
      return 1;
    }
    unsigned int* requested_ids =
      reinterpret_cast<unsigned int*>(outInfo->Get(UPDATE_COMPOSITE_INDICES()));
    unsigned int* existing_ids =
      reinterpret_cast<unsigned int*>(outInfo->Get(DATA_COMPOSITE_INDICES()));
    int length_req = outInfo->Length(UPDATE_COMPOSITE_INDICES());
    int length_ex = outInfo->Length(DATA_COMPOSITE_INDICES());

    if (length_req > length_ex)
    {
      // we are requesting more blocks than those generated.
      return 1;
    }
    int ri = 0, ei = 0;
    // NOTE: We are relying on the fact that both these id lists are sorted to
    // do a more efficient comparison.
    for (; ri < length_req; ri++)
    {
      while (ei < length_ex && existing_ids[ei] < requested_ids[ri])
      {
        ei++;
      }
      if (ei >= length_ex)
      {
        // we ran beyond the existing length.
        return 1;
      }
      if (existing_ids[ei] != requested_ids[ri])
      {
        return 1;
      }
    }
  }
  else
  {
    if (outInfo->Has(DATA_COMPOSITE_INDICES()))
    {
      // earlier request asked for a some blocks, but the new request is asking
      // for everything, so re-execute.
      return 1;
    }
  }
  return 0;
}

//----------------------------------------------------------------------------
int svtkCompositeDataPipeline::ForwardUpstream(svtkInformation* request)
{
  svtkDebugMacro(<< "ForwardUpstream");

  // Do not forward upstream if the input is shared with another
  // executive.
  if (this->SharedInputInformation)
  {
    return 1;
  }

  if (!this->Algorithm->ModifyRequest(request, BeforeForward))
  {
    return 0;
  }
  int port = request->Get(FROM_OUTPUT_PORT());

  // Forward the request upstream through all input connections.
  int result = 1;
  for (int i = 0; i < this->GetNumberOfInputPorts(); ++i)
  {
    int nic = this->Algorithm->GetNumberOfInputConnections(i);
    svtkInformationVector* inVector = this->GetInputInformation()[i];
    for (int j = 0; j < nic; ++j)
    {
      svtkInformation* info = inVector->GetInformationObject(j);
      // Get the executive producing this input.  If there is none, then
      // it is a nullptr input.
      svtkExecutive* e;
      int producerPort;
      svtkExecutive::PRODUCER()->Get(info, e, producerPort);
      if (e)
      {
        request->Set(FROM_OUTPUT_PORT(), producerPort);
        if (!e->ProcessRequest(request, e->GetInputInformation(), e->GetOutputInformation()))
        {
          result = 0;
        }
        request->Set(FROM_OUTPUT_PORT(), port);
      }
    }
  }

  if (!this->Algorithm->ModifyRequest(request, AfterForward))
  {
    return 0;
  }

  return result;
}

//----------------------------------------------------------------------------
int svtkCompositeDataPipeline::ForwardUpstream(int i, int j, svtkInformation* request)
{
  // Do not forward upstream if input information is shared.
  if (this->SharedInputInformation)
  {
    return 1;
  }

  if (!this->Algorithm->ModifyRequest(request, BeforeForward))
  {
    return 0;
  }

  int result = 1;
  if (svtkExecutive* e = this->GetInputExecutive(i, j))
  {
    svtkAlgorithmOutput* input = this->Algorithm->GetInputConnection(i, j);
    int port = request->Get(FROM_OUTPUT_PORT());
    request->Set(FROM_OUTPUT_PORT(), input->GetIndex());
    if (!e->ProcessRequest(request, e->GetInputInformation(), e->GetOutputInformation()))
    {
      result = 0;
    }
    request->Set(FROM_OUTPUT_PORT(), port);
  }

  if (!this->Algorithm->ModifyRequest(request, AfterForward))
  {
    return 0;
  }

  return result;
}
//----------------------------------------------------------------------------
void svtkCompositeDataPipeline::CopyDefaultInformation(svtkInformation* request, int direction,
  svtkInformationVector** inInfoVec, svtkInformationVector* outInfoVec)
{
  this->Superclass::CopyDefaultInformation(request, direction, inInfoVec, outInfoVec);

  if (request->Has(REQUEST_INFORMATION()) || request->Has(REQUEST_TIME_DEPENDENT_INFORMATION()))
  {
    if (this->GetNumberOfInputPorts() > 0)
    {
      if (svtkInformation* inInfo = inInfoVec[0]->GetInformationObject(0))
      {
        // Copy information from the first input to all outputs.
        for (int i = 0; i < outInfoVec->GetNumberOfInformationObjects(); ++i)
        {
          svtkInformation* outInfo = outInfoVec->GetInformationObject(i);
          outInfo->CopyEntry(inInfo, COMPOSITE_DATA_META_DATA());
        }
      }
    }
  }

  if (request->Has(REQUEST_UPDATE_EXTENT()))
  {
    int outputPort = -1;
    if (request->Has(FROM_OUTPUT_PORT()))
    {
      outputPort = request->Get(FROM_OUTPUT_PORT());
    }

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
          inInfo->CopyEntry(outInfo, UPDATE_COMPOSITE_INDICES());
          inInfo->CopyEntry(outInfo, LOAD_REQUESTED_BLOCKS());
        }
      }
    }

    // Find the port that has a data that we will iterator over.
    // If there is one, make sure that we use piece extent for
    // that port. Composite data pipeline works with piece extents
    // only.
    int compositePort;
    if (this->ShouldIterateOverInput(inInfoVec, compositePort))
    {
      // Get the output port from which to copy the extent.
      outputPort = -1;
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

        // Loop over all connections on this input port.
        int numInConnections = inInfoVec[compositePort]->GetNumberOfInformationObjects();
        for (int j = 0; j < numInConnections; j++)
        {
          // Get the pipeline information for this input connection.
          svtkInformation* inInfo = inInfoVec[compositePort]->GetInformationObject(j);

          svtkDebugMacro(<< "CopyEntry UPDATE_PIECE_NUMBER() " << outInfo->Get(UPDATE_PIECE_NUMBER())
                        << " " << outInfo);

          inInfo->CopyEntry(outInfo, UPDATE_PIECE_NUMBER());
          inInfo->CopyEntry(outInfo, UPDATE_NUMBER_OF_PIECES());
          inInfo->CopyEntry(outInfo, UPDATE_NUMBER_OF_GHOST_LEVELS());
          inInfo->CopyEntry(outInfo, UPDATE_EXTENT_INITIALIZED());
          inInfo->CopyEntry(outInfo, LOAD_REQUESTED_BLOCKS());
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
void svtkCompositeDataPipeline::ResetPipelineInformation(int port, svtkInformation* info)
{
  if (info->Has(SUPPRESS_RESET_PI()))
  {
    return;
  }

  this->Superclass::ResetPipelineInformation(port, info);
  info->Remove(COMPOSITE_DATA_META_DATA());
  info->Remove(UPDATE_COMPOSITE_INDICES());
  info->Remove(LOAD_REQUESTED_BLOCKS());
}

//----------------------------------------------------------------------------
void svtkCompositeDataPipeline::PushInformation(svtkInformation* inInfo)
{
  svtkDebugMacro(<< "PushInformation " << inInfo);
  this->InformationCache->CopyEntry(inInfo, WHOLE_EXTENT());
}

//----------------------------------------------------------------------------
void svtkCompositeDataPipeline::PopInformation(svtkInformation* inInfo)
{
  svtkDebugMacro(<< "PopInformation " << inInfo);
  inInfo->CopyEntry(this->InformationCache, WHOLE_EXTENT());
}

//----------------------------------------------------------------------------
int svtkCompositeDataPipeline::CheckCompositeData(
  svtkInformation*, svtkInformationVector** inInfoVec, svtkInformationVector* outInfoVec)
{
  // If this is a simple filter but has composite input,
  // create a composite output.
  int compositePort;

  if (this->ShouldIterateOverInput(inInfoVec, compositePort))
  {
    // This checks if each output port's data object is a composite data object.
    // If it is not already, then we need to create a composite data object for the outputs
    // on that port to be placed into.  If the output is already a composite data object, it
    // is assumed that the composite data pipeline is being re-run and the data object from
    // the last run can be re-used.
    bool needsToCreateCompositeOutput = false;
    for (int port = 0; port < outInfoVec->GetNumberOfInformationObjects(); ++port)
    {
      svtkInformation* outInfo = outInfoVec->GetInformationObject(port);

      svtkDataObject* doOutput = outInfo->Get(svtkDataObject::DATA_OBJECT());
      svtkCompositeDataSet* portOutput = svtkCompositeDataSet::SafeDownCast(doOutput);
      if (!portOutput)
      {
        needsToCreateCompositeOutput = true;
        break;
      }
    }
    if (needsToCreateCompositeOutput)
    {
      // Create the output objects
      std::vector<svtkSmartPointer<svtkDataObject> > output = this->CreateOutputCompositeDataSet(
        svtkCompositeDataSet::SafeDownCast(this->GetInputData(compositePort, 0, inInfoVec)),
        compositePort, outInfoVec->GetNumberOfInformationObjects());

      // For each port, assign the created output object back to the output information
      for (int port = 0; port < outInfoVec->GetNumberOfInformationObjects(); ++port)
      {
        svtkInformation* outInfo = outInfoVec->GetInformationObject(port);
        svtkDebugMacro(<< "CheckCompositeData created " << output[port]->GetClassName()
                      << "output for port " << port);

        outInfo->Set(svtkDataObject::DATA_OBJECT(), output[port]);
        // Copy extent type to the output port information because
        // CreateOutputCompositeDataSet() changes it and some algorithms need it.
        this->GetAlgorithm()->GetOutputPortInformation(port)->Set(
          svtkDataObject::DATA_EXTENT_TYPE(), output[port]->GetExtentType());
      }
    }
    return 1;
  }
  // Otherwise, create a simple output
  else
  {
    for (int port = 0; port < outInfoVec->GetNumberOfInformationObjects(); ++port)
    {
      if (!this->Superclass::CheckDataObject(port, outInfoVec))
      {
        return 0;
      }
    }
    return 1;
  }
}

//----------------------------------------------------------------------------
svtkDataObject* svtkCompositeDataPipeline::GetCompositeInputData(
  int port, int index, svtkInformationVector** inInfoVec)
{
  if (!inInfoVec[port])
  {
    return nullptr;
  }
  svtkInformation* info = inInfoVec[port]->GetInformationObject(index);
  if (!info)
  {
    return nullptr;
  }
  return info->Get(svtkDataObject::DATA_OBJECT());
}

//----------------------------------------------------------------------------
svtkDataObject* svtkCompositeDataPipeline::GetCompositeOutputData(int port)
{
  if (!this->OutputPortIndexInRange(port, "get data for"))
  {
    return nullptr;
  }

  // Check that the given output port has a valid data object.
  svtkDebugMacro(<< "GetCompositeOutputData calling CheckCompositeData ");

  this->CheckCompositeData(nullptr, this->GetInputInformation(), this->GetOutputInformation());

  // Return the data object.
  if (svtkInformation* info = this->GetOutputInformation(port))
  {
    return info->Get(svtkDataObject::DATA_OBJECT());
  }
  return nullptr;
}

//----------------------------------------------------------------------------
std::vector<svtkSmartPointer<svtkDataObject> > svtkCompositeDataPipeline::CreateOutputCompositeDataSet(
  svtkCompositeDataSet* input, int compositePort, int numOutputPorts)
{
  // pre: the algorithm is a non-composite algorithm.
  // pre: the question is
  //      whether to create svtkHierarchicalBoxDataSet or svtkMultiBlockDataSet.
  std::vector<svtkSmartPointer<svtkDataObject> > outputVector;

  if (input->IsA("svtkHierarchicalBoxDataSet") || input->IsA("svtkOverlappingAMR") ||
    input->IsA("svtkNonOverlappingAMR"))
  {
    svtkSmartPointer<svtkUniformGrid> tempInput = svtkSmartPointer<svtkUniformGrid>::New();

    // Check if the algorithm can accept UniformGrid on the input port.
    svtkInformation* inPortInfo = this->Algorithm->GetInputPortInformation(compositePort);
    const char* inputType = inPortInfo->Get(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
    if (!tempInput->IsA(inputType))
    {
      for (int i = 0; i < numOutputPorts; ++i)
      {
        outputVector.push_back(svtkSmartPointer<svtkMultiBlockDataSet>::New());
      }
    }
    else
    {

      svtkInformation* inInfo = this->GetInputInformation(compositePort, 0);
      svtkSmartPointer<svtkDataObject> curInput = inInfo->Get(svtkDataObject::DATA_OBJECT());

      svtkSmartPointer<svtkInformation> request = svtkSmartPointer<svtkInformation>::New();
      request->Set(FROM_OUTPUT_PORT(), PRODUCER()->GetPort(inInfo));

      // Set the input to be svtkUniformGrid.
      inInfo->Remove(svtkDataObject::DATA_OBJECT());
      inInfo->Set(svtkDataObject::DATA_OBJECT(), tempInput);
      // The request is forwarded upstream through the pipeline.
      request->Set(svtkExecutive::FORWARD_DIRECTION(), svtkExecutive::RequestUpstream);
      // Algorithms process this request after it is forwarded.
      request->Set(svtkExecutive::ALGORITHM_AFTER_FORWARD(), 1);
      request->Set(REQUEST_DATA_OBJECT());
      for (int i = 0; i < numOutputPorts; ++i)
      {
        svtkInformation* outInfo = this->GetOutputInformation(i);
        outInfo->Set(SUPPRESS_RESET_PI(), 1);
      }
      this->Superclass::ExecuteDataObject(
        request, this->GetInputInformation(), this->GetOutputInformation());
      request->Remove(REQUEST_DATA_OBJECT());

      // Restore input.
      inInfo->Remove(svtkDataObject::DATA_OBJECT());
      inInfo->Set(svtkDataObject::DATA_OBJECT(), curInput);

      for (int i = 0; i < numOutputPorts; ++i)
      {
        svtkInformation* outInfo = this->GetOutputInformation(i);
        outInfo->Remove(SUPPRESS_RESET_PI());
        // check the type of output data object created by the algorithm.
        svtkDataObject* curOutput = outInfo->Get(svtkDataObject::DATA_OBJECT());
        if (!curOutput->IsA("svtkUniformGrid"))
        {
          outputVector.push_back(svtkSmartPointer<svtkMultiBlockDataSet>::New());
        }
        else
        {
          svtkSmartPointer<svtkDataObject> newInstance;
          newInstance.TakeReference(input->NewInstance());
          outputVector.push_back(newInstance);
        }
      }
    }
  }
  else
  {
    for (int i = 0; i < numOutputPorts; ++i)
    {
      svtkSmartPointer<svtkDataObject> newInstance;
      newInstance.TakeReference(input->NewInstance());
      outputVector.push_back(newInstance);
    }
  }
  return outputVector;
}

//----------------------------------------------------------------------------
void svtkCompositeDataPipeline::MarkOutputsGenerated(
  svtkInformation* request, svtkInformationVector** inInfoVec, svtkInformationVector* outInfoVec)
{
  this->Superclass::MarkOutputsGenerated(request, inInfoVec, outInfoVec);

  for (int i = 0; i < outInfoVec->GetNumberOfInformationObjects(); ++i)
  {
    svtkInformation* outInfo = outInfoVec->GetInformationObject(i);
    svtkDataObject* data = outInfo->Get(svtkDataObject::DATA_OBJECT());
    if (data && !outInfo->Get(DATA_NOT_GENERATED()))
    {
      if (outInfo->Has(UPDATE_COMPOSITE_INDICES()))
      {
        size_t count = outInfo->Length(UPDATE_COMPOSITE_INDICES());
        int* indices = new int[count];
        // assume the source produced the blocks it was asked for:
        // the indices received are what was requested
        outInfo->Get(UPDATE_COMPOSITE_INDICES(), indices);
        outInfo->Set(DATA_COMPOSITE_INDICES(), indices, static_cast<int>(count));
        delete[] indices;
      }
      else
      {
        outInfo->Remove(DATA_COMPOSITE_INDICES());
      }
    }
  }
}

//----------------------------------------------------------------------------
void svtkCompositeDataPipeline::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
