/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLSkybox.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenGLSkybox
 * @brief   OpenGL Skybox
 *
 * svtkOpenGLSkybox is a concrete implementation of the abstract class svtkSkybox.
 * svtkOpenGLSkybox interfaces to the OpenGL rendering library.
 */

#ifndef svtkOpenGLSkybox_h
#define svtkOpenGLSkybox_h

#include "svtkNew.h"                    // for ivars
#include "svtkRenderingOpenGL2Module.h" // For export macro
#include "svtkSkybox.h"

class svtkOpenGLActor;
class svtkOpenGLPolyDataMapper;

class SVTKRENDERINGOPENGL2_EXPORT svtkOpenGLSkybox : public svtkSkybox
{
public:
  static svtkOpenGLSkybox* New();
  svtkTypeMacro(svtkOpenGLSkybox, svtkSkybox);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Actual Skybox render method.
   */
  void Render(svtkRenderer* ren, svtkMapper* mapper) override;

protected:
  svtkOpenGLSkybox();
  ~svtkOpenGLSkybox() override;

  int LastProjection;
  float LastCameraPosition[3];

  void UpdateUniforms(svtkObject*, unsigned long, void*);

  svtkNew<svtkOpenGLPolyDataMapper> CubeMapper;
  svtkNew<svtkOpenGLActor> OpenGLActor;
  svtkRenderer* CurrentRenderer;

private:
  svtkOpenGLSkybox(const svtkOpenGLSkybox&) = delete;
  void operator=(const svtkOpenGLSkybox&) = delete;
};

#endif
