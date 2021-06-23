/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWendlandQuinticKernel.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkWendlandQuinticKernel
 * @brief   a quintic SPH interpolation kernel
 *
 *
 * svtkWendlandQuinticKernel is an smooth particle hydrodynamics interpolation kernel as
 * described by D.J. Price. This is a quintic formulation.
 *
 * @warning
 * FOr more information see D.J. Price, Smoothed particle hydrodynamics and
 * magnetohydrodynamics, J. Comput. Phys. 231:759-794, 2012. Especially
 * equation 49.
 *
 * @par Acknowledgments:
 * The following work has been generously supported by Altair Engineering
 * and FluiDyna GmbH. Please contact Steve Cosgrove or Milos Stanic for
 * more information.
 *
 * @sa
 * svtkSPHKernel svtkSPHInterpolator
 */

#ifndef svtkWendlandQuinticKernel_h
#define svtkWendlandQuinticKernel_h

#include "svtkFiltersPointsModule.h" // For export macro
#include "svtkSPHKernel.h"
#include <algorithm> // For std::min()

class svtkIdList;
class svtkDoubleArray;

class SVTKFILTERSPOINTS_EXPORT svtkWendlandQuinticKernel : public svtkSPHKernel
{
public:
  //@{
  /**
   * Standard methods for instantiation, obtaining type information, and printing.
   */
  static svtkWendlandQuinticKernel* New();
  svtkTypeMacro(svtkWendlandQuinticKernel, svtkSPHKernel);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Produce the computational parameters for the kernel. Invoke this method
   * after setting initial values like SpatialStep.
   */
  void Initialize(svtkAbstractPointLocator* loc, svtkDataSet* ds, svtkPointData* pd) override;

  //@{
  /**
   * Compute weighting factor given a normalized distance from a sample point.
   * Note that the formulation is slightly different to avoid an extra operation
   * (which has the effect of affecting the NormFactor by 1/16).
   */
  double ComputeFunctionWeight(const double d) override
  {
    if (d >= 2.0)
    {
      return 0.0;
    }
    else
    {
      double tmp = 1.0 - 0.5 * d;
      return (tmp * tmp * tmp * tmp) * (1.0 + 2.0 * d);
    }
  }
  //@}

  //@{
  /**
   * Compute weighting factor for derivative quantities given a normalized
   * distance from a sample point.
   */
  double ComputeDerivWeight(const double d) override
  {
    if (d >= 2.0)
    {
      return 0.0;
    }
    else
    {
      double tmp = 1.0 - 0.5 * d;
      return -2.0 * (tmp * tmp * tmp) * (1.0 + 2.0 * d) + 2.0 * (tmp * tmp * tmp * tmp);
    }
  }
  //@}

protected:
  svtkWendlandQuinticKernel();
  ~svtkWendlandQuinticKernel() override;

private:
  svtkWendlandQuinticKernel(const svtkWendlandQuinticKernel&) = delete;
  void operator=(const svtkWendlandQuinticKernel&) = delete;
};

#endif
