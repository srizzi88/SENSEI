/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkShadowMapPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkShadowMapPass
 * @brief   Implement a shadow mapping render pass.
 *
 * Render the opaque polygonal geometry of a scene with shadow maps (a
 * technique to render hard shadows in hardware).
 *
 * This pass expects an initialized depth buffer and color buffer.
 * Initialized buffers means they have been cleared with farest z-value and
 * background color/gradient/transparent color.
 * An opaque pass may have been performed right after the initialization.
 *
 *
 *
 * Its delegate is usually set to a svtkOpaquePass.
 *
 * @par Implementation:
 * The first pass of the algorithm is to generate a shadow map per light
 * (depth map from the light point of view) by rendering the opaque objects
 * with the OCCLUDER property keys.
 * The second pass is to render the opaque objects with the RECEIVER keys.
 *
 * @sa
 * svtkRenderPass, svtkOpaquePass
 */

#ifndef svtkShadowMapPass_h
#define svtkShadowMapPass_h

#include "svtkOpenGLRenderPass.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro
#include <string>                      // For member variables.
#include <vector>                      // STL Header

class svtkOpenGLRenderWindow;
class svtkInformationIntegerKey;
class svtkCamera;
class svtkLight;
class svtkFrameBufferObject;
class svtkShadowMapPassTextures;     // internal
class svtkShadowMapPassLightCameras; // internal
class svtkShadowMapBakerPass;
class svtkInformationObjectBaseKey;
class svtkShaderProgram;

class SVTKRENDERINGOPENGL2_EXPORT svtkShadowMapPass : public svtkOpenGLRenderPass
{
public:
  static svtkShadowMapPass* New();
  svtkTypeMacro(svtkShadowMapPass, svtkOpenGLRenderPass);
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
   * Pass that generates the shadow maps.
   * the svtkShadowMapPass will use the Resolution ivar of
   * this pass.
   * Initial value is a NULL pointer.
   */
  svtkGetObjectMacro(ShadowMapBakerPass, svtkShadowMapBakerPass);
  virtual void SetShadowMapBakerPass(svtkShadowMapBakerPass* shadowMapBakerPass);
  //@}

  //@{
  /**
   * Pass that render the lights and opaque geometry
   * Typically a sequence pass with a light pass and opaque pass.
   */
  svtkGetObjectMacro(OpaqueSequence, svtkRenderPass);
  virtual void SetOpaqueSequence(svtkRenderPass* opaqueSequence);
  //@}

  /**
   * get the matricies for all the
   * shadow maps.
   */
  std::vector<double> ShadowMapTransforms() { return this->ShadowTransforms; }

  /**
   * get the texture units for the shadow maps
   * for each light. If a light does not cast a shadow
   * it is set to -1
   */
  std::vector<int> GetShadowMapTextureUnits() { return this->ShadowTextureUnits; }

  /**
   * this key will contain the shadow map pass
   */
  static svtkInformationObjectBaseKey* ShadowMapPass();

  /**
   * Get the shader code to compute light factors based
   * on a mappers vertexVC variable
   */
  std::string GetFragmentDeclaration() { return this->FragmentDeclaration; }
  std::string GetFragmentImplementation() { return this->FragmentImplementation; }

  // svtkOpenGLRenderPass virtuals:
  bool PreReplaceShaderValues(std::string& vertexShader, std::string& geometryShader,
    std::string& fragmentShader, svtkAbstractMapper* mapper, svtkProp* prop) override;
  bool PostReplaceShaderValues(std::string& vertexShader, std::string& geometryShader,
    std::string& fragmentShader, svtkAbstractMapper* mapper, svtkProp* prop) override;
  bool SetShaderParameters(svtkShaderProgram* program, svtkAbstractMapper* mapper, svtkProp* prop,
    svtkOpenGLVertexArrayObject* VAO = nullptr) override;

protected:
  /**
   * Default constructor. DelegatetPass is set to NULL.
   */
  svtkShadowMapPass();

  /**
   * Destructor.
   */
  ~svtkShadowMapPass() override;

  /**
   * Check if shadow mapping is supported by the current OpenGL context.
   * \pre w_exists: w!=0
   */
  void CheckSupport(svtkOpenGLRenderWindow* w);

  svtkShadowMapBakerPass* ShadowMapBakerPass;
  svtkRenderPass* CompositeRGBAPass;

  svtkRenderPass* OpaqueSequence;

  /**
   * Graphics resources.
   */
  svtkFrameBufferObject* FrameBufferObject;

  svtkShadowMapPassTextures* ShadowMaps;
  svtkShadowMapPassLightCameras* LightCameras;

  svtkTimeStamp LastRenderTime;

  // to store the shader code and settings
  void BuildShaderCode();
  std::string FragmentDeclaration;
  std::string FragmentImplementation;
  std::vector<int> ShadowTextureUnits;
  std::vector<double> ShadowTransforms;
  std::vector<float> ShadowAttenuation;
  std::vector<int> ShadowParallel;

private:
  svtkShadowMapPass(const svtkShadowMapPass&) = delete;
  void operator=(const svtkShadowMapPass&) = delete;
};

#endif
