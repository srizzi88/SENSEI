/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExternalOpenGLRenderer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExternalOpenGLRenderer
 * @brief   OpenGL renderer
 *
 * svtkExternalOpenGLRenderer is a secondary implementation of the class
 * svtkOpenGLRenderer. svtkExternalOpenGLRenderer interfaces to the
 * OpenGL graphics library. This class provides API to preserve the color and
 * depth buffers, thereby allowing external applications to manage the OpenGL
 * buffers. This becomes very useful when there are multiple OpenGL applications
 * sharing the same OpenGL context.
 *
 * svtkExternalOpenGLRenderer makes sure that the camera used in the scene if of
 * type svtkExternalOpenGLCamera. It manages light and camera transformations for
 * SVTK objects in the OpenGL context.
 *
 * \sa svtkExternalOpenGLCamera
 */

#ifndef svtkExternalOpenGLRenderer_h
#define svtkExternalOpenGLRenderer_h

#include "svtkOpenGLRenderer.h"
#include "svtkRenderingExternalModule.h" // For export macro

// Forward declarations
class svtkLightCollection;
class svtkExternalLight;

class SVTKRENDERINGEXTERNAL_EXPORT svtkExternalOpenGLRenderer : public svtkOpenGLRenderer
{
public:
  static svtkExternalOpenGLRenderer* New();
  svtkTypeMacro(svtkExternalOpenGLRenderer, svtkOpenGLRenderer);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Synchronize camera and light parameters
   */
  void Render(void) override;

  /**
   * Create a new Camera sutible for use with this type of Renderer.
   * This function creates the svtkExternalOpenGLCamera.
   */
  svtkCamera* MakeCamera() override;

  /**
   * Add an external light to the list of external lights.
   */
  virtual void AddExternalLight(svtkExternalLight*);

  /**
   * Remove an external light from the list of external lights.
   */
  virtual void RemoveExternalLight(svtkExternalLight*);

  /**
   * Remove all external lights
   */
  virtual void RemoveAllExternalLights();

  /**
   * If PreserveGLCameraMatrices is set to true, SVTK camera matrices
   * are copied from the current context GL_MODELVIEW_MATRIX and
   * GL_PROJECTION_MATRIX parameters before each render call.
   * This flag is on by default.
   */
  svtkGetMacro(PreserveGLCameraMatrices, svtkTypeBool);
  svtkSetMacro(PreserveGLCameraMatrices, svtkTypeBool);
  svtkBooleanMacro(PreserveGLCameraMatrices, svtkTypeBool);

  /**
   * If PreserveGLLights is set to true, existing GL lights are modified before
   * each render call to match the collection of lights added with
   * AddExternalLight(). This flag is on by default.
   */
  svtkGetMacro(PreserveGLLights, svtkTypeBool);
  svtkSetMacro(PreserveGLLights, svtkTypeBool);
  svtkBooleanMacro(PreserveGLLights, svtkTypeBool);

protected:
  svtkExternalOpenGLRenderer();
  ~svtkExternalOpenGLRenderer() override;

  /**
   * Copy the current OpenGL GL_MODELVIEW_MATRIX and GL_PROJECTION_MATRIX to
   * the active SVTK camera before each render call if PreserveGLCameraMatrices
   * is set to true (default behavior).
   */
  void SynchronizeGLCameraMatrices();

  /**
   * Query existing GL lights before each render call and tweak them to match
   * the external lights collection if PreserveGLLights is set to true (default
   * behavior).
   */
  void SynchronizeGLLights();

  svtkTypeBool PreserveGLCameraMatrices;
  svtkTypeBool PreserveGLLights;

  svtkLightCollection* ExternalLights;

private:
  svtkExternalOpenGLRenderer(const svtkExternalOpenGLRenderer&) = delete;
  void operator=(const svtkExternalOpenGLRenderer&) = delete;
};

#endif // svtkExternalOpenGLRenderer_h
