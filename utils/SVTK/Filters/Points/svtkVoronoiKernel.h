/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVoronoiKernel.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkVoronoiKernel
 * @brief   a Voronoi interpolation kernel
 *
 *
 * svtkVoronoiKernel is an interpolation kernel that simply returns the
 * closest point to a point to be interpolated. A single weight is returned
 * with value=1.0.
 *
 * @warning
 * In degenerate cases (where a point x is equidistance from more than one
 * point) the kernel basis arbitrarily chooses one of the equidistant points.
 *
 * @sa
 * svtkInterpolationKernel svtkGeneralizedKernel svtkProbabilisticVoronoiKernel
 */

#ifndef svtkVoronoiKernel_h
#define svtkVoronoiKernel_h

#include "svtkFiltersPointsModule.h" // For export macro
#include "svtkInterpolationKernel.h"

class svtkIdList;
class svtkDoubleArray;

class SVTKFILTERSPOINTS_EXPORT svtkVoronoiKernel : public svtkInterpolationKernel
{
public:
  //@{
  /**
   * Standard methods for instantiation, obtaining type information, and printing.
   */
  static svtkVoronoiKernel* New();
  svtkTypeMacro(svtkVoronoiKernel, svtkInterpolationKernel);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Given a point x (and optional associated ptId), determine the points
   * around x which form an interpolation basis. The user must provide the
   * svtkIdList pIds, which will be dynamically resized as necessary. The
   * method returns the number of points in the basis. Typically this method
   * is called before ComputeWeights().
   */
  svtkIdType ComputeBasis(double x[3], svtkIdList* pIds, svtkIdType ptId = 0) override;

  /**
   * Given a point x, and a list of basis points pIds, compute interpolation
   * weights associated with these basis points.  Note that both the nearby
   * basis points list pIds and the weights array are provided by the caller
   * of the method, and may be dynamically resized as necessary. Typically
   * this method is called after ComputeBasis(), although advanced users can
   * invoke ComputeWeights() and provide the interpolation basis points pIds
   * directly.
   */
  svtkIdType ComputeWeights(double x[3], svtkIdList* pIds, svtkDoubleArray* weights) override;

protected:
  svtkVoronoiKernel();
  ~svtkVoronoiKernel() override;

private:
  svtkVoronoiKernel(const svtkVoronoiKernel&) = delete;
  void operator=(const svtkVoronoiKernel&) = delete;
};

#endif
