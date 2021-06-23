/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkShadowMapBakerPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkShadowMapBakerPass
 * @brief   Implement a builder of shadow map pass.
 *
 * Bake a list of shadow maps, once per spot light.
 * It work in conjunction with the svtkShadowMapPass, which uses the
 * shadow maps for rendering the opaque geometry (a technique to render hard
 * shadows in hardware).
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
 *
 * @sa
 * svtkRenderPass, svtkOpaquePass, svtkShadowMapPass
 */

#ifndef svtkShadowMapBakerPass_h
#define svtkShadowMapBakerPass_h

#include "svtkOpenGLRenderPass.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro
#include "svtkSmartPointer.h"           // for ivars
#include <vector>                      // STL Header

class svtkOpenGLRenderWindow;
class svtkInformationIntegerKey;
class svtkCamera;
class svtkLight;
class svtkOpenGLFramebufferObject;
class svtkTextureObject;

class SVTKRENDERINGOPENGL2_EXPORT svtkShadowMapBakerPass : public svtkOpenGLRenderPass
{
public:
  static svtkShadowMapBakerPass* New();
  svtkTypeMacro(svtkShadowMapBakerPass, svtkOpenGLRenderPass);
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
   * Delegate for rendering the camera, lights, and opaque geometry.
   * If it is NULL, nothing will be rendered and a warning will be emitted.
   * It defaults to a svtkCameraPass with a sequence of
   * svtkLightPass/svtkOpaquePass.
   */
  svtkGetObjectMacro(OpaqueSequence, svtkRenderPass);
  virtual void SetOpaqueSequence(svtkRenderPass* opaqueSequence);
  //@}

  //@{
  /**
   * Delegate for compositing of the shadow maps across processors.
   * If it is NULL, there is no z compositing.
   * It is usually set to a svtkCompositeZPass (Parallel package).
   * Initial value is a NULL pointer.
   */
  svtkGetObjectMacro(CompositeZPass, svtkRenderPass);
  virtual void SetCompositeZPass(svtkRenderPass* compositeZPass);
  //@}

  //@{
  /**
   * Set/Get the number of pixels in each dimension of the shadow maps
   * (shadow maps are square). Initial value is 256. The greater the better.
   * Resolution does not have to be a power-of-two value.
   */
  svtkSetMacro(Resolution, unsigned int);
  svtkGetMacro(Resolution, unsigned int);
  //@}

  /**
   * INTERNAL USE ONLY.
   * Internally used by svtkShadowMapBakerPass and svtkShadowMapPass.

   * Tell if there is at least one shadow.
   * Initial value is false.
   */
  bool GetHasShadows();

  /**
   * INTERNAL USE ONLY.
   * Internally used by svtkShadowMapBakerPass and svtkShadowMapPass.

   * Tell if the light `l' can create shadows.
   * The light has to not be a head light and to be directional or
   * positional with an angle less than 180 degrees.
   * \pre l_exists: l!=0
   */
  bool LightCreatesShadow(svtkLight* l);

  /**
   * INTERNAL USE ONLY
   * Internally used by svtkShadowMapBakerPass and svtkShadowMapPass.

   * Give access to the baked shadow maps.
   */
  std::vector<svtkSmartPointer<svtkTextureObject> >* GetShadowMaps();

  /**
   * INTERNAL USE ONLY.
   * Internally used by svtkShadowMapBakerPass and svtkShadowMapPass.

   * Give access the cameras builds from the ligths.
   */
  std::vector<svtkSmartPointer<svtkCamera> >* GetLightCameras();

  /**
   * INTERNAL USE ONLY.
   * Internally used by svtkShadowMapBakerPass and svtkShadowMapPass.

   * Do the shadows need to be updated?
   * Value changed by svtkShadowMapBakerPass and used by svtkShadowMapPass.
   * Initial value is true.
   */
  bool GetNeedUpdate();

  // // Description:
  // INTERNAL USE ONLY.
  // Internally used by svtkShadowMapBakerPass and svtkShadowMapPass.
  //
  // Set NeedUpate to false. Called by svtkShadowMapPass.
  void SetUpToDate();

protected:
  /**
   * Default constructor. DelegatetPass is set to NULL.
   */
  svtkShadowMapBakerPass();

  /**
   * Destructor.
   */
  ~svtkShadowMapBakerPass() override;

  // svtkOpenGLRenderPass virtuals:
  bool PreReplaceShaderValues(std::string& vertexShader, std::string& geometryShader,
    std::string& fragmentShader, svtkAbstractMapper* mapper, svtkProp* prop) override;
  bool SetShaderParameters(svtkShaderProgram* program, svtkAbstractMapper* mapper, svtkProp* prop,
    svtkOpenGLVertexArrayObject* VAO = nullptr) override;

  /**
   * Helper method to compute the mNearest point in a given direction.
   * To be called several times, with initialized = false the first time.
   * v: point
   * pt: origin of the direction
   * dir: direction
   */
  void PointNearFar(
    double* v, double* pt, double* dir, double& mNear, double& mFar, bool initialized);

  /**
   * Compute the min/max of the projection of a box in a given direction.
   * bb: bounding box
   * pt: origin of the direction
   * dir: direction
   */
  void BoxNearFar(double* bb, double* pt, double* dir, double& mNear, double& mFar);

  /**
   * Build a camera from spot light parameters.
   * \pre light_exists: light!=0
   * \pre lcamera_exists: lcamera!=0
   */
  void BuildCameraLight(svtkLight* light, double* boundingBox, svtkCamera* lcamera);

  /**
   * Check if shadow mapping is supported by the current OpenGL context.
   * \pre w_exists: w!=0
   */
  void CheckSupport(svtkOpenGLRenderWindow* w);

  svtkRenderPass* OpaqueSequence;

  svtkRenderPass* CompositeZPass;

  unsigned int Resolution;

  bool HasShadows;

  /**
   * Graphics resources.
   */
  svtkOpenGLFramebufferObject* FrameBufferObject;

  std::vector<svtkSmartPointer<svtkTextureObject> >* ShadowMaps;
  std::vector<svtkSmartPointer<svtkCamera> >* LightCameras;

  svtkTimeStamp LastRenderTime;
  bool NeedUpdate;
  size_t CurrentLightIndex;

private:
  svtkShadowMapBakerPass(const svtkShadowMapBakerPass&) = delete;
  void operator=(const svtkShadowMapBakerPass&) = delete;
};

#endif
