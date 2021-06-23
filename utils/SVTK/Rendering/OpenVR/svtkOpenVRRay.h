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
* @class   svtkOpenVRRay
* @brief   OpenVR device model

* Represents a ray shooting from a VR controller, used for pointing or picking.
*/

#ifndef svtkOpenVRRay_h
#define svtkOpenVRRay_h

#include "svtkNew.h" // for ivar
#include "svtkObject.h"
#include "svtkOpenGLHelper.h"          // ivar
#include "svtkRenderingOpenVRModule.h" // For export macro
#include <openvr.h>                   // for ivars

class svtkOpenGLRenderWindow;
class svtkRenderWindow;
class svtkOpenGLVertexBufferObject;
class svtkMatrix4x4;

class SVTKRENDERINGOPENVR_EXPORT svtkOpenVRRay : public svtkObject
{
public:
  static svtkOpenVRRay* New();
  svtkTypeMacro(svtkOpenVRRay, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  bool Build(svtkOpenGLRenderWindow* win);
  void Render(svtkOpenGLRenderWindow* win, svtkMatrix4x4* poseMatrix);

  // show the model
  svtkSetMacro(Show, bool);
  svtkGetMacro(Show, bool);

  svtkSetMacro(Length, float);

  svtkSetVector3Macro(Color, float);

  void ReleaseGraphicsResources(svtkRenderWindow* win);

protected:
  svtkOpenVRRay();
  ~svtkOpenVRRay() override;

  bool Show;
  bool Loaded;

  svtkOpenGLHelper RayHelper;
  svtkOpenGLVertexBufferObject* RayVBO;
  svtkNew<svtkMatrix4x4> PoseMatrix;

  float Length;
  float Color[3];

private:
  svtkOpenVRRay(const svtkOpenVRRay&) = delete;
  void operator=(const svtkOpenVRRay&) = delete;
};

#endif
