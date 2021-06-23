/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPointFillPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPointFillPass
 * @brief   Implement a post-processing fillpass
 *
 *
 * This pass is designed to fill in rendering of sparse point sets/coulds
 * The delegate is used once and is usually set to a svtkCameraPass or
 * to a post-processing pass.
 *
 * @sa
 * svtkRenderPass
 */

#ifndef svtkPointFillPass_h
#define svtkPointFillPass_h

#include "svtkDepthImageProcessingPass.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

class svtkDepthPeelingPassLayerList; // Pimpl
class svtkOpenGLFramebufferObject;
class svtkOpenGLQuadHelper;
class svtkOpenGLRenderWindow;
class svtkTextureObject;

class SVTKRENDERINGOPENGL2_EXPORT svtkPointFillPass : public svtkDepthImageProcessingPass
{
public:
  static svtkPointFillPass* New();
  svtkTypeMacro(svtkPointFillPass, svtkDepthImageProcessingPass);
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
   * How far in front of a point must a neighboring point
   * be to be used as a filler candidate.  Expressed as
   * a multiple of the points distance from the camera.
   * Defaults to 0.95
   */
  svtkSetMacro(CandidatePointRatio, float);
  svtkGetMacro(CandidatePointRatio, float);
  //@}

  //@{
  /**
   * How large of an angle must the filler candidates
   * span before a point will be filled. Expressed in
   * radians. A value of pi will keep edges from growing out.
   * Large values require more support, lower values less.
   */
  svtkSetMacro(MinimumCandidateAngle, float);
  svtkGetMacro(MinimumCandidateAngle, float);
  //@}

protected:
  /**
   * Default constructor. DelegatePass is set to NULL.
   */
  svtkPointFillPass();

  /**
   * Destructor.
   */
  ~svtkPointFillPass() override;

  /**
   * Graphics resources.
   */
  svtkOpenGLFramebufferObject* FrameBufferObject;
  svtkTextureObject* Pass1;      // render target for the scene
  svtkTextureObject* Pass1Depth; // render target for the depth

  svtkOpenGLQuadHelper* QuadHelper;

  float CandidatePointRatio;
  float MinimumCandidateAngle;

private:
  svtkPointFillPass(const svtkPointFillPass&) = delete;
  void operator=(const svtkPointFillPass&) = delete;
};

#endif
