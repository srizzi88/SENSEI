/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTDxInteractorStyleCamera.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTDxInteractorStyleCamera
 * @brief   interactive manipulation of the camera with a 3DConnexion device
 *
 *
 * svtkTDxInteractorStyleCamera allows the end-user to manipulate tha camera
 * with a 3DConnexion device.
 *
 * @sa
 * svtkInteractorStyle svtkRenderWindowInteractor
 * svtkTDxInteractorStyle
 */

#ifndef svtkTDxInteractorStyleCamera_h
#define svtkTDxInteractorStyleCamera_h

#include "svtkRenderingCoreModule.h" // For export macro
#include "svtkTDxInteractorStyle.h"

class svtkTransform;

class SVTKRENDERINGCORE_EXPORT svtkTDxInteractorStyleCamera : public svtkTDxInteractorStyle
{
public:
  static svtkTDxInteractorStyleCamera* New();
  svtkTypeMacro(svtkTDxInteractorStyleCamera, svtkTDxInteractorStyle);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Action on motion event.
   * \pre: motionInfo_exist: motionInfo!=0
   */
  void OnMotionEvent(svtkTDxMotionEventInfo* motionInfo) override;

protected:
  svtkTDxInteractorStyleCamera();
  ~svtkTDxInteractorStyleCamera() override;

  svtkTransform* Transform; // Used for internal intermediate calculation.

private:
  svtkTDxInteractorStyleCamera(const svtkTDxInteractorStyleCamera&) = delete;
  void operator=(const svtkTDxInteractorStyleCamera&) = delete;
};
#endif
