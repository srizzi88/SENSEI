/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRenderWindowInteractor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkRenderWindowInteractor.h"

#include "svtkCamera.h"
#include "svtkCommand.h"
#include "svtkDebugLeaks.h"
#include "svtkGraphicsFactory.h"
#include "svtkHardwareWindow.h"
#include "svtkInteractorStyleSwitchBase.h"
#include "svtkMath.h"
#include "svtkObserverMediator.h"
#include "svtkPickingManager.h"
#include "svtkPropPicker.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkRendererCollection.h"

#include <map>

// PIMPL'd class to keep track of timers. It maps the ids returned by CreateTimer()
// to the platform-specific representation for timer ids.
struct svtkTimerStruct
{
  int Id;
  int Type;
  unsigned long Duration;
  svtkTimerStruct()
    : Id(0)
    , Type(svtkRenderWindowInteractor::OneShotTimer)
    , Duration(10)
  {
  }
  svtkTimerStruct(int platformTimerId, int timerType, unsigned long duration)
  {
    this->Id = platformTimerId;
    this->Type = timerType;
    this->Duration = duration;
  }
};

class svtkTimerIdMap : public std::map<int, svtkTimerStruct>
{
};
typedef std::map<int, svtkTimerStruct>::iterator svtkTimerIdMapIterator;

// Initialize static variable that keeps track of timer ids for
// render window interactors.
static int svtkTimerId = 1;

//----------------------------------------------------------------------------
svtkCxxSetObjectMacro(svtkRenderWindowInteractor, Picker, svtkAbstractPicker);
svtkCxxSetObjectMacro(svtkRenderWindowInteractor, HardwareWindow, svtkHardwareWindow);

//----------------------------------------------------------------------
// Construct object so that light follows camera motion.
svtkRenderWindowInteractor::svtkRenderWindowInteractor()
{
  this->RenderWindow = nullptr;
  this->HardwareWindow = nullptr;
  // Here we are using base, and relying on the graphics factory or standard
  // object factory logic to create the correct instance, which should be the
  // svtkInteractorStyleSwitch when linked to the interactor styles, or
  // svtkInteractorStyleSwitchBase if the style module is not linked.
  this->InteractorStyle = nullptr;
  this->SetInteractorStyle(svtkInteractorStyleSwitchBase::New());
  this->InteractorStyle->Delete();

  this->LightFollowCamera = 1;
  this->Initialized = 0;
  this->Enabled = 0;
  this->EnableRender = true;
  this->DesiredUpdateRate = 15;
  // default limit is 3 hours per frame
  this->StillUpdateRate = 0.0001;

  this->Picker = this->CreateDefaultPicker();
  this->Picker->Register(this);
  this->Picker->Delete();

  this->PickingManager = nullptr;
  svtkPickingManager* pm = this->CreateDefaultPickingManager();
  this->SetPickingManager(pm);
  pm->Delete();

  this->EventPosition[0] = this->LastEventPosition[0] = 0;
  this->EventPosition[1] = this->LastEventPosition[1] = 0;

  for (int i = 0; i < SVTKI_MAX_POINTERS; ++i)
  {
    this->EventPositions[i][0] = this->LastEventPositions[i][0] = 0;
    this->EventPositions[i][1] = this->LastEventPositions[i][1] = 0;
  }
  this->PointerIndex = 0;

  this->EventSize[0] = 0;
  this->EventSize[1] = 0;

  this->Size[0] = 0;
  this->Size[1] = 0;

  this->NumberOfFlyFrames = 15;
  this->Dolly = 0.30;

  this->AltKey = 0;
  this->ControlKey = 0;
  this->ShiftKey = 0;
  this->KeyCode = 0;
  this->Rotation = 0;
  this->LastRotation = 0;
  this->Scale = 0;
  this->LastScale = 0;
  this->RepeatCount = 0;
  this->KeySym = nullptr;
  this->TimerEventId = 0;
  this->TimerEventType = 0;
  this->TimerEventDuration = 0;
  this->TimerEventPlatformId = 0;

  this->TimerMap = new svtkTimerIdMap;
  this->TimerDuration = 10;
  this->ObserverMediator = nullptr;
  this->HandleEventLoop = false;

  this->UseTDx = false; // 3DConnexion device.

  for (int i = 0; i < SVTKI_MAX_POINTERS; i++)
  {
    this->PointerIndexLookup[i] = 0;
    this->PointersDown[i] = 0;
  }

  this->RecognizeGestures = true;
  this->PointersDownCount = 0;
  this->CurrentGesture = svtkCommand::StartEvent;
  this->Done = false;
}

//----------------------------------------------------------------------
svtkRenderWindowInteractor::~svtkRenderWindowInteractor()
{
  if (this->InteractorStyle != nullptr)
  {
    this->InteractorStyle->UnRegister(this);
  }
  if (this->Picker)
  {
    this->Picker->UnRegister(this);
  }
  delete[] this->KeySym;
  if (this->ObserverMediator)
  {
    this->ObserverMediator->Delete();
  }
  delete this->TimerMap;

  this->SetPickingManager(nullptr);
  this->SetRenderWindow(nullptr);
  this->SetHardwareWindow(nullptr);
}

//----------------------------------------------------------------------
svtkRenderWindowInteractor* svtkRenderWindowInteractor::New()
{
  // First try to create the object from the svtkObjectFactory
  svtkObject* ret = svtkGraphicsFactory::CreateInstance("svtkRenderWindowInteractor");
  if (ret)
  {
    return static_cast<svtkRenderWindowInteractor*>(ret);
  }

  svtkRenderWindowInteractor* o = new svtkRenderWindowInteractor;
  o->InitializeObjectBase();
  return o;
}

//----------------------------------------------------------------------
void svtkRenderWindowInteractor::Render()
{
  if (this->RenderWindow && this->Enabled && this->EnableRender)
  {
    this->RenderWindow->Render();
  }
  // outside the above test so that third-party code can redirect
  // the render to the appropriate class
  this->InvokeEvent(svtkCommand::RenderEvent, nullptr);
}

//----------------------------------------------------------------------
// treat renderWindow and interactor as one object.
// it might be easier if the GetReference count method were redefined.
void svtkRenderWindowInteractor::UnRegister(svtkObjectBase* o)
{
  if (this->RenderWindow && this->RenderWindow->GetInteractor() == this && this->RenderWindow != o)
  {
    if (this->GetReferenceCount() + this->RenderWindow->GetReferenceCount() == 3)
    {
      this->RenderWindow->SetInteractor(nullptr);
      this->SetRenderWindow(nullptr);
    }
  }

  this->svtkObject::UnRegister(o);
}

//----------------------------------------------------------------------
void svtkRenderWindowInteractor::Start()
{
  // Let the compositing handle the event loop if it wants to.
  if (this->HasObserver(svtkCommand::StartEvent) && !this->HandleEventLoop)
  {
    this->InvokeEvent(svtkCommand::StartEvent, nullptr);
    return;
  }

  // As a convenience, initialize if we aren't initialized yet.
  if (!this->Initialized)
  {
    this->Initialize();

    if (!this->Initialized)
    {
      return;
    }
  }

  // Pass execution to the subclass which will run the event loop,
  // this will not return until TerminateApp is called.
  this->Done = false;
  this->StartEventLoop();
}

//----------------------------------------------------------------------
void svtkRenderWindowInteractor::SetRenderWindow(svtkRenderWindow* aren)
{
  if (this->RenderWindow != aren)
  {
    // to avoid destructor recursion
    svtkRenderWindow* temp = this->RenderWindow;
    this->RenderWindow = aren;
    if (temp != nullptr)
    {
      temp->UnRegister(this);
    }
    if (this->RenderWindow != nullptr)
    {
      this->RenderWindow->Register(this);
      if (this->RenderWindow->GetInteractor() != this)
      {
        this->RenderWindow->SetInteractor(this);
      }
    }
  }
}

//----------------------------------------------------------------------
void svtkRenderWindowInteractor::SetInteractorStyle(svtkInteractorObserver* style)
{
  if (this->InteractorStyle != style)
  {
    // to avoid destructor recursion
    svtkInteractorObserver* temp = this->InteractorStyle;
    this->InteractorStyle = style;
    if (temp != nullptr)
    {
      temp->SetInteractor(nullptr);
      temp->UnRegister(this);
    }
    if (this->InteractorStyle != nullptr)
    {
      this->InteractorStyle->Register(this);
      if (this->InteractorStyle->GetInteractor() != this)
      {
        this->InteractorStyle->SetInteractor(this);
      }
    }
  }
}

//----------------------------------------------------------------------
void svtkRenderWindowInteractor::UpdateSize(int x, int y)
{
  // if the size changed send this on to the RenderWindow
  if ((x != this->Size[0]) || (y != this->Size[1]))
  {
    this->Size[0] = this->EventSize[0] = x;
    this->Size[1] = this->EventSize[1] = y;
    this->RenderWindow->SetSize(x, y);
    if (this->HardwareWindow)
    {
      this->HardwareWindow->SetSize(x, y);
    }
    this->InvokeEvent(svtkCommand::WindowResizeEvent);
  }
}

//------------------------------------------------------------------------------
// This function is used to return an index given an ID
// and allocate one if needed
int svtkRenderWindowInteractor::GetPointerIndexForContact(size_t dwID)
{
  for (int i = 0; i < SVTKI_MAX_POINTERS; i++)
  {
    if (this->PointerIndexLookup[i] == dwID + 1)
    {
      return i;
    }
  }

  for (int i = 0; i < SVTKI_MAX_POINTERS; i++)
  {
    if (this->PointerIndexLookup[i] == 0)
    {
      this->PointerIndexLookup[i] = dwID + 1;
      return i;
    }
  }

  // Out of contacts
  return -1;
}

//------------------------------------------------------------------------------
// This function is used to return an index given an ID
int svtkRenderWindowInteractor::GetPointerIndexForExistingContact(size_t dwID)
{
  for (int i = 0; i < SVTKI_MAX_POINTERS; i++)
  {
    if (this->PointerIndexLookup[i] == dwID + 1)
    {
      return i;
    }
  }

  // Not found
  return -1;
}

//------------------------------------------------------------------------------
void svtkRenderWindowInteractor::ClearContact(size_t dwID)
{
  for (int i = 0; i < SVTKI_MAX_POINTERS; i++)
  {
    if (this->PointerIndexLookup[i] == dwID + 1)
    {
      this->PointerIndexLookup[i] = 0;
      return;
    }
  }
}

//------------------------------------------------------------------------------
void svtkRenderWindowInteractor::ClearPointerIndex(int i)
{
  if (i < SVTKI_MAX_POINTERS)
  {
    this->PointerIndexLookup[i] = 0;
  }
}

//------------------------------------------------------------------------------
// This function is used to return an index given an ID
bool svtkRenderWindowInteractor::IsPointerIndexSet(int i)
{
  if (i < SVTKI_MAX_POINTERS)
  {
    return (this->PointerIndexLookup[i] != 0);
  }
  return false;
}

//----------------------------------------------------------------------
// Creates an instance of svtkPropPicker by default
svtkAbstractPropPicker* svtkRenderWindowInteractor::CreateDefaultPicker()
{
  return svtkPropPicker::New();
}

//----------------------------------------------------------------------
// Creates an instance of svtkPickingManager by default
svtkPickingManager* svtkRenderWindowInteractor::CreateDefaultPickingManager()
{
  return svtkPickingManager::New();
}

//----------------------------------------------------------------------
void svtkRenderWindowInteractor::SetPickingManager(svtkPickingManager* pm)
{
  if (this->PickingManager == pm)
  {
    return;
  }

  svtkPickingManager* tempPickingManager = this->PickingManager;
  this->PickingManager = pm;
  if (this->PickingManager)
  {
    this->PickingManager->Register(this);
    this->PickingManager->SetInteractor(this);
  }

  if (tempPickingManager)
  {
    tempPickingManager->SetInteractor(nullptr);
    tempPickingManager->UnRegister(this);
  }

  this->Modified();
}

//----------------------------------------------------------------------
void svtkRenderWindowInteractor::ExitCallback()
{
  if (this->HasObserver(svtkCommand::ExitEvent))
  {
    this->InvokeEvent(svtkCommand::ExitEvent, nullptr);
  }
  else
  {
    this->TerminateApp();
  }
}

//----------------------------------------------------------------------
void svtkRenderWindowInteractor::UserCallback()
{
  this->InvokeEvent(svtkCommand::UserEvent, nullptr);
}

//----------------------------------------------------------------------
void svtkRenderWindowInteractor::StartPickCallback()
{
  this->InvokeEvent(svtkCommand::StartPickEvent, nullptr);
}

//----------------------------------------------------------------------
void svtkRenderWindowInteractor::EndPickCallback()
{
  this->InvokeEvent(svtkCommand::EndPickEvent, nullptr);
}

//----------------------------------------------------------------------
void svtkRenderWindowInteractor::FlyTo(svtkRenderer* ren, double x, double y, double z)
{
  double flyFrom[3], flyTo[3];
  double d[3], focalPt[3];
  int i, j;

  flyTo[0] = x;
  flyTo[1] = y;
  flyTo[2] = z;
  ren->GetActiveCamera()->GetFocalPoint(flyFrom);
  for (i = 0; i < 3; i++)
  {
    d[i] = flyTo[i] - flyFrom[i];
  }
  double distance = svtkMath::Normalize(d);
  double delta = distance / this->NumberOfFlyFrames;

  for (i = 1; i <= NumberOfFlyFrames; i++)
  {
    for (j = 0; j < 3; j++)
    {
      focalPt[j] = flyFrom[j] + d[j] * i * delta;
    }
    ren->GetActiveCamera()->SetFocalPoint(focalPt);
    ren->GetActiveCamera()->Dolly(this->Dolly / this->NumberOfFlyFrames + 1.0);
    ren->GetActiveCamera()->OrthogonalizeViewUp();
    ren->ResetCameraClippingRange();
    this->Render();
  }
}

//----------------------------------------------------------------------
void svtkRenderWindowInteractor::FlyToImage(svtkRenderer* ren, double x, double y)
{
  double flyFrom[3], flyTo[3];
  double d[3], focalPt[3], position[3], positionFrom[3];
  int i, j;

  flyTo[0] = x;
  flyTo[1] = y;
  ren->GetActiveCamera()->GetFocalPoint(flyFrom);
  flyTo[2] = flyFrom[2];
  ren->GetActiveCamera()->GetPosition(positionFrom);
  for (i = 0; i < 2; i++)
  {
    d[i] = flyTo[i] - flyFrom[i];
  }
  d[2] = 0.0;
  double distance = svtkMath::Normalize(d);
  double delta = distance / this->NumberOfFlyFrames;

  for (i = 1; i <= NumberOfFlyFrames; i++)
  {
    for (j = 0; j < 3; j++)
    {
      focalPt[j] = flyFrom[j] + d[j] * i * delta;
      position[j] = positionFrom[j] + d[j] * i * delta;
    }
    ren->GetActiveCamera()->SetFocalPoint(focalPt);
    ren->GetActiveCamera()->SetPosition(position);
    ren->GetActiveCamera()->Dolly(this->Dolly / this->NumberOfFlyFrames + 1.0);
    ren->ResetCameraClippingRange();
    this->Render();
  }
}

//----------------------------------------------------------------------------
svtkRenderer* svtkRenderWindowInteractor::FindPokedRenderer(int x, int y)
{
  if (this->RenderWindow == nullptr)
  {
    return nullptr;
  }

  svtkRenderer *currentRenderer = nullptr, *interactiveren = nullptr, *viewportren = nullptr;

  svtkRendererCollection* rc = this->RenderWindow->GetRenderers();
  int numRens = rc->GetNumberOfItems();

  for (int i = numRens - 1; (i >= 0) && !currentRenderer; i--)
  {
    svtkRenderer* aren = static_cast<svtkRenderer*>(rc->GetItemAsObject(i));
    if (aren->IsInViewport(x, y) && aren->GetInteractive())
    {
      currentRenderer = aren;
    }

    if (interactiveren == nullptr && aren->GetInteractive())
    {
      // Save this renderer in case we can't find one in the viewport that
      // is interactive.
      interactiveren = aren;
    }
    if (viewportren == nullptr && aren->IsInViewport(x, y))
    {
      // Save this renderer in case we can't find one in the viewport that
      // is interactive.
      viewportren = aren;
    }
  } // for all renderers

  // We must have a value.  If we found an interactive renderer before, that's
  // better than a non-interactive renderer.
  if (currentRenderer == nullptr)
  {
    currentRenderer = interactiveren;
  }

  // We must have a value.  If we found a renderer that is in the viewport,
  // that is better than any old viewport (but not as good as an interactive
  // one).
  if (currentRenderer == nullptr)
  {
    currentRenderer = viewportren;
  }

  // We must have a value - take anything.
  if (currentRenderer == nullptr)
  {
    currentRenderer = rc->GetFirstRenderer();
  }

  return currentRenderer;
}

//----------------------------------------------------------------------------
void svtkRenderWindowInteractor::SetScale(double scale)
{
  this->LastScale = this->Scale;
  if (this->Scale != scale)
  {
    this->Scale = scale;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkRenderWindowInteractor::SetRotation(double rot)
{
  this->LastRotation = this->Rotation;
  if (this->Rotation != rot)
  {
    this->Rotation = rot;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkRenderWindowInteractor::SetTranslation(double val[2])
{
  this->LastTranslation[0] = this->Translation[0];
  this->LastTranslation[1] = this->Translation[1];
  if (this->Translation[0] != val[0] || this->Translation[1] != val[1])
  {
    this->Translation[0] = val[0];
    this->Translation[1] = val[1];
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkRenderWindowInteractor::RecognizeGesture(svtkCommand::EventIds event)
{
  // we know we are in multitouch now, so start recognizing

  // more than two pointers we ignore
  if (this->PointersDownCount > 2)
  {
    return;
  }

  // store the initial positions
  if (event == svtkCommand::LeftButtonPressEvent)
  {
    for (int i = 0; i < SVTKI_MAX_POINTERS; i++)
    {
      if (this->PointersDown[i])
      {
        this->StartingEventPositions[i][0] = this->EventPositions[i][0];
        this->StartingEventPositions[i][1] = this->EventPositions[i][1];
      }
    }
    // we do not know what the gesture is yet
    this->CurrentGesture = svtkCommand::StartEvent;
    return;
  }

  // end the gesture if needed
  if (event == svtkCommand::LeftButtonReleaseEvent)
  {
    if (this->CurrentGesture == svtkCommand::PinchEvent)
    {
      this->EndPinchEvent();
    }
    if (this->CurrentGesture == svtkCommand::RotateEvent)
    {
      this->EndRotateEvent();
    }
    if (this->CurrentGesture == svtkCommand::PanEvent)
    {
      this->EndPanEvent();
    }
    this->CurrentGesture = svtkCommand::StartEvent;
    return;
  }

  // what are the two pointers we are working with
  int count = 0;
  int* posVals[2];
  int* startVals[2];
  for (int i = 0; i < SVTKI_MAX_POINTERS; i++)
  {
    if (this->PointersDown[i])
    {
      posVals[count] = this->EventPositions[i];
      startVals[count] = this->StartingEventPositions[i];
      count++;
    }
  }

  // The meat of the algorithm
  // on move events we analyze them to determine what type
  // of movement it is and then deal with it.
  if (event == svtkCommand::MouseMoveEvent)
  {
    // calculate the distances
    double originalDistance = sqrt(static_cast<double>(
      (startVals[0][0] - startVals[1][0]) * (startVals[0][0] - startVals[1][0]) +
      (startVals[0][1] - startVals[1][1]) * (startVals[0][1] - startVals[1][1])));
    double newDistance =
      sqrt(static_cast<double>((posVals[0][0] - posVals[1][0]) * (posVals[0][0] - posVals[1][0]) +
        (posVals[0][1] - posVals[1][1]) * (posVals[0][1] - posVals[1][1])));

    // calculate rotations
    double originalAngle = svtkMath::DegreesFromRadians(
      atan2((double)startVals[1][1] - startVals[0][1], (double)startVals[1][0] - startVals[0][0]));
    double newAngle = svtkMath::DegreesFromRadians(
      atan2((double)posVals[1][1] - posVals[0][1], (double)posVals[1][0] - posVals[0][0]));

    // angles are cyclic so watch for that, 1 and 359 are only 2 apart :)
    double angleDeviation = newAngle - originalAngle;
    newAngle = (newAngle + 180.0 >= 360.0 ? newAngle - 180.0 : newAngle + 180.0);
    originalAngle =
      (originalAngle + 180.0 >= 360.0 ? originalAngle - 180.0 : originalAngle + 180.0);
    if (fabs(newAngle - originalAngle) < fabs(angleDeviation))
    {
      angleDeviation = newAngle - originalAngle;
    }

    // calculate the translations
    double trans[2];
    trans[0] = (posVals[0][0] - startVals[0][0] + posVals[1][0] - startVals[1][0]) / 2.0;
    trans[1] = (posVals[0][1] - startVals[0][1] + posVals[1][1] - startVals[1][1]) / 2.0;

    // OK we want to
    // - immediately respond to the user
    // - allow the user to zoom without panning (saves focal point)
    // - allow the user to rotate without panning (saves focal point)

    // do we know what gesture we are doing yet? If not
    // see if we can figure it out
    if (this->CurrentGesture == svtkCommand::StartEvent)
    {
      // pinch is a move to/from the center point
      // rotate is a move along the circumference
      // pan is a move of the center point
      // compute the distance along each of these axes in pixels
      // the first to break thresh wins
      double thresh = 0.01 *
        sqrt(static_cast<double>(this->Size[0] * this->Size[0] + this->Size[1] * this->Size[1]));
      if (thresh < 15.0)
      {
        thresh = 15.0;
      }
      double pinchDistance = fabs(newDistance - originalDistance);
      double rotateDistance = newDistance * svtkMath::Pi() * fabs(angleDeviation) / 360.0;
      double panDistance = sqrt(trans[0] * trans[0] + trans[1] * trans[1]);
      if (pinchDistance > thresh && pinchDistance > rotateDistance && pinchDistance > panDistance)
      {
        this->CurrentGesture = svtkCommand::PinchEvent;
        this->Scale = 1.0;
        this->StartPinchEvent();
      }
      else if (rotateDistance > thresh && rotateDistance > panDistance)
      {
        this->CurrentGesture = svtkCommand::RotateEvent;
        this->Rotation = 0.0;
        this->StartRotateEvent();
      }
      else if (panDistance > thresh)
      {
        this->CurrentGesture = svtkCommand::PanEvent;
        this->Translation[0] = 0.0;
        this->Translation[1] = 0.0;
        this->StartPanEvent();
      }
    }

    // if we have found a specific type of movement then
    // handle it
    if (this->CurrentGesture == svtkCommand::RotateEvent)
    {
      this->SetRotation(angleDeviation);
      this->RotateEvent();
    }

    if (this->CurrentGesture == svtkCommand::PinchEvent)
    {
      svtkErrorMacro("See pinch");
      this->SetScale(newDistance / originalDistance);
      this->PinchEvent();
    }

    if (this->CurrentGesture == svtkCommand::PanEvent)
    {
      this->SetTranslation(trans);
      this->PanEvent();
    }
  }
}

//------------------------------------------------------------------------------
// Timer methods. There are two basic groups of methods, those for backward
// compatibility (group #1) and those that operate on specific timers (i.e.,
// use timer ids). The first group of methods implicitly assume that there is
// only one timer at a time running. This was okay in the old days of SVTK when
// only the interactors used timers. However with the introduction of new 3D
// widgets into SVTK multiple timers often run simultaneously.
//
// old-style group #1
int svtkRenderWindowInteractor::CreateTimer(int timerType)
{
  int platformTimerId, timerId;
  if (timerType == SVTKI_TIMER_FIRST)
  {
    unsigned long duration = this->TimerDuration;
    timerId = svtkTimerId; // just use current id, assume we don't have multiple timers
    platformTimerId = this->InternalCreateTimer(timerId, RepeatingTimer, duration);
    if (0 == platformTimerId)
    {
      return 0;
    }
    (*this->TimerMap)[timerId] = svtkTimerStruct(platformTimerId, RepeatingTimer, duration);
    return timerId;
  }

  else // SVTKI_TIMER_UPDATE is just updating last created timer
  {
    return 1; // do nothing because repeating timer has been created
  }
}

//------------------------------------------------------------------------------
// old-style group #1
// just destroy last one created
int svtkRenderWindowInteractor::DestroyTimer()
{
  int timerId = svtkTimerId;
  svtkTimerIdMapIterator iter = this->TimerMap->find(timerId);
  if (iter != this->TimerMap->end())
  {
    this->InternalDestroyTimer((*iter).second.Id);
    this->TimerMap->erase(iter);
    return 1;
  }

  return 0;
}

//------------------------------------------------------------------------------
// new-style group #2 returns timer id
int svtkRenderWindowInteractor::CreateRepeatingTimer(unsigned long duration)
{
  int timerId = ++svtkTimerId;
  int platformTimerId = this->InternalCreateTimer(timerId, RepeatingTimer, duration);
  if (0 == platformTimerId)
  {
    return 0;
  }
  (*this->TimerMap)[timerId] = svtkTimerStruct(platformTimerId, RepeatingTimer, duration);
  return timerId;
}

//------------------------------------------------------------------------------
// new-style group #2 returns timer id
int svtkRenderWindowInteractor::CreateOneShotTimer(unsigned long duration)
{
  int timerId = ++svtkTimerId;
  int platformTimerId = this->InternalCreateTimer(timerId, OneShotTimer, duration);
  if (0 == platformTimerId)
  {
    return 0;
  }
  (*this->TimerMap)[timerId] = svtkTimerStruct(platformTimerId, OneShotTimer, duration);
  return timerId;
}

//------------------------------------------------------------------------------
// new-style group #2 returns type (non-zero unless bad timerId)
int svtkRenderWindowInteractor::IsOneShotTimer(int timerId)
{
  svtkTimerIdMapIterator iter = this->TimerMap->find(timerId);
  if (iter != this->TimerMap->end())
  {
    return ((*iter).second.Type == OneShotTimer);
  }
  return 0;
}

//------------------------------------------------------------------------------
// new-style group #2 returns duration (non-zero unless bad timerId)
unsigned long svtkRenderWindowInteractor::GetTimerDuration(int timerId)
{
  svtkTimerIdMapIterator iter = this->TimerMap->find(timerId);
  if (iter != this->TimerMap->end())
  {
    return (*iter).second.Duration;
  }
  return 0;
}

//------------------------------------------------------------------------------
// new-style group #2 returns non-zero if timer reset
int svtkRenderWindowInteractor::ResetTimer(int timerId)
{
  svtkTimerIdMapIterator iter = this->TimerMap->find(timerId);
  if (iter != this->TimerMap->end())
  {
    this->InternalDestroyTimer((*iter).second.Id);
    int platformTimerId =
      this->InternalCreateTimer(timerId, (*iter).second.Type, (*iter).second.Duration);
    if (platformTimerId != 0)
    {
      (*iter).second.Id = platformTimerId;
      return 1;
    }
    else
    {
      this->TimerMap->erase(iter);
    }
  }
  return 0;
}

//------------------------------------------------------------------------------
// new-style group #2 returns non-zero if timer destroyed
int svtkRenderWindowInteractor::DestroyTimer(int timerId)
{
  svtkTimerIdMapIterator iter = this->TimerMap->find(timerId);
  if (iter != this->TimerMap->end())
  {
    this->InternalDestroyTimer((*iter).second.Id);
    this->TimerMap->erase(iter);
    return 1;
  }
  return 0;
}

//------------------------------------------------------------------------------
// Stubbed out dummys
int svtkRenderWindowInteractor::InternalCreateTimer(
  int svtkNotUsed(timerId), int svtkNotUsed(timerType), unsigned long svtkNotUsed(duration))
{
  return 0;
}

//------------------------------------------------------------------------------
int svtkRenderWindowInteractor::InternalDestroyTimer(int svtkNotUsed(platformTimerId))
{
  return 0;
}

//------------------------------------------------------------------------------
// Translate from platformTimerId to the corresponding (SVTK) timerId.
// Returns 0 (invalid SVTK timerId) if platformTimerId is not found in the map.
// This first stab at an implementation just iterates the map until it finds
// the sought platformTimerId. If performance becomes an issue (lots of timers,
// all firing frequently...) we could speed this up by making a reverse map so
// this method is a quick map lookup.
int svtkRenderWindowInteractor::GetSVTKTimerId(int platformTimerId)
{
  int timerId = 0;
  svtkTimerIdMapIterator iter = this->TimerMap->begin();
  for (; iter != this->TimerMap->end(); ++iter)
  {
    if ((*iter).second.Id == platformTimerId)
    {
      timerId = (*iter).first;
      break;
    }
  }

  return timerId;
}

//------------------------------------------------------------------------------
// Access to the static variable
int svtkRenderWindowInteractor::GetCurrentTimerId()
{
  return svtkTimerId;
}

//----------------------------------------------------------------------------
void svtkRenderWindowInteractor::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "InteractorStyle:    " << this->InteractorStyle << "\n";
  os << indent << "RenderWindow:    " << this->RenderWindow << "\n";
  if (this->Picker)
  {
    os << indent << "Picker: " << this->Picker << "\n";
  }
  else
  {
    os << indent << "Picker: (none)\n";
  }
  if (this->ObserverMediator)
  {
    os << indent << "Observer Mediator: " << this->ObserverMediator << "\n";
  }
  else
  {
    os << indent << "Observer Mediator: (none)\n";
  }
  os << indent << "LightFollowCamera: " << (this->LightFollowCamera ? "On\n" : "Off\n");
  os << indent << "DesiredUpdateRate: " << this->DesiredUpdateRate << "\n";
  os << indent << "StillUpdateRate: " << this->StillUpdateRate << "\n";
  os << indent << "Initialized: " << this->Initialized << "\n";
  os << indent << "Enabled: " << this->Enabled << "\n";
  os << indent << "EnableRender: " << this->EnableRender << "\n";
  os << indent << "EventPosition: "
     << "( " << this->EventPosition[0] << ", " << this->EventPosition[1] << " )\n";
  os << indent << "LastEventPosition: "
     << "( " << this->LastEventPosition[0] << ", " << this->LastEventPosition[1] << " )\n";
  os << indent << "EventSize: "
     << "( " << this->EventSize[0] << ", " << this->EventSize[1] << " )\n";
  os << indent << "Viewport Size: "
     << "( " << this->Size[0] << ", " << this->Size[1] << " )\n";
  os << indent << "Number of Fly Frames: " << this->NumberOfFlyFrames << "\n";
  os << indent << "Dolly: " << this->Dolly << "\n";
  os << indent << "ControlKey: " << this->ControlKey << "\n";
  os << indent << "AltKey: " << this->AltKey << "\n";
  os << indent << "ShiftKey: " << this->ShiftKey << "\n";
  os << indent << "KeyCode: " << this->KeyCode << "\n";
  os << indent << "KeySym: " << (this->KeySym ? this->KeySym : "(null)") << "\n";
  os << indent << "RepeatCount: " << this->RepeatCount << "\n";
  os << indent << "Timer Duration: " << this->TimerDuration << "\n";
  os << indent << "TimerEventId: " << this->TimerEventId << "\n";
  os << indent << "TimerEventType: " << this->TimerEventType << "\n";
  os << indent << "TimerEventDuration: " << this->TimerEventDuration << "\n";
  os << indent << "TimerEventPlatformId: " << this->TimerEventPlatformId << "\n";
  os << indent << "UseTDx: " << this->UseTDx << endl;
  os << indent << "Recognize Gestures: " << this->RecognizeGestures << endl;
}

//----------------------------------------------------------------------------
void svtkRenderWindowInteractor::Initialize()
{
  this->Initialized = 1;
  this->Enable();
  this->Render();
}

//----------------------------------------------------------------------------
void svtkRenderWindowInteractor::HideCursor()
{
  this->RenderWindow->HideCursor();
}

//----------------------------------------------------------------------------
void svtkRenderWindowInteractor::ShowCursor()
{
  this->RenderWindow->ShowCursor();
}

//----------------------------------------------------------------------------
svtkObserverMediator* svtkRenderWindowInteractor::GetObserverMediator()
{
  if (!this->ObserverMediator)
  {
    this->ObserverMediator = svtkObserverMediator::New();
    this->ObserverMediator->SetInteractor(this);
  }

  return this->ObserverMediator;
}

//------------------------------------------------------------------
void svtkRenderWindowInteractor::MouseMoveEvent()
{
  if (!this->Enabled)
  {
    return;
  }

  // handle gestures or not?
  if (this->RecognizeGestures && this->PointersDownCount > 1)
  {
    // handle the gesture
    this->RecognizeGesture(svtkCommand::MouseMoveEvent);
  }
  else
  {
    this->InvokeEvent(svtkCommand::MouseMoveEvent, nullptr);
  }
}

//------------------------------------------------------------------
void svtkRenderWindowInteractor::RightButtonPressEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(svtkCommand::RightButtonPressEvent, nullptr);
}

//------------------------------------------------------------------
void svtkRenderWindowInteractor::RightButtonReleaseEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(svtkCommand::RightButtonReleaseEvent, nullptr);
}

//------------------------------------------------------------------
void svtkRenderWindowInteractor::LeftButtonPressEvent()
{
  if (!this->Enabled)
  {
    return;
  }

  // are we translating multitouch into gestures?
  if (this->RecognizeGestures)
  {
    if (!this->PointersDown[this->PointerIndex])
    {
      this->PointersDown[this->PointerIndex] = 1;
      this->PointersDownCount++;
    }
    // do we have multitouch
    if (this->PointersDownCount > 1)
    {
      // did we just transition to multitouch?
      if (this->PointersDownCount == 2)
      {
        this->InvokeEvent(svtkCommand::LeftButtonReleaseEvent, nullptr);
      }
      // handle the gesture
      this->RecognizeGesture(svtkCommand::LeftButtonPressEvent);
      return;
    }
  }

  this->InvokeEvent(svtkCommand::LeftButtonPressEvent, nullptr);
}

//------------------------------------------------------------------
void svtkRenderWindowInteractor::LeftButtonReleaseEvent()
{
  if (!this->Enabled)
  {
    return;
  }

  if (this->RecognizeGestures)
  {
    if (this->PointersDown[this->PointerIndex])
    {
      this->PointersDown[this->PointerIndex] = 0;
      this->PointersDownCount--;
    }
    // do we have multitouch
    if (this->PointersDownCount > 1)
    {
      // handle the gesture
      this->RecognizeGesture(svtkCommand::LeftButtonReleaseEvent);
      return;
    }
  }
  this->InvokeEvent(svtkCommand::LeftButtonReleaseEvent, nullptr);
}

//------------------------------------------------------------------
void svtkRenderWindowInteractor::MiddleButtonPressEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(svtkCommand::MiddleButtonPressEvent, nullptr);
}

//------------------------------------------------------------------
void svtkRenderWindowInteractor::MiddleButtonReleaseEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(svtkCommand::MiddleButtonReleaseEvent, nullptr);
}

//------------------------------------------------------------------
void svtkRenderWindowInteractor::MouseWheelForwardEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(svtkCommand::MouseWheelForwardEvent, nullptr);
}

//------------------------------------------------------------------
void svtkRenderWindowInteractor::MouseWheelBackwardEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(svtkCommand::MouseWheelBackwardEvent, nullptr);
}

//------------------------------------------------------------------
void svtkRenderWindowInteractor::ExposeEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(svtkCommand::ExposeEvent, nullptr);
}

//------------------------------------------------------------------
void svtkRenderWindowInteractor::ConfigureEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(svtkCommand::ConfigureEvent, nullptr);
}

//------------------------------------------------------------------
void svtkRenderWindowInteractor::EnterEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(svtkCommand::EnterEvent, nullptr);
}

//------------------------------------------------------------------
void svtkRenderWindowInteractor::LeaveEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(svtkCommand::LeaveEvent, nullptr);
}

//------------------------------------------------------------------
void svtkRenderWindowInteractor::KeyPressEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(svtkCommand::KeyPressEvent, nullptr);
}

//------------------------------------------------------------------
void svtkRenderWindowInteractor::KeyReleaseEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(svtkCommand::KeyReleaseEvent, nullptr);
}

//------------------------------------------------------------------
void svtkRenderWindowInteractor::CharEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(svtkCommand::CharEvent, nullptr);
}

//------------------------------------------------------------------
void svtkRenderWindowInteractor::ExitEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(svtkCommand::ExitEvent, nullptr);
}

//------------------------------------------------------------------
void svtkRenderWindowInteractor::FourthButtonPressEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(svtkCommand::FourthButtonPressEvent, nullptr);
}

//------------------------------------------------------------------
void svtkRenderWindowInteractor::FourthButtonReleaseEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(svtkCommand::FourthButtonReleaseEvent, nullptr);
}

//------------------------------------------------------------------
void svtkRenderWindowInteractor::FifthButtonPressEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(svtkCommand::FifthButtonPressEvent, nullptr);
}

//------------------------------------------------------------------
void svtkRenderWindowInteractor::FifthButtonReleaseEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(svtkCommand::FifthButtonReleaseEvent, nullptr);
}

//------------------------------------------------------------------
void svtkRenderWindowInteractor::StartPinchEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(svtkCommand::StartPinchEvent, nullptr);
}

//------------------------------------------------------------------
void svtkRenderWindowInteractor::PinchEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(svtkCommand::PinchEvent, nullptr);
}

//------------------------------------------------------------------
void svtkRenderWindowInteractor::EndPinchEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(svtkCommand::EndPinchEvent, nullptr);
}

//------------------------------------------------------------------
void svtkRenderWindowInteractor::StartRotateEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(svtkCommand::StartRotateEvent, nullptr);
}

//------------------------------------------------------------------
void svtkRenderWindowInteractor::RotateEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(svtkCommand::RotateEvent, nullptr);
}

//------------------------------------------------------------------
void svtkRenderWindowInteractor::EndRotateEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(svtkCommand::EndRotateEvent, nullptr);
}

//------------------------------------------------------------------
void svtkRenderWindowInteractor::StartPanEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(svtkCommand::StartPanEvent, nullptr);
}

//------------------------------------------------------------------
void svtkRenderWindowInteractor::PanEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(svtkCommand::PanEvent, nullptr);
}

//------------------------------------------------------------------
void svtkRenderWindowInteractor::EndPanEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(svtkCommand::EndPanEvent, nullptr);
}

//------------------------------------------------------------------
void svtkRenderWindowInteractor::TapEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(svtkCommand::TapEvent, nullptr);
}

//------------------------------------------------------------------
void svtkRenderWindowInteractor::LongTapEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(svtkCommand::LongTapEvent, nullptr);
}

//------------------------------------------------------------------
void svtkRenderWindowInteractor::SwipeEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(svtkCommand::SwipeEvent, nullptr);
}
