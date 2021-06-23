//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_cuda_internal_Testing_h
#define svtk_m_cont_cuda_internal_Testing_h

#include <svtkm/cont/testing/Testing.h>

#include <cuda.h>

namespace svtkm
{
namespace cont
{
namespace cuda
{
namespace internal
{

struct Testing
{
public:
  static SVTKM_CONT int CheckCudaBeforeExit(int result)
  {
    cudaError_t cudaError = cudaPeekAtLastError();
    if (cudaError != cudaSuccess)
    {
      std::cout << "***** Unchecked Cuda error." << std::endl
                << cudaGetErrorString(cudaError) << std::endl;
      return 1;
    }
    else
    {
      std::cout << "No Cuda error detected." << std::endl;
    }
    return result;
  }

  template <class Func>
  static SVTKM_CONT int Run(Func function)
  {
    int result = svtkm::cont::testing::Testing::Run(function);
    return CheckCudaBeforeExit(result);
  }
};
}
}
}
} // namespace svtkm::cont::cuda::internal

#endif //svtk_m_cont_cuda_internal_Testing_h
