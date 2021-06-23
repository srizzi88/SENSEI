/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInteractorStyleMultiTouchCamera.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkInteractorStyleMultiTouchCamera
 * @brief   multitouch manipulation of the camera
 *
 * svtkInteractorStyleMultiTouchCamera allows the user to interactively
 * manipulate (rotate, pan, etc.) the camera, the viewpoint of the scene
 * using multitouch gestures in addition to regular gestures
 *
 * @sa
 * svtkInteractorStyleTrackballActor svtkInteractorStyleJoystickCamera
 * svtkInteractorStyleJoystickActor
 */

#ifndef svtkInteractorStyleMultiTouchCamera_h
#define svtkInteractorStyleMultiTouchCamera_h

#include "svtkInteractionStyleModule.h" // For export macro
#include "svtkInteractorStyleTrackballCamera.h"
#include "svtkRenderWindowInteractor.h" // for max pointers

class SVTKINTERACTIONSTYLE_EXPORT svtkInteractorStyleMultiTouchCamera
  : public svtkInteractorStyleTrackballCamera
{
public:
  static svtkInteractorStyleMultiTouchCamera* New();
  svtkTypeMacro(svtkInteractorStyleMultiTouchCamera, svtkInteractorStyleTrackballCamera);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Event bindings for gestures
   */
  void OnStartRotate() override;
  void OnRotate() override;
  void OnEndRotate() override;
  void OnStartPinch() override;
  void OnPinch() override;
  void OnEndPinch() override;
  void OnStartPan() override;
  void OnPan() override;
  void OnEndPan() override;

  //@}

protected:
  svtkInteractorStyleMultiTouchCamera();
  ~svtkInteractorStyleMultiTouchCamera() override;

private:
  svtkInteractorStyleMultiTouchCamera(const svtkInteractorStyleMultiTouchCamera&) = delete;
  void operator=(const svtkInteractorStyleMultiTouchCamera&) = delete;
};

#endif
