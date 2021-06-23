/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLGPUVolumeRayCastMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class svtkOpenGLGPUVolumeRayCastMapper
 * @brief OpenGL implementation of volume rendering through ray-casting.
 *
 * @section multi Multiple Inputs

 * When multiple inputs are rendered simultaneously, it is possible to
 * composite overlapping areas correctly. Inputs are connected directly to
 * the mapper and their parameters (transfer functions, transformations, etc.)
 * are specified through standard svtkVolume instances. These svtkVolume
 * instances are to be registered in a special svtkProp3D, svtkMultiVolume.
 *
 * Structures related to a particular active input are stored in a helper
 * class (svtkVolumeInputHelper) and helper structures are kept in a
 * port-referenced map (VolumeInputMap). The order of the inputs in the
 * map is important as it defines the order in which parameters are
 * bound to uniform variables (transformation matrices, bias, scale and every
 * other required rendering parameter).
 *
 * A separate code path is used when rendering multiple-inputs in order to
 * facilitate the co-existance of these two modes (single/multiple), due to
 * current feature incompatibilities with multiple inputs (e.g. texture-streaming,
 * cropping, etc.).
 *
 * @note A limited set of the mapper features are currently supported for
 * multiple inputs:
 *
 * - Blending
 *   - Composite (front-to-back)
 *
 * - Transfer functions (defined separately for per input)
 *   - 1D color
 *   - 1D scalar opacity
 *   - 1D gradient magnitude opacity
 *   - 2D scalar-gradient magnitude
 *
 * - Point and cell data
 *   - With the limitation that all of the inputs are assumed to share the same
 *     name/id.
 *
 * @sa svtkGPUVolumeRayCastMapper svtkVolumeInputHelper svtkVolumeTexture
 * svtkMultiVolume
 *
 */

#ifndef svtkOpenGLGPUVolumeRayCastMapper_h
#define svtkOpenGLGPUVolumeRayCastMapper_h
#include <map> // For methods

#include "svtkGPUVolumeRayCastMapper.h"
#include "svtkNew.h"                          // For svtkNew
#include "svtkRenderingVolumeOpenGL2Module.h" // For export macro
#include "svtkShader.h"                       // For methods
#include "svtkSmartPointer.h"                 // For smartptr

class svtkGenericOpenGLResourceFreeCallback;
class svtkImplicitFunction;
class svtkOpenGLCamera;
class svtkOpenGLTransferFunctions2D;
class svtkOpenGLVolumeGradientOpacityTables;
class svtkOpenGLVolumeOpacityTables;
class svtkOpenGLVolumeRGBTables;
class svtkShaderProgram;
class svtkTextureObject;
class svtkVolume;
class svtkVolumeInputHelper;
class svtkVolumeTexture;
class svtkOpenGLShaderProperty;

class SVTKRENDERINGVOLUMEOPENGL2_EXPORT svtkOpenGLGPUVolumeRayCastMapper
  : public svtkGPUVolumeRayCastMapper
{
public:
  static svtkOpenGLGPUVolumeRayCastMapper* New();

  enum Passes
  {
    RenderPass,
    DepthPass = 1
  };

  svtkTypeMacro(svtkOpenGLGPUVolumeRayCastMapper, svtkGPUVolumeRayCastMapper);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  // Description:
  // Low level API to enable access to depth texture in
  // RenderToTexture mode. It will return either nullptr if
  // RenderToImage was never turned on or texture captured
  // the last time RenderToImage was on.
  svtkTextureObject* GetDepthTexture();

  // Description:
  // Low level API to enable access to color texture in
  // RenderToTexture mode. It will return either nullptr if
  // RenderToImage was never turned on or texture captured
  // the last time RenderToImage was on.
  svtkTextureObject* GetColorTexture();

  // Description:
  // Low level API to export the depth texture as svtkImageData in
  // RenderToImage mode.
  void GetDepthImage(svtkImageData* im) override;

  // Description:
  // Low level API to export the color texture as svtkImageData in
  // RenderToImage mode.
  void GetColorImage(svtkImageData* im) override;

  // Description:
  // Mapper can have multiple passes and internally it will set
  // the state. The state can not be set externally explicitly
  // but can be set indirectly depending on the options set by
  // the user.
  svtkGetMacro(CurrentPass, int);

  // Sets a depth texture for this mapper to use
  // This allows many mappers to use the same
  // texture reducing GPU usage. If this is set
  // the standard depth texture code is skipped
  // The depth texture should be activated
  // and deactivated outside of this class
  void SetSharedDepthTexture(svtkTextureObject* nt);

  /**
   * Set a fixed number of partitions in which to split the volume
   * during rendring. This will force by-block rendering without
   * trying to compute an optimum number of partitions.
   */
  void SetPartitions(unsigned short x, unsigned short y, unsigned short z);

  /**
   *  Load the volume texture into GPU memory.  Actual loading occurs
   *  in svtkVolumeTexture::LoadVolume.  The mapper by default loads data
   *  lazily (at render time), so it is most commonly not necessary to call
   *  this function.  This method is only exposed in order to support on-site
   *  loading which is useful in cases where the user needs to know a-priori
   *  whether loading will succeed or not.
   */
  bool PreLoadData(svtkRenderer* ren, svtkVolume* vol);

  // Description:
  // Delete OpenGL objects.
  // \post done: this->OpenGLObjectsCreated==0
  void ReleaseGraphicsResources(svtkWindow* window) override;

protected:
  svtkOpenGLGPUVolumeRayCastMapper();
  ~svtkOpenGLGPUVolumeRayCastMapper() override;

  svtkGenericOpenGLResourceFreeCallback* ResourceCallback;

  // Description:
  // Build vertex and fragment shader for the volume rendering
  void BuildDepthPassShader(
    svtkRenderer* ren, svtkVolume* vol, int noOfComponents, int independentComponents);

  // Description:
  // Build vertex and fragment shader for the volume rendering
  void BuildShader(svtkRenderer* ren);

  // TODO Take these out as these are no longer needed
  // Methods called by the AMR Volume Mapper.
  void PreRender(svtkRenderer* svtkNotUsed(ren), svtkVolume* svtkNotUsed(vol),
    double svtkNotUsed(datasetBounds)[6], double svtkNotUsed(scalarRange)[2],
    int svtkNotUsed(noOfComponents), unsigned int svtkNotUsed(numberOfLevels)) override
  {
  }

  // \pre input is up-to-date
  void RenderBlock(svtkRenderer* svtkNotUsed(ren), svtkVolume* svtkNotUsed(vol),
    unsigned int svtkNotUsed(level)) override
  {
  }

  void PostRender(svtkRenderer* svtkNotUsed(ren), int svtkNotUsed(noOfComponents)) override {}

  // Description:
  // Rendering volume on GPU
  void GPURender(svtkRenderer* ren, svtkVolume* vol) override;

  // Description:
  // Method that performs the actual rendering given a volume and a shader
  void DoGPURender(svtkRenderer* ren, svtkOpenGLCamera* cam, svtkShaderProgram* shaderProgram,
    svtkOpenGLShaderProperty* shaderProperty);

  // Description:
  // Update the reduction factor of the render viewport (this->ReductionFactor)
  // according to the time spent in seconds to render the previous frame
  // (this->TimeToDraw) and a time in seconds allocated to render the next
  // frame (allocatedTime).
  // \pre valid_current_reduction_range: this->ReductionFactor>0.0 && this->ReductionFactor<=1.0
  // \pre positive_TimeToDraw: this->TimeToDraw>=0.0
  // \pre positive_time: allocatedTime>0
  // \post valid_new_reduction_range: this->ReductionFactor>0.0 && this->ReductionFactor<=1.0
  void ComputeReductionFactor(double allocatedTime);

  // Description:
  // Empty implementation.
  void GetReductionRatio(double* ratio) override { ratio[0] = ratio[1] = ratio[2] = 1.0; }

  // Description:
  // Empty implementation.
  int IsRenderSupported(
    svtkRenderWindow* svtkNotUsed(window), svtkVolumeProperty* svtkNotUsed(property)) override
  {
    return 1;
  }

  //@{
  /**
   *  \brief svtkOpenGLRenderPass API
   */
  svtkMTimeType GetRenderPassStageMTime(svtkVolume* vol);

  /**
   * Create the basic shader template strings before substitutions
   */
  void GetShaderTemplate(
    std::map<svtkShader::Type, svtkShader*>& shaders, svtkOpenGLShaderProperty* p);

  /**
   * Perform string replacements on the shader templates
   */
  void ReplaceShaderValues(
    std::map<svtkShader::Type, svtkShader*>& shaders, svtkRenderer* ren, svtkVolume* vol, int numComps);

  /**
   *  RenderPass string replacements on shader templates called from
   *  ReplaceShaderValues.
   */
  void ReplaceShaderCustomUniforms(
    std::map<svtkShader::Type, svtkShader*>& shaders, svtkOpenGLShaderProperty* p);
  void ReplaceShaderBase(
    std::map<svtkShader::Type, svtkShader*>& shaders, svtkRenderer* ren, svtkVolume* vol, int numComps);
  void ReplaceShaderTermination(
    std::map<svtkShader::Type, svtkShader*>& shaders, svtkRenderer* ren, svtkVolume* vol, int numComps);
  void ReplaceShaderShading(
    std::map<svtkShader::Type, svtkShader*>& shaders, svtkRenderer* ren, svtkVolume* vol, int numComps);
  void ReplaceShaderCompute(
    std::map<svtkShader::Type, svtkShader*>& shaders, svtkRenderer* ren, svtkVolume* vol, int numComps);
  void ReplaceShaderCropping(
    std::map<svtkShader::Type, svtkShader*>& shaders, svtkRenderer* ren, svtkVolume* vol, int numComps);
  void ReplaceShaderClipping(
    std::map<svtkShader::Type, svtkShader*>& shaders, svtkRenderer* ren, svtkVolume* vol, int numComps);
  void ReplaceShaderMasking(
    std::map<svtkShader::Type, svtkShader*>& shaders, svtkRenderer* ren, svtkVolume* vol, int numComps);
  void ReplaceShaderPicking(
    std::map<svtkShader::Type, svtkShader*>& shaders, svtkRenderer* ren, svtkVolume* vol, int numComps);
  void ReplaceShaderRTT(
    std::map<svtkShader::Type, svtkShader*>& shaders, svtkRenderer* ren, svtkVolume* vol, int numComps);
  void ReplaceShaderRenderPass(
    std::map<svtkShader::Type, svtkShader*>& shaders, svtkVolume* vol, bool prePass);

  /**
   *  Update parameters from RenderPass
   */
  void SetShaderParametersRenderPass();

  /**
   *  Caches the svtkOpenGLRenderPass::RenderPasses() information.
   *  Note: Do not dereference the pointers held by this object. There is no
   *  guarantee that they are still valid!
   */
  svtkNew<svtkInformation> LastRenderPassInfo;
  //@}

  double ReductionFactor;
  int CurrentPass;

public:
  using VolumeInput = svtkVolumeInputHelper;
  using VolumeInputMap = std::map<int, svtkVolumeInputHelper>;
  VolumeInputMap AssembledInputs;

private:
  class svtkInternal;
  svtkInternal* Impl;

  friend class svtkVolumeTexture;

  svtkOpenGLGPUVolumeRayCastMapper(const svtkOpenGLGPUVolumeRayCastMapper&) = delete;
  void operator=(const svtkOpenGLGPUVolumeRayCastMapper&) = delete;
};

#endif // svtkOpenGLGPUVolumeRayCastMapper_h
