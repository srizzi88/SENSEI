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
 * @class   svtkPointGaussianMapper
 * @brief   draw PointGaussians using imposters
 *
 *
 * A mapper that uses imposters to draw gaussian splats or other shapes if
 * custom shader code is set. Supports transparency and picking as well. It
 * draws all the points and does not require cell arrays. If cell arrays are
 * provided it will only draw the points used by the Verts cell array. The shape
 * of the imposter is a triangle.
 */

#ifndef svtkPointGaussianMapper_h
#define svtkPointGaussianMapper_h

#include "svtkPolyDataMapper.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkPiecewiseFunction;

class SVTKRENDERINGCORE_EXPORT svtkPointGaussianMapper : public svtkPolyDataMapper
{
public:
  static svtkPointGaussianMapper* New();
  svtkTypeMacro(svtkPointGaussianMapper, svtkPolyDataMapper);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the optional scale transfer function. This is only
   * used when a ScaleArray is also specified.
   */
  void SetScaleFunction(svtkPiecewiseFunction*);
  svtkGetObjectMacro(ScaleFunction, svtkPiecewiseFunction);
  //@}

  //@{
  /**
   * The size of the table used in computing scale, used when
   * converting a svtkPiecewiseFunction to a table
   */
  svtkSetMacro(ScaleTableSize, int);
  svtkGetMacro(ScaleTableSize, int);
  //@}

  //@{
  /**
   * Convenience method to set the array to scale with.
   */
  svtkSetStringMacro(ScaleArray);
  svtkGetStringMacro(ScaleArray);
  //@}

  //@{
  /**
   * Convenience method to set the component of the array to scale with.
   */
  svtkSetMacro(ScaleArrayComponent, int);
  svtkGetMacro(ScaleArrayComponent, int);
  //@}

  //@{
  /**
   * Set the default scale factor of the point gaussians.  This
   * defaults to 1.0. All radius computations will be scaled by the factor
   * including the ScaleArray. If a svtkPiecewideFunction is used the
   * scaling happens prior to the function lookup.
   * A scale factor of 0.0 indicates that the splats should be rendered
   * as simple points.
   */
  svtkSetMacro(ScaleFactor, double);
  svtkGetMacro(ScaleFactor, double);
  //@}

  //@{
  /**
   * Treat the points/splats as emissive light sources. The default is true.
   */
  svtkSetMacro(Emissive, svtkTypeBool);
  svtkGetMacro(Emissive, svtkTypeBool);
  svtkBooleanMacro(Emissive, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the optional opacity transfer function. This is only
   * used when an OpacityArray is also specified.
   */
  void SetScalarOpacityFunction(svtkPiecewiseFunction*);
  svtkGetObjectMacro(ScalarOpacityFunction, svtkPiecewiseFunction);
  //@}

  //@{
  /**
   * The size of the table used in computing opacities, used when
   * converting a svtkPiecewiseFunction to a table
   */
  svtkSetMacro(OpacityTableSize, int);
  svtkGetMacro(OpacityTableSize, int);
  //@}

  //@{
  /**
   * Method to set the optional opacity array.  If specified this
   * array will be used to generate the opacity values.
   */
  svtkSetStringMacro(OpacityArray);
  svtkGetStringMacro(OpacityArray);
  //@}

  //@{
  /**
   * Convenience method to set the component of the array to opacify with.
   */
  svtkSetMacro(OpacityArrayComponent, int);
  svtkGetMacro(OpacityArrayComponent, int);
  //@}

  //@{
  /**
   * Method to override the fragment shader code for the splat.  You can
   * set this to draw other shapes. For the OPenGL2 backend some of
   * the variables you can use and/or modify include,
   * opacity - 0.0 to 1.0
   * diffuseColor - vec3
   * ambientColor - vec3
   * offsetVCVSOutput - vec2 offset in view coordinates from the splat center
   */
  svtkSetStringMacro(SplatShaderCode);
  svtkGetStringMacro(SplatShaderCode);
  //@}

  //@{
  /**
   * When drawing triangles as opposed too point mode
   * (triangles are for splats shaders that are bigger than a pixel)
   * this controls how large the triangle will be. By default it
   * is large enough to contain a cicle of radius 3.0*scale which works
   * well for gaussian splats as after 3.0 standard deviations the
   * opacity is near zero. For custom shader codes a different
   * value can be used. Generally you should use the lowest value you can
   * as it will result in fewer fragments. For example if your custom
   * shader only draws a disc of radius 1.0*scale, then set this to 1.0
   * to avoid sending many fragments to the shader that will just get
   * discarded.
   */
  svtkSetMacro(TriangleScale, float);
  svtkGetMacro(TriangleScale, float);
  //@}

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   * Used by svtkHardwareSelector to determine if the prop supports hardware
   * selection.
   */
  bool GetSupportsSelection() override { return true; }

protected:
  svtkPointGaussianMapper();
  ~svtkPointGaussianMapper() override;

  char* ScaleArray;
  int ScaleArrayComponent;
  char* OpacityArray;
  int OpacityArrayComponent;
  char* SplatShaderCode;

  svtkPiecewiseFunction* ScaleFunction;
  int ScaleTableSize;

  svtkPiecewiseFunction* ScalarOpacityFunction;
  int OpacityTableSize;

  double ScaleFactor;
  svtkTypeBool Emissive;

  float TriangleScale;

private:
  svtkPointGaussianMapper(const svtkPointGaussianMapper&) = delete;
  void operator=(const svtkPointGaussianMapper&) = delete;
};

#endif
