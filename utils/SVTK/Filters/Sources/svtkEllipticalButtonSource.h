/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkEllipticalButtonSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkEllipticalButtonSource
 * @brief   create a ellipsoidal-shaped button
 *
 * svtkEllipticalButtonSource creates a ellipsoidal shaped button with
 * texture coordinates suitable for application of a texture map. This
 * provides a way to make nice looking 3D buttons. The buttons are
 * represented as svtkPolyData that includes texture coordinates and
 * normals. The button lies in the x-y plane.
 *
 * To use this class you must define the major and minor axes lengths of an
 * ellipsoid (expressed as width (x), height (y) and depth (z)). The button
 * has a rectangular mesh region in the center with texture coordinates that
 * range smoothly from (0,1). (This flat region is called the texture
 * region.) The outer, curved portion of the button (called the shoulder) has
 * texture coordinates set to a user specified value (by default (0,0).
 * (This results in coloring the button curve the same color as the (s,t)
 * location of the texture map.) The resolution in the radial direction, the
 * texture region, and the shoulder region must also be set. The button can
 * be moved by specifying an origin.
 *
 * @sa
 * svtkButtonSource svtkRectangularButtonSource
 */

#ifndef svtkEllipticalButtonSource_h
#define svtkEllipticalButtonSource_h

#include "svtkButtonSource.h"
#include "svtkFiltersSourcesModule.h" // For export macro

class svtkCellArray;
class svtkFloatArray;
class svtkPoints;

class SVTKFILTERSSOURCES_EXPORT svtkEllipticalButtonSource : public svtkButtonSource
{
public:
  void PrintSelf(ostream& os, svtkIndent indent) override;
  svtkTypeMacro(svtkEllipticalButtonSource, svtkButtonSource);

  /**
   * Construct a circular button with depth 10% of its height.
   */
  static svtkEllipticalButtonSource* New();

  //@{
  /**
   * Set/Get the width of the button (the x-ellipsoid axis length * 2).
   */
  svtkSetClampMacro(Width, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(Width, double);
  //@}

  //@{
  /**
   * Set/Get the height of the button (the y-ellipsoid axis length * 2).
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
   * Specify the resolution of the button in the circumferential direction.
   */
  svtkSetClampMacro(CircumferentialResolution, int, 4, SVTK_INT_MAX);
  svtkGetMacro(CircumferentialResolution, int);
  //@}

  //@{
  /**
   * Specify the resolution of the texture in the radial direction in the
   * texture region.
   */
  svtkSetClampMacro(TextureResolution, int, 1, SVTK_INT_MAX);
  svtkGetMacro(TextureResolution, int);
  //@}

  //@{
  /**
   * Specify the resolution of the texture in the radial direction in the
   * shoulder region.
   */
  svtkSetClampMacro(ShoulderResolution, int, 1, SVTK_INT_MAX);
  svtkGetMacro(ShoulderResolution, int);
  //@}

  //@{
  /**
   * Set/Get the radial ratio. This is the measure of the radius of the
   * outer ellipsoid to the inner ellipsoid of the button. The outer
   * ellipsoid is the boundary of the button defined by the height and
   * width. The inner ellipsoid circumscribes the texture region. Larger
   * RadialRatio's cause the button to be more rounded (and the texture
   * region to be smaller); smaller ratios produce sharply curved shoulders
   * with a larger texture region.
   */
  svtkSetClampMacro(RadialRatio, double, 1.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(RadialRatio, double);
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
  svtkEllipticalButtonSource();
  ~svtkEllipticalButtonSource() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  double Width;
  double Height;
  double Depth;
  int CircumferentialResolution;
  int TextureResolution;
  int ShoulderResolution;
  int OutputPointsPrecision;
  double RadialRatio;

private:
  // internal variable related to axes of ellipsoid
  double A;
  double A2;
  double B;
  double B2;
  double C;
  double C2;

  double ComputeDepth(int inTextureRegion, double x, double y, double n[3]);
  void InterpolateCurve(int inTextureRegion, svtkPoints* newPts, int numPts, svtkFloatArray* normals,
    svtkFloatArray* tcoords, int res, int c1StartPoint, int c1Incr, int c2StartPoint, int s2Incr,
    int startPoint, int incr);
  void CreatePolygons(svtkCellArray* newPolys, int num, int res, int startIdx);
  void IntersectEllipseWithLine(double a2, double b2, double dX, double dY, double& xe, double& ye);

  svtkEllipticalButtonSource(const svtkEllipticalButtonSource&) = delete;
  void operator=(const svtkEllipticalButtonSource&) = delete;
};

#endif
