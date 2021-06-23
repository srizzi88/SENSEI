/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompositeZPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCompositeZPass
 * @brief   Merge depth buffers of processes.
 *
 * Merge the depth buffers of satellite processes into the root process depth
 * buffer. It assumes that all the depth buffers have the same number of bits.
 * The depth buffer of the satellite processes are not changed.
 *
 * This pass requires a OpenGL context that supports texture objects (TO),
 * and pixel buffer objects (PBO). If not, it will emit an error message
 * and will render its delegate and return.
 *
 * @sa
 * svtkRenderPass
 */

#ifndef svtkCompositeZPass_h
#define svtkCompositeZPass_h

#include "svtkRenderPass.h"
#include "svtkRenderingParallelModule.h" // For export macro

class svtkMultiProcessController;

class svtkPixelBufferObject;
class svtkTextureObject;
class svtkOpenGLRenderWindow;
class svtkOpenGLHelper;

class SVTKRENDERINGPARALLEL_EXPORT svtkCompositeZPass : public svtkRenderPass
{
public:
  static svtkCompositeZPass* New();
  svtkTypeMacro(svtkCompositeZPass, svtkRenderPass);
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

  /**
   * Is the pass supported by the OpenGL context?
   */
  bool IsSupported(svtkOpenGLRenderWindow* context);

protected:
  /**
   * Default constructor. Controller is set to NULL.
   */
  svtkCompositeZPass();

  /**
   * Destructor.
   */
  ~svtkCompositeZPass() override;

  /**
   * Create program for texture mapping.
   * \pre context_exists: context!=0
   * \pre Program_void: this->Program==0
   * \post Program_exists: this->Program!=0
   */
  void CreateProgram(svtkOpenGLRenderWindow* context);

  svtkMultiProcessController* Controller;

  svtkPixelBufferObject* PBO;
  svtkTextureObject* ZTexture;
  svtkOpenGLHelper* Program;
  float* RawZBuffer;
  size_t RawZBufferSize;

private:
  svtkCompositeZPass(const svtkCompositeZPass&) = delete;
  void operator=(const svtkCompositeZPass&) = delete;
};

#endif
