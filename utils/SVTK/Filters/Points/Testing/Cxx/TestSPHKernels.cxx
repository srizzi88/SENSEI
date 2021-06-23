/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSPHKernels.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Perform unit tests on SPH kernels. This means:
// + integrating the kernels in 2D and 3D to ensure that the "volume"
//   contained in the kernel sums to 1.0 (within epsilon)
// + ensuring that the kernel function is symmetric
// + ensuring that the kernel derivative takes on the correct sign
//   and value on either side of the central point.

#include "svtkSPHCubicKernel.h"
#include "svtkSPHQuarticKernel.h"
#include "svtkSPHQuinticKernel.h"
#include "svtkSmartPointer.h"
#include "svtkWendlandQuinticKernel.h"

#include "svtkMath.h"
#include "svtkMathUtilities.h"

#include <cmath>
#include <sstream>
#include <string>

//-----------------------------------------------------------------------------
// Helper function
template <class T>
int TestSPHKernel(svtkSmartPointer<T> kernel, const std::string& description)
{
  int status = EXIT_SUCCESS;
  double res = 100;
  double smoothingLen = 1.0;
  double area, volume;
  int i, j, k;
  double x, y, z, r;
  double cutoff, inc, normFactor;
  double integral;

  // Test 2D
  kernel->SetDimension(2);
  kernel->SetSpatialStep(smoothingLen);
  kernel->Initialize(nullptr, nullptr, nullptr);
  normFactor = kernel->GetNormFactor();
  cutoff = kernel->GetCutoffFactor();
  inc = 2.0 * cutoff / static_cast<double>(res);
  area = inc * inc;

  integral = 0.0;
  for (j = 0; j < res; ++j)
  {
    y = -cutoff + j * inc;
    for (i = 0; i < res; ++i)
    {
      x = -cutoff + i * inc;
      r = sqrt(x * x + y * y);
      integral += area * normFactor * kernel->ComputeFunctionWeight(r);
    }
  }
  std::cout << "SPH " << description << " Kernel Integral (2D): " << integral << std::endl;
  if (integral < 0.99 || integral > 1.01)
  {
    status = EXIT_FAILURE;
  }

  // Test 3D
  kernel->SetDimension(3);
  kernel->SetSpatialStep(smoothingLen);
  kernel->Initialize(nullptr, nullptr, nullptr);
  normFactor = kernel->GetNormFactor();
  cutoff = kernel->GetCutoffFactor();
  inc = 2.0 * cutoff / static_cast<double>(res);
  volume = inc * inc * inc;

  integral = 0.0;
  for (k = 0; k < res; ++k)
  {
    z = -cutoff + k * inc;
    for (j = 0; j < res; ++j)
    {
      y = -cutoff + j * inc;
      for (i = 0; i < res; ++i)
      {
        x = -cutoff + i * inc;
        r = sqrt(x * x + y * y + z * z);
        integral += volume * normFactor * kernel->ComputeFunctionWeight(r);
      }
    }
  }
  std::cout << "SPH " << description << " Kernel Integral (3D): " << integral << std::endl;
  if (integral < 0.99 || integral > 1.01)
  {
    status = EXIT_FAILURE;
  }

  return status;
}

//-----------------------------------------------------------------------------
int TestSPHKernels(int, char*[])
{
  int status = EXIT_SUCCESS;

  // We will integrate over the kernel starting at a point beyond the cutoff
  // distance (since these should make zero contribution to the
  // integral). Note the integration occurs over a 2D or 3D domain

  // Cubic SPH Kernel
  svtkSmartPointer<svtkSPHCubicKernel> cubic = svtkSmartPointer<svtkSPHCubicKernel>::New();
  status += TestSPHKernel<svtkSPHCubicKernel>(cubic, "Cubic");

  // Quartic Kernel
  svtkSmartPointer<svtkSPHQuarticKernel> quartic = svtkSmartPointer<svtkSPHQuarticKernel>::New();
  status += TestSPHKernel<svtkSPHQuarticKernel>(quartic, "Quartic");

  // Quintic Kernel
  svtkSmartPointer<svtkSPHQuinticKernel> quintic = svtkSmartPointer<svtkSPHQuinticKernel>::New();
  status += TestSPHKernel<svtkSPHQuinticKernel>(quintic, "Quintic");

  // Wendland C2 (quintic) Kernel
  svtkSmartPointer<svtkWendlandQuinticKernel> wendland =
    svtkSmartPointer<svtkWendlandQuinticKernel>::New();
  status += TestSPHKernel<svtkWendlandQuinticKernel>(wendland, "Wendland Quintic");

  // Get out
  if (status == EXIT_SUCCESS)
  {
    std::cout << " PASSED" << std::endl;
  }
  else
  {
    std::cout << " FAILED" << std::endl;
  }

  return status;
}
