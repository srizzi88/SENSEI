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
* @class   svtkOpenVRModel
* @brief   OpenVR device model

* This internal class is used to load models
* such as for the trackers and controllers and to
* render them in the scene
*/

#ifndef svtkOpenVRModel_h
#define svtkOpenVRModel_h

#include "svtkNew.h" // for ivar
#include "svtkObject.h"
#include "svtkOpenGLHelper.h"          // ivar
#include "svtkRenderingOpenVRModule.h" // For export macro
#include <openvr.h>                   // for ivars

class svtkOpenVRRenderWindow;
class svtkRenderWindow;
class svtkOpenGLVertexBufferObject;
class svtkTextureObject;
class svtkMatrix4x4;
class svtkOpenVRRay;

class SVTKRENDERINGOPENVR_EXPORT svtkOpenVRModel : public svtkObject
{
public:
  static svtkOpenVRModel* New();
  svtkTypeMacro(svtkOpenVRModel, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  bool Build(svtkOpenVRRenderWindow* win);
  void Render(svtkOpenVRRenderWindow* win, const vr::TrackedDevicePose_t& pose);

  const std::string& GetName() const { return this->ModelName; }
  void SetName(const std::string& modelName) { this->ModelName = modelName; }

  // show the model
  void SetVisibility(bool v) { this->Visibility = v; }
  bool GetVisibility() { return this->Visibility; }

  // Set Ray parameters
  void SetShowRay(bool v);
  void SetRayLength(double length);
  void SetRayColor(double r, double g, double b);
  svtkOpenVRRay* GetRay() { return this->Ray; }

  void ReleaseGraphicsResources(svtkWindow* win);

  // the tracked device this model represents if any
  vr::TrackedDeviceIndex_t TrackedDevice;

  vr::RenderModel_t* RawModel;

protected:
  svtkOpenVRModel();
  ~svtkOpenVRModel() override;

  std::string ModelName;

  bool Visibility;
  bool Loaded;
  bool FailedToLoad;

  vr::RenderModel_TextureMap_t* RawTexture;
  svtkOpenGLHelper ModelHelper;
  svtkOpenGLVertexBufferObject* ModelVBO;
  svtkNew<svtkTextureObject> TextureObject;
  svtkNew<svtkMatrix4x4> PoseMatrix;

  // Controller ray
  svtkNew<svtkOpenVRRay> Ray;

private:
  svtkOpenVRModel(const svtkOpenVRModel&) = delete;
  void operator=(const svtkOpenVRModel&) = delete;
};

#endif
