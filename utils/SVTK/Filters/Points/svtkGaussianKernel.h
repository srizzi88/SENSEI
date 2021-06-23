/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGaussianKernel.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkGaussianKernel
 * @brief   a spherical Gaussian interpolation kernel
 *
 *
 * svtkGaussianKernel is an interpolation kernel that simply returns the
 * weights for all points found in the sphere defined by radius R. The
 * weights are computed as: exp(-(s*r/R)^2) where r is the distance from the
 * point to be interpolated to a neighboring point within R. The sharpness s
 * simply affects the rate of fall off of the Gaussian. (A more general
 * Gaussian kernel is available from svtkEllipsoidalGaussianKernel.)
 *
 * @warning
 * The weights are normalized sp that SUM(Wi) = 1. If a neighbor point p
 * precisely lies on the point to be interpolated, then the interpolated
 * point takes on the values associated with p.
 *
 * @sa
 * svtkPointInterpolator svtkInterpolationKernel svtkEllipsoidalGaussianKernel
 * svtkVoronoiKernel svtkSPHKernel svtkShepardKernel
 */

#ifndef svtkGaussianKernel_h
#define svtkGaussianKernel_h

#include "svtkFiltersPointsModule.h" // For export macro
#include "svtkGeneralizedKernel.h"

class svtkIdList;
class svtkDoubleArray;

class SVTKFILTERSPOINTS_EXPORT svtkGaussianKernel : public svtkGeneralizedKernel
{
public:
  //@{
  /**
   * Standard methods for instantiation, obtaining type information, and printing.
   */
  static svtkGaussianKernel* New();
  svtkTypeMacro(svtkGaussianKernel, svtkGeneralizedKernel);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Initialize the kernel. Overload the superclass to set up internal
   * computational values.
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
   * Set / Get the sharpness (i.e., falloff) of the Gaussian. By default
   * Sharpness=2. As the sharpness increases the effects of distant points
   * are reduced.
   */
  svtkSetClampMacro(Sharpness, double, 1, SVTK_FLOAT_MAX);
  svtkGetMacro(Sharpness, double);
  //@}

protected:
  svtkGaussianKernel();
  ~svtkGaussianKernel() override;

  double Sharpness;

  // Internal structure to reduce computation
  double F2;

private:
  svtkGaussianKernel(const svtkGaussianKernel&) = delete;
  void operator=(const svtkGaussianKernel&) = delete;
};

#endif
