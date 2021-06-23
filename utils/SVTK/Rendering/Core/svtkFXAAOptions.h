/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFXAAOptions.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkFXAAOptions
 * @brief   Configuration for FXAA implementations.
 *
 *
 * This class encapsulates the settings for svtkOpenGLFXAAFilter.
 */

#ifndef svtkFXAAOptions_h
#define svtkFXAAOptions_h

#include "svtkObject.h"
#include "svtkRenderingCoreModule.h" // For export macro

class SVTKRENDERINGCORE_EXPORT svtkFXAAOptions : public svtkObject
{
public:
  /**
   * Debugging options that affect the output color buffer. See
   * svtkFXAAFilterFS.glsl for details.
   */
  enum DebugOption
  {
    FXAA_NO_DEBUG = 0,
    FXAA_DEBUG_SUBPIXEL_ALIASING,
    FXAA_DEBUG_EDGE_DIRECTION,
    FXAA_DEBUG_EDGE_NUM_STEPS,
    FXAA_DEBUG_EDGE_DISTANCE,
    FXAA_DEBUG_EDGE_SAMPLE_OFFSET,
    FXAA_DEBUG_ONLY_SUBPIX_AA,
    FXAA_DEBUG_ONLY_EDGE_AA
  };

  static svtkFXAAOptions* New();
  svtkTypeMacro(svtkFXAAOptions, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Threshold for applying FXAA to a pixel, relative to the maximum luminosity
   * of its 4 immediate neighbors.

   * The luminosity of the current pixel and it's NSWE neighbors is computed.
   * The maximum luminosity and luminosity range (contrast) of all 5 pixels is
   * found. If the contrast is less than RelativeContrastThreshold * maxLum,
   * the pixel is not considered aliased and will not be affected by FXAA.

   * Suggested settings:
   * - 1/3: Too little
   * - 1/4: Low quality
   * - 1/8: High quality (default)
   * - 1/16: Overkill
   */
  svtkSetClampMacro(RelativeContrastThreshold, float, 0.f, 1.f);
  svtkGetMacro(RelativeContrastThreshold, float);
  //@}

  //@{
  /**
   * Similar to RelativeContrastThreshold, but not scaled by the maximum
   * luminosity.

   * If the contrast of the current pixel and it's 4 immediate NSWE neighbors is
   * less than HardContrastThreshold, the pixel is not considered aliased and
   * will not be affected by FXAA.

   * Suggested settings:
   * - 1/32: Visible limit
   * - 1/16: High quality (default)
   * - 1/12: Upper limit (start of visible unfiltered edges)
   */
  svtkSetClampMacro(HardContrastThreshold, float, 0.f, 1.f);
  svtkGetMacro(HardContrastThreshold, float);
  //@}

  //@{
  /**
   * Subpixel aliasing is corrected by applying a lowpass filter to the current
   * pixel. This is implemented by blending an average of the 3x3 neighborhood
   * around the pixel into the final result. The amount of blending is
   * determined by comparing the detected amount of subpixel aliasing to the
   * total contrasting of the CNSWE pixels:

   * SubpixelBlending = abs(lumC - lumAveNSWE) / (lumMaxCNSWE - lumMinCNSWE)

   * This parameter sets an upper limit to the amount of subpixel blending to
   * prevent the image from simply getting blurred.

   * Suggested settings:
   * - 1/2: Low amount of blending.
   * - 3/4: Medium amount of blending (default)
   * - 7/8: High amount of blending.
   * - 1: Maximum amount of blending.
   */
  svtkSetClampMacro(SubpixelBlendLimit, float, 0.f, 1.f);
  svtkGetMacro(SubpixelBlendLimit, float);
  //@}

  //@{
  /**
   * Minimum amount of subpixel aliasing required for subpixel antialiasing to
   * be applied.

   * Subpixel aliasing is corrected by applying a lowpass filter to the current
   * pixel. This is implemented by blending an average of the 3x3 neighborhood
   * around the pixel into the final result. The amount of blending is
   * determined by comparing the detected amount of subpixel aliasing to the
   * total contrasting of the CNSWE pixels:

   * SubpixelBlending = abs(lumC - lumAveNSWE) / (lumMaxCNSWE - lumMinCNSWE)

   * If SubpixelBlending is less than this threshold, no lowpass blending will
   * occur.

   * Suggested settings:
   * - 1/2: Low subpixel aliasing removal
   * - 1/3: Medium subpixel aliasing removal
   * - 1/4: Default subpixel aliasing removal
   * - 1/8: High subpixel aliasing removal
   * - 0: Complete subpixel aliasing removal
   */
  svtkSetClampMacro(SubpixelContrastThreshold, float, 0.f, 1.f);
  svtkGetMacro(SubpixelContrastThreshold, float);
  //@}

  //@{
  /**
   * Use an improved edge endpoint detection algorithm.

   * If true, a modified edge endpoint detection algorithm is used that requires
   * more texture lookups, but will properly detect aliased single-pixel lines.

   * If false, the edge endpoint algorithm proposed by NVIDIA will by used. This
   * algorithm is faster (fewer lookups), but will fail to detect endpoints of
   * single pixel edge steps.

   * Default setting is true.
   */
  svtkSetMacro(UseHighQualityEndpoints, bool);
  svtkGetMacro(UseHighQualityEndpoints, bool);
  svtkBooleanMacro(UseHighQualityEndpoints, bool);
  //@}

  //@{
  /**
   * Set the number of iterations for the endpoint search algorithm. Increasing
   * this value will increase runtime, but also properly detect longer edges.
   * The current implementation steps one pixel in both the positive and
   * negative directions per iteration. The default value is 12, which will
   * resolve endpoints of edges < 25 pixels long (2 * 12 + 1).
   */
  svtkSetClampMacro(EndpointSearchIterations, int, 0, SVTK_INT_MAX);
  svtkGetMacro(EndpointSearchIterations, int);
  //@}

  //@{
  /**
   * Debugging options that affect the output color buffer. See
   * svtkFXAAFilterFS.glsl for details. Only one may be active at a time.
   */
  svtkSetMacro(DebugOptionValue, DebugOption);
  svtkGetMacro(DebugOptionValue, DebugOption);
  //@}

protected:
  svtkFXAAOptions();
  ~svtkFXAAOptions() override;

  float RelativeContrastThreshold;
  float HardContrastThreshold;
  float SubpixelBlendLimit;
  float SubpixelContrastThreshold;
  int EndpointSearchIterations;
  bool UseHighQualityEndpoints;
  DebugOption DebugOptionValue;

private:
  svtkFXAAOptions(const svtkFXAAOptions&) = delete;
  void operator=(const svtkFXAAOptions&) = delete;
};

#endif // svtkFXAAOptions_h
