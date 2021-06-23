/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkMPIController.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkMPIController.h"

#include "svtkIntArray.h"
#include "svtkObjectFactory.h"
#include "svtkOutputWindow.h"

#include "svtkMPI.h"

#include "svtkSmartPointer.h"

#include <cassert>

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

int svtkMPIController::Initialized = 0;
char svtkMPIController::ProcessorName[MPI_MAX_PROCESSOR_NAME] = "";
int svtkMPIController::UseSsendForRMI = 0;

// Output window which prints out the process id
// with the error or warning messages
class svtkMPIOutputWindow : public svtkOutputWindow
{
public:
  svtkTypeMacro(svtkMPIOutputWindow, svtkOutputWindow);

  void DisplayText(const char* t) override
  {
    if (this->Controller && svtkMPIController::Initialized)
    {
      cout << "Process id: " << this->Controller->GetLocalProcessId() << " >> ";
    }
    cout << t;
  }

  svtkMPIOutputWindow() { this->Controller = 0; }

  friend class svtkMPIController;

protected:
  svtkMPIController* Controller;
  svtkMPIOutputWindow(const svtkMPIOutputWindow&);
  void operator=(const svtkMPIOutputWindow&);
};

void svtkMPIController::CreateOutputWindow()
{
  svtkMPIOutputWindow* window = new svtkMPIOutputWindow;
  window->InitializeObjectBase();
  window->Controller = this;
  this->OutputWindow = window;
  svtkOutputWindow::SetInstance(this->OutputWindow);
}

svtkStandardNewMacro(svtkMPIController);

//----------------------------------------------------------------------------
svtkMPIController::svtkMPIController()
{
  // If MPI was already initialized obtain rank and size.
  if (svtkMPIController::Initialized)
  {
    this->InitializeCommunicator(svtkMPICommunicator::GetWorldCommunicator());
    // Copy svtkMPIController::WorldRMICommunicataor which is created when
    // MPI is initialized
    svtkMPICommunicator* comm = svtkMPICommunicator::New();
    comm->CopyFrom(svtkMPIController::WorldRMICommunicator);
    this->RMICommunicator = comm;
  }

  this->OutputWindow = 0;
}

//----------------------------------------------------------------------------
svtkMPIController::~svtkMPIController()
{
  this->SetCommunicator(0);
  if (this->RMICommunicator)
  {
    this->RMICommunicator->Delete();
  }
}

//----------------------------------------------------------------------------
void svtkMPIController::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Initialized: " << (svtkMPIController::Initialized ? "(yes)" : "(no)") << endl;
}

svtkMPICommunicator* svtkMPIController::WorldRMICommunicator = 0;

//----------------------------------------------------------------------------
void svtkMPIController::TriggerRMIInternal(
  int remoteProcessId, void* arg, int argLength, int rmiTag, bool propagate)
{
  svtkMPICommunicator* mpiComm = svtkMPICommunicator::SafeDownCast(this->RMICommunicator);
  int use_ssend = mpiComm->GetUseSsend();
  if (svtkMPIController::UseSsendForRMI == 1 && use_ssend == 0)
  {
    mpiComm->SetUseSsend(1);
  }

  this->Superclass::TriggerRMIInternal(remoteProcessId, arg, argLength, rmiTag, propagate);

  if (svtkMPIController::UseSsendForRMI == 1 && use_ssend == 0)
  {
    mpiComm->SetUseSsend(0);
  }
}

//----------------------------------------------------------------------------
void svtkMPIController::Initialize()
{
  this->Initialize(0, 0, 1);
}

//----------------------------------------------------------------------------
void svtkMPIController::Initialize(int* argc, char*** argv, int initializedExternally)
{
  if (svtkMPIController::Initialized)
  {
    svtkWarningMacro("Already initialized.");
    return;
  }

  // Can be done once in the program.
  svtkMPIController::Initialized = 1;
  if (initializedExternally == 0)
  {
    MPI_Init(argc, argv);
  }
  this->InitializeCommunicator(svtkMPICommunicator::GetWorldCommunicator());

  int tmp;
  MPI_Get_processor_name(ProcessorName, &tmp);
  // Make a copy of MPI_COMM_WORLD creating a new context.
  // This is used in the creating of the communicators after
  // Initialize() has been called. It has to be done here
  // because for this to work, all processes have to call
  // MPI_Comm_dup and this is the only method which is
  // guaranteed to be called by all processes.
  svtkMPIController::WorldRMICommunicator = svtkMPICommunicator::New();
  svtkMPIController::WorldRMICommunicator->Duplicate((svtkMPICommunicator*)this->Communicator);
  this->RMICommunicator = svtkMPIController::WorldRMICommunicator;
  // Since we use Delete to get rid of the reference, we should use nullptr to register.
  this->RMICommunicator->Register(nullptr);

  this->Modified();
}

const char* svtkMPIController::GetProcessorName()
{
  return ProcessorName;
}

// Good-bye world
// There should be no MPI calls after this.
// (Except maybe MPI_XXX_free()) unless finalized externally.
void svtkMPIController::Finalize(int finalizedExternally)
{
  if (svtkMPIController::Initialized)
  {
    svtkMPIController::WorldRMICommunicator->Delete();
    svtkMPIController::WorldRMICommunicator = 0;
    svtkMPICommunicator::WorldCommunicator->Delete();
    svtkMPICommunicator::WorldCommunicator = 0;
    this->SetCommunicator(0);
    if (this->RMICommunicator)
    {
      this->RMICommunicator->Delete();
      this->RMICommunicator = 0;
    }
    if (finalizedExternally == 0)
    {
      MPI_Finalize();
    }
    svtkMPIController::Initialized = 0;
    this->Modified();
  }
}

// Called by SetCommunicator and constructor. It frees but does
// not set RMIHandle (which should not be set by using MPI_Comm_dup
// during construction).
void svtkMPIController::InitializeCommunicator(svtkMPICommunicator* comm)
{
  if (this->Communicator != comm)
  {
    if (this->Communicator != 0)
    {
      this->Communicator->UnRegister(this);
    }
    this->Communicator = comm;
    if (this->Communicator != 0)
    {
      this->Communicator->Register(this);
    }

    this->Modified();
  }
}

// Delete the previous RMI communicator and creates a new one
// by duplicating the user communicator.
void svtkMPIController::InitializeRMICommunicator()
{
  if (this->RMICommunicator)
  {
    this->RMICommunicator->Delete();
    this->RMICommunicator = 0;
  }
  if (this->Communicator)
  {
    this->RMICommunicator = svtkMPICommunicator::New();
    ((svtkMPICommunicator*)this->RMICommunicator)
      ->Duplicate((svtkMPICommunicator*)this->Communicator);
  }
}

void svtkMPIController::SetCommunicator(svtkMPICommunicator* comm)
{
  this->InitializeCommunicator(comm);
  this->InitializeRMICommunicator();
}

//----------------------------------------------------------------------------
// Execute the method set as the SingleMethod.
void svtkMPIController::SingleMethodExecute()
{
  if (!svtkMPIController::Initialized)
  {
    svtkWarningMacro("MPI has to be initialized first.");
    return;
  }

  if (this->GetLocalProcessId() < this->GetNumberOfProcesses())
  {
    if (this->SingleMethod)
    {
      svtkMultiProcessController::SetGlobalController(this);
      (this->SingleMethod)(this, this->SingleData);
    }
    else
    {
      svtkWarningMacro("SingleMethod not set.");
    }
  }
}

//----------------------------------------------------------------------------
// Execute the methods set as the MultipleMethods.
void svtkMPIController::MultipleMethodExecute()
{
  if (!svtkMPIController::Initialized)
  {
    svtkWarningMacro("MPI has to be initialized first.");
    return;
  }

  int i = this->GetLocalProcessId();

  if (i < this->GetNumberOfProcesses())
  {
    svtkProcessFunctionType multipleMethod;
    void* multipleData;
    this->GetMultipleMethod(i, multipleMethod, multipleData);
    if (multipleMethod)
    {
      svtkMultiProcessController::SetGlobalController(this);
      (multipleMethod)(this, multipleData);
    }
    else
    {
      svtkWarningMacro("MultipleMethod " << i << " not set.");
    }
  }
}

char* svtkMPIController::ErrorString(int err)
{
  char* buffer = new char[MPI_MAX_ERROR_STRING];
  int resLen;
  MPI_Error_string(err, buffer, &resLen);
  return buffer;
}

//-----------------------------------------------------------------------------
svtkMPIController* svtkMPIController::CreateSubController(svtkProcessGroup* group)
{
  SVTK_CREATE(svtkMPICommunicator, subcomm);

  if (!subcomm->Initialize(group))
  {
    return nullptr;
  }

  // MPI is kind of funny in that in order to create a communicator from a
  // subgroup of another communicator, it is a collective operation involving
  // all of the processes in the original communicator, not just those belonging
  // to the group.  In any process not part of the group, the communicator is
  // created with MPI_COMM_NULL.  Check for that and return nullptr ourselves,
  // which is not really an error condition.
  if (*(subcomm->GetMPIComm()->Handle) == MPI_COMM_NULL)
  {
    return nullptr;
  }

  svtkMPIController* controller = svtkMPIController::New();
  controller->SetCommunicator(subcomm);
  return controller;
}

//-----------------------------------------------------------------------------
svtkMPIController* svtkMPIController::PartitionController(int localColor, int localKey)
{
  SVTK_CREATE(svtkMPICommunicator, subcomm);

  if (!subcomm->SplitInitialize(this->Communicator, localColor, localKey))
  {
    return nullptr;
  }

  svtkMPIController* controller = svtkMPIController::New();
  controller->SetCommunicator(subcomm);
  return controller;
}

//-----------------------------------------------------------------------------
int svtkMPIController::WaitSome(
  const int count, svtkMPICommunicator::Request rqsts[], svtkIntArray* completed)
{
  assert("pre: completed array is nullptr!" && (completed != nullptr));

  // Allocate set of completed requests
  completed->SetNumberOfComponents(1);
  completed->SetNumberOfTuples(count);

  // Downcast to MPI communicator
  svtkMPICommunicator* myMPICommunicator = (svtkMPICommunicator*)this->Communicator;

  // Delegate to MPI communicator
  int N = 0;
  int rc = myMPICommunicator->WaitSome(count, rqsts, N, completed->GetPointer(0));
  assert("post: Number of completed requests must N > 0" && (N > 0) && (N < (count - 1)));
  completed->Resize(N);

  return (rc);
}

//-----------------------------------------------------------------------------
bool svtkMPIController::TestAll(const int count, svtkMPICommunicator::Request requests[])
{
  int flag = 0;

  // Downcast to MPI communicator
  svtkMPICommunicator* myMPICommunicator = (svtkMPICommunicator*)this->Communicator;

  myMPICommunicator->TestAll(count, requests, flag);
  if (flag)
  {
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool svtkMPIController::TestAny(const int count, svtkMPICommunicator::Request requests[], int& idx)
{
  int flag = 0;

  // Downcast to MPI communicator
  svtkMPICommunicator* myMPICommunicator = (svtkMPICommunicator*)this->Communicator;

  myMPICommunicator->TestAny(count, requests, idx, flag);
  if (flag)
  {
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool svtkMPIController::TestSome(
  const int count, svtkMPICommunicator::Request requests[], svtkIntArray* completed)
{
  assert("pre: completed array is nullptr" && (completed != nullptr));

  // Allocate set of completed requests
  completed->SetNumberOfComponents(1);
  completed->SetNumberOfTuples(count);

  // Downcast to MPI communicator
  svtkMPICommunicator* myMPICommunicator = (svtkMPICommunicator*)this->Communicator;

  int N = 0;
  myMPICommunicator->TestSome(count, requests, N, completed->GetPointer(0));
  assert("post: Number of completed requests must N > 0" && (N > 0) && (N < (count - 1)));

  if (N > 0)
  {
    completed->Resize(N);
    return true;
  }
  else
  {
    completed->Resize(0);
    return false;
  }
}
