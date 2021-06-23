//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/serial/DeviceAdapterSerial.h>

#include <svtkm/cont/testing/TestingColorTable.h>

int UnitTestSerialColorTable(int argc, char* argv[])
{
  //TestingColorTable forces the device
  return svtkm::cont::testing::TestingColorTable<svtkm::cont::DeviceAdapterTagSerial>::Run(argc,
                                                                                         argv);
}
