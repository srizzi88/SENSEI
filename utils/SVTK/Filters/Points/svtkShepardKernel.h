/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkShepardKernel.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkShepardKernel
 * @brief   a Shepard method interpolation kernel
 *
 *
 * svtkShepardKernel is an interpolation kernel that uses the method of
 * Shepard to perform interpolation. The weights are computed as 1/r^p, where
 * r is the distance to a neighbor point within the kernel radius R; and p
 * (the power parameter) is a positive exponent (typically p=2).
 *
 * @warning
 * The weights are normalized sp that SUM(Wi) = 1. If a neighbor point p
 * precisely lies on the point to be interpolated, then the interpolated
 * point takes on the values associated with p.
 *
 * @sa
 * svtkPointInterpolator svtkPointInterpolator2D svtkInterpolationKernel
 * svtkGaussianKernel svtkSPHKernel svtkShepardKernel
 */

#ifndef svtkShepardKernel_h
#define svtkShepardKernel_h

#include "svtkFiltersPointsModule.h" // For export macro
#include "svtkGeneralizedKernel.h"

class svtkIdList;
class svtkDoubleArray;

class SVTKFILTERSPOINTS_EXPORT svtkShepardKernel : public svtkGeneralizedKernel
{
public:
  //@{
  /**
   * Standard methods for instantiation, obtaining type information, and printing.
   */
  static svtkShepardKernel* New();
  svtkTypeMacro(svtkShepardKernel, svtkGeneralizedKernel);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

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
   * Set / Get the power parameter p. By default p=2. Values (which must be
   * a positive, real value) != 2 may affect performance significantly.
   */
  svtkSetClampMacro(PowerParameter, double, 0.001, 100);
  svtkGetMacro(PowerParameter, double);
  //@}

protected:
  svtkShepardKernel();
  ~svtkShepardKernel() override;

  // The exponent of the weights, =2 by default (l2 norm)
  double PowerParameter;

private:
  svtkShepardKernel(const svtkShepardKernel&) = delete;
  void operator=(const svtkShepardKernel&) = delete;
};

#endif
