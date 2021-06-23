/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkIOSRenderWindowInteractor.mm

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#import "svtkIOSRenderWindowInteractor.h"
#import "svtkCommand.h"
#import "svtkObjectFactory.h"
#import "svtkRenderWindow.h"

//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkIOSRenderWindowInteractor);

//----------------------------------------------------------------------------
void (*svtkIOSRenderWindowInteractor::ClassExitMethod)(void*) = (void (*)(void*))NULL;
void* svtkIOSRenderWindowInteractor::ClassExitMethodArg = (void*)NULL;
void (*svtkIOSRenderWindowInteractor::ClassExitMethodArgDelete)(void*) = (void (*)(void*))NULL;

//----------------------------------------------------------------------------
svtkIOSRenderWindowInteractor::svtkIOSRenderWindowInteractor() {}

//----------------------------------------------------------------------------
svtkIOSRenderWindowInteractor::~svtkIOSRenderWindowInteractor()
{
  this->Enabled = 0;
}

//----------------------------------------------------------------------------
void svtkIOSRenderWindowInteractor::StartEventLoop() {}

//----------------------------------------------------------------------------
// Begin processing keyboard strokes.
void svtkIOSRenderWindowInteractor::Initialize()
{
  // make sure we have a RenderWindow and camera
  if (!this->RenderWindow)
  {
    svtkErrorMacro(<< "No renderer defined!");
    return;
  }
  if (this->Initialized)
  {
    return;
  }
  this->Initialized = 1;
  // get the info we need from the RenderingWindow
  svtkRenderWindow* renWin = this->RenderWindow;
  renWin->Start();
  const int* size = renWin->GetSize();

  renWin->GetPosition(); // update values of this->Position[2]

  this->Enable();
  this->Size[0] = size[0];
  this->Size[1] = size[1];
}

//----------------------------------------------------------------------------
void svtkIOSRenderWindowInteractor::Enable()
{
  if (this->Enabled)
  {
    return;
  }

  // Set the RenderWindow's interactor so that when the svtkIOSGLView tries
  // to handle events from the OS it will either handle them or ignore them
  this->GetRenderWindow()->SetInteractor(this);

  this->Enabled = 1;
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkIOSRenderWindowInteractor::Disable()
{
  if (!this->Enabled)
  {
    return;
  }

#ifdef SVTK_USE_TDX
  if (this->Device->GetInitialized())
  {
    this->Device->Close();
  }
#endif

  // Set the RenderWindow's interactor so that when the svtkIOSGLView tries
  // to handle events from the OS it will either handle them or ignore them
  this->GetRenderWindow()->SetInteractor(NULL);

  this->Enabled = 0;
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkIOSRenderWindowInteractor::TerminateApp() {}

//----------------------------------------------------------------------------
int svtkIOSRenderWindowInteractor::InternalCreateTimer(
  int timerId, int timerType, unsigned long duration)
{
  // In this implementation, timerId and platformTimerId are the same
  int platformTimerId = timerId;

  return platformTimerId;
}

//----------------------------------------------------------------------------
int svtkIOSRenderWindowInteractor::InternalDestroyTimer(int platformTimerId)
{
  // In this implementation, timerId and platformTimerId are the same;
  // but calling this anyway is more correct
  int timerId = this->GetSVTKTimerId(platformTimerId);

  return 0; // fail
}

//----------------------------------------------------------------------------
// Specify the default function to be called when an interactor needs to exit.
// This callback is overridden by an instance ExitMethod that is defined.
void svtkIOSRenderWindowInteractor::SetClassExitMethod(void (*f)(void*), void* arg)
{
  if (f != svtkIOSRenderWindowInteractor::ClassExitMethod ||
    arg != svtkIOSRenderWindowInteractor::ClassExitMethodArg)
  {
    // delete the current arg if there is a delete method
    if ((svtkIOSRenderWindowInteractor::ClassExitMethodArg) &&
      (svtkIOSRenderWindowInteractor::ClassExitMethodArgDelete))
    {
      (*svtkIOSRenderWindowInteractor::ClassExitMethodArgDelete)(
        svtkIOSRenderWindowInteractor::ClassExitMethodArg);
    }
    svtkIOSRenderWindowInteractor::ClassExitMethod = f;
    svtkIOSRenderWindowInteractor::ClassExitMethodArg = arg;

    // no call to this->Modified() since this is a class member function
  }
}

//----------------------------------------------------------------------------
// Set the arg delete method.  This is used to free user memory.
void svtkIOSRenderWindowInteractor::SetClassExitMethodArgDelete(void (*f)(void*))
{
  if (f != svtkIOSRenderWindowInteractor::ClassExitMethodArgDelete)
  {
    svtkIOSRenderWindowInteractor::ClassExitMethodArgDelete = f;

    // no call to this->Modified() since this is a class member function
  }
}

//----------------------------------------------------------------------------
void svtkIOSRenderWindowInteractor::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void svtkIOSRenderWindowInteractor::ExitCallback()
{
  if (this->HasObserver(svtkCommand::ExitEvent))
  {
    this->InvokeEvent(svtkCommand::ExitEvent, NULL);
  }
  else if (this->ClassExitMethod)
  {
    (*this->ClassExitMethod)(this->ClassExitMethodArg);
  }
  this->TerminateApp();
}
