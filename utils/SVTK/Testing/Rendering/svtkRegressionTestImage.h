/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRegressionTestImage.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef svtkRegressionTestImage_h
#define svtkRegressionTestImage_h
#ifndef __SVTK_WRAP__

// Includes and a macro necessary for saving the image produced by a cxx
// example program. This capability is critical for regression testing.
// This function returns 1 if test passed, 0 if test failed.

#include "svtkTesting.h"

class svtkRegressionTester : public svtkTesting
{
protected:
  svtkRegressionTester() {}
  ~svtkRegressionTester() override {}

private:
  svtkRegressionTester(const svtkRegressionTester&) = delete;
  void operator=(const svtkRegressionTester&) = delete;
};

// 0.15 threshold is arbitrary but found to
// allow most graphics system variances to pass
// when they should and fail when they should
#define svtkRegressionTestImage(rw) svtkTesting::Test(argc, argv, rw, 0.15)

#define svtkRegressionTestImageThreshold(rw, t) svtkTesting::Test(argc, argv, rw, t)

#endif
#endif // svtkRegressionTestImage_h
// SVTK-HeaderTest-Exclude: svtkRegressionTestImage.h
