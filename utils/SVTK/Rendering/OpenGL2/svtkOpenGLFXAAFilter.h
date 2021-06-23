/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLFXAAFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkOpenGLFXAAFilter
 * @brief   Perform FXAA antialiasing on the current
 * framebuffer.
 *
 *
 * Call Execute() to run a FXAA antialiasing pass on the current OpenGL
 * framebuffer. See method documentation for tunable parameters.
 *
 * Based on the following implementation and description:
 *
 * Whitepaper:
 * http://developer.download.nvidia.com/assets/gamedev/files/sdk/11/FXAA_WhitePaper.pdf
 *
 * Sample implementation:
 * https://github.com/NVIDIAGameWorks/GraphicsSamples/blob/master/samples/es3-kepler/FXAA/FXAA3_11.h
 *
 * TODO there are currently some "banding" artifacts on some edges, particularly
 * single pixel lines. These seem to be caused by using a linear RGB input,
 * rather than a gamma-correct sRGB input. Future work should combine this pass
 * with a gamma correction pass to correct this. Bonus points for precomputing
 * luminosity into the sRGB's alpha channel to save cycles in the FXAA shader!
 */

#ifndef svtkOpenGLFXAAFilter_h
#define svtkOpenGLFXAAFilter_h

#include "svtkFXAAOptions.h" // For DebugOptions enum
#include "svtkObject.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

#include <string> // For std::string

class svtkFXAAOptions;
class svtkOpenGLRenderer;
class svtkOpenGLRenderTimer;
class svtkShaderProgram;
class svtkTextureObject;
class svtkOpenGLQuadHelper;

class SVTKRENDERINGOPENGL2_EXPORT svtkOpenGLFXAAFilter : public svtkObject
{
public:
  static svtkOpenGLFXAAFilter* New();
  svtkTypeMacro(svtkOpenGLFXAAFilter, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Perform FXAA on the current render buffer in @a ren.
   */
  void Execute(svtkOpenGLRenderer* ren);

  /**
   * Release all OpenGL state.
   */
  void ReleaseGraphicsResources();

  /**
   * Copy the configuration values from @a opts into this filter. Note that
   * this copies the configuration values from opts -- it does not save the
   * @a opts pointer.
   */
  void UpdateConfiguration(svtkFXAAOptions* opts);

  //@{
  /**
   * Parameter for tuning the FXAA implementation. See svtkFXAAOptions for
   * details and suggested values.
   */
  svtkSetClampMacro(RelativeContrastThreshold, float, 0.f, 1.f);
  svtkGetMacro(RelativeContrastThreshold, float);
  svtkSetClampMacro(HardContrastThreshold, float, 0.f, 1.f);
  svtkGetMacro(HardContrastThreshold, float);
  svtkSetClampMacro(SubpixelBlendLimit, float, 0.f, 1.f);
  svtkGetMacro(SubpixelBlendLimit, float);
  svtkSetClampMacro(SubpixelContrastThreshold, float, 0.f, 1.f);
  svtkGetMacro(SubpixelContrastThreshold, float);
  virtual void SetUseHighQualityEndpoints(bool val);
  svtkGetMacro(UseHighQualityEndpoints, bool);
  svtkBooleanMacro(UseHighQualityEndpoints, bool);
  svtkSetClampMacro(EndpointSearchIterations, int, 0, SVTK_INT_MAX);
  svtkGetMacro(EndpointSearchIterations, int);
  virtual void SetDebugOptionValue(svtkFXAAOptions::DebugOption opt);
  svtkGetMacro(DebugOptionValue, svtkFXAAOptions::DebugOption);
  //@}

protected:
  svtkOpenGLFXAAFilter();
  ~svtkOpenGLFXAAFilter() override;

  void Prepare();
  void FreeGLObjects();
  void CreateGLObjects();
  void LoadInput();
  void ApplyFilter();
  void SubstituteFragmentShader(std::string& fragShader);
  void Finalize();

  void StartTimeQuery(svtkOpenGLRenderTimer* timer);
  void EndTimeQuery(svtkOpenGLRenderTimer* timer);
  void PrintBenchmark();

  // Cache GL state that we modify
  bool BlendState;
  bool DepthTestState;

  int Viewport[4]; // x, y, width, height

  // Used to measure execution time:
  svtkOpenGLRenderTimer* PreparationTimer;
  svtkOpenGLRenderTimer* FXAATimer;

  // Parameters:
  float RelativeContrastThreshold;
  float HardContrastThreshold;
  float SubpixelBlendLimit;
  float SubpixelContrastThreshold;
  int EndpointSearchIterations;

  bool UseHighQualityEndpoints;
  svtkFXAAOptions::DebugOption DebugOptionValue;

  // Set to true when the shader definitions change so we know when to rebuild.
  bool NeedToRebuildShader;

  svtkOpenGLRenderer* Renderer;
  svtkTextureObject* Input;

  svtkOpenGLQuadHelper* QHelper;

private:
  svtkOpenGLFXAAFilter(const svtkOpenGLFXAAFilter&) = delete;
  void operator=(const svtkOpenGLFXAAFilter&) = delete;
};

#endif // svtkOpenGLFXAAFilter_h
