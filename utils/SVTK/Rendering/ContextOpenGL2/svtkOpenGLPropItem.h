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

/**
 * @class   svtkOpenGLPropItem
 * @brief   Sync Context2D state with svtk camera.
 *
 *
 * The svtkContext2D framework modifies the GL state directly, while some actors
 * and mappers rely on the modelview/projection matrices from svtkCamera. This
 * class is a layer between the two that updates the camera with the current
 * OpenGL state.
 */

#ifndef svtkOpenGLPropItem_h
#define svtkOpenGLPropItem_h

#include "svtkNew.h" // for svtkNew
#include "svtkPropItem.h"
#include "svtkRenderingContextOpenGL2Module.h" // For export macro

class svtkCamera;

class SVTKRENDERINGCONTEXTOPENGL2_EXPORT svtkOpenGLPropItem : public svtkPropItem
{
public:
  static svtkOpenGLPropItem* New();
  svtkTypeMacro(svtkOpenGLPropItem, svtkPropItem);

  bool Paint(svtkContext2D* painter) override;

protected:
  svtkOpenGLPropItem();
  ~svtkOpenGLPropItem() override;

  // Sync the active svtkCamera with the GL state set by the painter.
  void UpdateTransforms() override;

  // Restore the svtkCamera state.
  void ResetTransforms() override;

private:
  svtkNew<svtkCamera> CameraCache;
  svtkContext2D* Painter;

  svtkOpenGLPropItem(const svtkOpenGLPropItem&) = delete;
  void operator=(const svtkOpenGLPropItem&) = delete;
};

#endif // svtkOpenGLPropItem_h
