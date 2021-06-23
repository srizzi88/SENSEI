/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInteractorStyleTrackballActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkInteractorStyleTrackballActor
 * @brief   manipulate objects in the scene independent of each other
 *
 * svtkInteractorStyleTrackballActor allows the user to interact with (rotate,
 * pan, etc.) objects in the scene indendent of each other.  In trackball
 * interaction, the magnitude of the mouse motion is proportional to the
 * actor motion associated with a particular mouse binding. For example,
 * small left-button motions cause small changes in the rotation of the
 * actor around its center point.
 *
 * The mouse bindings are as follows. For a 3-button mouse, the left button
 * is for rotation, the right button for zooming, the middle button for
 * panning, and ctrl + left button for spinning.  (With fewer mouse buttons,
 * ctrl + shift + left button is for zooming, and shift + left button is for
 * panning.)
 *
 * @sa
 * svtkInteractorStyleTrackballCamera svtkInteractorStyleJoystickActor
 * svtkInteractorStyleJoystickCamera
 */

#ifndef svtkInteractorStyleTrackballActor_h
#define svtkInteractorStyleTrackballActor_h

#include "svtkInteractionStyleModule.h" // For export macro
#include "svtkInteractorStyle.h"

class svtkCellPicker;

class SVTKINTERACTIONSTYLE_EXPORT svtkInteractorStyleTrackballActor : public svtkInteractorStyle
{
public:
  static svtkInteractorStyleTrackballActor* New();
  svtkTypeMacro(svtkInteractorStyleTrackballActor, svtkInteractorStyle);
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
  svtkInteractorStyleTrackballActor();
  ~svtkInteractorStyleTrackballActor() override;

  void FindPickedActor(int x, int y);

  void Prop3DTransform(
    svtkProp3D* prop3D, double* boxCenter, int NumRotation, double** rotate, double* scale);

  double MotionFactor;

  svtkProp3D* InteractionProp;
  svtkCellPicker* InteractionPicker;

private:
  svtkInteractorStyleTrackballActor(const svtkInteractorStyleTrackballActor&) = delete;
  void operator=(const svtkInteractorStyleTrackballActor&) = delete;
};

#endif
