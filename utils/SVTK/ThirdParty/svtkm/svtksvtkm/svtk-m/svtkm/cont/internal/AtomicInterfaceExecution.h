//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_internal_AtomicInterfaceExecution_h
#define svtk_m_cont_internal_AtomicInterfaceExecution_h

#include <svtkm/Types.h>

namespace svtkm
{
namespace cont
{
namespace internal
{

/// Class template that provides a collection of static methods that perform
/// atomic operations on raw addresses. It is the responsibility of the caller
/// to ensure that the addresses are properly aligned.
///
/// The class defines a WordTypePreferred member that is the fastest available
/// for bitwise operations of the given device. At minimum, the interface must
/// support operations on WordTypePreferred and svtkm::WordTypeDefault, which may
/// be the same. A full list of supported word types is advertised in the type
/// list @a WordTypes.
///
/// In addition, each atomic interface must support Add and CompareAndSwap on
/// UInt32 and UInt64, as these are required for the AtomicArray implementation.
///
/// To implement this on devices that share the control environment, subclass
/// svtkm::cont::internal::AtomicInterfaceControl, which may also be used
/// directly from control-side code.
template <typename DeviceTag>
class AtomicInterfaceExecution
#ifdef SVTKM_DOXYGEN_ONLY
{
  /// The preferred word type for the target device for bitwise atomic
  /// operations.
  using WordTypePreferred = FastestWordTypeForDevice;

  using WordTypes = svtkm::List<svtkm::WordTypeDefault, WordTypePreferred>;

  /// Atomically load a value from memory while enforcing, at minimum, "acquire"
  /// memory ordering.
  SVTKM_EXEC static svtkm::WordTypeDefault Load(svtkm::WordTypeDefault* addr);
  SVTKM_EXEC static WordTypePreferred Load(WordTypePreferred* addr);

  /// Atomically write a value to memory while enforcing, at minimum, "release"
  /// memory ordering.
  SVTKM_EXEC static void Store(svtkm::WordTypeDefault* addr, svtkm::WordTypeDefault value);
  SVTKM_EXEC static void Store(WordTypePreferred* addr, WordTypePreferred value);

  /// Perform an atomic integer add operation on the word at @a addr, adding
  /// @arg. This operation performs a full memory barrier around the atomic
  /// access.
  ///
  /// The value at @a addr prior to the addition is returned.
  ///
  /// @note Overflow behavior is not defined for this operation.
  /// @{
  SVTKM_EXEC static svtkm::WordTypeDefault Add(svtkm::WordTypeDefault* addr,
                                             svtkm::WordTypeDefault arg);
  SVTKM_EXEC static WordTypePreferred Add(WordTypePreferred* addr, WordTypePreferred arg);
  /// @}

  /// Perform a bitwise atomic not operation on the word at @a addr.
  /// This operation performs a full memory barrier around the atomic access.
  /// @{
  SVTKM_EXEC static svtkm::WordTypeDefault Not(svtkm::WordTypeDefault* addr);
  SVTKM_EXEC static WordTypePreferred Not(WordTypePreferred* addr);
  /// @}

  /// Perform a bitwise atomic and operation on the word at @a addr.
  /// This operation performs a full memory barrier around the atomic access.
  /// @{
  SVTKM_EXEC static svtkm::WordTypeDefault And(svtkm::WordTypeDefault* addr,
                                             svtkm::WordTypeDefault mask);
  SVTKM_EXEC static WordTypePreferred And(WordTypePreferred* addr, WordTypePreferred mask);
  /// @}

  /// Perform a bitwise atomic or operation on the word at @a addr.
  /// This operation performs a full memory barrier around the atomic access.
  /// @{
  SVTKM_EXEC static svtkm::WordTypeDefault Or(svtkm::WordTypeDefault* addr,
                                            svtkm::WordTypeDefault mask);
  SVTKM_EXEC static WordTypePreferred Or(WordTypePreferred* addr, WordTypePreferred mask);
  /// @}

  /// Perform a bitwise atomic xor operation on the word at @a addr.
  /// This operation performs a full memory barrier around the atomic access.
  /// @{
  SVTKM_EXEC static svtkm::WordTypeDefault Xor(svtkm::WordTypeDefault* addr,
                                             svtkm::WordTypeDefault mask);
  SVTKM_EXEC static WordTypePreferred Xor(WordTypePreferred* addr, WordTypePreferred mask);
  /// @}

  /// Perform an atomic CAS operation on the word at @a addr.
  ///
  /// If the value at @a addr equals @a expected, @a addr will be set to
  /// @a newWord and @a expected is returned. Otherwise, the value at @a addr
  /// is returned and not modified.
  ///
  /// This operation performs a full memory barrier around the atomic access.
  /// @{
  SVTKM_EXEC static svtkm::WordTypeDefault CompareAndSwap(svtkm::WordTypeDefault* addr,
                                                        svtkm::WordTypeDefault newWord,
                                                        svtkm::WordTypeDefault expected);
  SVTKM_EXEC static WordTypePreferred CompareAndSwap(WordTypePreferred* addr,
                                                    WordTypePreferred newWord,
                                                    WordTypePreferred expected);
  /// @}
}
#endif // SVTKM_DOXYGEN_ONLY
;
}
}
} // end namespace svtkm::cont::internal

#endif // svtk_m_cont_internal_AtomicInterfaceExecution_h
