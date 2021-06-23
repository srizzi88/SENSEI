//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_cuda_internal_AtomicInterfaceExecutionCuda_h
#define svtk_m_cont_cuda_internal_AtomicInterfaceExecutionCuda_h

#include <svtkm/cont/cuda/internal/DeviceAdapterTagCuda.h>

#include <svtkm/cont/internal/AtomicInterfaceExecution.h>

#include <svtkm/List.h>
#include <svtkm/Types.h>

namespace svtkm
{
namespace cont
{
namespace internal
{

template <>
class AtomicInterfaceExecution<DeviceAdapterTagCuda>
{

public:
  // Note: There are 64-bit atomics available, but not on all devices. Stick
  // with 32-bit only until we require compute capability 3.5+
  using WordTypes = svtkm::List<svtkm::UInt32>;
  using WordTypePreferred = svtkm::UInt32;

#define SVTKM_ATOMIC_OPS_FOR_TYPE(type)                                                             \
  SVTKM_SUPPRESS_EXEC_WARNINGS __device__ static type Load(const type* addr)                        \
  {                                                                                                \
    const volatile type* vaddr = addr; /* volatile to bypass cache*/                               \
    const type value = *vaddr;                                                                     \
    /* fence to ensure that dependent reads are correctly ordered */                               \
    __threadfence();                                                                               \
    return value;                                                                                  \
  }                                                                                                \
  SVTKM_SUPPRESS_EXEC_WARNINGS __device__ static void Store(type* addr, type value)                 \
  {                                                                                                \
    volatile type* vaddr = addr; /* volatile to bypass cache */                                    \
    /* fence to ensure that previous non-atomic stores are visible to other threads */             \
    __threadfence();                                                                               \
    *vaddr = value;                                                                                \
  }                                                                                                \
  SVTKM_SUPPRESS_EXEC_WARNINGS __device__ static type Add(type* addr, type arg)                     \
  {                                                                                                \
    __threadfence();                                                                               \
    auto result = atomicAdd(addr, arg);                                                            \
    __threadfence();                                                                               \
    return result;                                                                                 \
  }                                                                                                \
  SVTKM_SUPPRESS_EXEC_WARNINGS __device__ static type Not(type* addr)                               \
  {                                                                                                \
    return AtomicInterfaceExecution::Xor(addr, static_cast<type>(~type{ 0u }));                    \
  }                                                                                                \
  SVTKM_SUPPRESS_EXEC_WARNINGS __device__ static type And(type* addr, type mask)                    \
  {                                                                                                \
    __threadfence();                                                                               \
    auto result = atomicAnd(addr, mask);                                                           \
    __threadfence();                                                                               \
    return result;                                                                                 \
  }                                                                                                \
  SVTKM_SUPPRESS_EXEC_WARNINGS __device__ static type Or(type* addr, type mask)                     \
  {                                                                                                \
    __threadfence();                                                                               \
    auto result = atomicOr(addr, mask);                                                            \
    __threadfence();                                                                               \
    return result;                                                                                 \
  }                                                                                                \
  SVTKM_SUPPRESS_EXEC_WARNINGS __device__ static type Xor(type* addr, type mask)                    \
  {                                                                                                \
    __threadfence();                                                                               \
    auto result = atomicXor(addr, mask);                                                           \
    __threadfence();                                                                               \
    return result;                                                                                 \
  }                                                                                                \
  SVTKM_SUPPRESS_EXEC_WARNINGS __device__ static type CompareAndSwap(                               \
    type* addr, type newWord, type expected)                                                       \
  {                                                                                                \
    __threadfence();                                                                               \
    auto result = atomicCAS(addr, expected, newWord);                                              \
    __threadfence();                                                                               \
    return result;                                                                                 \
  }

  SVTKM_ATOMIC_OPS_FOR_TYPE(svtkm::UInt32)

#undef SVTKM_ATOMIC_OPS_FOR_TYPE

  // We also support Load, Add & CAS for 64-bit unsigned ints in order to
  // support AtomicArray usecases. We can't generally support UInt64 without
  // bumping our minimum device req to compute capability 3.5 (though we could
  // just use CAS for everything if this becomes a need). All of our supported
  // devices do support add / CAS on UInt64, just not all the bit stuff.
  SVTKM_SUPPRESS_EXEC_WARNINGS __device__ static svtkm::UInt64 Load(const svtkm::UInt64* addr)
  {
    const volatile svtkm::UInt64* vaddr = addr; /* volatile to bypass cache*/
    const svtkm::UInt64 value = *vaddr;
    /* fence to ensure that dependent reads are correctly ordered */
    __threadfence();
    return value;
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS __device__ static void Store(svtkm::UInt64* addr, svtkm::UInt64 value)
  {
    volatile svtkm::UInt64* vaddr = addr; /* volatile to bypass cache */
    /* fence to ensure that previous non-atomic stores are visible to other threads */
    __threadfence();
    *vaddr = value;
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS __device__ static svtkm::UInt64 Add(svtkm::UInt64* addr,
                                                                 svtkm::UInt64 arg)
  {
    __threadfence();
    auto result = atomicAdd(addr, arg);
    __threadfence();
    return result;
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS __device__ static svtkm::UInt64 CompareAndSwap(svtkm::UInt64* addr,
                                                                            svtkm::UInt64 newWord,
                                                                            svtkm::UInt64 expected)
  {
    __threadfence();
    auto result = atomicCAS(addr, expected, newWord);
    __threadfence();
    return result;
  }
};
}
}
} // end namespace svtkm::cont::internal

#endif // svtk_m_cont_cuda_internal_AtomicInterfaceExecutionCuda_h
