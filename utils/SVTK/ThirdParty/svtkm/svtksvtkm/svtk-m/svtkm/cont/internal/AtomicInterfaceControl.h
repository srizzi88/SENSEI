//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_internal_AtomicInterfaceControl_h
#define svtk_m_cont_internal_AtomicInterfaceControl_h

#include <svtkm/internal/Configure.h>
#include <svtkm/internal/Windows.h>

#include <svtkm/List.h>
#include <svtkm/Types.h>

#if defined(SVTKM_MSVC) && !defined(SVTKM_CUDA)
#include <intrin.h> // For MSVC atomics
#endif

#include <atomic>
#include <cstdint>
#include <cstring>

namespace svtkm
{
namespace cont
{
namespace internal
{

/**
 * Implementation of AtomicInterfaceDevice that uses control-side atomics.
 */
class AtomicInterfaceControl
{
public:
  using WordTypes = svtkm::List<svtkm::UInt8, svtkm::UInt16, svtkm::UInt32, svtkm::UInt64>;

  // TODO These support UInt64, too. This should be benchmarked to see which
  // is faster.
  using WordTypePreferred = svtkm::UInt32;

#ifdef SVTKM_MSVC
private:
  template <typename To, typename From>
  SVTKM_EXEC_CONT static To BitCast(const From& src)
  {
    // The memcpy should be removed by the compiler when possible, but this
    // works around a host of issues with bitcasting using reinterpret_cast.
    SVTKM_STATIC_ASSERT(sizeof(From) == sizeof(To));
    To dst;
    std::memcpy(&dst, &src, sizeof(From));
    return dst;
  }

  template <typename T>
  SVTKM_EXEC_CONT static T BitCast(T&& src)
  {
    return std::forward<T>(src);
  }

public:
  // Note about Load and Store implementations:
  //
  // "Simple reads and writes to properly-aligned 32-bit variables are atomic
  //  operations"
  //
  // "Simple reads and writes to properly aligned 64-bit variables are atomic on
  // 64-bit Windows. Reads and writes to 64-bit values are not guaranteed to be
  // atomic on 32-bit Windows."
  //
  // "Reads and writes to variables of other sizes [than 32 or 64 bits] are not
  // guaranteed to be atomic on any platform."
  //
  // https://docs.microsoft.com/en-us/windows/desktop/sync/interlocked-variable-access

  SVTKM_SUPPRESS_EXEC_WARNINGS SVTKM_EXEC_CONT static svtkm::UInt8 Load(const svtkm::UInt8* addr)
  {
    // This assumes that the memory interface is smart enough to load a 32-bit
    // word atomically and a properly aligned 8-bit word from it.
    // We could build address masks and do shifts to perform this manually if
    // this assumption is incorrect.
    auto result = *static_cast<volatile const svtkm::UInt8*>(addr);
    std::atomic_thread_fence(std::memory_order_acquire);
    return result;
  }
  SVTKM_SUPPRESS_EXEC_WARNINGS SVTKM_EXEC_CONT static svtkm::UInt16 Load(const svtkm::UInt16* addr)
  {
    // This assumes that the memory interface is smart enough to load a 32-bit
    // word atomically and a properly aligned 16-bit word from it.
    // We could build address masks and do shifts to perform this manually if
    // this assumption is incorrect.
    auto result = *static_cast<volatile const svtkm::UInt16*>(addr);
    std::atomic_thread_fence(std::memory_order_acquire);
    return result;
  }
  SVTKM_SUPPRESS_EXEC_WARNINGS SVTKM_EXEC_CONT static svtkm::UInt32 Load(const svtkm::UInt32* addr)
  {
    auto result = *static_cast<volatile const svtkm::UInt32*>(addr);
    std::atomic_thread_fence(std::memory_order_acquire);
    return result;
  }
  SVTKM_SUPPRESS_EXEC_WARNINGS SVTKM_EXEC_CONT static svtkm::UInt64 Load(const svtkm::UInt64* addr)
  {
    auto result = *static_cast<volatile const svtkm::UInt64*>(addr);
    std::atomic_thread_fence(std::memory_order_acquire);
    return result;
  }
  SVTKM_SUPPRESS_EXEC_WARNINGS SVTKM_EXEC_CONT static void Store(svtkm::UInt8* addr, svtkm::UInt8 val)
  {
    // There doesn't seem to be an atomic store instruction in the windows
    // API, so just exchange and discard the result.
    _InterlockedExchange8(reinterpret_cast<volatile CHAR*>(addr), BitCast<CHAR>(val));
  }
  SVTKM_SUPPRESS_EXEC_WARNINGS SVTKM_EXEC_CONT static void Store(svtkm::UInt16* addr, svtkm::UInt16 val)
  {
    // There doesn't seem to be an atomic store instruction in the windows
    // API, so just exchange and discard the result.
    _InterlockedExchange16(reinterpret_cast<volatile SHORT*>(addr), BitCast<SHORT>(val));
  }
  SVTKM_SUPPRESS_EXEC_WARNINGS SVTKM_EXEC_CONT static void Store(svtkm::UInt32* addr, svtkm::UInt32 val)
  {
    std::atomic_thread_fence(std::memory_order_release);
    *addr = val;
  }
  SVTKM_SUPPRESS_EXEC_WARNINGS SVTKM_EXEC_CONT static void Store(svtkm::UInt64* addr, svtkm::UInt64 val)
  {
    std::atomic_thread_fence(std::memory_order_release);
    *addr = val;
  }

#define SVTKM_ATOMIC_OPS_FOR_TYPE(svtkmType, winType, suffix)                                        \
  SVTKM_SUPPRESS_EXEC_WARNINGS SVTKM_EXEC_CONT static svtkmType Add(svtkmType* addr, svtkmType arg)     \
  {                                                                                                \
    return BitCast<svtkmType>(_InterlockedExchangeAdd##suffix(                                      \
      reinterpret_cast<volatile winType*>(addr), BitCast<winType>(arg)));                          \
  }                                                                                                \
  SVTKM_SUPPRESS_EXEC_WARNINGS SVTKM_EXEC_CONT static svtkmType Not(svtkmType* addr)                   \
  {                                                                                                \
    return Xor(addr, static_cast<svtkmType>(~svtkmType{ 0u }));                                      \
  }                                                                                                \
  SVTKM_SUPPRESS_EXEC_WARNINGS SVTKM_EXEC_CONT static svtkmType And(svtkmType* addr, svtkmType mask)    \
  {                                                                                                \
    return BitCast<svtkmType>(                                                                      \
      _InterlockedAnd##suffix(reinterpret_cast<volatile winType*>(addr), BitCast<winType>(mask))); \
  }                                                                                                \
  SVTKM_SUPPRESS_EXEC_WARNINGS SVTKM_EXEC_CONT static svtkmType Or(svtkmType* addr, svtkmType mask)     \
  {                                                                                                \
    return BitCast<svtkmType>(                                                                      \
      _InterlockedOr##suffix(reinterpret_cast<volatile winType*>(addr), BitCast<winType>(mask)));  \
  }                                                                                                \
  SVTKM_SUPPRESS_EXEC_WARNINGS SVTKM_EXEC_CONT static svtkmType Xor(svtkmType* addr, svtkmType mask)    \
  {                                                                                                \
    return BitCast<svtkmType>(                                                                      \
      _InterlockedXor##suffix(reinterpret_cast<volatile winType*>(addr), BitCast<winType>(mask))); \
  }                                                                                                \
  SVTKM_SUPPRESS_EXEC_WARNINGS SVTKM_EXEC_CONT static svtkmType CompareAndSwap(                       \
    svtkmType* addr, svtkmType newWord, svtkmType expected)                                           \
  {                                                                                                \
    return BitCast<svtkmType>(                                                                      \
      _InterlockedCompareExchange##suffix(reinterpret_cast<volatile winType*>(addr),               \
                                          BitCast<winType>(newWord),                               \
                                          BitCast<winType>(expected)));                            \
  }

  SVTKM_ATOMIC_OPS_FOR_TYPE(svtkm::UInt8, CHAR, 8)
  SVTKM_ATOMIC_OPS_FOR_TYPE(svtkm::UInt16, SHORT, 16)
  SVTKM_ATOMIC_OPS_FOR_TYPE(svtkm::UInt32, LONG, )
  SVTKM_ATOMIC_OPS_FOR_TYPE(svtkm::UInt64, LONG64, 64)

#undef SVTKM_ATOMIC_OPS_FOR_TYPE

#else // gcc/clang

#define SVTKM_ATOMIC_OPS_FOR_TYPE(type)                                                             \
  SVTKM_SUPPRESS_EXEC_WARNINGS SVTKM_EXEC_CONT static type Load(const type* addr)                    \
  {                                                                                                \
    return __atomic_load_n(addr, __ATOMIC_ACQUIRE);                                                \
  }                                                                                                \
  SVTKM_SUPPRESS_EXEC_WARNINGS SVTKM_EXEC_CONT static void Store(type* addr, type value)             \
  {                                                                                                \
    return __atomic_store_n(addr, value, __ATOMIC_RELEASE);                                        \
  }                                                                                                \
  SVTKM_SUPPRESS_EXEC_WARNINGS SVTKM_EXEC_CONT static type Add(type* addr, type arg)                 \
  {                                                                                                \
    return __atomic_fetch_add(addr, arg, __ATOMIC_SEQ_CST);                                        \
  }                                                                                                \
  SVTKM_SUPPRESS_EXEC_WARNINGS SVTKM_EXEC_CONT static type Not(type* addr)                           \
  {                                                                                                \
    return Xor(addr, static_cast<type>(~type{ 0u }));                                              \
  }                                                                                                \
  SVTKM_SUPPRESS_EXEC_WARNINGS SVTKM_EXEC_CONT static type And(type* addr, type mask)                \
  {                                                                                                \
    return __atomic_fetch_and(addr, mask, __ATOMIC_SEQ_CST);                                       \
  }                                                                                                \
  SVTKM_SUPPRESS_EXEC_WARNINGS SVTKM_EXEC_CONT static type Or(type* addr, type mask)                 \
  {                                                                                                \
    return __atomic_fetch_or(addr, mask, __ATOMIC_SEQ_CST);                                        \
  }                                                                                                \
  SVTKM_SUPPRESS_EXEC_WARNINGS SVTKM_EXEC_CONT static type Xor(type* addr, type mask)                \
  {                                                                                                \
    return __atomic_fetch_xor(addr, mask, __ATOMIC_SEQ_CST);                                       \
  }                                                                                                \
  SVTKM_SUPPRESS_EXEC_WARNINGS SVTKM_EXEC_CONT static type CompareAndSwap(                           \
    type* addr, type newWord, type expected)                                                       \
  {                                                                                                \
    __atomic_compare_exchange_n(                                                                   \
      addr, &expected, newWord, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);                        \
    return expected;                                                                               \
  }

  SVTKM_ATOMIC_OPS_FOR_TYPE(svtkm::UInt8)
  SVTKM_ATOMIC_OPS_FOR_TYPE(svtkm::UInt16)
  SVTKM_ATOMIC_OPS_FOR_TYPE(svtkm::UInt32)
  SVTKM_ATOMIC_OPS_FOR_TYPE(svtkm::UInt64)

#undef SVTKM_ATOMIC_OPS_FOR_TYPE

#endif
};
}
}
} // end namespace svtkm::cont::internal

#endif // svtk_m_cont_internal_AtomicInterfaceControl_h
