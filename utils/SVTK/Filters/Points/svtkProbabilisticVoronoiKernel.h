/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProbabilisticVoronoiKernel.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkProbabilisticVoronoiKernel
 * @brief   interpolate from the weighted closest point
 *
 *
 * svtkProbabilisticVoronoiKernel is an interpolation kernel that interpolates
 * from the closest weighted point from a neighborhood of points. The weights
 * refer to the probabilistic weighting that can be provided to the
 * ComputeWeights() method.
 *
 * Note that the local neighborhood is taken from the kernel footprint
 * specified in the superclass svtkGeneralizedKernel.
 *
 * @warning
 * If probability weightings are not defined, then the kernel provides the
 * same results as svtkVoronoiKernel, except a less efficiently.
 *
 * @sa
 * svtkInterpolationKernel svtkGeneralizedKernel svtkVoronoiKernel
 */

#ifndef svtkProbabilisticVoronoiKernel_h
#define svtkProbabilisticVoronoiKernel_h

#include "svtkFiltersPointsModule.h" // For export macro
#include "svtkGeneralizedKernel.h"

class svtkIdList;
class svtkDoubleArray;

class SVTKFILTERSPOINTS_EXPORT svtkProbabilisticVoronoiKernel : public svtkGeneralizedKernel
{
public:
  //@{
  /**
   * Standard methods for instantiation, obtaining type information, and printing.
   */
  static svtkProbabilisticVoronoiKernel* New();
  svtkTypeMacro(svtkProbabilisticVoronoiKernel, svtkGeneralizedKernel);
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
  svtkProbabilisticVoronoiKernel();
  ~svtkProbabilisticVoronoiKernel() override;

private:
  svtkProbabilisticVoronoiKernel(const svtkProbabilisticVoronoiKernel&) = delete;
  void operator=(const svtkProbabilisticVoronoiKernel&) = delete;
};

#endif
