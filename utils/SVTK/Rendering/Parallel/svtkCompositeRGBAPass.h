/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompositeRGBAPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCompositeRGBAPass
 * @brief   Blend RGBA buffers of processes.
 *
 * Blend the RGBA buffers of satellite processes over the root process RGBA
 * buffer.
 * The RGBA buffer of the satellite processes are not changed.
 *
 * This pass requires a OpenGL context that supports texture objects (TO),
 * and pixel buffer objects (PBO). If not, it will emit an error message
 * and will render its delegate and return.
 *
 * @sa
 * svtkRenderPass
 */

#ifndef svtkCompositeRGBAPass_h
#define svtkCompositeRGBAPass_h

#include "svtkRenderPass.h"
#include "svtkRenderingParallelModule.h" // For export macro

class svtkMultiProcessController;

class svtkPixelBufferObject;
class svtkTextureObject;
class svtkOpenGLRenderWindow;
class svtkPKdTree;

class SVTKRENDERINGPARALLEL_EXPORT svtkCompositeRGBAPass : public svtkRenderPass
{
public:
  static svtkCompositeRGBAPass* New();
  svtkTypeMacro(svtkCompositeRGBAPass, svtkRenderPass);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Perform rendering according to a render state \p s.
   * \pre s_exists: s!=0
   */
  void Render(const svtkRenderState* s) override;

  /**
   * Release graphics resources and ask components to release their own
   * resources.
   * \pre w_exists: w!=0
   */
  void ReleaseGraphicsResources(svtkWindow* w) override;

  //@{
  /**
   * Controller
   * If it is NULL, nothing will be rendered and a warning will be emitted.
   * Initial value is a NULL pointer.
   */
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  virtual void SetController(svtkMultiProcessController* controller);
  //@}

  //@{
  /**
   * kd tree that gives processes ordering. Initial value is a NULL pointer.
   */
  svtkGetObjectMacro(Kdtree, svtkPKdTree);
  virtual void SetKdtree(svtkPKdTree* kdtree);
  //@}

  /**
   * Is the pass supported by the OpenGL context?
   */
  bool IsSupported(svtkOpenGLRenderWindow* context);

protected:
  /**
   * Default constructor. Controller is set to NULL.
   */
  svtkCompositeRGBAPass();

  /**
   * Destructor.
   */
  ~svtkCompositeRGBAPass() override;

  svtkMultiProcessController* Controller;
  svtkPKdTree* Kdtree;

  svtkPixelBufferObject* PBO;
  svtkTextureObject* RGBATexture;
  svtkTextureObject* RootTexture;
  float* RawRGBABuffer;
  size_t RawRGBABufferSize;

private:
  svtkCompositeRGBAPass(const svtkCompositeRGBAPass&) = delete;
  void operator=(const svtkCompositeRGBAPass&) = delete;
};

#endif
