//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/testing/Testing.h>
#include <svtkm/testing/TestingMath.h>

#include <svtkm/cont/serial/DeviceAdapterSerial.h>

int UnitTestMath(int argc, char* argv[])
{
  return svtkm::testing::Testing::Run(
    UnitTestMathNamespace::RunMathTests<svtkm::cont::DeviceAdapterTagSerial>, argc, argv);
}
