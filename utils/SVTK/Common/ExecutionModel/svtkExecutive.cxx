/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExecutive.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExecutive.h"

#include "svtkAlgorithm.h"
#include "svtkAlgorithmOutput.h"
#include "svtkDataObject.h"
#include "svtkGarbageCollector.h"
#include "svtkInformation.h"
#include "svtkInformationExecutivePortKey.h"
#include "svtkInformationExecutivePortVectorKey.h"
#include "svtkInformationIntegerKey.h"
#include "svtkInformationIterator.h"
#include "svtkInformationKeyVectorKey.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"

#include <sstream>
#include <vector>

#include "svtkCompositeDataPipeline.h"

svtkInformationKeyMacro(svtkExecutive, ALGORITHM_AFTER_FORWARD, Integer);
svtkInformationKeyMacro(svtkExecutive, ALGORITHM_BEFORE_FORWARD, Integer);
svtkInformationKeyMacro(svtkExecutive, ALGORITHM_DIRECTION, Integer);
svtkInformationKeyMacro(svtkExecutive, CONSUMERS, ExecutivePortVector);
svtkInformationKeyMacro(svtkExecutive, FORWARD_DIRECTION, Integer);
svtkInformationKeyMacro(svtkExecutive, FROM_OUTPUT_PORT, Integer);
svtkInformationKeyMacro(svtkExecutive, KEYS_TO_COPY, KeyVector);
svtkInformationKeyMacro(svtkExecutive, PRODUCER, ExecutivePort);

//----------------------------------------------------------------------------
class svtkExecutiveInternals
{
public:
  std::vector<svtkInformationVector*> InputInformation;
  svtkExecutiveInternals();
  ~svtkExecutiveInternals();
  svtkInformationVector** GetInputInformation(int newNumberOfPorts);
};

//----------------------------------------------------------------------------
svtkExecutiveInternals::svtkExecutiveInternals() = default;

//----------------------------------------------------------------------------
svtkExecutiveInternals::~svtkExecutiveInternals()
{
  // Delete all the input information vectors.
  for (std::vector<svtkInformationVector*>::iterator i = this->InputInformation.begin();
       i != this->InputInformation.end(); ++i)
  {
    if (svtkInformationVector* v = *i)
    {
      v->Delete();
    }
  }
}

//----------------------------------------------------------------------------
svtkInformationVector** svtkExecutiveInternals::GetInputInformation(int newNumberOfPorts)
{
  // Adjust the number of vectors.
  int oldNumberOfPorts = static_cast<int>(this->InputInformation.size());
  if (newNumberOfPorts > oldNumberOfPorts)
  {
    // Create new vectors.
    this->InputInformation.resize(newNumberOfPorts, nullptr);
    for (int i = oldNumberOfPorts; i < newNumberOfPorts; ++i)
    {
      this->InputInformation[i] = svtkInformationVector::New();
    }
  }
  else if (newNumberOfPorts < oldNumberOfPorts)
  {
    // Delete old vectors.
    for (int i = newNumberOfPorts; i < oldNumberOfPorts; ++i)
    {
      if (svtkInformationVector* v = this->InputInformation[i])
      {
        // Set the pointer to nullptr first to avoid reporting of the
        // entry if deleting the vector causes a garbage collection
        // reference walk.
        this->InputInformation[i] = nullptr;
        v->Delete();
      }
    }
    this->InputInformation.resize(newNumberOfPorts);
  }

  // Return the array of information vector pointers.
  if (newNumberOfPorts > 0)
  {
    return &this->InputInformation[0];
  }
  else
  {
    return nullptr;
  }
}

//----------------------------------------------------------------------------
svtkExecutive::svtkExecutive()
{
  this->ExecutiveInternal = new svtkExecutiveInternals;
  this->OutputInformation = svtkInformationVector::New();
  this->Algorithm = nullptr;
  this->InAlgorithm = 0;
  this->SharedInputInformation = nullptr;
  this->SharedOutputInformation = nullptr;
}

//----------------------------------------------------------------------------
svtkExecutive::~svtkExecutive()
{
  this->SetAlgorithm(nullptr);
  if (this->OutputInformation)
  {
    this->OutputInformation->Delete();
  }
  delete this->ExecutiveInternal;
}

//----------------------------------------------------------------------------
void svtkExecutive::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->Algorithm)
  {
    os << indent << "Algorithm: " << this->Algorithm << "\n";
  }
  else
  {
    os << indent << "Algorithm: (none)\n";
  }
}

//----------------------------------------------------------------------------
void svtkExecutive::Register(svtkObjectBase* o)
{
  this->RegisterInternal(o, 1);
}

//----------------------------------------------------------------------------
void svtkExecutive::UnRegister(svtkObjectBase* o)
{
  this->UnRegisterInternal(o, 1);
}

//----------------------------------------------------------------------------
void svtkExecutive::SetAlgorithm(svtkAlgorithm* newAlgorithm)
{
  svtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting Algorithm to "
                << newAlgorithm);
  svtkAlgorithm* oldAlgorithm = this->Algorithm;
  if (oldAlgorithm != newAlgorithm)
  {
    if (newAlgorithm)
    {
      newAlgorithm->Register(this);
    }
    this->Algorithm = newAlgorithm;
    if (oldAlgorithm)
    {
      oldAlgorithm->UnRegister(this);
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------------
svtkAlgorithm* svtkExecutive::GetAlgorithm()
{
  return this->Algorithm;
}

//----------------------------------------------------------------------------
svtkInformationVector** svtkExecutive::GetInputInformation()
{
  // Use the shared input information vector if any is set.
  if (this->SharedInputInformation)
  {
    return this->SharedInputInformation;
  }

  // Use this executive's input information vector.
  if (this->Algorithm)
  {
    int numPorts = this->Algorithm->GetNumberOfInputPorts();
    return this->ExecutiveInternal->GetInputInformation(numPorts);
  }
  else
  {
    return this->ExecutiveInternal->GetInputInformation(0);
  }
}

//----------------------------------------------------------------------------
svtkInformation* svtkExecutive::GetInputInformation(int port, int connection)
{
  if (!this->InputPortIndexInRange(port, "get connected input information from"))
  {
    return nullptr;
  }
  svtkInformationVector* inVector = this->GetInputInformation()[port];
  return inVector->GetInformationObject(connection);
}

//----------------------------------------------------------------------------
svtkInformationVector* svtkExecutive::GetInputInformation(int port)
{
  if (!this->InputPortIndexInRange(port, "get input information vector from"))
  {
    return nullptr;
  }
  return this->GetInputInformation()[port];
}

//----------------------------------------------------------------------------
svtkInformationVector* svtkExecutive::GetOutputInformation()
{
  // Use the shared output information vector if any is set.
  if (this->SharedOutputInformation)
  {
    return this->SharedOutputInformation;
  }

  // Use this executive's output information vector.
  if (!this->Algorithm)
  {
    return nullptr;
  }
  // Set the length of the vector to match the number of ports.
  int oldNumberOfPorts = this->OutputInformation->GetNumberOfInformationObjects();
  this->OutputInformation->SetNumberOfInformationObjects(this->GetNumberOfOutputPorts());

  // For any new information obects, set the executive pointer and
  // port number on the information object to tell it what produces
  // it.
  int nop = this->Algorithm->GetNumberOfOutputPorts();
  for (int i = oldNumberOfPorts; i < nop; ++i)
  {
    svtkInformation* info = this->OutputInformation->GetInformationObject(i);
    svtkExecutive::PRODUCER()->Set(info, this, i);
  }

  return this->OutputInformation;
}

//----------------------------------------------------------------------------
svtkInformation* svtkExecutive::GetOutputInformation(int port)
{
  return this->GetOutputInformation()->GetInformationObject(port);
}

//----------------------------------------------------------------------------
svtkExecutive* svtkExecutive::GetInputExecutive(int port, int index)
{
  if (index < 0 || index >= this->GetNumberOfInputConnections(port))
  {
    svtkErrorMacro("Attempt to get executive for connection index "
      << index << " on input port " << port << " of algorithm " << this->Algorithm->GetClassName()
      << "(" << this->Algorithm << "), which has " << this->GetNumberOfInputConnections(port)
      << " connections.");
    return nullptr;
  }
  if (svtkAlgorithmOutput* input = this->Algorithm->GetInputConnection(port, index))
  {
    return input->GetProducer()->GetExecutive();
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void svtkExecutive::ReportReferences(svtkGarbageCollector* collector)
{
  // Report reference to our algorithm.
  svtkGarbageCollectorReport(collector, this->Algorithm, "Algorithm");

  for (int i = 0; i < int(this->ExecutiveInternal->InputInformation.size()); ++i)
  {
    svtkGarbageCollectorReport(
      collector, this->ExecutiveInternal->InputInformation[i], "Input Information Vector");
  }

  svtkGarbageCollectorReport(collector, this->OutputInformation, "Output Information Vector");
  this->Superclass::ReportReferences(collector);
}

//----------------------------------------------------------------------------
svtkTypeBool svtkExecutive::Update()
{
  if (this->Algorithm->GetNumberOfOutputPorts())
  {
    return this->Update(0);
  }
  return this->Update(-1);
}

//----------------------------------------------------------------------------
svtkTypeBool svtkExecutive::Update(int)
{
  svtkErrorMacro("This class does not implement Update.");
  return 0;
}

//----------------------------------------------------------------------------
int svtkExecutive::GetNumberOfInputPorts()
{
  if (this->Algorithm)
  {
    return this->Algorithm->GetNumberOfInputPorts();
  }
  return 0;
}

//----------------------------------------------------------------------------
int svtkExecutive::GetNumberOfOutputPorts()
{
  if (this->Algorithm)
  {
    return this->Algorithm->GetNumberOfOutputPorts();
  }
  return 0;
}

//----------------------------------------------------------------------------
int svtkExecutive::GetNumberOfInputConnections(int port)
{
  svtkInformationVector* inputs = this->GetInputInformation(port);
  if (inputs)
  {
    return inputs->GetNumberOfInformationObjects();
  }
  return 0;
}

//----------------------------------------------------------------------------
int svtkExecutive::InputPortIndexInRange(int port, const char* action)
{
  // Make sure the algorithm is set.
  if (!this->Algorithm)
  {
    svtkErrorMacro("Attempt to " << (action ? action : "access") << " input port index " << port
                                << " with no algorithm set.");
    return 0;
  }

  // Make sure the index of the input port is in range.
  if (port < 0 || port >= this->Algorithm->GetNumberOfInputPorts())
  {
    svtkErrorMacro("Attempt to " << (action ? action : "access") << " input port index " << port
                                << " for algorithm " << this->Algorithm->GetClassName() << "("
                                << this->Algorithm << "), which has "
                                << this->Algorithm->GetNumberOfInputPorts() << " input ports.");
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkExecutive::OutputPortIndexInRange(int port, const char* action)
{
  // Make sure the algorithm is set.
  if (!this->Algorithm)
  {
    svtkErrorMacro("Attempt to " << (action ? action : "access") << " output port index " << port
                                << " with no algorithm set.");
    return 0;
  }

  // Make sure the index of the output port is in range.
  if (port < 0 || port >= this->Algorithm->GetNumberOfOutputPorts())
  {
    svtkErrorMacro("Attempt to " << (action ? action : "access") << " output port index " << port
                                << " for algorithm " << this->Algorithm->GetClassName() << "("
                                << this->Algorithm << "), which has "
                                << this->Algorithm->GetNumberOfOutputPorts() << " output ports.");
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
// svtkAlgorithmOutput* svtkExecutive::GetProducerPort(svtkDataObject* d)
// {
//   if (!this->Algorithm)
//     {
//     return 0;
//     }

//   int numPorts = this->GetNumberOfOutputPorts();
//   for (int i=0; i<numPorts; i++)
//     {
//     svtkInformation* info = this->GetOutputInformation(i);
//     if (info->Has(svtkDataObject::DATA_OBJECT()) &&
//         info->Get(svtkDataObject::DATA_OBJECT()) == d)
//       {
//       return this->Algorithm->GetOutputPort(port);
//       }
//     }
//   return 0;

// }

//----------------------------------------------------------------------------
void svtkExecutive::SetSharedInputInformation(svtkInformationVector** inInfoVec)
{
  this->SharedInputInformation = inInfoVec;
}

//----------------------------------------------------------------------------
void svtkExecutive::SetSharedOutputInformation(svtkInformationVector* outInfoVec)
{
  this->SharedOutputInformation = outInfoVec;
}

//----------------------------------------------------------------------------
svtkDataObject* svtkExecutive::GetOutputData(int port)
{
  if (!this->OutputPortIndexInRange(port, "get data for"))
  {
    return nullptr;
  }

  svtkInformation* info = this->GetOutputInformation(port);
  if (!info)
  {
    return nullptr;
  }

  // for backward compatibility we bring Outputs up to date if they do not
  // already exist
  if (!this->InAlgorithm && !info->Has(svtkDataObject::DATA_OBJECT()))
  {
    // Bring the data object up to date only if it isn't already there
    this->UpdateDataObject();
  }

  // Return the data object.
  return info->Get(svtkDataObject::DATA_OBJECT());
}

//----------------------------------------------------------------------------
void svtkExecutive::SetOutputData(int newPort, svtkDataObject* newOutput)
{
  svtkInformation* info = this->GetOutputInformation(newPort);
  this->SetOutputData(newPort, newOutput, info);
}

//----------------------------------------------------------------------------
void svtkExecutive::SetOutputData(int newPort, svtkDataObject* newOutput, svtkInformation* info)
{
  if (info)
  {
    svtkDataObject* currentOutput = info->Get(svtkDataObject::DATA_OBJECT());
    if (newOutput != currentOutput)
    {
      info->Set(svtkDataObject::DATA_OBJECT(), newOutput);

      // Output has changed.  Reset the pipeline information.
      this->ResetPipelineInformation(newPort, info);
    }
  }
  else
  {
    svtkErrorMacro("Could not set output on port " << newPort << ".");
  }
}

//----------------------------------------------------------------------------
svtkDataObject* svtkExecutive::GetInputData(int port, int index)
{
  if (index < 0 || index >= this->GetNumberOfInputConnections(port))
  {
    return nullptr;
  }

  svtkInformationVector* inVector = this->GetInputInformation()[port];
  svtkInformation* info = inVector->GetInformationObject(index);
  svtkExecutive* e;
  int producerPort;
  svtkExecutive::PRODUCER()->Get(info, e, producerPort);
  if (e)
  {
    return e->GetOutputData(producerPort);
  }
  else
  {
    return nullptr;
  }
}

//----------------------------------------------------------------------------
svtkDataObject* svtkExecutive::GetInputData(int port, int index, svtkInformationVector** inInfoVec)
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
svtkTypeBool svtkExecutive::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inInfo, svtkInformationVector* outInfo)
{
  if (request->Has(FORWARD_DIRECTION()))
  {
    // Request will be forwarded.
    if (request->Get(FORWARD_DIRECTION()) == svtkExecutive::RequestUpstream)
    {
      if (this->Algorithm && request->Get(ALGORITHM_BEFORE_FORWARD()))
      {
        if (!this->CallAlgorithm(request, svtkExecutive::RequestUpstream, inInfo, outInfo))
        {
          return 0;
        }
      }
      if (!this->ForwardUpstream(request))
      {
        return 0;
      }
      if (this->Algorithm && request->Get(ALGORITHM_AFTER_FORWARD()))
      {
        if (!this->CallAlgorithm(request, svtkExecutive::RequestDownstream, inInfo, outInfo))
        {
          return 0;
        }
      }
    }
    if (request->Get(FORWARD_DIRECTION()) == svtkExecutive::RequestDownstream)
    {
      svtkErrorMacro("Downstream forwarding not yet implemented.");
      return 0;
    }
  }
  else
  {
    // Request will not be forwarded.
    svtkErrorMacro("Non-forwarded requests are not yet implemented.");
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkExecutive::ComputePipelineMTime(
  svtkInformation*, svtkInformationVector**, svtkInformationVector*, int, svtkMTimeType*)
{
  // Demand-driven executives that use this request should implement
  // this method.
  svtkErrorMacro("ComputePipelineMTime not implemented for this executive.");
  return 0;
}

//----------------------------------------------------------------------------
int svtkExecutive::ForwardDownstream(svtkInformation*)
{
  // Do not forward downstream if the output is shared with another
  // executive.
  if (this->SharedOutputInformation)
  {
    return 1;
  }

  // Forwarding downstream is not yet implemented.
  svtkErrorMacro("ForwardDownstream not yet implemented.");
  return 0;
}

//----------------------------------------------------------------------------
int svtkExecutive::ForwardUpstream(svtkInformation* request)
{
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
        int port = request->Get(FROM_OUTPUT_PORT());
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
void svtkExecutive::CopyDefaultInformation(svtkInformation* request, int direction,
  svtkInformationVector** inInfoVec, svtkInformationVector* outInfoVec)
{
  if (direction == svtkExecutive::RequestDownstream)
  {
    // Copy information from the first input to all outputs.
    if (this->GetNumberOfInputPorts() > 0 && inInfoVec[0]->GetNumberOfInformationObjects() > 0)
    {
      svtkInformationKey** keys = request->Get(KEYS_TO_COPY());
      int length = request->Length(KEYS_TO_COPY());
      svtkInformation* inInfo = inInfoVec[0]->GetInformationObject(0);

      svtkSmartPointer<svtkInformationIterator> infoIter =
        svtkSmartPointer<svtkInformationIterator>::New();
      infoIter->SetInformationWeak(inInfo);

      int oiobj = outInfoVec->GetNumberOfInformationObjects();
      for (int i = 0; i < oiobj; ++i)
      {
        svtkInformation* outInfo = outInfoVec->GetInformationObject(i);
        for (int j = 0; j < length; ++j)
        {
          // Copy the entry.
          outInfo->CopyEntry(inInfo, keys[j]);

          // If the entry is a key vector, copy all the keys listed.
          if (svtkInformationKeyVectorKey* vkey = svtkInformationKeyVectorKey::SafeDownCast(keys[j]))
          {
            outInfo->CopyEntries(inInfo, vkey);
          }
        }

        // Give the keys an opportunity to copy themselves.
        infoIter->InitTraversal();
        while (!infoIter->IsDoneWithTraversal())
        {
          svtkInformationKey* key = infoIter->GetCurrentKey();
          key->CopyDefaultInformation(request, inInfo, outInfo);
          infoIter->GoToNextItem();
        }
      }
    }
  }
  else
  {
    // Get the output port from which the request was made.  Use zero
    // if output port was not specified.
    int outputPort = 0;
    if (request->Has(FROM_OUTPUT_PORT()))
    {
      outputPort = request->Get(FROM_OUTPUT_PORT());
      outputPort = outputPort == -1 ? 0 : outputPort;
    }

    // Copy information from the requesting output to all inputs.
    if (outputPort >= 0 && outputPort < outInfoVec->GetNumberOfInformationObjects())
    {
      svtkInformationKey** keys = request->Get(KEYS_TO_COPY());
      int length = request->Length(KEYS_TO_COPY());
      svtkInformation* outInfo = outInfoVec->GetInformationObject(outputPort);

      svtkSmartPointer<svtkInformationIterator> infoIter =
        svtkSmartPointer<svtkInformationIterator>::New();
      infoIter->SetInformationWeak(outInfo);

      for (int i = 0; i < this->GetNumberOfInputPorts(); ++i)
      {
        for (int j = 0; j < inInfoVec[i]->GetNumberOfInformationObjects(); ++j)
        {
          svtkInformation* inInfo = inInfoVec[i]->GetInformationObject(j);
          for (int k = 0; k < length; ++k)
          {
            // Copy the entry.
            inInfo->CopyEntry(outInfo, keys[k]);

            // If the entry is a key vector, copy all the keys listed.
            if (svtkInformationKeyVectorKey* vkey =
                  svtkInformationKeyVectorKey::SafeDownCast(keys[k]))
            {
              inInfo->CopyEntries(outInfo, vkey);
            }
          }

          // Give the keys an opportunity to copy themselves.
          infoIter->InitTraversal();
          while (!infoIter->IsDoneWithTraversal())
          {
            svtkInformationKey* key = infoIter->GetCurrentKey();
            key->CopyDefaultInformation(request, outInfo, inInfo);
            infoIter->GoToNextItem();
          }
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
int svtkExecutive::CallAlgorithm(svtkInformation* request, int direction,
  svtkInformationVector** inInfo, svtkInformationVector* outInfo)
{
  // Copy default information in the direction of information flow.
  this->CopyDefaultInformation(request, direction, inInfo, outInfo);

  // Invoke the request on the algorithm.
  this->InAlgorithm = 1;
  int result = this->Algorithm->ProcessRequest(request, inInfo, outInfo);
  this->InAlgorithm = 0;

  // If the algorithm failed report it now.
  if (!result)
  {
    svtkErrorMacro("Algorithm " << this->Algorithm->GetClassName() << "(" << this->Algorithm
                               << ") returned failure for request: " << *request);
  }

  return result;
}

//----------------------------------------------------------------------------
int svtkExecutive::CheckAlgorithm(const char* method, svtkInformation* request)
{
  if (this->InAlgorithm)
  {
    if (request)
    {
      std::ostringstream rqmsg;
      request->Print(rqmsg);
      svtkErrorMacro(<< method
                    << " invoked during another request.  "
                       "Returning failure to algorithm "
                    << this->Algorithm->GetClassName() << "(" << this->Algorithm
                    << ") for the recursive request:\n"
                    << rqmsg.str().c_str());
    }
    else
    {
      svtkErrorMacro(<< method
                    << " invoked during another request.  "
                       "Returning failure to algorithm "
                    << this->Algorithm->GetClassName() << "(" << this->Algorithm << ").");
    }

    // Tests should fail when this happens because there is a bug in
    // the code.
    if (getenv("DASHBOARD_TEST_FROM_CTEST") || getenv("DART_TEST_FROM_DART"))
    {
      abort();
    }
    return 0;
  }
  return 1;
}
