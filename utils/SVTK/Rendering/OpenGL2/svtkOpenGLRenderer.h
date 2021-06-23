/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLRenderer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenGLRenderer
 * @brief   OpenGL renderer
 *
 * svtkOpenGLRenderer is a concrete implementation of the abstract class
 * svtkRenderer. svtkOpenGLRenderer interfaces to the OpenGL graphics library.
 */

#ifndef svtkOpenGLRenderer_h
#define svtkOpenGLRenderer_h

#include "svtkRenderer.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro
#include "svtkSmartPointer.h"           // For svtkSmartPointer
#include <string>                      // Ivars
#include <vector>                      // STL Header

class svtkOpenGLFXAAFilter;
class svtkRenderPass;
class svtkOpenGLState;
class svtkOpenGLTexture;
class svtkOrderIndependentTranslucentPass;
class svtkTextureObject;
class svtkDepthPeelingPass;
class svtkPBRIrradianceTexture;
class svtkPBRLUTTexture;
class svtkPBRPrefilterTexture;
class svtkShaderProgram;
class svtkShadowMapPass;

class SVTKRENDERINGOPENGL2_EXPORT svtkOpenGLRenderer : public svtkRenderer
{
public:
  static svtkOpenGLRenderer* New();
  svtkTypeMacro(svtkOpenGLRenderer, svtkRenderer);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Concrete open gl render method.
   */
  void DeviceRender(void) override;

  /**
   * Overridden to support hidden line removal.
   */
  void DeviceRenderOpaqueGeometry(svtkFrameBufferObjectBase* fbo = nullptr) override;

  /**
   * Render translucent polygonal geometry. Default implementation just call
   * UpdateTranslucentPolygonalGeometry().
   * Subclasses of svtkRenderer that can deal with depth peeling must
   * override this method.
   */
  void DeviceRenderTranslucentPolygonalGeometry(svtkFrameBufferObjectBase* fbo = nullptr) override;

  void Clear(void) override;

  /**
   * Ask lights to load themselves into graphics pipeline.
   */
  int UpdateLights(void) override;

  /**
   * Is rendering at translucent geometry stage using depth peeling and
   * rendering a layer other than the first one? (Boolean value)
   * If so, the uniform variables UseTexture and Texture can be set.
   * (Used by svtkOpenGLProperty or svtkOpenGLTexture)
   */
  int GetDepthPeelingHigherLayer();

  /**
   * Indicate if this system is subject to the Apple/AMD bug
   * of not having a working glPrimitiveId <rdar://20747550>.
   * The bug is fixed on macOS 10.11 and later, and this method
   * will return false when the OS is new enough.
   */
  bool HaveApplePrimitiveIdBug();

  /**
   * Indicate if this system is subject to the apple/NVIDIA bug that causes
   * crashes in the driver when too many query objects are allocated.
   */
  static bool HaveAppleQueryAllocationBug();

  /**
   * Dual depth peeling may be disabled for certain runtime configurations.
   * This method returns true if svtkDualDepthPeelingPass will be used in place
   * of svtkDepthPeelingPass.
   */
  bool IsDualDepthPeelingSupported();

  // Get the state object used to keep track of
  // OpenGL state
  svtkOpenGLState* GetState();

  // get the standard lighting uniform declarations
  // for the current set of lights
  const char* GetLightingUniforms();

  // update the lighting uniforms for this shader if they
  // are out of date
  void UpdateLightingUniforms(svtkShaderProgram* prog);

  // get the complexity of the current lights as a int
  // 0 = no lighting
  // 1 = headlight
  // 2 = directional lights
  // 3 = positional lights
  enum LightingComplexityEnum
  {
    NoLighting = 0,
    Headlight = 1,
    Directional = 2,
    Positional = 3
  };
  svtkGetMacro(LightingComplexity, int);

  // get the number of lights turned on
  svtkGetMacro(LightingCount, int);

  //@{
  /**
   * Set the user light transform applied after the camera transform.
   * Can be null to disable it.
   */
  void SetUserLightTransform(svtkTransform* transform);
  svtkTransform* GetUserLightTransform();
  //@}

  //@{
  /**
   * Get environment textures used for image based lighting.
   */
  svtkPBRLUTTexture* GetEnvMapLookupTable();
  svtkPBRIrradianceTexture* GetEnvMapIrradiance();
  svtkPBRPrefilterTexture* GetEnvMapPrefiltered();
  //@}

  /**
   * Overriden in order to connect the texture to the environment map textures.
   */
  void SetEnvironmentTexture(svtkTexture* texture, bool isSRGB = false) override;

  // Method to release graphics resources
  void ReleaseGraphicsResources(svtkWindow* w) override;

protected:
  svtkOpenGLRenderer();
  ~svtkOpenGLRenderer() override;

  /**
   * Check the compilation status of some fragment shader source.
   */
  void CheckCompilation(unsigned int fragmentShader);

  /**
   * Ask all props to update and draw any opaque and translucent
   * geometry. This includes both svtkActors and svtkVolumes
   * Returns the number of props that rendered geometry.
   */
  int UpdateGeometry(svtkFrameBufferObjectBase* fbo = nullptr) override;

  /**
   * Check and return the textured background for the current state
   * If monocular or stereo left eye, check BackgroundTexture
   * If stereo right eye, check RightBackgroundTexture
   */
  svtkTexture* GetCurrentTexturedBackground();

  friend class svtkOpenGLProperty;
  friend class svtkOpenGLTexture;
  friend class svtkOpenGLImageSliceMapper;
  friend class svtkOpenGLImageResliceMapper;

  /**
   * FXAA is delegated to an instance of svtkOpenGLFXAAFilter
   */
  svtkOpenGLFXAAFilter* FXAAFilter;

  /**
   * Depth peeling is delegated to an instance of svtkDepthPeelingPass
   */
  svtkDepthPeelingPass* DepthPeelingPass;

  /**
   * Fallback for transparency
   */
  svtkOrderIndependentTranslucentPass* TranslucentPass;

  /**
   * Shadows are delegated to an instance of svtkShadowMapPass
   */
  svtkShadowMapPass* ShadowMapPass;

  // Is rendering at translucent geometry stage using depth peeling and
  // rendering a layer other than the first one? (Boolean value)
  // If so, the uniform variables UseTexture and Texture can be set.
  // (Used by svtkOpenGLProperty or svtkOpenGLTexture)
  int DepthPeelingHigherLayer;

  friend class svtkRenderPass;

  std::string LightingDeclaration;
  int LightingComplexity;
  int LightingCount;
  svtkMTimeType LightingUpdateTime;

  /**
   * Optional user transform for lights
   */
  svtkSmartPointer<svtkTransform> UserLightTransform;

  svtkPBRLUTTexture* EnvMapLookupTable;
  svtkPBRIrradianceTexture* EnvMapIrradiance;
  svtkPBRPrefilterTexture* EnvMapPrefiltered;

private:
  svtkOpenGLRenderer(const svtkOpenGLRenderer&) = delete;
  void operator=(const svtkOpenGLRenderer&) = delete;
};

#endif
