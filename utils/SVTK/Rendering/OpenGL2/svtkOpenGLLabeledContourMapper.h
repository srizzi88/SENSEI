/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLLabeledContourMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenGLLabeledContourMapper
 *
 * svtkOpenGLLabeledContourMapper is an override for svtkLabeledContourMapper
 * that implements stenciling using the OpenGL2 API.
 */

#ifndef svtkOpenGLLabeledContourMapper_h
#define svtkOpenGLLabeledContourMapper_h

#include "svtkLabeledContourMapper.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

class svtkMatrix4x4;
class svtkOpenGLHelper;

class SVTKRENDERINGOPENGL2_EXPORT svtkOpenGLLabeledContourMapper : public svtkLabeledContourMapper
{
public:
  static svtkOpenGLLabeledContourMapper* New();
  svtkTypeMacro(svtkOpenGLLabeledContourMapper, svtkLabeledContourMapper);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Release graphics resources
   */
  void ReleaseGraphicsResources(svtkWindow* win) override;

protected:
  svtkOpenGLLabeledContourMapper();
  ~svtkOpenGLLabeledContourMapper() override;

  // We override this for compatibility with the OpenGL backend:
  // The old backend pushes actor matrices onto the matrix stack, so the text
  // actors already accounted for any transformations on this mapper's actor.
  // The new backend passes each actor's matrix to the shader individually, and
  // this mapper's actor matrix doesn't affect the label rendering.
  bool CreateLabels(svtkActor* actor) override;

  bool ApplyStencil(svtkRenderer* ren, svtkActor* act) override;
  bool RemoveStencil(svtkRenderer* ren) override;

  svtkOpenGLHelper* StencilBO;
  svtkMatrix4x4* TempMatrix4;

private:
  svtkOpenGLLabeledContourMapper(const svtkOpenGLLabeledContourMapper&) = delete;
  void operator=(const svtkOpenGLLabeledContourMapper&) = delete;
};

#endif
