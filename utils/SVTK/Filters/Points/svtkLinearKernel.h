/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLinearKernel.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkLinearKernel
 * @brief   a linear interpolation kernel
 *
 *
 * svtkLinearKernel is an interpolation kernel that averages the contributions
 * of all points in the basis.
 *
 * @sa
 * svtkPointInterpolator svtkInterpolationKernel svtkGeneralizedKernel
 * svtkGaussianKernel svtkLinearKernel svtkShepardKernel
 */

#ifndef svtkLinearKernel_h
#define svtkLinearKernel_h

#include "svtkFiltersPointsModule.h" // For export macro
#include "svtkGeneralizedKernel.h"

class svtkIdList;
class svtkDoubleArray;

class SVTKFILTERSPOINTS_EXPORT svtkLinearKernel : public svtkGeneralizedKernel
{
public:
  //@{
  /**
   * Standard methods for instantiation, obtaining type information, and printing.
   */
  static svtkLinearKernel* New();
  svtkTypeMacro(svtkLinearKernel, svtkGeneralizedKernel);
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

protected:
  svtkLinearKernel();
  ~svtkLinearKernel() override;

private:
  svtkLinearKernel(const svtkLinearKernel&) = delete;
  void operator=(const svtkLinearKernel&) = delete;
};

#endif
