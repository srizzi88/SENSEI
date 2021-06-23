//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_cuda_ErrorCuda_h
#define svtk_m_cont_cuda_ErrorCuda_h

#include <svtkm/Types.h>
#include <svtkm/cont/Error.h>

#include <cuda.h>

#include <sstream>

/// A macro that can be used to check to see if there are any unchecked
/// CUDA errors. Will throw an ErrorCuda if there are.
///
#define SVTKM_CUDA_CHECK_ASYNCHRONOUS_ERROR()                                                       \
  SVTKM_SWALLOW_SEMICOLON_PRE_BLOCK                                                                 \
  {                                                                                                \
    const cudaError_t svtkm_cuda_check_async_error = cudaGetLastError();                            \
    if (svtkm_cuda_check_async_error != cudaSuccess)                                                \
    {                                                                                              \
      throw ::svtkm::cont::cuda::ErrorCuda(                                                         \
        svtkm_cuda_check_async_error, __FILE__, __LINE__, "Unchecked asynchronous error");          \
    }                                                                                              \
  }                                                                                                \
  SVTKM_SWALLOW_SEMICOLON_POST_BLOCK

/// A macro that can be wrapped around a CUDA command and will throw an
/// ErrorCuda exception if the CUDA command fails.
///
#define SVTKM_CUDA_CALL(command)                                                                    \
  SVTKM_CUDA_CHECK_ASYNCHRONOUS_ERROR();                                                            \
  SVTKM_SWALLOW_SEMICOLON_PRE_BLOCK                                                                 \
  {                                                                                                \
    const cudaError_t svtkm_cuda_call_error = command;                                              \
    if (svtkm_cuda_call_error != cudaSuccess)                                                       \
    {                                                                                              \
      throw ::svtkm::cont::cuda::ErrorCuda(svtkm_cuda_call_error, __FILE__, __LINE__, #command);     \
    }                                                                                              \
  }                                                                                                \
  SVTKM_SWALLOW_SEMICOLON_POST_BLOCK

namespace svtkm
{
namespace cont
{
namespace cuda
{

SVTKM_SILENCE_WEAK_VTABLE_WARNING_START

/// This error is thrown whenever an unidentified CUDA runtime error is
/// encountered.
///
class SVTKM_ALWAYS_EXPORT ErrorCuda : public svtkm::cont::Error
{
public:
  ErrorCuda(cudaError_t error)
  {
    std::stringstream message;
    message << "CUDA Error: " << cudaGetErrorString(error);
    this->SetMessage(message.str());
  }

  ErrorCuda(cudaError_t error,
            const std::string& file,
            svtkm::Id line,
            const std::string& description)
  {
    std::stringstream message;
    message << "CUDA Error: " << cudaGetErrorString(error) << std::endl
            << description << " @ " << file << ":" << line;
    this->SetMessage(message.str());
  }
};

SVTKM_SILENCE_WEAK_VTABLE_WARNING_END
}
}
} // namespace svtkm::cont:cuda

#endif //svtk_m_cont_cuda_ErrorCuda_h
