//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

// This tests a previous problem where code templated on the device adapter and
// used one of the device adapter algorithms (for example, the dispatcher) had
// to be declared after any device adapter it was ever used with.
#include <svtkm/cont/DeviceAdapter.h>

#include <svtkm/cont/testing/Testing.h>

#include <svtkm/cont/ArrayHandle.h>

// Important for this test!
//This file must be included after ArrayHandle.h
#include <svtkm/cont/serial/DeviceAdapterSerial.h>

namespace
{

struct ExampleWorklet
{
  template <typename T>
  void operator()(T svtkmNotUsed(v)) const
  {
  }
};

void CheckPostDefinedDeviceAdapter()
{
  // Nothing to really check. If this compiles, then the test is probably
  // successful.
  svtkm::cont::ArrayHandle<svtkm::Id> test;
}

} // anonymous namespace

int UnitTestDeviceAdapterAlgorithmDependency(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(CheckPostDefinedDeviceAdapter, argc, argv);
}
