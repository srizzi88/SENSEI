/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkEllipsoidalGaussianKernel.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkEllipsoidalGaussianKernel
 * @brief   an ellipsoidal Gaussian interpolation kernel
 *
 *
 * svtkEllipsoidalGaussianKernel is an interpolation kernel that returns the
 * weights for all points found in the ellipsoid defined by radius R in
 * combination with local data (normals and/or scalars). For example, "pancake"
 * weightings (the local normal parallel to the minimum ellisoidal axis); or
 * "needle" weightings (the local normal parallel to the maximum ellipsoidal
 * axis) are possible. (Note that spherical Gaussian weightings are more
 * efficiently computed using svtkGaussianKernel.)
 *
 * The ellipsoidal Gaussian can be described by:
 *
 *     W(x) = S * exp( -( Sharpness^2 * ((rxy/E)**2 + z**2)/R**2) )
 *
 * where S is the local scalar value; E is a user-defined eccentricity factor
 * that controls the elliptical shape of the splat; z is the distance of the
 * current voxel sample point along the local normal N; and rxy is the
 * distance to neighbor point x in the direction prependicular to N.
 *
 * @warning
 * The weights are normalized so that SUM(Wi) = 1. If a neighbor point p
 * precisely lies on the point to be interpolated, then the interpolated
 * point takes on the values associated with p.
 *
 * @sa
 * svtkPointInterpolator svtkInterpolationKernel svtkGeneralizedKernel
 * svtkGaussianKernel svtkVoronoiKernel svtkSPHKernel svtkShepardKernel
 */

#ifndef svtkEllipsoidalGaussianKernel_h
#define svtkEllipsoidalGaussianKernel_h

#include "svtkFiltersPointsModule.h" // For export macro
#include "svtkGeneralizedKernel.h"
#include "svtkStdString.h" // For svtkStdString ivars

class svtkIdList;
class svtkDataArray;
class svtkDoubleArray;

class SVTKFILTERSPOINTS_EXPORT svtkEllipsoidalGaussianKernel : public svtkGeneralizedKernel
{
public:
  //@{
  /**
   * Standard methods for instantiation, obtaining type information, and printing.
   */
  static svtkEllipsoidalGaussianKernel* New();
  svtkTypeMacro(svtkEllipsoidalGaussianKernel, svtkGeneralizedKernel);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Initialize the kernel. Overload the superclass to set up scalars and
   * vectors.
   */
  void Initialize(svtkAbstractPointLocator* loc, svtkDataSet* ds, svtkPointData* pd) override;

  // Re-use any superclass signatures that we don't override.
  using svtkGeneralizedKernel::ComputeWeights;

  /**
   * Given a point x, a list of basis points pIds, and a probability
   * weighting function prob, compute interpolation weights associated with
   * these basis points.  Note that basis points list pIds, the probability
   * weighting prob, and the weights array are provided by the caller of the
   * method, and may be dynamically resized as necessary. The method returns
   * the number of weights (pIds may be resized in some cases). Typically
   * this method is called after ComputeBasis(), although advanced users can
   * invoke ComputeWeights() and provide the interpolation basis points pIds
   * directly. The probably weighting prob are numbers 0<=prob<=1 which are
   * multiplied against the interpolation weights before normalization. They
   * are estimates of local confidence of weights. The prob may be nullptr in
   * which all probabilities are considered =1.
   */
  svtkIdType ComputeWeights(
    double x[3], svtkIdList* pIds, svtkDoubleArray* prob, svtkDoubleArray* weights) override;

  //@{
  /**
   * Specify whether vector values should be used to affect the shape
   * of the Gaussian distribution. By default this is on.
   */
  svtkSetMacro(UseNormals, bool);
  svtkGetMacro(UseNormals, bool);
  svtkBooleanMacro(UseNormals, bool);
  //@}

  //@{
  /**
   * Specify the normals array name. Used to orient the ellipsoid. Note that
   * by default the input normals are used (i.e. the input to
   * svtkPointInterpolator). If no input normals are available, then the named
   * NormalsArrayName is used.
   */
  svtkSetMacro(NormalsArrayName, svtkStdString);
  svtkGetMacro(NormalsArrayName, svtkStdString);
  //@}

  //@{
  /**
   * Specify whether scalar values should be used to scale the weights.
   * By default this is off.
   */
  svtkSetMacro(UseScalars, bool);
  svtkGetMacro(UseScalars, bool);
  svtkBooleanMacro(UseScalars, bool);
  //@}

  //@{
  /**
   * Specify the scalars array name. Used to scale the ellipsoid. Note that
   * by default the input scalars are used (i.e. the input to
   * svtkPointInterpolator). If no input scalars are available, then the named
   * ScalarsArrayName is used.
   */
  svtkSetMacro(ScalarsArrayName, svtkStdString);
  svtkGetMacro(ScalarsArrayName, svtkStdString);
  //@}

  //@{
  /**
   * Multiply the Gaussian splat distribution by this value. If UseScalars is
   * on and a scalar array is provided, then the scalar value will be
   * multiplied by the ScaleFactor times the Gaussian function.
   */
  svtkSetClampMacro(ScaleFactor, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(ScaleFactor, double);
  //@}

  //@{
  /**
   * Set / Get the sharpness (i.e., falloff) of the Gaussian. By default
   * Sharpness=2. As the sharpness increases the effects of distant points
   * are reduced.
   */
  svtkSetClampMacro(Sharpness, double, 1, SVTK_FLOAT_MAX);
  svtkGetMacro(Sharpness, double);
  //@}

  //@{
  /**
   * Set / Get the eccentricity of the ellipsoidal Gaussian. A value=1.0
   * produces a spherical distribution. Values < 1 produce a needle like
   * distribution (in the direction of the normal); values > 1 produce a
   * pancake like distribution (orthogonal to the normal).
   */
  svtkSetClampMacro(Eccentricity, double, 0.000001, SVTK_FLOAT_MAX);
  svtkGetMacro(Eccentricity, double);
  //@}

protected:
  svtkEllipsoidalGaussianKernel();
  ~svtkEllipsoidalGaussianKernel() override;

  bool UseNormals;
  bool UseScalars;

  svtkStdString NormalsArrayName;
  svtkStdString ScalarsArrayName;

  double ScaleFactor;
  double Sharpness;
  double Eccentricity;

  // Internal structure to reduce computation
  double F2, E2;
  svtkDataArray* NormalsArray;
  svtkDataArray* ScalarsArray;

  void FreeStructures() override;

private:
  svtkEllipsoidalGaussianKernel(const svtkEllipsoidalGaussianKernel&) = delete;
  void operator=(const svtkEllipsoidalGaussianKernel&) = delete;
};

#endif
