/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkOpenVRInteractorStyle.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenVRInteractorStyle
 * @brief   extended from svtkInteractorStyle3D to override command methods
 */

#ifndef svtkOpenVRInteractorStyle_h
#define svtkOpenVRInteractorStyle_h

#include "svtkRenderingOpenVRModule.h" // For export macro

#include "svtkEventData.h" // for enums
#include "svtkInteractorStyle3D.h"
#include "svtkNew.h"                // for ivars
#include "svtkOpenVRRenderWindow.h" // for enums

class svtkCell;
class svtkPlane;
class svtkOpenVRControlsHelper;
class svtkOpenVRHardwarePicker;
class svtkOpenVRMenuRepresentation;
class svtkOpenVRMenuWidget;
class svtkTextActor3D;
class svtkSelection;
class svtkSphereSource;

class SVTKRENDERINGOPENVR_EXPORT svtkOpenVRInteractorStyle : public svtkInteractorStyle3D
{
public:
  static svtkOpenVRInteractorStyle* New();
  svtkTypeMacro(svtkOpenVRInteractorStyle, svtkInteractorStyle3D);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Override generic event bindings to call the corresponding action.
   */
  void OnButton3D(svtkEventData* edata) override;
  void OnMove3D(svtkEventData* edata) override;
  //@}

  //@{
  /**
   * Interaction mode entry points.
   */
  virtual void StartPick(svtkEventDataDevice3D*);
  virtual void EndPick(svtkEventDataDevice3D*);
  virtual void StartLoadCamPose(svtkEventDataDevice3D*);
  virtual void EndLoadCamPose(svtkEventDataDevice3D*);
  virtual void StartPositionProp(svtkEventDataDevice3D*);
  virtual void EndPositionProp(svtkEventDataDevice3D*);
  virtual void StartClip(svtkEventDataDevice3D*);
  virtual void EndClip(svtkEventDataDevice3D*);
  virtual void StartDolly3D(svtkEventDataDevice3D*);
  virtual void EndDolly3D(svtkEventDataDevice3D*);
  //@}

  //@{
  /**
   * Multitouch events binding.
   */
  void OnPan() override;
  void OnPinch() override;
  void OnRotate() override;
  //@}

  //@{
  /**
   * Methods for intertaction.
   */
  void ProbeData(svtkEventDataDevice controller);
  void LoadNextCameraPose();
  virtual void PositionProp(svtkEventData*);
  virtual void Clip(svtkEventDataDevice3D*);
  //@}

  //@{
  /**
   * Map controller inputs to actions.
   * Actions are defined by a SVTKIS_*STATE*, interaction entry points,
   * and the corresponding method for interaction.
   */
  void MapInputToAction(svtkEventDataDevice device, svtkEventDataDeviceInput input, int state);
  //@}

  //@{
  /**
   * Define the helper text that goes with an input
   */
  void AddTooltipForInput(
    svtkEventDataDevice device, svtkEventDataDeviceInput input, const std::string& text);
  //@}

  //@{
  /**
   * Indicates if picking should be updated every frame. If so, the interaction
   * picker will try to pick a prop and rays will be updated accordingly.
   * Default is set to off.
   */
  svtkSetMacro(HoverPick, bool);
  svtkGetMacro(HoverPick, bool);
  svtkBooleanMacro(HoverPick, bool);
  //@}

  //@{
  /**
   * Specify if the grab mode use the ray to grab distant objects
   */
  svtkSetMacro(GrabWithRay, bool);
  svtkGetMacro(GrabWithRay, bool);
  svtkBooleanMacro(GrabWithRay, bool);
  //@}

  int GetInteractionState(svtkEventDataDevice device)
  {
    return this->InteractionState[static_cast<int>(device)];
  }

  void ShowRay(svtkEventDataDevice controller);
  void HideRay(svtkEventDataDevice controller);

  void ShowBillboard(const std::string& text);
  void HideBillboard();

  void ShowPickSphere(double* pos, double radius, svtkProp3D*);
  void ShowPickCell(svtkCell* cell, svtkProp3D*);
  void HidePickActor();

  void ToggleDrawControls();
  void SetDrawControls(bool);

  void SetInteractor(svtkRenderWindowInteractor* iren) override;

  // allow the user to add options to the menu
  svtkOpenVRMenuWidget* GetMenu() { return this->Menu.Get(); }

protected:
  svtkOpenVRInteractorStyle();
  ~svtkOpenVRInteractorStyle() override;

  void EndPickCallback(svtkSelection* sel);

  // Ray drawing
  void UpdateRay(svtkEventDataDevice controller);

  svtkNew<svtkOpenVRMenuWidget> Menu;
  svtkNew<svtkOpenVRMenuRepresentation> MenuRepresentation;
  svtkCallbackCommand* MenuCommand;
  static void MenuCallback(
    svtkObject* object, unsigned long event, void* clientdata, void* calldata);

  svtkNew<svtkTextActor3D> TextActor3D;
  svtkNew<svtkActor> PickActor;
  svtkNew<svtkSphereSource> Sphere;

  // device input to interaction state mapping
  int InputMap[svtkEventDataNumberOfDevices][svtkEventDataNumberOfInputs];
  svtkOpenVRControlsHelper* ControlsHelpers[svtkEventDataNumberOfDevices][svtkEventDataNumberOfInputs];

  // Utility routines
  void StartAction(int SVTKIS_STATE, svtkEventDataDevice3D* edata);
  void EndAction(int SVTKIS_STATE, svtkEventDataDevice3D* edata);

  // Pick using hardware selector
  bool HardwareSelect(svtkEventDataDevice controller, bool actorPassOnly);

  bool HoverPick;
  bool GrabWithRay;

  /**
   * Store required controllers information when performing action
   */
  int InteractionState[svtkEventDataNumberOfDevices];
  svtkProp3D* InteractionProps[svtkEventDataNumberOfDevices];
  svtkPlane* ClippingPlanes[svtkEventDataNumberOfDevices];

  svtkNew<svtkOpenVRHardwarePicker> HardwarePicker;

  /**
   * Controls helpers drawing
   */
  void AddTooltipForInput(svtkEventDataDevice device, svtkEventDataDeviceInput input);

private:
  svtkOpenVRInteractorStyle(const svtkOpenVRInteractorStyle&) = delete; // Not implemented.
  void operator=(const svtkOpenVRInteractorStyle&) = delete;           // Not implemented.
};

#endif
