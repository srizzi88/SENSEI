/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSSAOPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSSAOPass
 * @brief   Implement a screen-space ambient occlusion pass.
 *
 * SSAO darkens some pixels to improve depth perception simulating ambient occlusion
 * in screen space.
 * For each fragment, random samples inside a hemisphere at the fragment position oriented with
 * the normal are tested against other fragments to compute an average occlusion.
 * The number of samples and the radius of the hemisphere are configurables.
 *
 * @sa
 * svtkRenderPass
 */

#ifndef svtkSSAOPass_h
#define svtkSSAOPass_h

#include "svtkImageProcessingPass.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

#include <vector> // For vector

class svtkMatrix4x4;
class svtkOpenGLFramebufferObject;
class svtkOpenGLQuadHelper;
class svtkTextureObject;

class SVTKRENDERINGOPENGL2_EXPORT svtkSSAOPass : public svtkImageProcessingPass
{
public:
  static svtkSSAOPass* New();
  svtkTypeMacro(svtkSSAOPass, svtkImageProcessingPass);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Perform rendering according to a render state.
   */
  void Render(const svtkRenderState* s) override;

  /**
   * Release graphics resources and ask components to release their own resources.
   */
  void ReleaseGraphicsResources(svtkWindow* w) override;

  /**
   * Pre replace shader values
   */
  bool PreReplaceShaderValues(std::string& vertexShader, std::string& geometryShader,
    std::string& fragmentShader, svtkAbstractMapper* mapper, svtkProp* prop) override;

  /**
   * Post replace shader values
   */
  bool PostReplaceShaderValues(std::string& vertexShader, std::string& geometryShader,
    std::string& fragmentShader, svtkAbstractMapper* mapper, svtkProp* prop) override;

  /**
   * Set shader parameters. Set the draw buffers depending on the mapper.
   */
  bool SetShaderParameters(svtkShaderProgram* program, svtkAbstractMapper* mapper, svtkProp* prop,
    svtkOpenGLVertexArrayObject* VAO = nullptr) override;

  //@{
  /**
   * Get/Set the SSAO hemisphere radius.
   * Default is 0.5
   */
  svtkGetMacro(Radius, double);
  svtkSetMacro(Radius, double);
  //@}

  //@{
  /**
   * Get/Set the number of samples.
   * Default is 32
   */
  svtkGetMacro(KernelSize, unsigned int);
  svtkSetClampMacro(KernelSize, unsigned int, 1, 1000);
  //@}

  //@{
  /**
   * Get/Set the bias when comparing samples.
   * Default is 0.01
   */
  svtkGetMacro(Bias, double);
  svtkSetMacro(Bias, double);
  //@}

  //@{
  /**
   * Get/Set blurring of the ambient occlusion.
   * Blurring can help to improve the result if samples number is low.
   * Default is false
   */
  svtkGetMacro(Blur, bool);
  svtkSetMacro(Blur, bool);
  svtkBooleanMacro(Blur, bool);
  //@}

protected:
  svtkSSAOPass() = default;
  ~svtkSSAOPass() override = default;

  void ComputeKernel();
  void InitializeGraphicsResources(svtkOpenGLRenderWindow* renWin, int w, int h);

  void RenderDelegate(const svtkRenderState* s, int w, int h);
  void RenderSSAO(svtkOpenGLRenderWindow* renWin, svtkMatrix4x4* projection, int w, int h);
  void RenderCombine(svtkOpenGLRenderWindow* renWin);

  svtkTextureObject* ColorTexture = nullptr;
  svtkTextureObject* PositionTexture = nullptr;
  svtkTextureObject* NormalTexture = nullptr;
  svtkTextureObject* SSAOTexture = nullptr;
  svtkTextureObject* DepthTexture = nullptr;

  svtkOpenGLFramebufferObject* FrameBufferObject = nullptr;

  svtkOpenGLQuadHelper* SSAOQuadHelper = nullptr;
  svtkOpenGLQuadHelper* CombineQuadHelper = nullptr;

  std::vector<float> Kernel;
  unsigned int KernelSize = 32;
  double Radius = 0.5;
  double Bias = 0.01;
  bool Blur = false;

private:
  svtkSSAOPass(const svtkSSAOPass&) = delete;
  void operator=(const svtkSSAOPass&) = delete;
};

#endif
