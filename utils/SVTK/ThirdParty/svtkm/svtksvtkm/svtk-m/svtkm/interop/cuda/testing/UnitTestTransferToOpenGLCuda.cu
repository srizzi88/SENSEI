//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

//This sets up testing with the cuda device adapter
#include <svtkm/cont/cuda/DeviceAdapterCuda.h>
#include <svtkm/cont/cuda/internal/testing/Testing.h>

#include <svtkm/interop/testing/TestingOpenGLInterop.h>

int UnitTestTransferToOpenGLCuda(int argc, char* argv[])
{
  svtkm::cont::Initialize(argc, argv);
  int result = 1;
  result =
    svtkm::interop::testing::TestingOpenGLInterop<svtkm::cont::cuda::DeviceAdapterTagCuda>::Run();
  return svtkm::cont::cuda::internal::Testing::CheckCudaBeforeExit(result);
}
