/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInteractorStyleSwitch.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkInteractorStyleSwitch
 * @brief   class to swap between interactory styles
 *
 * The class svtkInteractorStyleSwitch allows handles interactively switching
 * between four interactor styles -- joystick actor, joystick camera,
 * trackball actor, and trackball camera.  Type 'j' or 't' to select
 * joystick or trackball, and type 'c' or 'a' to select camera or actor.
 * The default interactor style is joystick camera.
 * @sa
 * svtkInteractorStyleJoystickActor svtkInteractorStyleJoystickCamera
 * svtkInteractorStyleTrackballActor svtkInteractorStyleTrackballCamera
 */

#ifndef svtkInteractorStyleSwitch_h
#define svtkInteractorStyleSwitch_h

#include "svtkInteractionStyleModule.h" // For export macro
#include "svtkInteractorStyleSwitchBase.h"

#define SVTKIS_JOYSTICK 0
#define SVTKIS_TRACKBALL 1

#define SVTKIS_CAMERA 0
#define SVTKIS_ACTOR 1

class svtkInteractorStyleJoystickActor;
class svtkInteractorStyleJoystickCamera;
class svtkInteractorStyleTrackballActor;
class svtkInteractorStyleTrackballCamera;
class svtkInteractorStyleMultiTouchCamera;

class SVTKINTERACTIONSTYLE_EXPORT svtkInteractorStyleSwitch : public svtkInteractorStyleSwitchBase
{
public:
  static svtkInteractorStyleSwitch* New();
  svtkTypeMacro(svtkInteractorStyleSwitch, svtkInteractorStyleSwitchBase);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * The sub styles need the interactor too.
   */
  void SetInteractor(svtkRenderWindowInteractor* iren) override;

  /**
   * We must override this method in order to pass the setting down to
   * the underlying styles
   */
  void SetAutoAdjustCameraClippingRange(svtkTypeBool value) override;

  //@{
  /**
   * Set/Get current style
   */
  svtkGetObjectMacro(CurrentStyle, svtkInteractorStyle);
  void SetCurrentStyleToJoystickActor();
  void SetCurrentStyleToJoystickCamera();
  void SetCurrentStyleToTrackballActor();
  void SetCurrentStyleToTrackballCamera();
  void SetCurrentStyleToMultiTouchCamera();
  //@}

  /**
   * Only care about the char event, which is used to switch between
   * different styles.
   */
  void OnChar() override;

  //@{
  /**
   * Overridden from svtkInteractorObserver because the interactor styles
   * used by this class must also be updated.
   */
  void SetDefaultRenderer(svtkRenderer*) override;
  void SetCurrentRenderer(svtkRenderer*) override;
  //@}

protected:
  svtkInteractorStyleSwitch();
  ~svtkInteractorStyleSwitch() override;

  void SetCurrentStyle();

  svtkInteractorStyleJoystickActor* JoystickActor;
  svtkInteractorStyleJoystickCamera* JoystickCamera;
  svtkInteractorStyleTrackballActor* TrackballActor;
  svtkInteractorStyleTrackballCamera* TrackballCamera;
  svtkInteractorStyleMultiTouchCamera* MultiTouchCamera;
  svtkInteractorStyle* CurrentStyle;

  int JoystickOrTrackball;
  int CameraOrActor;
  bool MultiTouch;

private:
  svtkInteractorStyleSwitch(const svtkInteractorStyleSwitch&) = delete;
  void operator=(const svtkInteractorStyleSwitch&) = delete;
};

#endif
