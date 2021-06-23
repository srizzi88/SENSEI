/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRectangularButtonSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkRectangularButtonSource
 * @brief   create a rectangular button
 *
 * svtkRectangularButtonSource creates a rectangular shaped button with
 * texture coordinates suitable for application of a texture map. This
 * provides a way to make nice looking 3D buttons. The buttons are
 * represented as svtkPolyData that includes texture coordinates and
 * normals. The button lies in the x-y plane.
 *
 * To use this class you must define its width, height and length. These
 * measurements are all taken with respect to the shoulder of the button.
 * The shoulder is defined as follows. Imagine a box sitting on the floor.
 * The distance from the floor to the top of the box is the depth; the other
 * directions are the length (x-direction) and height (y-direction). In
 * this particular widget the box can have a smaller bottom than top. The
 * ratio in size between bottom and top is called the box ratio (by
 * default=1.0). The ratio of the texture region to the shoulder region
 * is the texture ratio. And finally the texture region may be out of plane
 * compared to the shoulder. The texture height ratio controls this.
 *
 * @sa
 * svtkButtonSource svtkEllipticalButtonSource
 *
 * @warning
 * The button is defined in the x-y plane. Use svtkTransformPolyDataFilter
 * or svtkGlyph3D to orient the button in a different direction.
 */

#ifndef svtkRectangularButtonSource_h
#define svtkRectangularButtonSource_h

#include "svtkButtonSource.h"
#include "svtkFiltersSourcesModule.h" // For export macro

class svtkCellArray;
class svtkFloatArray;
class svtkPoints;

class SVTKFILTERSSOURCES_EXPORT svtkRectangularButtonSource : public svtkButtonSource
{
public:
  void PrintSelf(ostream& os, svtkIndent indent) override;
  svtkTypeMacro(svtkRectangularButtonSource, svtkButtonSource);

  /**
   * Construct a circular button with depth 10% of its height.
   */
  static svtkRectangularButtonSource* New();

  //@{
  /**
   * Set/Get the width of the button.
   */
  svtkSetClampMacro(Width, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(Width, double);
  //@}

  //@{
  /**
   * Set/Get the height of the button.
   */
  svtkSetClampMacro(Height, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(Height, double);
  //@}

  //@{
  /**
   * Set/Get the depth of the button (the z-eliipsoid axis length).
   */
  svtkSetClampMacro(Depth, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(Depth, double);
  //@}

  //@{
  /**
   * Set/Get the ratio of the bottom of the button with the
   * shoulder region. Numbers greater than one produce buttons
   * with a wider bottom than shoulder; ratios less than one
   * produce buttons that have a wider shoulder than bottom.
   */
  svtkSetClampMacro(BoxRatio, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(BoxRatio, double);
  //@}

  //@{
  /**
   * Set/Get the ratio of the texture region to the
   * shoulder region. This number must be 0<=tr<=1.
   * If the texture style is to fit the image, then satisfying
   * the texture ratio may only be possible in one of the
   * two directions (length or width) depending on the
   * dimensions of the texture.
   */
  svtkSetClampMacro(TextureRatio, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(TextureRatio, double);
  //@}

  //@{
  /**
   * Set/Get the ratio of the height of the texture region
   * to the shoulder height. Values greater than 1.0 yield
   * convex buttons with the texture region raised above the
   * shoulder. Values less than 1.0 yield concave buttons with
   * the texture region below the shoulder.
   */
  svtkSetClampMacro(TextureHeightRatio, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(TextureHeightRatio, double);
  //@}

  //@{
  /**
   * Set/get the desired precision for the output points.
   * svtkAlgorithm::SINGLE_PRECISION - Output single-precision floating point.
   * svtkAlgorithm::DOUBLE_PRECISION - Output double-precision floating point.
   */
  svtkSetMacro(OutputPointsPrecision, int);
  svtkGetMacro(OutputPointsPrecision, int);
  //@}

protected:
  svtkRectangularButtonSource();
  ~svtkRectangularButtonSource() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  double Width;
  double Height;
  double Depth;

  double BoxRatio;
  double TextureRatio;
  double TextureHeightRatio;

  int OutputPointsPrecision;

private:
  svtkRectangularButtonSource(const svtkRectangularButtonSource&) = delete;
  void operator=(const svtkRectangularButtonSource&) = delete;
};

#endif
