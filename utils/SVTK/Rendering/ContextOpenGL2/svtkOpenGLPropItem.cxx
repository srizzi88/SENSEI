/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLPropItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkOpenGLPropItem.h"

#include "svtkCamera.h"
#include "svtkContext2D.h"
#include "svtkContextScene.h"
#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLContextDevice2D.h"
#include "svtkProp3D.h"
#include "svtkRenderer.h"
#include "svtkTransform.h"

svtkStandardNewMacro(svtkOpenGLPropItem);

svtkOpenGLPropItem::svtkOpenGLPropItem() = default;

svtkOpenGLPropItem::~svtkOpenGLPropItem() = default;

void svtkOpenGLPropItem::UpdateTransforms()
{
  svtkContextDevice2D* dev = this->Painter->GetDevice();
  svtkOpenGLContextDevice2D* glDev = svtkOpenGLContextDevice2D::SafeDownCast(dev);
  if (!glDev)
  {
    svtkErrorMacro(<< "Context device is not svtkOpenGLContextDevice2D.");
    return;
  }

  // Get the active camera:
  svtkRenderer* ren = this->Scene->GetRenderer();
  svtkCamera* activeCamera = ren->GetActiveCamera();

  // Cache the current state:
  this->CameraCache->DeepCopy(activeCamera);

  // Reset the info that computes the view:
  svtkNew<svtkTransform> identity;
  identity->Identity();
  activeCamera->SetUserViewTransform(identity);
  activeCamera->SetFocalPoint(0.0, 0.0, 0.0);
  activeCamera->SetPosition(0.0, 0.0, 1.0);
  activeCamera->SetViewUp(0.0, 1.0, 0.0);

  // Update the camera model matrix with the current context2D modelview matrix:
  double mv[16];
  double* glMv = glDev->GetModelMatrix()->Element[0];
  std::copy(glMv, glMv + 16, mv);
  activeCamera->SetModelTransformMatrix(mv);

  /* The perspective updates aren't nearly as straight-forward, and take a bit
   * of code-spelunking and algebra. By inspecting the following methods, we
   * see how the perspective matrix gets built at render-time:
   *
   * 1) svtkOpenGLCamera::Render() calls
   *    svtkCamera::GetProjectionTransformMatrix() with zRange = [-1, 1] and
   *    aspect = aspectModification * usize / vsize (see below).
   * 2) svtkCamera::GetProjectionTransformMatrix() calls
   *    svtkCamera::ComputeProjectionTransform with the same arguments.
   * 3) svtkCamera::ComputeProjectionTransform calls
   *    svtkPerspectiveTransform::Ortho with:
   *    xminGL = (svtkCamera::WindowCenter[0] - 1) * svtkCamera::ParallelScale * aspect
   *    xmaxGL = (svtkCamera::WindowCenter[0] + 1) * svtkCamera::ParallelScale * aspect
   *    yminGL = (svtkCamera::WindowCenter[1] - 1) * svtkCamera::ParallelScale
   *    ymaxGL = (svtkCamera::WindowCenter[1] + 1) * svtkCamera::ParallelScale
   *    zminGL = svtkCamera::ClippingRange[0]
   *    zmaxGL = svtkCamera::ClippingRange[1]
   *
   * In svtkOpenGLContext2D::Begin, glOrtho is called with:
   *    xminCTX = 0.5
   *    xmaxCTX = glViewport[0] - 0.5
   *    yminCTX = 0.5
   *    ymaxCTX = glViewport[1] - 0.5
   *    zminCTX = -2000
   *    zmaxCTX = 2000
   *
   * To set the camera parameters to reproduce the Context2D projective matrix,
   * the following set of equations can be built:
   *
   * Using:
   *   Cx = svtkCamera::WindowCenter[0] (unknown)
   *   Cy = svtkCamera::WindowCenter[1] (unknown)
   *   P = svtkCamera::ParallelScale (unknown)
   *   a = aspect (known)
   *
   * The equations are:
   *   xminCTX = (Cx - 1)aP
   *   xmaxCTX = (Cx + 1)aP
   *   yminCTX = (Cy - 1)P
   *   ymaxCTX = (Cy + 1)P
   *
   * Solving simultaneously for the unknowns Cx, Cy, and P, we get:
   *   Cx = (xminCTX * a) / (xmaxCTX - xminCTX) + 1
   *   Cy = a * (yminCTX + ymaxCTX) / (xmaxCTX - xminCTX)
   *   P = (xmaxCTX - xminCTX) / (2 * a)
   */

  // Collect the parameters to compute the projection matrix:
  // (see svtkOpenGLCamera::Render)
  int lowerLeft[2];
  int usize, vsize;
  double aspect1[2];
  double aspect2[2];
  svtkRecti vp = glDev->GetViewportRect();
  ren->GetTiledSizeAndOrigin(&usize, &vsize, lowerLeft, lowerLeft + 1);
  ren->ComputeAspect();
  ren->GetAspect(aspect1);
  ren->svtkViewport::ComputeAspect();
  ren->svtkViewport::GetAspect(aspect2);
  double aspectModification = (aspect1[0] * aspect2[1]) / (aspect1[1] * aspect2[0]);

  // Set the variables for the equations:
  double a = aspectModification * usize / vsize;
  double xminCTX = 0.5;
  double xmaxCTX = vp[2] - 0.5;
  double yminCTX = 0.5;
  double ymaxCTX = vp[3] - 0.5;
  double zminCTX = -2000;
  double zmaxCTX = 2000;

  double Cx = (xminCTX * a) / (xmaxCTX - xminCTX) + 1.;
  double Cy = a * (yminCTX + ymaxCTX) / (xmaxCTX - xminCTX);
  double P = (xmaxCTX - xminCTX) / (2 * a);

  // Set the camera state
  activeCamera->SetParallelProjection(1);
  activeCamera->SetParallelScale(P);
  activeCamera->SetWindowCenter(Cx, Cy);
  activeCamera->SetClippingRange(zminCTX, zmaxCTX);
}

void svtkOpenGLPropItem::ResetTransforms()
{
  // Reset the active camera:
  svtkCamera* activeCamera = this->Scene->GetRenderer()->GetActiveCamera();
  activeCamera->DeepCopy(this->CameraCache);
}

bool svtkOpenGLPropItem::Paint(svtkContext2D* painter)
{
  this->Painter = painter;
  bool result = this->Superclass::Paint(painter);
  this->Painter = nullptr;

  return result;
}
