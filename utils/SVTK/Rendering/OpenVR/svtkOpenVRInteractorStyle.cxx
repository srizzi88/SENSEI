/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkOpenVRInteractorStyle.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOpenVRInteractorStyle.h"

#include "svtkCompositeDataSet.h"
#include "svtkDataObjectTreeIterator.h"

#include "svtkBillboardTextActor3D.h"
#include "svtkCoordinate.h"
#include "svtkTextActor.h"
#include "svtkTextActor3D.h"

#include "svtkAssemblyPath.h"
#include "svtkCallbackCommand.h"
#include "svtkCamera.h"
#include "svtkCellData.h"
#include "svtkCellPicker.h"
#include "svtkDataObject.h"
#include "svtkDataSet.h"
#include "svtkHardwareSelector.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkMapper.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkOpenVRCamera.h"
#include "svtkOpenVRControlsHelper.h"
#include "svtkOpenVRHardwarePicker.h"
#include "svtkOpenVRModel.h"
#include "svtkOpenVROverlay.h"
#include "svtkOpenVRRenderWindow.h"
#include "svtkOpenVRRenderWindowInteractor.h"
#include "svtkPlane.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkPropPicker.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSphereSource.h"
#include "svtkStringArray.h"
#include "svtkTextProperty.h"
#include "svtkTimerLog.h"

#include "svtkOpenVRMenuRepresentation.h"
#include "svtkOpenVRMenuWidget.h"

#include <sstream>

// Map controller inputs to interaction states
svtkStandardNewMacro(svtkOpenVRInteractorStyle);

//----------------------------------------------------------------------------
svtkOpenVRInteractorStyle::svtkOpenVRInteractorStyle()
{
  for (int d = 0; d < svtkEventDataNumberOfDevices; ++d)
  {
    this->InteractionState[d] = SVTKIS_NONE;
    this->InteractionProps[d] = nullptr;
    this->ClippingPlanes[d] = nullptr;

    for (int i = 0; i < svtkEventDataNumberOfInputs; i++)
    {
      this->InputMap[d][i] = -1;
      this->ControlsHelpers[d][i] = nullptr;
    }
  }

  // Create default inputs mapping
  this->MapInputToAction(
    svtkEventDataDevice::RightController, svtkEventDataDeviceInput::Trigger, SVTKIS_POSITION_PROP);
  this->MapInputToAction(
    svtkEventDataDevice::RightController, svtkEventDataDeviceInput::TrackPad, SVTKIS_DOLLY);
  this->MapInputToAction(
    svtkEventDataDevice::RightController, svtkEventDataDeviceInput::ApplicationMenu, SVTKIS_MENU);

  this->MapInputToAction(svtkEventDataDevice::LeftController,
    svtkEventDataDeviceInput::ApplicationMenu, SVTKIS_TOGGLE_DRAW_CONTROLS);
  this->MapInputToAction(
    svtkEventDataDevice::LeftController, svtkEventDataDeviceInput::Trigger, SVTKIS_LOAD_CAMERA_POSE);

  this->AddTooltipForInput(svtkEventDataDevice::RightController,
    svtkEventDataDeviceInput::ApplicationMenu, "Application Menu");

  this->MenuCommand = svtkCallbackCommand::New();
  this->MenuCommand->SetClientData(this);
  this->MenuCommand->SetCallback(svtkOpenVRInteractorStyle::MenuCallback);

  this->Menu->SetRepresentation(this->MenuRepresentation);
  this->Menu->PushFrontMenuItem("exit", "Exit", this->MenuCommand);
  this->Menu->PushFrontMenuItem("togglelabel", "Toggle Controller Labels", this->MenuCommand);
  this->Menu->PushFrontMenuItem("clipmode", "Clipping Mode", this->MenuCommand);
  this->Menu->PushFrontMenuItem("probemode", "Probe Mode", this->MenuCommand);
  this->Menu->PushFrontMenuItem("grabmode", "Grab Mode", this->MenuCommand);

  svtkNew<svtkPolyDataMapper> pdm;
  this->PickActor->SetMapper(pdm);
  this->PickActor->GetProperty()->SetLineWidth(4);
  this->PickActor->GetProperty()->RenderLinesAsTubesOn();
  this->PickActor->GetProperty()->SetRepresentationToWireframe();
  this->PickActor->DragableOff();

  this->HoverPickOff();
  this->GrabWithRayOff();

  svtkNew<svtkCellPicker> exactPicker;
  this->SetInteractionPicker(exactPicker);
}

//----------------------------------------------------------------------------
svtkOpenVRInteractorStyle::~svtkOpenVRInteractorStyle()
{
  for (int d = 0; d < svtkEventDataNumberOfDevices; ++d)
  {
    if (this->ClippingPlanes[d])
    {
      this->ClippingPlanes[d]->Delete();
      this->ClippingPlanes[d] = nullptr;
    }
  }
  for (int d = 0; d < svtkEventDataNumberOfDevices; ++d)
  {
    for (int i = 0; i < svtkEventDataNumberOfInputs; i++)
    {
      if (this->ControlsHelpers[d][i])
      {
        this->ControlsHelpers[d][i]->Delete();
        this->ControlsHelpers[d][i] = nullptr;
      }
    }
  }
  this->MenuCommand->Delete();
}

void svtkOpenVRInteractorStyle::SetInteractor(svtkRenderWindowInteractor* iren)
{
  this->Superclass::SetInteractor(iren);
}

//----------------------------------------------------------------------------
void svtkOpenVRInteractorStyle::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void svtkOpenVRInteractorStyle::MenuCallback(
  svtkObject* svtkNotUsed(object), unsigned long, void* clientdata, void* calldata)
{
  std::string name = static_cast<const char*>(calldata);
  svtkOpenVRInteractorStyle* self = static_cast<svtkOpenVRInteractorStyle*>(clientdata);

  if (name == "exit")
  {
    if (self->Interactor)
    {
      self->Interactor->ExitCallback();
    }
  }
  if (name == "togglelabel")
  {
    self->ToggleDrawControls();
  }
  if (name == "clipmode")
  {
    self->MapInputToAction(
      svtkEventDataDevice::RightController, svtkEventDataDeviceInput::Trigger, SVTKIS_CLIP);
  }
  if (name == "grabmode")
  {
    self->MapInputToAction(
      svtkEventDataDevice::RightController, svtkEventDataDeviceInput::Trigger, SVTKIS_POSITION_PROP);
  }
  if (name == "probemode")
  {
    self->MapInputToAction(
      svtkEventDataDevice::RightController, svtkEventDataDeviceInput::Trigger, SVTKIS_PICK);
  }
}

//----------------------------------------------------------------------------
// Generic events binding
//----------------------------------------------------------------------------
void svtkOpenVRInteractorStyle::OnMove3D(svtkEventData* edata)
{
  svtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
  if (!edd)
  {
    return;
  }

  // joystick moves?
  int idev = static_cast<int>(edd->GetDevice());

  // Update current state
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  // Set current state and interaction prop
  this->InteractionProp = this->InteractionProps[idev];

  switch (this->InteractionState[idev])
  {
    case SVTKIS_POSITION_PROP:
      this->FindPokedRenderer(x, y);
      this->PositionProp(edd);
      this->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
      break;
    case SVTKIS_DOLLY:
      this->FindPokedRenderer(x, y);
      this->Dolly3D(edata);
      this->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
      break;
    case SVTKIS_CLIP:
      this->FindPokedRenderer(x, y);
      this->Clip(edd);
      this->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
      break;
  }

  // Update rays
  this->UpdateRay(edd->GetDevice());
}

//----------------------------------------------------------------------------
void svtkOpenVRInteractorStyle::OnButton3D(svtkEventData* edata)
{
  svtkEventDataDevice3D* bd = edata->GetAsEventDataDevice3D();
  if (!bd)
  {
    return;
  }

  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];
  this->FindPokedRenderer(x, y);

  int state = this->InputMap[static_cast<int>(bd->GetDevice())][static_cast<int>(bd->GetInput())];
  if (state == -1)
  {
    return;
  }

  // right trigger press/release
  if (bd->GetAction() == svtkEventDataAction::Press)
  {
    this->StartAction(state, bd);
  }
  if (bd->GetAction() == svtkEventDataAction::Release)
  {
    this->EndAction(state, bd);
  }
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Interaction entry points
//----------------------------------------------------------------------------
void svtkOpenVRInteractorStyle::StartPick(svtkEventDataDevice3D* edata)
{
  this->HideBillboard();
  this->HidePickActor();

  this->InteractionState[static_cast<int>(edata->GetDevice())] = SVTKIS_PICK;

  // update ray
  this->UpdateRay(edata->GetDevice());
}
//----------------------------------------------------------------------------
void svtkOpenVRInteractorStyle::EndPick(svtkEventDataDevice3D* edata)
{
  // perform probe
  this->ProbeData(edata->GetDevice());

  this->InteractionState[static_cast<int>(edata->GetDevice())] = SVTKIS_NONE;

  // turn off ray
  this->UpdateRay(edata->GetDevice());
}

//----------------------------------------------------------------------------
void svtkOpenVRInteractorStyle::StartLoadCamPose(svtkEventDataDevice3D* edata)
{
  int iDevice = static_cast<int>(edata->GetDevice());
  this->InteractionState[iDevice] = SVTKIS_LOAD_CAMERA_POSE;
}
//----------------------------------------------------------------------------
void svtkOpenVRInteractorStyle::EndLoadCamPose(svtkEventDataDevice3D* edata)
{
  this->LoadNextCameraPose();

  int iDevice = static_cast<int>(edata->GetDevice());
  this->InteractionState[iDevice] = SVTKIS_NONE;
}

//----------------------------------------------------------------------------
bool svtkOpenVRInteractorStyle::HardwareSelect(svtkEventDataDevice controller, bool actorPassOnly)
{
  svtkRenderer* ren = this->CurrentRenderer;
  svtkOpenVRRenderWindow* renWin =
    svtkOpenVRRenderWindow::SafeDownCast(this->Interactor->GetRenderWindow());
  svtkOpenVRRenderWindowInteractor* iren =
    static_cast<svtkOpenVRRenderWindowInteractor*>(this->Interactor);

  if (!ren || !renWin || !iren)
  {
    return false;
  }

  svtkOpenVRModel* cmodel = renWin->GetTrackedDeviceModel(controller);
  if (!cmodel)
  {
    return false;
  }

  cmodel->SetVisibility(false);

  // Compute controller position and world orientation
  double p0[3];   // Ray start point
  double wxyz[4]; // Controller orientation
  double dummy_ppos[3];
  double wdir[3];
  vr::TrackedDevicePose_t& tdPose = renWin->GetTrackedDevicePose(cmodel->TrackedDevice);
  iren->ConvertPoseToWorldCoordinates(tdPose, p0, wxyz, dummy_ppos, wdir);

  this->HardwarePicker->PickProp(p0, wxyz, ren, ren->GetViewProps(), actorPassOnly);

  cmodel->SetVisibility(true);

  return true;
}

//----------------------------------------------------------------------------
void svtkOpenVRInteractorStyle::StartPositionProp(svtkEventDataDevice3D* edata)
{
  if (this->GrabWithRay)
  {
    if (!this->HardwareSelect(edata->GetDevice(), true))
    {
      return;
    }

    svtkSelection* selection = this->HardwarePicker->GetSelection();

    if (!selection || selection->GetNumberOfNodes() == 0)
    {
      return;
    }

    svtkSelectionNode* node = selection->GetNode(0);
    this->InteractionProp =
      svtkProp3D::SafeDownCast(node->GetProperties()->Get(svtkSelectionNode::PROP()));
  }
  else
  {
    double pos[3];
    edata->GetWorldPosition(pos);
    this->FindPickedActor(pos, nullptr);
  }

  if (this->InteractionProp == nullptr)
  {
    return;
  }

  this->InteractionState[static_cast<int>(edata->GetDevice())] = SVTKIS_POSITION_PROP;
  this->InteractionProps[static_cast<int>(edata->GetDevice())] = this->InteractionProp;

  // Don't start action if a controller is already positioning the prop
  int rc = static_cast<int>(svtkEventDataDevice::RightController);
  int lc = static_cast<int>(svtkEventDataDevice::LeftController);
  if (this->InteractionProps[rc] == this->InteractionProps[lc])
  {
    this->EndPositionProp(edata);
    return;
  }
}

//----------------------------------------------------------------------------
void svtkOpenVRInteractorStyle::EndPositionProp(svtkEventDataDevice3D* edata)
{
  svtkEventDataDevice dev = edata->GetDevice();
  this->InteractionState[static_cast<int>(dev)] = SVTKIS_NONE;
  this->InteractionProps[static_cast<int>(dev)] = nullptr;
}

//----------------------------------------------------------------------------
void svtkOpenVRInteractorStyle::StartClip(svtkEventDataDevice3D* ed)
{
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  svtkEventDataDevice dev = ed->GetDevice();
  this->InteractionState[static_cast<int>(dev)] = SVTKIS_CLIP;

  if (!this->ClippingPlanes[static_cast<int>(dev)])
  {
    this->ClippingPlanes[static_cast<int>(dev)] = svtkPlane::New();
  }

  svtkActorCollection* ac;
  svtkActor *anActor, *aPart;
  svtkAssemblyPath* path;
  if (this->CurrentRenderer != 0)
  {
    ac = this->CurrentRenderer->GetActors();
    svtkCollectionSimpleIterator ait;
    for (ac->InitTraversal(ait); (anActor = ac->GetNextActor(ait));)
    {
      for (anActor->InitPathTraversal(); (path = anActor->GetNextPath());)
      {
        aPart = static_cast<svtkActor*>(path->GetLastNode()->GetViewProp());
        if (aPart->GetMapper())
        {
          aPart->GetMapper()->AddClippingPlane(this->ClippingPlanes[static_cast<int>(dev)]);
          continue;
        }
      }
    }
  }
  else
  {
    svtkWarningMacro(<< "no current renderer on the interactor style.");
  }
}

//----------------------------------------------------------------------------
void svtkOpenVRInteractorStyle::EndClip(svtkEventDataDevice3D* ed)
{
  svtkEventDataDevice dev = ed->GetDevice();
  this->InteractionState[static_cast<int>(dev)] = SVTKIS_NONE;

  svtkActorCollection* ac;
  svtkActor *anActor, *aPart;
  svtkAssemblyPath* path;
  if (this->CurrentRenderer != 0)
  {
    ac = this->CurrentRenderer->GetActors();
    svtkCollectionSimpleIterator ait;
    for (ac->InitTraversal(ait); (anActor = ac->GetNextActor(ait));)
    {
      for (anActor->InitPathTraversal(); (path = anActor->GetNextPath());)
      {
        aPart = static_cast<svtkActor*>(path->GetLastNode()->GetViewProp());
        if (aPart->GetMapper())
        {
          aPart->GetMapper()->RemoveClippingPlane(this->ClippingPlanes[static_cast<int>(dev)]);
          continue;
        }
      }
    }
  }
  else
  {
    svtkWarningMacro(<< "no current renderer on the interactor style.");
  }
}

//----------------------------------------------------------------------------
void svtkOpenVRInteractorStyle::StartDolly3D(svtkEventDataDevice3D* ed)
{
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }
  svtkEventDataDevice dev = ed->GetDevice();
  this->InteractionState[static_cast<int>(dev)] = SVTKIS_DOLLY;
  this->LastDolly3DEventTime->StartTimer();

  // this->GrabFocus(this->EventCallbackCommand);
}
//----------------------------------------------------------------------------
void svtkOpenVRInteractorStyle::EndDolly3D(svtkEventDataDevice3D* ed)
{
  svtkEventDataDevice dev = ed->GetDevice();
  this->InteractionState[static_cast<int>(dev)] = SVTKIS_NONE;

  this->LastDolly3DEventTime->StopTimer();
}

//----------------------------------------------------------------------------
void svtkOpenVRInteractorStyle::ToggleDrawControls()
{
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  // Enable helpers
  for (int d = 0; d < svtkEventDataNumberOfDevices; ++d)
  {
    // No helper for HMD
    if (static_cast<svtkEventDataDevice>(d) == svtkEventDataDevice::HeadMountedDisplay)
    {
      continue;
    }

    for (int i = 0; i < svtkEventDataNumberOfInputs; i++)
    {
      if (this->ControlsHelpers[d][i])
      {
        if (this->ControlsHelpers[d][i]->GetRenderer() != this->CurrentRenderer)
        {
          svtkRenderer* ren = this->ControlsHelpers[d][i]->GetRenderer();
          if (ren)
          {
            ren->RemoveViewProp(this->ControlsHelpers[d][i]);
          }
          this->ControlsHelpers[d][i]->SetRenderer(this->CurrentRenderer);
          this->ControlsHelpers[d][i]->BuildRepresentation();
          // this->ControlsHelpers[iDevice][iInput]->SetEnabled(false);
          this->CurrentRenderer->AddViewProp(this->ControlsHelpers[d][i]);
        }

        this->ControlsHelpers[d][i]->SetEnabled(!this->ControlsHelpers[d][i]->GetEnabled());
      }
    }
  }
}

void svtkOpenVRInteractorStyle::SetDrawControls(bool val)
{
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  // Enable helpers
  for (int d = 0; d < svtkEventDataNumberOfDevices; ++d)
  {
    // No helper for HMD
    if (static_cast<svtkEventDataDevice>(d) == svtkEventDataDevice::HeadMountedDisplay)
    {
      continue;
    }

    for (int i = 0; i < svtkEventDataNumberOfInputs; i++)
    {
      if (this->ControlsHelpers[d][i])
      {
        if (this->ControlsHelpers[d][i]->GetRenderer() != this->CurrentRenderer)
        {
          svtkRenderer* ren = this->ControlsHelpers[d][i]->GetRenderer();
          if (ren)
          {
            ren->RemoveViewProp(this->ControlsHelpers[d][i]);
          }
          this->ControlsHelpers[d][i]->SetRenderer(this->CurrentRenderer);
          this->ControlsHelpers[d][i]->BuildRepresentation();
          // this->ControlsHelpers[iDevice][iInput]->SetEnabled(false);
          this->CurrentRenderer->AddViewProp(this->ControlsHelpers[d][i]);
        }

        this->ControlsHelpers[d][i]->SetEnabled(val);
      }
    }
  }
}

//----------------------------------------------------------------------------
// Interaction methods
//----------------------------------------------------------------------------
void svtkOpenVRInteractorStyle::ProbeData(svtkEventDataDevice controller)
{
  // Invoke start pick method if defined
  this->InvokeEvent(svtkCommand::StartPickEvent, nullptr);

  if (!this->HardwareSelect(controller, false))
  {
    return;
  }

  // Invoke end pick method if defined
  if (this->HandleObservers && this->HasObserver(svtkCommand::EndPickEvent))
  {
    this->InvokeEvent(svtkCommand::EndPickEvent, this->HardwarePicker->GetSelection());
  }
  else
  {
    this->EndPickCallback(this->HardwarePicker->GetSelection());
  }
}

void svtkOpenVRInteractorStyle::EndPickCallback(svtkSelection* sel)
{
  if (!sel)
  {
    return;
  }

  svtkSelectionNode* node = sel->GetNode(0);
  if (!node || !node->GetProperties()->Has(svtkSelectionNode::PROP()))
  {
    return;
  }

  svtkProp3D* prop = svtkProp3D::SafeDownCast(node->GetProperties()->Get(svtkSelectionNode::PROP()));
  if (!prop)
  {
    return;
  }
  this->ShowPickSphere(prop->GetCenter(), prop->GetLength() / 2.0, nullptr);
}

//----------------------------------------------------------------------------
void svtkOpenVRInteractorStyle::LoadNextCameraPose()
{
  svtkOpenVRRenderWindow* renWin =
    svtkOpenVRRenderWindow::SafeDownCast(this->Interactor->GetRenderWindow());
  if (!renWin)
  {
    return;
  }
  svtkOpenVROverlay* ovl = renWin->GetDashboardOverlay();
  ovl->LoadNextCameraPose();
}

//----------------------------------------------------------------------------
void svtkOpenVRInteractorStyle::PositionProp(svtkEventData* ed)
{
  if (this->InteractionProp == nullptr || !this->InteractionProp->GetDragable())
  {
    return;
  }
  this->Superclass::PositionProp(ed);
}

//----------------------------------------------------------------------------
void svtkOpenVRInteractorStyle::Clip(svtkEventDataDevice3D* ed)
{
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  const double* wpos = ed->GetWorldPosition();
  const double* wori = ed->GetWorldOrientation();

  double ori[4];
  ori[0] = svtkMath::RadiansFromDegrees(wori[0]);
  ori[1] = wori[1];
  ori[2] = wori[2];
  ori[3] = wori[3];

  // we have a position and a normal, that defines our plane
  // plane->SetOrigin(wpos[0], wpos[1], wpos[2]);

  double r[3];
  double up[3];
  up[0] = 0;
  up[1] = -1;
  up[2] = 0;
  svtkMath::RotateVectorByWXYZ(up, ori, r);
  // plane->SetNormal(r);

  svtkEventDataDevice dev = ed->GetDevice();
  int idev = static_cast<int>(dev);
  this->ClippingPlanes[idev]->SetNormal(r);
  this->ClippingPlanes[idev]->SetOrigin(wpos[0], wpos[1], wpos[2]);
}

//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Multitouch interaction methods
//----------------------------------------------------------------------------
void svtkOpenVRInteractorStyle::OnPan()
{
  int rc = static_cast<int>(svtkEventDataDevice::RightController);
  int lc = static_cast<int>(svtkEventDataDevice::LeftController);

  if (!this->InteractionProps[rc] && !this->InteractionProps[lc])
  {
    this->InteractionState[rc] = SVTKIS_PAN;
    this->InteractionState[lc] = SVTKIS_PAN;

    int pointer = this->Interactor->GetPointerIndex();

    this->FindPokedRenderer(this->Interactor->GetEventPositions(pointer)[0],
      this->Interactor->GetEventPositions(pointer)[1]);

    if (this->CurrentRenderer == nullptr)
    {
      return;
    }

    svtkCamera* camera = this->CurrentRenderer->GetActiveCamera();
    svtkRenderWindowInteractor3D* rwi = static_cast<svtkRenderWindowInteractor3D*>(this->Interactor);

    double t[3] = { rwi->GetTranslation3D()[0] - rwi->GetLastTranslation3D()[0],
      rwi->GetTranslation3D()[1] - rwi->GetLastTranslation3D()[1],
      rwi->GetTranslation3D()[2] - rwi->GetLastTranslation3D()[2] };

    double* ptrans = rwi->GetPhysicalTranslation(camera);
    double physicalScale = rwi->GetPhysicalScale();

    rwi->SetPhysicalTranslation(camera, ptrans[0] + t[0] * physicalScale,
      ptrans[1] + t[1] * physicalScale, ptrans[2] + t[2] * physicalScale);

    // clean up
    if (this->Interactor->GetLightFollowCamera())
    {
      this->CurrentRenderer->UpdateLightsGeometryToFollowCamera();
    }
  }
}
//----------------------------------------------------------------------------
void svtkOpenVRInteractorStyle::OnPinch()
{
  int rc = static_cast<int>(svtkEventDataDevice::RightController);
  int lc = static_cast<int>(svtkEventDataDevice::LeftController);

  if (!this->InteractionProps[rc] && !this->InteractionProps[lc])
  {
    this->InteractionState[rc] = SVTKIS_ZOOM;
    this->InteractionState[lc] = SVTKIS_ZOOM;

    int pointer = this->Interactor->GetPointerIndex();

    this->FindPokedRenderer(this->Interactor->GetEventPositions(pointer)[0],
      this->Interactor->GetEventPositions(pointer)[1]);

    if (this->CurrentRenderer == nullptr)
    {
      return;
    }

    double dyf = this->Interactor->GetScale() / this->Interactor->GetLastScale();
    svtkCamera* camera = this->CurrentRenderer->GetActiveCamera();
    svtkRenderWindowInteractor3D* rwi = static_cast<svtkRenderWindowInteractor3D*>(this->Interactor);
    double physicalScale = rwi->GetPhysicalScale();

    this->SetScale(camera, physicalScale / dyf);
  }
}
//----------------------------------------------------------------------------
void svtkOpenVRInteractorStyle::OnRotate()
{
  int rc = static_cast<int>(svtkEventDataDevice::RightController);
  int lc = static_cast<int>(svtkEventDataDevice::LeftController);

  // Rotate only when one controller is not interacting
  if ((this->InteractionProps[rc] || this->InteractionProps[lc]) &&
    (!this->InteractionProps[rc] || !this->InteractionProps[lc]))
  {
    this->InteractionState[rc] = SVTKIS_ROTATE;
    this->InteractionState[lc] = SVTKIS_ROTATE;

    double angle = this->Interactor->GetRotation() - this->Interactor->GetLastRotation();

    if (this->InteractionProps[rc])
    {
      this->InteractionProps[rc]->RotateY(angle);
    }
    if (this->InteractionProps[lc])
    {
      this->InteractionProps[lc]->RotateY(angle);
    }
  }
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Utility routines
//----------------------------------------------------------------------------
void svtkOpenVRInteractorStyle::MapInputToAction(
  svtkEventDataDevice device, svtkEventDataDeviceInput input, int state)
{
  if (input >= svtkEventDataDeviceInput::NumberOfInputs || state < SVTKIS_NONE)
  {
    return;
  }

  int oldstate = this->InputMap[static_cast<int>(device)][static_cast<int>(input)];
  if (oldstate == state)
  {
    return;
  }

  this->InputMap[static_cast<int>(device)][static_cast<int>(input)] = state;
  this->AddTooltipForInput(device, input);

  this->Modified();
}

//----------------------------------------------------------------------------
void svtkOpenVRInteractorStyle::StartAction(int state, svtkEventDataDevice3D* edata)
{
  switch (state)
  {
    case SVTKIS_POSITION_PROP:
      this->StartPositionProp(edata);
      break;
    case SVTKIS_DOLLY:
      this->StartDolly3D(edata);
      break;
    case SVTKIS_CLIP:
      this->StartClip(edata);
      break;
    case SVTKIS_PICK:
      this->StartPick(edata);
      break;
    case SVTKIS_LOAD_CAMERA_POSE:
      this->StartLoadCamPose(edata);
      break;
  }
}
//----------------------------------------------------------------------------
void svtkOpenVRInteractorStyle::EndAction(int state, svtkEventDataDevice3D* edata)
{
  switch (state)
  {
    case SVTKIS_POSITION_PROP:
      this->EndPositionProp(edata);
      break;
    case SVTKIS_DOLLY:
      this->EndDolly3D(edata);
      break;
    case SVTKIS_CLIP:
      this->EndClip(edata);
      break;
    case SVTKIS_PICK:
      this->EndPick(edata);
      break;
    case SVTKIS_MENU:
      this->Menu->SetInteractor(this->Interactor);
      this->Menu->Show(edata);
      break;
    case SVTKIS_LOAD_CAMERA_POSE:
      this->EndLoadCamPose(edata);
      break;
    case SVTKIS_TOGGLE_DRAW_CONTROLS:
      this->ToggleDrawControls();
      break;
    case SVTKIS_EXIT:
      if (this->Interactor)
      {
        this->Interactor->ExitCallback();
      }
      break;
  }

  // Reset multitouch state because a button has been released
  for (int d = 0; d < svtkEventDataNumberOfDevices; ++d)
  {
    switch (this->InteractionState[d])
    {
      case SVTKIS_PAN:
      case SVTKIS_ZOOM:
      case SVTKIS_ROTATE:
        this->InteractionState[d] = SVTKIS_NONE;
        break;
    }
  }
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Handle Ray drawing and update
//----------------------------------------------------------------------------
void svtkOpenVRInteractorStyle::ShowRay(svtkEventDataDevice controller)
{
  svtkOpenVRRenderWindow* renWin =
    svtkOpenVRRenderWindow::SafeDownCast(this->Interactor->GetRenderWindow());
  if (!renWin ||
    (controller != svtkEventDataDevice::LeftController &&
      controller != svtkEventDataDevice::RightController))
  {
    return;
  }
  svtkOpenVRModel* cmodel = renWin->GetTrackedDeviceModel(controller);
  if (cmodel)
  {
    cmodel->SetShowRay(true);
  }
}
//----------------------------------------------------------------------------
void svtkOpenVRInteractorStyle::HideRay(svtkEventDataDevice controller)
{
  svtkOpenVRRenderWindow* renWin =
    svtkOpenVRRenderWindow::SafeDownCast(this->Interactor->GetRenderWindow());
  if (!renWin ||
    (controller != svtkEventDataDevice::LeftController &&
      controller != svtkEventDataDevice::RightController))
  {
    return;
  }
  svtkOpenVRModel* cmodel = renWin->GetTrackedDeviceModel(controller);
  if (cmodel)
  {
    cmodel->SetShowRay(false);
  }
}
//----------------------------------------------------------------------------
void svtkOpenVRInteractorStyle::UpdateRay(svtkEventDataDevice controller)
{
  if (!this->Interactor)
  {
    return;
  }

  svtkRenderer* ren = this->CurrentRenderer;
  svtkOpenVRRenderWindow* renWin =
    svtkOpenVRRenderWindow::SafeDownCast(this->Interactor->GetRenderWindow());
  svtkOpenVRRenderWindowInteractor* iren =
    static_cast<svtkOpenVRRenderWindowInteractor*>(this->Interactor);

  if (!ren || !renWin || !iren)
  {
    return;
  }

  vr::TrackedDeviceIndex_t idx = renWin->GetTrackedDeviceIndexForDevice(controller);
  if (idx == vr::k_unTrackedDeviceIndexInvalid)
  {
    return;
  }
  svtkOpenVRModel* mod = renWin->GetTrackedDeviceModel(idx);
  if (!mod)
  {
    return;
  }

  int idev = static_cast<int>(controller);

  // Keep the same ray if a controller is interacting with a prop
  if (this->InteractionProps[idev] != nullptr)
  {
    return;
  }

  // Check if interacting with a widget
  svtkPropCollection* props = ren->GetViewProps();

  svtkIdType nbProps = props->GetNumberOfItems();
  for (svtkIdType i = 0; i < nbProps; i++)
  {
    svtkWidgetRepresentation* rep = svtkWidgetRepresentation::SafeDownCast(props->GetItemAsObject(i));

    if (rep && rep->GetInteractionState() != 0)
    {
      mod->SetShowRay(true);
      mod->SetRayLength(ren->GetActiveCamera()->GetClippingRange()[1]);
      mod->SetRayColor(0.0, 0.0, 1.0);
      return;
    }
  }

  if (this->GetGrabWithRay() || this->InteractionState[idev] == SVTKIS_PICK)
  {
    mod->SetShowRay(true);
  }
  else
  {
    mod->SetShowRay(false);
    return;
  }

  // Set length to its max if interactive picking is off
  if (!this->HoverPick)
  {
    mod->SetRayColor(1.0, 0.0, 0.0);
    mod->SetRayLength(ren->GetActiveCamera()->GetClippingRange()[1]);
    return;
  }

  // Compute controller position and world orientation
  double p0[3];   // Ray start point
  double wxyz[4]; // Controller orientation
  double dummy_ppos[3];
  double wdir[3];
  vr::TrackedDevicePose_t& tdPose = renWin->GetTrackedDevicePose(mod->TrackedDevice);
  iren->ConvertPoseToWorldCoordinates(tdPose, p0, wxyz, dummy_ppos, wdir);

  // Compute ray length.
  this->InteractionPicker->Pick3DRay(p0, wxyz, ren);

  // If something is picked, set the length accordingly
  svtkProp3D* prop = this->InteractionPicker->GetProp3D();
  if (prop)
  {
    double p1[3];
    this->InteractionPicker->GetPickPosition(p1);
    mod->SetRayLength(sqrt(svtkMath::Distance2BetweenPoints(p0, p1)));
    mod->SetRayColor(0.0, 1.0, 0.0);
  }
  // Otherwise set the length to its max
  else
  {
    mod->SetRayLength(ren->GetActiveCamera()->GetClippingRange()[1]);
    mod->SetRayColor(1.0, 0.0, 0.0);
  }

  return;
}
//----------------------------------------------------------------------------

void svtkOpenVRInteractorStyle::ShowBillboard(const std::string& text)
{
  svtkOpenVRRenderWindow* renWin =
    svtkOpenVRRenderWindow::SafeDownCast(this->Interactor->GetRenderWindow());
  svtkRenderer* ren = this->CurrentRenderer;
  if (!renWin || !ren)
  {
    return;
  }

  renWin->UpdateHMDMatrixPose();
  double dop[3];
  ren->GetActiveCamera()->GetDirectionOfProjection(dop);
  double vr[3];
  double* vup = renWin->GetPhysicalViewUp();
  double dtmp[3];
  double vupdot = svtkMath::Dot(dop, vup);
  if (fabs(vupdot) < 0.999)
  {
    dtmp[0] = dop[0] - vup[0] * vupdot;
    dtmp[1] = dop[1] - vup[1] * vupdot;
    dtmp[2] = dop[2] - vup[2] * vupdot;
    svtkMath::Normalize(dtmp);
  }
  else
  {
    renWin->GetPhysicalViewDirection(dtmp);
  }
  svtkMath::Cross(dtmp, vup, vr);
  svtkNew<svtkMatrix4x4> rot;
  for (int i = 0; i < 3; ++i)
  {
    rot->SetElement(0, i, vr[i]);
    rot->SetElement(1, i, vup[i]);
    rot->SetElement(2, i, -dtmp[i]);
  }
  rot->Transpose();
  double orient[3];
  svtkTransform::GetOrientation(orient, rot);
  svtkTextProperty* prop = this->TextActor3D->GetTextProperty();
  this->TextActor3D->SetOrientation(orient);
  this->TextActor3D->RotateX(-30.0);

  double tpos[3];
  double scale = renWin->GetPhysicalScale();
  ren->GetActiveCamera()->GetPosition(tpos);
  tpos[0] += (0.7 * scale * dop[0] - 0.1 * scale * vr[0] - 0.4 * scale * vup[0]);
  tpos[1] += (0.7 * scale * dop[1] - 0.1 * scale * vr[1] - 0.4 * scale * vup[1]);
  tpos[2] += (0.7 * scale * dop[2] - 0.1 * scale * vr[2] - 0.4 * scale * vup[2]);
  this->TextActor3D->SetPosition(tpos);
  // scale should cover 10% of FOV
  double fov = ren->GetActiveCamera()->GetViewAngle();
  double tsize = 0.1 * 2.0 * atan(fov * 0.5); // 10% of fov
  tsize /= 200.0;                             // about 200 pixel texture map
  scale *= tsize;
  this->TextActor3D->SetScale(scale, scale, scale);
  this->TextActor3D->SetInput(text.c_str());
  this->CurrentRenderer->AddActor(this->TextActor3D);

  prop->SetFrame(1);
  prop->SetFrameColor(1.0, 1.0, 1.0);
  prop->SetBackgroundOpacity(1.0);
  prop->SetBackgroundColor(0.0, 0.0, 0.0);
  prop->SetFontSize(14);
}

void svtkOpenVRInteractorStyle::HideBillboard()
{
  this->CurrentRenderer->RemoveActor(this->TextActor3D);
}

void svtkOpenVRInteractorStyle::ShowPickSphere(double* pos, double radius, svtkProp3D* prop)
{
  this->PickActor->GetProperty()->SetColor(this->PickColor);

  this->Sphere->SetCenter(pos);
  this->Sphere->SetRadius(radius);
  this->PickActor->GetMapper()->SetInputConnection(this->Sphere->GetOutputPort());
  if (prop)
  {
    this->PickActor->SetPosition(prop->GetPosition());
    this->PickActor->SetScale(prop->GetScale());
  }
  else
  {
    this->PickActor->SetPosition(0.0, 0.0, 0.0);
    this->PickActor->SetScale(1.0, 1.0, 1.0);
  }
  this->CurrentRenderer->AddActor(this->PickActor);
}

void svtkOpenVRInteractorStyle::ShowPickCell(svtkCell* cell, svtkProp3D* prop)
{
  svtkNew<svtkPolyData> pd;
  svtkNew<svtkPoints> pdpts;
  pdpts->SetDataTypeToDouble();
  svtkNew<svtkCellArray> lines;

  this->PickActor->GetProperty()->SetColor(this->PickColor);

  int nedges = cell->GetNumberOfEdges();

  if (nedges)
  {
    for (int edgenum = 0; edgenum < nedges; ++edgenum)
    {
      svtkCell* edge = cell->GetEdge(edgenum);
      svtkPoints* pts = edge->GetPoints();
      int npts = edge->GetNumberOfPoints();
      lines->InsertNextCell(npts);
      for (int ep = 0; ep < npts; ++ep)
      {
        svtkIdType newpt = pdpts->InsertNextPoint(pts->GetPoint(ep));
        lines->InsertCellPoint(newpt);
      }
    }
  }
  else if (cell->GetCellType() == SVTK_LINE || cell->GetCellType() == SVTK_POLY_LINE)
  {
    svtkPoints* pts = cell->GetPoints();
    int npts = cell->GetNumberOfPoints();
    lines->InsertNextCell(npts);
    for (int ep = 0; ep < npts; ++ep)
    {
      svtkIdType newpt = pdpts->InsertNextPoint(pts->GetPoint(ep));
      lines->InsertCellPoint(newpt);
    }
  }
  else
  {
    return;
  }

  pd->SetPoints(pdpts.Get());
  pd->SetLines(lines.Get());

  if (prop)
  {
    this->PickActor->SetPosition(prop->GetPosition());
    this->PickActor->SetScale(prop->GetScale());
    this->PickActor->SetUserMatrix(prop->GetUserMatrix());
  }
  else
  {
    this->PickActor->SetPosition(0.0, 0.0, 0.0);
    this->PickActor->SetScale(1.0, 1.0, 1.0);
  }
  this->PickActor->SetOrientation(prop->GetOrientation());
  static_cast<svtkPolyDataMapper*>(this->PickActor->GetMapper())->SetInputData(pd);
  this->CurrentRenderer->AddActor(this->PickActor);
}

void svtkOpenVRInteractorStyle::HidePickActor()
{
  if (this->CurrentRenderer)
  {
    this->CurrentRenderer->RemoveActor(this->PickActor);
  }
}

//----------------------------------------------------------------------------
void svtkOpenVRInteractorStyle::AddTooltipForInput(
  svtkEventDataDevice device, svtkEventDataDeviceInput input)
{
  this->AddTooltipForInput(device, input, "");
}
//----------------------------------------------------------------------------
void svtkOpenVRInteractorStyle::AddTooltipForInput(
  svtkEventDataDevice device, svtkEventDataDeviceInput input, const std::string& text)
{
  int iInput = static_cast<int>(input);
  int iDevice = static_cast<int>(device);
  int state = this->InputMap[iDevice][iInput];

  // if (state == -1)
  // {
  //   svtkWarningMacro(<< "Input " << iInput <<
  //     " not found. Inputs need to be mapped to actions first.");
  //   return;
  // }

  svtkStdString controlName = svtkStdString();
  svtkStdString controlText = svtkStdString();
  int drawSide = -1;
  int buttonSide = -1;

  // Setup default text and layout
  switch (input)
  {
    case svtkEventDataDeviceInput::Trigger:
      controlName = "trigger";
      drawSide = svtkOpenVRControlsHelper::Left;
      buttonSide = svtkOpenVRControlsHelper::Back;
      controlText = "Trigger :\n";
      break;
    case svtkEventDataDeviceInput::TrackPad:
      controlName = "trackpad";
      drawSide = svtkOpenVRControlsHelper::Right;
      buttonSide = svtkOpenVRControlsHelper::Front;
      controlText = "Trackpad :\n";
      break;
    case svtkEventDataDeviceInput::Grip:
      controlName = "lgrip";
      drawSide = svtkOpenVRControlsHelper::Right;
      buttonSide = svtkOpenVRControlsHelper::Back;
      controlText = "Grip :\n";
      break;
    case svtkEventDataDeviceInput::ApplicationMenu:
      controlName = "button";
      drawSide = svtkOpenVRControlsHelper::Left;
      buttonSide = svtkOpenVRControlsHelper::Front;
      controlText = "Application Menu :\n";
      break;
  }

  if (text != "")
  {
    controlText += text;
  }
  else
  {
    // Setup default action text
    switch (state)
    {
      case SVTKIS_POSITION_PROP:
        controlText += "Pick objects to\nadjust their pose";
        break;
      case SVTKIS_DOLLY:
        controlText += "Apply translation\nto the camera";
        break;
      case SVTKIS_CLIP:
        controlText += "Clip objects";
        break;
      case SVTKIS_PICK:
        controlText += "Probe data";
        break;
      case SVTKIS_LOAD_CAMERA_POSE:
        controlText += "Load next\ncamera pose.";
        break;
      case SVTKIS_TOGGLE_DRAW_CONTROLS:
        controlText += "Toggle control visibility";
        break;
      case SVTKIS_EXIT:
        controlText += "Exit";
        break;
      default:
        controlText = "No action assigned\nto this input.";
        break;
    }
  }

  // Clean already existing helpers
  if (this->ControlsHelpers[iDevice][iInput] != nullptr)
  {
    if (this->CurrentRenderer)
    {
      this->CurrentRenderer->RemoveViewProp(this->ControlsHelpers[iDevice][iInput]);
    }
    this->ControlsHelpers[iDevice][iInput]->Delete();
    this->ControlsHelpers[iDevice][iInput] = nullptr;
  }

  // Create an input helper and add it to the renderer
  svtkOpenVRControlsHelper* inputHelper = svtkOpenVRControlsHelper::New();
  inputHelper->SetTooltipInfo(controlName.c_str(), buttonSide, drawSide, controlText.c_str());

  this->ControlsHelpers[iDevice][iInput] = inputHelper;
  this->ControlsHelpers[iDevice][iInput]->SetDevice(device);

  if (this->CurrentRenderer)
  {
    this->ControlsHelpers[iDevice][iInput]->SetRenderer(this->CurrentRenderer);
    this->ControlsHelpers[iDevice][iInput]->BuildRepresentation();
    // this->ControlsHelpers[iDevice][iInput]->SetEnabled(false);
    this->CurrentRenderer->AddViewProp(this->ControlsHelpers[iDevice][iInput]);
  }
}
