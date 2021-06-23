/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenGLFluidMapper
 * @brief   Render fluid from position data (and color, if available)
 *
 * An OpenGL mapper that display fluid volume using a screen space
 * fluid rendering technique. Thanks to Nghia Truong for the algorihtm
 * and initial implementation.
 */

#ifndef svtkOpenGLFluidMapper_h
#define svtkOpenGLFluidMapper_h

#include "svtkAbstractVolumeMapper.h"

#include "svtkOpenGLHelper.h"           // used for ivars
#include "svtkRenderingOpenGL2Module.h" // For export macro
#include "svtkShader.h"                 // for methods
#include "svtkSmartPointer.h"           // for ivars

#include <map> //for methods

class svtkMatrix3x3;
class svtkMatrix4x4;
class svtkOpenGLFramebufferObject;
class svtkOpenGLRenderWindow;
class svtkOpenGLState;
class svtkOpenGLQuadHelper;
class svtkOpenGLVertexBufferObjectGroup;
class svtkPolyData;
class svtkTextureObject;

class SVTKRENDERINGOPENGL2_EXPORT svtkOpenGLFluidMapper : public svtkAbstractVolumeMapper
{
public:
  static svtkOpenGLFluidMapper* New();
  svtkTypeMacro(svtkOpenGLFluidMapper, svtkAbstractVolumeMapper);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify the input data to map.
   */
  void SetInputData(svtkPolyData* in);
  svtkPolyData* GetInput();
  //@}

  //@{
  /**
   * Turn on/off flag to control whether scalar data is used to color objects.
   */
  svtkSetMacro(ScalarVisibility, bool);
  svtkGetMacro(ScalarVisibility, bool);
  svtkBooleanMacro(ScalarVisibility, bool);
  //@}

  //@{
  /**
   * Set/Get the particle radius, must be explicitly set by user
   * To fuse the gaps between particles and obtain a smooth surface,
   * this parameter need to be slightly larger than the actual particle radius,
   * (particle radius is the half distance between two consecutive particles in
   * regular pattern sampling)
   */
  svtkSetMacro(ParticleRadius, float);
  svtkGetMacro(ParticleRadius, float);
  //@}

  //@{
  /**
   * Get/Set the number of filter iterations to filter the depth surface
   * This is an optional parameter, default value is 3
   * Usually set this to around 3-5
   * Too many filter iterations will over-smooth the surface
   */
  svtkSetMacro(SurfaceFilterIterations, uint32_t);
  svtkGetMacro(SurfaceFilterIterations, uint32_t);
  //@}

  //@{
  /**
   * Get/Set the number of filter iterations to filter the volume thickness
   * and particle color This is an optional parameter, default value is 3
   */
  svtkSetMacro(ThicknessAndVolumeColorFilterIterations, uint32_t);
  svtkGetMacro(ThicknessAndVolumeColorFilterIterations, uint32_t);
  //@}

  //@{
  /**
   * Get/Set the filter radius for smoothing the depth surface
   * This is an optional parameter, default value is 5
   * This is not exactly the radius in pixels,
   * instead it is just a parameter used for computing the actual filter
   * radius in the screen space filtering
   */
  svtkSetMacro(SurfaceFilterRadius, uint32_t);
  svtkGetMacro(SurfaceFilterRadius, uint32_t);
  //@}

  //@{
  /**
   * Get/Set the filter radius to filter the volume thickness and particle
   * color This is an optional parameter, default value is 10 (pixels)
   */
  svtkSetMacro(ThicknessAndVolumeColorFilterRadius, float);
  svtkGetMacro(ThicknessAndVolumeColorFilterRadius, float);
  //@}

  /**
   * Filter method to filter the depth buffer
   */
  enum FluidSurfaceFilterMethod
  {
    BilateralGaussian = 0,
    NarrowRange,
    // New filter method can be added here,
    NumFilterMethods
  };

  //@{
  /**
   * Get/Set the filter method for filtering fluid surface
   */
  svtkSetMacro(SurfaceFilterMethod, svtkOpenGLFluidMapper::FluidSurfaceFilterMethod);
  svtkGetMacro(SurfaceFilterMethod, svtkOpenGLFluidMapper::FluidSurfaceFilterMethod);
  //@}

  /**
   * Optional parameters, exclusively for narrow range filter
   * The first parameter is to control smoothing between surface depth values
   * The second parameter is to control curvature of the surface edges
   */
  void SetNarrowRangeFilterParameters(float lambda, float mu)
  {
    this->NRFilterLambda = lambda;
    this->NRFilterMu = mu;
  }

  /**
   * Optional parameters, exclusively for bilateral gaussian filter
   * The parameter is for controlling smoothing between surface depth values
   */
  void SetBilateralGaussianFilterParameter(float sigmaDepth)
  {
    this->BiGaussFilterSigmaDepth = sigmaDepth;
  }

  /**
   * Display mode for the fluid, default value is TransparentFluidVolume
   */
  enum FluidDisplayMode
  {
    UnfilteredOpaqueSurface = 0,
    FilteredOpaqueSurface,
    UnfilteredSurfaceNormal,
    FilteredSurfaceNormal,
    TransparentFluidVolume,
    NumDisplayModes
  };

  //@{
  /**
   * Get/Set the display mode
   */
  svtkSetMacro(DisplayMode, svtkOpenGLFluidMapper::FluidDisplayMode);
  svtkGetMacro(DisplayMode, svtkOpenGLFluidMapper::FluidDisplayMode);
  //@}

  //@{
  /**
   * Get/Set the fluid attenuation color
   * (color that will be absorpted exponentially when going through the fluid
   * volume)
   */
  svtkSetVector3Macro(AttenuationColor, float);
  svtkGetVector3Macro(AttenuationColor, float);
  //@}

  //@{
  /**
   * Get/Set the fluid surface color if rendered in opaque surface mode
   * without particle color
   */
  svtkSetVector3Macro(OpaqueColor, float);
  svtkGetVector3Macro(OpaqueColor, float);
  //@}

  //@{
  /**
   * Get/Set the power value for particle color if input data has particle
   * color Default value is 0.1, and can be set to any non-negative number The
   * particle color is then recomputed as newColor = pow(oldColor, power) *
   * scale
   */
  svtkSetMacro(ParticleColorPower, float);
  svtkGetMacro(ParticleColorPower, float);
  //@}

  //@{
  /**
   * Get/Set the scale value for particle color if input data has particle
   * color Default value is 1.0, and can be set to any non-negative number The
   * particle color is then recomputed as newColor = pow(oldColor, power) *
   * scale
   */
  svtkSetMacro(ParticleColorScale, float);
  svtkGetMacro(ParticleColorScale, float);
  //@}

  //@{
  /**
   * Get/Set the fluid volume attenuation scale, which will be multiplied
   * with attennuation color Default value is 1.0, and can be set to any
   * non-negative number The larger attennuation scale, the darker fluid
   * color
   */
  svtkSetMacro(AttenuationScale, float);
  svtkGetMacro(AttenuationScale, float);
  //@}

  //@{
  /**
   * Get/Set the fluid surface additional reflection scale This value is in
   * [0, 1], which 0 means using the reflection color computed from fresnel
   * equation, and 1 means using reflection color as [1, 1, 1] Default value
   * is 0
   */
  svtkSetMacro(AdditionalReflection, float);
  svtkGetMacro(AdditionalReflection, float);
  //@}

  //@{
  /**
   * Get/Set the scale value for refraction This is needed for tweak
   * refraction of volumes with different size scales For example, fluid
   * volume having diameter of 100.0 will refract light much more than
   * volume with diameter 1.0 This value is in [0, 1], default value is 1.0
   */
  svtkSetMacro(RefractionScale, float);
  svtkGetMacro(RefractionScale, float);
  //@}

  //@{
  /**
   * Get/Set the fluid refraction index. The default value is 1.33 (water)
   */
  svtkSetMacro(RefractiveIndex, float);
  svtkGetMacro(RefractiveIndex, float);
  //@}

  /**
   * This calls RenderPiece
   */
  void Render(svtkRenderer* ren, svtkVolume* vol) override;

  /**
   * Release graphics resources and ask components to release their own
   * resources.
   * \pre w_exists: w!=0
   */
  void ReleaseGraphicsResources(svtkWindow* w) override;

protected:
  svtkOpenGLFluidMapper();
  ~svtkOpenGLFluidMapper() override;

  /**
   * Perform string replacements on the shader templates
   */
  void UpdateDepthThicknessColorShaders(
    svtkOpenGLHelper& glHelper, svtkRenderer* renderer, svtkVolume* vol);

  /**
   * Set the shader parameters related to the actor/mapper/camera
   */
  void SetDepthThicknessColorShaderParameters(
    svtkOpenGLHelper& glHelper, svtkRenderer* renderer, svtkVolume* vol);

  /**
   * Setup the texture buffers
   */
  void SetupBuffers(svtkOpenGLRenderWindow* const renderWindow);

  /**
   * Render the fluid particles
   */
  void RenderParticles(svtkRenderer* renderer, svtkVolume* vol);

  // Public parameters, their usage are stated at their Get/Set functions
  // ======>>>>>
  float ParticleRadius = 1.0f;

  FluidSurfaceFilterMethod SurfaceFilterMethod = FluidSurfaceFilterMethod::NarrowRange;
  uint32_t SurfaceFilterIterations = 3u;
  uint32_t SurfaceFilterRadius = 5u;
  float NRFilterLambda = 10.0f;
  float NRFilterMu = 1.0f;
  float BiGaussFilterSigmaDepth = 10.0f;

  uint32_t ThicknessAndVolumeColorFilterIterations = 3u;
  uint32_t ThicknessAndVolumeColorFilterRadius = 10u;

  FluidDisplayMode DisplayMode = FluidDisplayMode::TransparentFluidVolume;

  float OpaqueColor[3]{ 0.0f, 0.0f, 0.95f };
  float AttenuationColor[3]{ 0.5f, 0.2f, 0.05f };
  float ParticleColorPower = 0.1f;
  float ParticleColorScale = 1.0f;
  float AttenuationScale = 1.0f;
  float AdditionalReflection = 0.0f;
  float RefractionScale = 1.0f;
  float RefractiveIndex = 1.33f;

  bool ScalarVisibility = false;
  bool InDepthPass = true;

  // Private parameters ======>>>>>

  // Indicate that the input data has a color buffer
  bool HasVertexColor = false;

  // Cache viewport dimensions
  int ViewportX;
  int ViewportY;
  int ViewportWidth;
  int ViewportHeight;

  // Cache camera parameters
  svtkMatrix4x4* CamWCVC;
  svtkMatrix3x3* CamInvertedNorms;
  svtkMatrix4x4* CamVCDC;
  svtkMatrix4x4* CamWCDC;
  svtkMatrix4x4* CamDCVC;
  svtkTypeBool CamParallelProjection;

  // Frame buffers
  svtkSmartPointer<svtkOpenGLFramebufferObject> FBFluidEyeZ;
  svtkSmartPointer<svtkOpenGLFramebufferObject> FBThickness;
  svtkSmartPointer<svtkOpenGLFramebufferObject> FBFilterThickness;
  svtkSmartPointer<svtkOpenGLFramebufferObject> FBCompNormal;
  svtkSmartPointer<svtkOpenGLFramebufferObject> FBFilterDepth;

  // Screen quad render
  svtkOpenGLQuadHelper* QuadFluidDepthFilter[NumFilterMethods]{ nullptr, nullptr };
  svtkOpenGLQuadHelper* QuadThicknessFilter = nullptr;
  svtkOpenGLQuadHelper* QuadFluidNormal = nullptr;
  svtkOpenGLQuadHelper* QuadFinalBlend = nullptr;

  // The VBO and its layout for rendering particles
  svtkSmartPointer<svtkOpenGLVertexBufferObjectGroup> VBOs;
  svtkTimeStamp VBOBuildTime; // When was the OpenGL VBO updated?
  svtkOpenGLHelper GLHelperDepthThickness;

  // Texture buffers
  enum TextureBuffers
  {
    OpaqueZ = 0,
    OpaqueRGBA,
    FluidZ,
    FluidEyeZ,
    SmoothedFluidEyeZ,
    FluidThickness,
    SmoothedFluidThickness,
    FluidNormal,
    NumTexBuffers
  };

  // These are optional texture buffers
  enum OptionalTextureBuffers
  {
    Color = 0,
    SmoothedColor,
    NumOptionalTexBuffers
  };

  svtkTextureObject* TexBuffer[NumTexBuffers];
  svtkTextureObject* OptionalTexBuffer[NumOptionalTexBuffers];
  svtkMatrix4x4* TempMatrix4;

private:
  svtkOpenGLFluidMapper(const svtkOpenGLFluidMapper&) = delete;
  void operator=(const svtkOpenGLFluidMapper&) = delete;
};

#endif
