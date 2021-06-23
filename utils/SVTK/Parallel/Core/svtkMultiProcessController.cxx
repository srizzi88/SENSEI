/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMultiProcessController.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This will be the default.
#include "svtkMultiProcessController.h"

#include "svtkBoundingBox.h"
#include "svtkByteSwap.h"
#include "svtkCollection.h"
#include "svtkCommand.h"
#include "svtkDummyController.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkOutputWindow.h"
#include "svtkProcess.h"
#include "svtkProcessGroup.h"
#include "svtkSubCommunicator.h"
#include "svtkToolkits.h"
#include "svtkWeakPointer.h"

#include <list>
#include <unordered_map>
#include <vector>

//-----------------------------------------------------------------------------
// Stores internal members that cannot or should not be exposed in the header
// file (for example, because they use templated types).
class svtkMultiProcessController::svtkInternal
{
public:
  std::unordered_map<int, svtkProcessFunctionType> MultipleMethod;
  std::unordered_map<int, void*> MultipleData;

  class svtkRMICallback
  {
  public:
    unsigned long Id;
    svtkRMIFunctionType Function;
    void* LocalArgument;
  };

  typedef std::vector<svtkRMICallback> RMICallbackVector;

  // key == tag, value == vector of svtkRMICallback instances.
  typedef std::unordered_map<int, RMICallbackVector> RMICallbackMap;
  RMICallbackMap RMICallbacks;
};

//----------------------------------------------------------------------------
// An RMI function that will break the "ProcessRMIs" loop.
static void svtkMultiProcessControllerBreakRMI(
  void* localArg, void* remoteArg, int remoteArgLength, int svtkNotUsed(remoteId))
{
  (void)remoteArg;
  (void)remoteArgLength;
  svtkMultiProcessController* controller = (svtkMultiProcessController*)(localArg);
  controller->SetBreakFlag(1);
}

// ----------------------------------------------------------------------------
// Single method used when launching a single process.
static void svtkMultiProcessControllerRun(svtkMultiProcessController* c, void* arg)
{
  svtkProcess* p = reinterpret_cast<svtkProcess*>(arg);
  p->SetController(c);
  p->Execute();
}

//----------------------------------------------------------------------------
svtkMultiProcessController::svtkMultiProcessController()
{
  this->Internal = new svtkInternal;

  this->RMICount = 1;

  this->SingleMethod = nullptr;
  this->SingleData = nullptr;

  this->Communicator = nullptr;
  this->RMICommunicator = nullptr;

  this->BreakFlag = 0;
  this->ForceDeepCopy = 1;

  this->BroadcastTriggerRMI = false;

  this->OutputWindow = nullptr;

  // Define an rmi internally to exit from the processing loop.
  this->AddRMI(svtkMultiProcessControllerBreakRMI, this, BREAK_RMI_TAG);
}

//----------------------------------------------------------------------------
// This is an old comment that I do not know is true:
// (We need to have a "GetNetReferenceCount" to avoid memory leaks.)
svtkMultiProcessController::~svtkMultiProcessController()
{
  if (this->OutputWindow && (this->OutputWindow == svtkOutputWindow::GetInstance()))
  {
    svtkOutputWindow::SetInstance(nullptr);
  }

  if (this->OutputWindow)
  {
    this->OutputWindow->Delete();
  }

  delete this->Internal;
}

//----------------------------------------------------------------------------
void svtkMultiProcessController::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  svtkIndent nextIndent = indent.GetNextIndent();

  os << indent << "Break flag: " << (this->BreakFlag ? "(yes)" : "(no)") << endl;
  os << indent << "Force deep copy: " << (this->ForceDeepCopy ? "(yes)" : "(no)") << endl;
  os << indent << "Output window: ";
  os << indent << "BroadcastTriggerRMI: " << (this->BroadcastTriggerRMI ? "(yes)" : "(no)");
  if (this->OutputWindow)
  {
    os << endl;
    this->OutputWindow->PrintSelf(os, nextIndent);
  }
  else
  {
    os << "(none)" << endl;
  }
  os << indent << "Communicator: ";
  if (this->Communicator)
  {
    os << endl;
    this->Communicator->PrintSelf(os, nextIndent);
  }
  else
  {
    os << "(none)" << endl;
  }
  os << indent << "RMI communicator: ";
  if (this->RMICommunicator)
  {
    os << endl;
    this->RMICommunicator->PrintSelf(os, nextIndent);
  }
  else
  {
    os << "(none)" << endl;
  }
}

//----------------------------------------------------------------------------
void svtkMultiProcessController::SetNumberOfProcesses(int num)
{
  if (this->Communicator)
  {
    this->Communicator->SetNumberOfProcesses(num);
  }
  else
  {
    svtkErrorMacro("Communicator not set.");
  }
}

//----------------------------------------------------------------------------
int svtkMultiProcessController::GetNumberOfProcesses()
{
  if (this->Communicator)
  {
    return this->Communicator->GetNumberOfProcesses();
  }
  else
  {
    svtkErrorMacro("Communicator not set.");
    return 0;
  }
}

//----------------------------------------------------------------------------
int svtkMultiProcessController::GetLocalProcessId()
{
  if (this->Communicator)
  {
    return this->Communicator->GetLocalProcessId();
  }
  else
  {
    svtkErrorMacro("Communicator not set.");
    return -1;
  }
}

//----------------------------------------------------------------------------
void svtkMultiProcessController::SetSingleMethod(svtkProcessFunctionType f, void* data)
{
  this->SingleMethod = f;
  this->SingleData = data;
}

// ----------------------------------------------------------------------------
void svtkMultiProcessController::SetSingleProcessObject(svtkProcess* p)
{
  this->SetSingleMethod(svtkMultiProcessControllerRun, p);
}

//----------------------------------------------------------------------------
// Set one of the user defined methods that will be run on NumberOfProcesses
// processes when MultipleMethodExecute is called. This method should be
// called with index = 0, 1, ..,  NumberOfProcesses-1 to set up all the
// required user defined methods
void svtkMultiProcessController::SetMultipleMethod(int index, svtkProcessFunctionType f, void* data)
{
  // You can only set the method for 0 through NumberOfProcesses-1
  if (index >= this->GetNumberOfProcesses())
  {
    svtkErrorMacro(<< "Can't set method " << index << " with a processes count of "
                  << this->GetNumberOfProcesses());
  }
  else
  {
    this->Internal->MultipleMethod[index] = f;
    this->Internal->MultipleData[index] = data;
  }
}

//-----------------------------------------------------------------------------
void svtkMultiProcessController::GetMultipleMethod(
  int index, svtkProcessFunctionType& func, void*& data)
{
  if (this->Internal->MultipleMethod.find(index) != this->Internal->MultipleMethod.end())
  {
    func = this->Internal->MultipleMethod[index];
    data = this->Internal->MultipleData[index];
  }
  else
  {
    func = nullptr;
    data = nullptr;
  }
}

//-----------------------------------------------------------------------------
svtkMultiProcessController* svtkMultiProcessController::CreateSubController(svtkProcessGroup* group)
{
  if (group->GetCommunicator() != this->Communicator)
  {
    svtkErrorMacro(<< "Invalid group for creating a sub controller.");
    return nullptr;
  }

  if (group->FindProcessId(this->GetLocalProcessId()) < 0)
  {
    // The group does not contain this process.  Just return nullptr.
    return nullptr;
  }

  svtkSubCommunicator* subcomm = svtkSubCommunicator::New();
  subcomm->SetGroup(group);

  // We only need a basic implementation of svtkMultiProcessController for the
  // subgroup, so we just use svtkDummyController here.  It's a bit of a misnomer
  // and may lead to confusion, but I think it's better than creating yet
  // another class we have to maintain.
  svtkDummyController* subcontroller = svtkDummyController::New();
  subcontroller->SetCommunicator(subcomm);
  subcontroller->SetRMICommunicator(subcomm);

  subcomm->Delete();

  return subcontroller;
}

//-----------------------------------------------------------------------------
svtkMultiProcessController* svtkMultiProcessController::PartitionController(
  int localColor, int localKey)
{
  svtkMultiProcessController* subController = nullptr;

  int numProc = this->GetNumberOfProcesses();

  std::vector<int> allColors(numProc);
  this->AllGather(&localColor, &allColors[0], 1);

  std::vector<int> allKeys(numProc);
  this->AllGather(&localKey, &allKeys[0], 1);

  std::vector<bool> inPartition;
  inPartition.assign(numProc, false);

  for (int i = 0; i < numProc; i++)
  {
    if (inPartition[i])
      continue;
    int targetColor = allColors[i];
    std::list<int> partitionIds; // Make sorted list, then put in group.
    for (int j = i; j < numProc; j++)
    {
      if (allColors[j] != targetColor)
        continue;
      inPartition[j] = true;
      std::list<int>::iterator iter = partitionIds.begin();
      while ((iter != partitionIds.end()) && (allKeys[*iter] <= allKeys[j]))
      {
        ++iter;
      }
      partitionIds.insert(iter, j);
    }
    // Copy list into process group.
    svtkNew<svtkProcessGroup> group;
    group->Initialize(this);
    group->RemoveAllProcessIds();
    for (std::list<int>::iterator iter = partitionIds.begin(); iter != partitionIds.end(); ++iter)
    {
      group->AddProcessId(*iter);
    }
    // Use group to create controller.
    svtkMultiProcessController* sc = this->CreateSubController(group);
    if (sc)
    {
      subController = sc;
    }
  }

  return subController;
}

//----------------------------------------------------------------------------
unsigned long svtkMultiProcessController::AddRMICallback(
  svtkRMIFunctionType callback, void* localArg, int tag)
{
  svtkInternal::svtkRMICallback callbackInfo;
  callbackInfo.Id = this->RMICount++;
  callbackInfo.Function = callback;
  callbackInfo.LocalArgument = localArg;
  this->Internal->RMICallbacks[tag].push_back(callbackInfo);
  return callbackInfo.Id;
}

//----------------------------------------------------------------------------
void svtkMultiProcessController::RemoveAllRMICallbacks(int tag)
{
  svtkInternal::RMICallbackMap::iterator iter = this->Internal->RMICallbacks.find(tag);
  if (iter != this->Internal->RMICallbacks.end())
  {
    this->Internal->RMICallbacks.erase(iter);
  }
}

//----------------------------------------------------------------------------
bool svtkMultiProcessController::RemoveRMICallback(unsigned long id)
{
  svtkInternal::RMICallbackMap::iterator iterMap;
  for (iterMap = this->Internal->RMICallbacks.begin();
       iterMap != this->Internal->RMICallbacks.end(); ++iterMap)
  {
    svtkInternal::RMICallbackVector::iterator iterVec;
    for (iterVec = iterMap->second.begin(); iterVec != iterMap->second.end(); ++iterVec)
    {
      if (iterVec->Id == id)
      {
        iterMap->second.erase(iterVec);
        return true;
      }
    }
  }
  return false;
}

//----------------------------------------------------------------------------
int svtkMultiProcessController::RemoveFirstRMI(int tag)
{
  svtkInternal::RMICallbackMap::iterator iter = this->Internal->RMICallbacks.find(tag);
  if (iter != this->Internal->RMICallbacks.end())
  {
    if (iter->second.begin() != iter->second.end())
    {
      iter->second.erase(iter->second.begin());
      return 1;
    }
  }
  return 0;
}

//----------------------------------------------------------------------------
int svtkMultiProcessController::RemoveRMI(unsigned long id)
{
  return this->RemoveRMICallback(id) ? 1 : 0;
}

//----------------------------------------------------------------------------
unsigned long svtkMultiProcessController::AddRMI(svtkRMIFunctionType f, void* localArg, int tag)
{
  // Remove any previously registered RMI handler for the tag.
  this->RemoveAllRMICallbacks(tag);
  return this->AddRMICallback(f, localArg, tag);
}

//----------------------------------------------------------------------------
void svtkMultiProcessController::TriggerRMIOnAllChildren(void* arg, int argLength, int rmiTag)
{

  if (this->BroadcastTriggerRMI)
  {
    this->BroadcastTriggerRMIOnAllChildren(arg, argLength, rmiTag);
  } // END if use broadcast method
  else
  {
    int myid = this->GetLocalProcessId();
    int childid = 2 * myid + 1;
    int numProcs = this->GetNumberOfProcesses();
    if (numProcs > childid)
    {
      this->TriggerRMIInternal(childid, arg, argLength, rmiTag, true);
    }
    childid++;
    if (numProcs > childid)
    {
      this->TriggerRMIInternal(childid, arg, argLength, rmiTag, true);
    }
  } // END else
}

//----------------------------------------------------------------------------
void svtkMultiProcessController::TriggerRMI(
  int remoteProcessId, void* arg, int argLength, int rmiTag)
{
  if (this->BroadcastTriggerRMI)
  {
    svtkErrorMacro("TriggerRMI should not be called when BroadcastTriggerRMI is ON");
  }

  // Deal with sending RMI to ourself here for now.
  if (remoteProcessId == this->GetLocalProcessId())
  {
    this->ProcessRMI(remoteProcessId, arg, argLength, rmiTag);
    return;
  }

  this->TriggerRMIInternal(remoteProcessId, arg, argLength, rmiTag, false);
}

//----------------------------------------------------------------------------
void svtkMultiProcessController::BroadcastTriggerRMIOnAllChildren(
  void* arg, int argLength, int rmiTag)
{
  // This is called by the root process, namely rank 0. The satellite ranks
  // call BroadcastProcessRMIs().

  int triggerMessage[128];
  triggerMessage[0] = rmiTag;
  triggerMessage[1] = argLength;

  // Note: We don't need to set the local process ID & propagate

  // We send the header in Little Endian order.
  svtkByteSwap::SwapLERange(triggerMessage, 2);

  // If possible, send the data over a single broadcast
  int buffer_size = sizeof(int) * (126 /*128-2*/);
  if ((argLength >= 0) && (argLength < buffer_size))
  {
    if (argLength > 0)
    {
      memcpy(&triggerMessage[2], arg, argLength);
    }
    this->RMICommunicator->Broadcast(triggerMessage, 128, 0);
  }
  else
  {
    this->RMICommunicator->Broadcast(triggerMessage, 128, 0);
    this->RMICommunicator->Broadcast(reinterpret_cast<unsigned char*>(arg), argLength, 0);
  }
}

//----------------------------------------------------------------------------
int svtkMultiProcessController::BroadcastProcessRMIs(int svtkNotUsed(reportErrors), int dont_loop)
{
  int triggerMessage[128];
  int rmiTag;
  int argLength;
  unsigned char* arg = nullptr;
  int error = RMI_NO_ERROR;

  this->InvokeEvent(svtkCommand::StartEvent);

  do
  {
    this->RMICommunicator->Broadcast(triggerMessage, 128, 0);

#ifdef SVTK_WORDS_BIGENDIAN
    // Header is sent in little-endian form. We need to convert it to big
    // endian.
    svtkByteSwap::SwapLERange(triggerMessage, 2);
#endif

    rmiTag = triggerMessage[0];
    argLength = triggerMessage[1];

    if (argLength > 0)
    {
      arg = new unsigned char[argLength];
      int buffer_size = sizeof(int) * (126 /*128-2*/);

      if (argLength < buffer_size)
      {
        memcpy(arg, &triggerMessage[2], argLength);
      } // END if message is inline
      else
      {
        this->RMICommunicator->Broadcast(arg, argLength, 0);
      } // END else message was not inline

    } // END if non-null arguments

    this->ProcessRMI(0 /*we broadcast from rank 0*/, arg, argLength, rmiTag);

    delete[] arg;
    arg = nullptr;

    // check for break
    if (this->BreakFlag)
    {
      this->BreakFlag = 0;
      this->InvokeEvent(svtkCommand::EndEvent);
      return error;
    }

  } while (!dont_loop);

  this->InvokeEvent(svtkCommand::EndEvent);
  return error;
}

//----------------------------------------------------------------------------
void svtkMultiProcessController::TriggerRMIInternal(
  int remoteProcessId, void* arg, int argLength, int rmiTag, bool propagate)
{
  int triggerMessage[128];
  triggerMessage[0] = rmiTag;
  triggerMessage[1] = argLength;

  // It is important for the remote process to know what process invoked it.
  // Multiple processes might try to invoke the method at the same time.
  // The remote method will know where to get additional args.
  triggerMessage[2] = this->GetLocalProcessId();

  // Pass the propagate flag.
  triggerMessage[3] = propagate ? 1 : 0;

  // We send the header in Little Endian order.
  svtkByteSwap::SwapLERange(triggerMessage, 4);

  // If the message is small, we will try to get the message sent over using a
  // single Send(), rather than two. This helps speed up communication
  // significantly, since sending multiple small messages is generally slower
  // than sending a single large message.
  if (argLength >= 0 && static_cast<unsigned int>(argLength) < sizeof(int) * (128 - 4))
  {
    if (argLength > 0)
    {
      memcpy(&triggerMessage[4], arg, argLength);
    }
    int num_bytes = static_cast<int>(4 * sizeof(int)) + argLength;
    this->RMICommunicator->Send(
      reinterpret_cast<unsigned char*>(triggerMessage), num_bytes, remoteProcessId, RMI_TAG);
  }
  else
  {
    this->RMICommunicator->Send(reinterpret_cast<unsigned char*>(triggerMessage),
      static_cast<int>(4 * sizeof(int)), remoteProcessId, RMI_TAG);
    if (argLength > 0)
    {
      this->RMICommunicator->Send((char*)arg, argLength, remoteProcessId, RMI_ARG_TAG);
    }
  }
}

//----------------------------------------------------------------------------
void svtkMultiProcessController::TriggerBreakRMIs()
{
  int idx, num;

  if (this->BroadcastTriggerRMI == 1)
  {
    this->BroadcastTriggerRMIOnAllChildren(nullptr, 0, BREAK_RMI_TAG);
    return;
  }

  if (this->GetLocalProcessId() != 0)
  {
    svtkErrorMacro("Break should be triggered from process 0.");
    return;
  }

  num = this->GetNumberOfProcesses();
  for (idx = 1; idx < num; ++idx)
  {
    this->TriggerRMI(idx, nullptr, 0, BREAK_RMI_TAG);
  }
}

//----------------------------------------------------------------------------
int svtkMultiProcessController::ProcessRMIs()
{
  return this->ProcessRMIs(1, 0);
}

//----------------------------------------------------------------------------
int svtkMultiProcessController::ProcessRMIs(int reportErrors, int dont_loop)
{
  if (this->BroadcastTriggerRMI)
  {
    return this->BroadcastProcessRMIs(reportErrors, dont_loop);
  }

  this->InvokeEvent(svtkCommand::StartEvent);
  int triggerMessage[128];
  unsigned char* arg = nullptr;
  int error = RMI_NO_ERROR;

  do
  {
    if (!this->RMICommunicator->Receive(reinterpret_cast<unsigned char*>(triggerMessage),
          static_cast<svtkIdType>(128 * sizeof(int)), ANY_SOURCE, RMI_TAG) ||
      this->RMICommunicator->GetCount() < static_cast<int>(4 * sizeof(int)))
    {
      if (reportErrors)
      {
        svtkErrorMacro("Could not receive RMI trigger message.");
      }
      error = RMI_TAG_ERROR;
      break;
    }
#ifdef SVTK_WORDS_BIGENDIAN
    // Header is sent in little-endian form. We need to convert it to big
    // endian.
    svtkByteSwap::SwapLERange(triggerMessage, 4);
#endif

    if (triggerMessage[1] > 0)
    {
      arg = new unsigned char[triggerMessage[1]];
      // If the message length is small enough, the TriggerRMIInternal() call
      // packs the message data inline. So depending on the message length we
      // use the inline data or make a second receive to fetch the data.
      if (static_cast<unsigned int>(triggerMessage[1]) < sizeof(int) * (128 - 4))
      {
        int num_bytes = static_cast<int>(4 * sizeof(int)) + triggerMessage[1];
        if (this->RMICommunicator->GetCount() != num_bytes)
        {
          if (reportErrors)
          {
            svtkErrorMacro("Could not receive the RMI argument in its entirety.");
          }
          error = RMI_ARG_ERROR;
          break;
        }
        memcpy(arg, &triggerMessage[4], triggerMessage[1]);
      }
      else
      {
        if (!this->RMICommunicator->Receive(
              (char*)(arg), triggerMessage[1], triggerMessage[2], RMI_ARG_TAG) ||
          this->RMICommunicator->GetCount() != triggerMessage[1])
        {
          if (reportErrors)
          {
            svtkErrorMacro("Could not receive RMI argument.");
          }
          error = RMI_ARG_ERROR;
          break;
        }
      }
    }
    if (triggerMessage[3] == 1 && this->GetNumberOfProcesses() > 3) // propagate==true
    {
      this->TriggerRMIOnAllChildren(arg, triggerMessage[1], triggerMessage[0]);
    }
    this->ProcessRMI(triggerMessage[2], arg, triggerMessage[1], triggerMessage[0]);
    delete[] arg;
    arg = nullptr;

    // check for break
    if (this->BreakFlag)
    {
      this->BreakFlag = 0;
      this->InvokeEvent(svtkCommand::EndEvent);
      return error;
    }
  } while (!dont_loop);

  this->InvokeEvent(svtkCommand::EndEvent);
  return error;
}

//----------------------------------------------------------------------------
void svtkMultiProcessController::ProcessRMI(
  int remoteProcessId, void* arg, int argLength, int rmiTag)
{
  // we build the list of callbacks to call and then invoke them to handle the
  // case where the callback removes the callback.
  std::vector<svtkInternal::svtkRMICallback> callbacks;

  svtkInternal::RMICallbackMap::iterator iter = this->Internal->RMICallbacks.find(rmiTag);
  if (iter != this->Internal->RMICallbacks.end())
  {
    svtkInternal::RMICallbackVector::iterator iterVec;
    for (iterVec = iter->second.begin(); iterVec != iter->second.end(); ++iterVec)
    {
      if (iterVec->Function)
      {
        callbacks.push_back(*iterVec);
      }
    }
  }

  if (callbacks.empty())
  {
    svtkErrorMacro(
      "Process " << this->GetLocalProcessId() << " Could not find RMI with tag " << rmiTag);
  }

  std::vector<svtkInternal::svtkRMICallback>::iterator citer;
  for (citer = callbacks.begin(); citer != callbacks.end(); ++citer)
  {
    (*citer->Function)(citer->LocalArgument, arg, argLength, remoteProcessId);
  }
}
//----------------------------------------------------------------------------
int svtkMultiProcessController::Reduce(
  const svtkBoundingBox& sendBuffer, svtkBoundingBox& recvBuffer, int destProcessId)
{
  if (this->GetNumberOfProcesses() <= 1)
  {
    recvBuffer = sendBuffer;
    return 1;
  }

  double send_min[3] = { SVTK_DOUBLE_MAX, SVTK_DOUBLE_MAX, SVTK_DOUBLE_MAX };
  double send_max[3] = { SVTK_DOUBLE_MIN, SVTK_DOUBLE_MIN, SVTK_DOUBLE_MIN };
  if (sendBuffer.IsValid())
  {
    sendBuffer.GetMinPoint(send_min);
    sendBuffer.GetMaxPoint(send_max);
  }
  double recv_min[3], recv_max[3];
  if (this->Reduce(send_min, recv_min, 3, svtkCommunicator::MIN_OP, destProcessId) &&
    this->Reduce(send_max, recv_max, 3, svtkCommunicator::MAX_OP, destProcessId))
  {
    if (this->GetLocalProcessId() == destProcessId)
    {
      const double bds[6] = { recv_min[0], recv_max[0], recv_min[1], recv_max[1], recv_min[2],
        recv_max[2] };
      recvBuffer.SetBounds(bds);
    }
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
int svtkMultiProcessController::AllReduce(
  const svtkBoundingBox& sendBuffer, svtkBoundingBox& recvBuffer)
{
  if (this->GetNumberOfProcesses() <= 1)
  {
    recvBuffer = sendBuffer;
    return 1;
  }

  double send_min[3] = { SVTK_DOUBLE_MAX, SVTK_DOUBLE_MAX, SVTK_DOUBLE_MAX };
  double send_max[3] = { SVTK_DOUBLE_MIN, SVTK_DOUBLE_MIN, SVTK_DOUBLE_MIN };
  if (sendBuffer.IsValid())
  {
    sendBuffer.GetMinPoint(send_min);
    sendBuffer.GetMaxPoint(send_max);
  }
  double recv_min[3], recv_max[3];
  if (this->AllReduce(send_min, recv_min, 3, svtkCommunicator::MIN_OP) &&
    this->AllReduce(send_max, recv_max, 3, svtkCommunicator::MAX_OP))
  {
    const double bds[6] = { recv_min[0], recv_max[0], recv_min[1], recv_max[1], recv_min[2],
      recv_max[2] };
    recvBuffer.SetBounds(bds);
    return 1;
  }
  return 0;
}

//============================================================================
// The intent is to give access to a processes controller from a static method.
static svtkWeakPointer<svtkMultiProcessController> SVTK_GLOBAL_MULTI_PROCESS_CONTROLLER;
//----------------------------------------------------------------------------
svtkMultiProcessController* svtkMultiProcessController::GetGlobalController()
{
  if (SVTK_GLOBAL_MULTI_PROCESS_CONTROLLER == nullptr)
  {
    return nullptr;
  }

  return SVTK_GLOBAL_MULTI_PROCESS_CONTROLLER->GetLocalController();
}
//----------------------------------------------------------------------------
// This can be overridden in the subclass to translate controllers.
svtkMultiProcessController* svtkMultiProcessController::GetLocalController()
{
  return SVTK_GLOBAL_MULTI_PROCESS_CONTROLLER;
}
//----------------------------------------------------------------------------
// This can be overridden in the subclass to translate controllers.
void svtkMultiProcessController::SetGlobalController(svtkMultiProcessController* controller)
{
  SVTK_GLOBAL_MULTI_PROCESS_CONTROLLER = controller;
}
