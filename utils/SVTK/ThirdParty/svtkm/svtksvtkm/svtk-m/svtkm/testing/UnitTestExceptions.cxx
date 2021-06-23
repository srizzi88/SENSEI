//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/Error.h>
#include <svtkm/cont/ErrorBadValue.h>
#include <svtkm/cont/Initialize.h>
#include <svtkm/cont/RuntimeDeviceInformation.h>
#include <svtkm/cont/RuntimeDeviceTracker.h>

//------------------------------------------------------------------------------
// This test ensures that exceptions thrown internally by the svtkm_cont library
// can be correctly caught across library boundaries.
int UnitTestExceptions(int argc, char* argv[])
{
  svtkm::cont::Initialize(argc, argv);
  auto& tracker = svtkm::cont::GetRuntimeDeviceTracker();

  try
  {
    // This throws a ErrorBadValue from RuntimeDeviceTracker::CheckDevice,
    // which is compiled into the svtkm_cont library:
    tracker.ResetDevice(svtkm::cont::DeviceAdapterTagUndefined());
  }
  catch (svtkm::cont::ErrorBadValue&)
  {
    return EXIT_SUCCESS;
  }

  std::cerr << "Did not catch expected ErrorBadValue exception.\n";
  return EXIT_FAILURE;
}
