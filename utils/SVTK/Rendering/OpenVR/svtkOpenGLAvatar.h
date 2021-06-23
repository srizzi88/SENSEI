/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLAvatar.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenGLAvatar
 * @brief   OpenGL Avatar
 *
 * svtkOpenGLAvatar is a concrete implementation of the abstract class svtkAvatar.
 * svtkOpenGLAvatar interfaces to the OpenGL rendering library.
 */

#ifndef svtkOpenGLAvatar_h
#define svtkOpenGLAvatar_h

#include "svtkAvatar.h"
#include "svtkNew.h"                   // for ivars
#include "svtkRenderingOpenVRModule.h" // For export macro

class svtkOpenGLActor;
class svtkOpenGLPolyDataMapper;
class svtkOpenGLRenderer;
class svtkOpenVRRay;
class svtkFlagpoleLabel;
class svtkTextProperty;

class SVTKRENDERINGOPENVR_EXPORT svtkOpenGLAvatar : public svtkAvatar
{
public:
  static svtkOpenGLAvatar* New();
  svtkTypeMacro(svtkOpenGLAvatar, svtkAvatar);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Actual Avatar render method.
   */
  // void Render(svtkRenderer *ren, svtkMapper *mapper) override;
  int RenderOpaqueGeometry(svtkViewport* vp) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport* vp) override;

  double* GetBounds() SVTK_SIZEHINT(6) override;

  void SetUseLeftHand(bool val) override;
  void SetUseRightHand(bool val) override;
  void SetShowHandsOnly(bool val) override;

  // Set Ray parameters
  void SetLeftShowRay(bool v);
  void SetRightShowRay(bool v);
  void SetRayLength(double length);

  void SetLabel(const char* label);
  svtkTextProperty* GetLabelTextProperty();

protected:
  svtkOpenGLAvatar();
  ~svtkOpenGLAvatar() override;

  // move the torso and arms based on head/hand inputs.
  void CalcBody();

  svtkNew<svtkOpenGLPolyDataMapper> HeadMapper;
  svtkNew<svtkOpenGLActor> HeadActor;
  svtkNew<svtkOpenGLPolyDataMapper> LeftHandMapper;
  svtkNew<svtkOpenGLActor> LeftHandActor;
  svtkNew<svtkOpenGLPolyDataMapper> RightHandMapper;
  svtkNew<svtkOpenGLActor> RightHandActor;
  svtkNew<svtkOpenGLPolyDataMapper> BodyMapper[NUM_BODY];
  svtkNew<svtkOpenGLActor> BodyActor[NUM_BODY];

  svtkNew<svtkOpenVRRay> LeftRay;
  svtkNew<svtkOpenVRRay> RightRay;

  svtkNew<svtkFlagpoleLabel> LabelActor;

private:
  svtkOpenGLAvatar(const svtkOpenGLAvatar&) = delete;
  void operator=(const svtkOpenGLAvatar&) = delete;
};

#endif
