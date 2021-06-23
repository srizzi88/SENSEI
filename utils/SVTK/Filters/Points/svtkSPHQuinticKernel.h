/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSPHQuinticKernel.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSPHQuinticKernel
 * @brief   a quintic SPH interpolation kernel
 *
 *
 * svtkSPHQuinticKernel is an smooth particle hydrodynamics interpolation kernel as
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

#ifndef svtkSPHQuinticKernel_h
#define svtkSPHQuinticKernel_h

#include "svtkFiltersPointsModule.h" // For export macro
#include "svtkSPHKernel.h"
#include <algorithm> // For std::min()

class svtkIdList;
class svtkDoubleArray;

class SVTKFILTERSPOINTS_EXPORT svtkSPHQuinticKernel : public svtkSPHKernel
{
public:
  //@{
  /**
   * Standard methods for instantiation, obtaining type information, and printing.
   */
  static svtkSPHQuinticKernel* New();
  svtkTypeMacro(svtkSPHQuinticKernel, svtkSPHKernel);
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
   */
  double ComputeFunctionWeight(const double d) override
  {
    double tmp1 = 3.0 - std::min(d, 3.0);
    double tmp2 = 2.0 - std::min(d, 2.0);
    double tmp3 = 1.0 - std::min(d, 1.0);
    return (tmp1 * tmp1 * tmp1 * tmp1 * tmp1 - 6.0 * tmp2 * tmp2 * tmp2 * tmp2 * tmp2 +
      15.0 * tmp3 * tmp3 * tmp3 * tmp3 * tmp3);
  }
  //@}

  //@{
  /**
   * Compute weighting factor for derivative quantities given a normalized
   * distance from a sample point.
   */
  double ComputeDerivWeight(const double d) override
  {
    double tmp1 = 3.0 - std::min(d, 3.0);
    double tmp2 = 2.0 - std::min(d, 2.0);
    double tmp3 = 1.0 - std::min(d, 1.0);
    return (-5.0 * tmp1 * tmp1 * tmp1 * tmp1 + 30.0 * tmp2 * tmp2 * tmp2 * tmp2 +
      -75.0 * tmp3 * tmp3 * tmp3 * tmp3);
  }
  //@}

protected:
  svtkSPHQuinticKernel();
  ~svtkSPHQuinticKernel() override;

private:
  svtkSPHQuinticKernel(const svtkSPHQuinticKernel&) = delete;
  void operator=(const svtkSPHQuinticKernel&) = delete;
};

#endif
