/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenVRCamera
 * @brief   OpenVR camera
 *
 * svtkOpenVRCamera is a concrete implementation of the abstract class
 * svtkCamera.  svtkOpenVRCamera interfaces to the OpenVR rendering library.
 */

#ifndef svtkOpenVRCamera_h
#define svtkOpenVRCamera_h

#include "svtkNew.h" // ivars
#include "svtkOpenGLCamera.h"
#include "svtkRenderingOpenVRModule.h" // For export macro
#include "svtkTransform.h"             // ivars

class svtkOpenVRRenderer;
class svtkOpenVRRenderWindow;
class svtkMatrix3x3;
class svtkMatrix4x4;

class SVTKRENDERINGOPENVR_EXPORT svtkOpenVRCamera : public svtkOpenGLCamera
{
public:
  static svtkOpenVRCamera* New();
  svtkTypeMacro(svtkOpenVRCamera, svtkOpenGLCamera);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Implement base class method.
   */
  virtual void Render(svtkRenderer* ren);

  virtual void GetKeyMatrices(svtkRenderer* ren, svtkMatrix4x4*& WCVCMatrix,
    svtkMatrix3x3*& normalMatrix, svtkMatrix4x4*& VCDCMatrix, svtkMatrix4x4*& WCDCMatrix);

  /**
   * Provides a matrix to go from absolute OpenVR tracking coordinates
   * to device coordinates. Used for rendering devices.
   */
  virtual void GetTrackingToDCMatrix(svtkMatrix4x4*& TCDCMatrix);

  // apply the left or right eye pose to the camera
  // position and focal point.  Factor is typically
  // 1.0 to add or -1.0 to subtract
  void ApplyEyePose(svtkOpenVRRenderWindow*, bool left, double factor);

  // Get the OpenVR Physical Space to World coordinate matrix
  svtkTransform* GetPhysicalToWorldTransform() { return this->PoseTransform.Get(); }

protected:
  svtkOpenVRCamera();
  ~svtkOpenVRCamera() override;

  // gets the pose and projections for the left and right eves from
  // the openvr library
  void GetHMDEyePoses(svtkRenderer*);
  void GetHMDEyeProjections(svtkRenderer*);

  double LeftEyePose[3];
  double RightEyePose[3];
  svtkMatrix4x4* LeftEyeProjection;
  svtkMatrix4x4* RightEyeProjection;

  svtkMatrix4x4* LeftEyeTCDCMatrix;
  svtkMatrix4x4* RightEyeTCDCMatrix;

  // used to translate the
  // View to the HMD space
  svtkNew<svtkTransform> PoseTransform;

private:
  svtkOpenVRCamera(const svtkOpenVRCamera&) = delete;
  void operator=(const svtkOpenVRCamera&) = delete;
};

#endif
