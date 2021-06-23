/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInteractorStyleJoystickActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkInteractorStyleJoystickActor
 * @brief   manipulate objects in the scene independently of one another
 *
 * The class svtkInteractorStyleJoystickActor allows the user to interact
 * with (rotate, zoom, etc.) separate objects in the scene independent of
 * each other.  The position of the mouse relative to the center of the
 * object determines the speed of the object's motion.  The mouse's velocity
 * determines the acceleration of the object's motion, so the object will
 * continue moving even when the mouse is not moving.
 * For a 3-button mouse, the left button is for rotation, the right button
 * for zooming, the middle button for panning, and ctrl + left button for
 * spinning.  (With fewer mouse buttons, ctrl + shift + left button is
 * for zooming, and shift + left button is for panning.)
 * @sa
 * svtkInteractorStyleJoystickCamera svtkInteractorStyleTrackballActor
 * svtkInteractorStyleTrackballCamera
 */

#ifndef svtkInteractorStyleJoystickActor_h
#define svtkInteractorStyleJoystickActor_h

#include "svtkInteractionStyleModule.h" // For export macro
#include "svtkInteractorStyle.h"

class svtkCellPicker;

// motion flags

class SVTKINTERACTIONSTYLE_EXPORT svtkInteractorStyleJoystickActor : public svtkInteractorStyle
{
public:
  static svtkInteractorStyleJoystickActor* New();

  svtkTypeMacro(svtkInteractorStyleJoystickActor, svtkInteractorStyle);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Event bindings controlling the effects of pressing mouse buttons
   * or moving the mouse.
   */
  void OnMouseMove() override;
  void OnLeftButtonDown() override;
  void OnLeftButtonUp() override;
  void OnMiddleButtonDown() override;
  void OnMiddleButtonUp() override;
  void OnRightButtonDown() override;
  void OnRightButtonUp() override;
  //@}

  // These methods for the different interactions in different modes
  // are overridden in subclasses to perform the correct motion. Since
  // they might be called from OnTimer, they do not have mouse coord parameters
  // (use interactor's GetEventPosition and GetLastEventPosition)
  void Rotate() override;
  void Spin() override;
  void Pan() override;
  void Dolly() override;
  void UniformScale() override;

protected:
  svtkInteractorStyleJoystickActor();
  ~svtkInteractorStyleJoystickActor() override;

  void FindPickedActor(int x, int y);

  void Prop3DTransform(
    svtkProp3D* prop3D, double* boxCenter, int numRotation, double** rotate, double* scale);

  double MotionFactor;

  svtkProp3D* InteractionProp;
  svtkCellPicker* InteractionPicker;

private:
  svtkInteractorStyleJoystickActor(const svtkInteractorStyleJoystickActor&) = delete;
  void operator=(const svtkInteractorStyleJoystickActor&) = delete;
};

#endif
