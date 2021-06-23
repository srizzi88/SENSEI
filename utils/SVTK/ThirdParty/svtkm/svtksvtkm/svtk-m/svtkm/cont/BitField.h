//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_cont_BitField_h
#define svtk_m_cont_BitField_h

#include <svtkm/cont/internal/AtomicInterfaceControl.h>
#include <svtkm/cont/internal/AtomicInterfaceExecution.h>

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/Logging.h>

#include <svtkm/List.h>
#include <svtkm/Types.h>

#include <cassert>
#include <climits>
#include <memory>
#include <type_traits>

namespace svtkm
{
namespace cont
{

class BitField;

namespace detail
{

struct BitFieldTraits
{
  // Allocations will occur in blocks of BlockSize bytes. This ensures that
  // power-of-two word sizes up to BlockSize will not access invalid data
  // during word-based access, and that atomic values will be properly aligned.
  // We use the default StorageBasic alignment for this.
  constexpr static svtkm::Id BlockSize = SVTKM_ALLOCATION_ALIGNMENT;

  // Make sure the blocksize is at least 64. Eventually we may implement SIMD
  // bit operations, and the current largest vector width is 512 bits.
  SVTKM_STATIC_ASSERT(BlockSize >= 64);

  /// Require an unsigned integral type that is <= BlockSize bytes.
  template <typename WordType>
  using IsValidWordType =
    std::integral_constant<bool,
                           /* is unsigned */
                           std::is_unsigned<WordType>::value &&
                             /* doesn't exceed blocksize */
                             sizeof(WordType) <= static_cast<size_t>(BlockSize) &&
                             /* BlockSize is a multiple of WordType */
                             static_cast<size_t>(BlockSize) % sizeof(WordType) == 0>;

  /// Require an unsigned integral type that is <= BlockSize bytes, and is
  /// is supported by the specified AtomicInterface.
  template <typename WordType, typename AtomicInterface>
  using IsValidWordTypeAtomic =
    std::integral_constant<bool,
                           /* is unsigned */
                           std::is_unsigned<WordType>::value &&
                             /* doesn't exceed blocksize */
                             sizeof(WordType) <= static_cast<size_t>(BlockSize) &&
                             /* BlockSize is a multiple of WordType */
                             static_cast<size_t>(BlockSize) % sizeof(WordType) == 0 &&
                             /* Supported by atomic interface */
                             svtkm::ListHas<typename AtomicInterface::WordTypes, WordType>::value>;
};

/// Identifies a bit in a BitField by Word and BitOffset. Note that these
/// values are dependent on the type of word used to generate the coordinate.
struct BitCoordinate
{
  /// The word containing the specified bit.
  svtkm::Id WordIndex;

  /// The zero-indexed bit in the word.
  svtkm::Int32 BitOffset; // [0, bitsInWord)
};

/// Portal for performing bit or word operations on a BitField.
///
/// This is the implementation used by BitPortal and BitPortalConst.
template <typename AtomicInterface_, bool IsConst>
class BitPortalBase
{
  // Checks if PortalType has a GetIteratorBegin() method that returns a
  // pointer.
  template <typename PortalType,
            typename PointerType = decltype(std::declval<PortalType>().GetIteratorBegin())>
  struct HasPointerAccess : public std::is_pointer<PointerType>
  {
  };

  // Determine whether we should store a const vs. mutable pointer:
  template <typename T>
  using MaybeConstPointer = typename std::conditional<IsConst, T const*, T*>::type;
  using BufferType = MaybeConstPointer<void>; // void* or void const*, as appropriate

public:
  /// The atomic interface used to carry out atomic operations. See
  /// AtomicInterfaceExecution<Device> and AtomicInterfaceControl
  using AtomicInterface = AtomicInterface_;

  /// The fastest word type for performing bitwise operations through AtomicInterface.
  using WordTypePreferred = typename AtomicInterface::WordTypePreferred;

  /// MPL check for whether a WordType may be used for non-atomic operations.
  template <typename WordType>
  using IsValidWordType = BitFieldTraits::IsValidWordType<WordType>;

  /// MPL check for whether a WordType may be used for atomic operations.
  template <typename WordType>
  using IsValidWordTypeAtomic = BitFieldTraits::IsValidWordTypeAtomic<WordType, AtomicInterface>;

  SVTKM_STATIC_ASSERT_MSG(IsValidWordType<WordTypeDefault>::value,
                         "Internal error: Default word type is invalid.");
  SVTKM_STATIC_ASSERT_MSG(IsValidWordType<WordTypePreferred>::value,
                         "Device-specific fast word type is invalid.");

  SVTKM_STATIC_ASSERT_MSG(IsValidWordTypeAtomic<WordTypeDefault>::value,
                         "Internal error: Default word type is invalid.");
  SVTKM_STATIC_ASSERT_MSG(IsValidWordTypeAtomic<WordTypePreferred>::value,
                         "Device-specific fast word type is invalid for atomic operations.");

protected:
  friend class svtkm::cont::BitField;

  /// Construct a BitPortal from an ArrayHandle with basic storage's portal.
  template <typename PortalType>
  SVTKM_EXEC_CONT BitPortalBase(const PortalType& portal, svtkm::Id numberOfBits)
    : Data{ portal.GetIteratorBegin() }
    , NumberOfBits{ numberOfBits }
  {
    SVTKM_STATIC_ASSERT_MSG(HasPointerAccess<PortalType>::value,
                           "Source portal must return a pointer from "
                           "GetIteratorBegin().");
  }

public:
  BitPortalBase() noexcept = default;
  BitPortalBase(const BitPortalBase&) noexcept = default;
  BitPortalBase(BitPortalBase&&) noexcept = default;
  BitPortalBase& operator=(const BitPortalBase&) noexcept = default;
  BitPortalBase& operator=(BitPortalBase&&) noexcept = default;

  /// Returns the number of bits in the BitField.
  SVTKM_EXEC_CONT
  svtkm::Id GetNumberOfBits() const noexcept { return this->NumberOfBits; }

  /// Returns how many words of type @a WordTypePreferred exist in the dataset.
  /// Note that this is rounded up and may contain partial words. See
  /// also GetFinalWordMask to handle the trailing partial word.
  template <typename WordType = WordTypePreferred>
  SVTKM_EXEC_CONT svtkm::Id GetNumberOfWords() const noexcept
  {
    SVTKM_STATIC_ASSERT(IsValidWordType<WordType>::value);
    static constexpr svtkm::Id WordSize = static_cast<svtkm::Id>(sizeof(WordType));
    static constexpr svtkm::Id WordBits = WordSize * CHAR_BIT;
    return (this->NumberOfBits + WordBits - 1) / WordBits;
  }

  /// Return a mask in which the valid bits in the final word (of type @a
  /// WordType) are set to 1.
  template <typename WordType = WordTypePreferred>
  SVTKM_EXEC_CONT WordType GetFinalWordMask() const noexcept
  {
    if (this->NumberOfBits == 0)
    {
      return WordType{ 0 };
    }

    static constexpr svtkm::Int32 BitsPerWord =
      static_cast<svtkm::Int32>(sizeof(WordType) * CHAR_BIT);

    const auto maxBit = this->NumberOfBits - 1;
    const auto coord = this->GetBitCoordinateFromIndex<WordType>(maxBit);
    const svtkm::Int32 shift = BitsPerWord - coord.BitOffset - 1;
    return (~WordType{ 0 }) >> shift;
  }

  /// Given a bit index, compute a @a BitCoordinate that identifies the
  /// corresponding word index and bit offset.
  template <typename WordType = WordTypePreferred>
  SVTKM_EXEC_CONT static BitCoordinate GetBitCoordinateFromIndex(svtkm::Id bitIdx) noexcept
  {
    SVTKM_STATIC_ASSERT(IsValidWordType<WordType>::value);
    static constexpr svtkm::Id BitsPerWord = static_cast<svtkm::Id>(sizeof(WordType) * CHAR_BIT);
    return { static_cast<svtkm::Id>(bitIdx / BitsPerWord),
             static_cast<svtkm::Int32>(bitIdx % BitsPerWord) };
  }

  /// Set the bit at @a bitIdx to @a val. This method is not thread-safe --
  /// threads modifying bits nearby may interfere with this operation.
  /// Additionally, this should not be used for synchronization, as there are
  /// no memory ordering requirements. See SetBitAtomic for those usecases.
  SVTKM_EXEC_CONT
  void SetBit(svtkm::Id bitIdx, bool val) const noexcept
  {
    SVTKM_STATIC_ASSERT_MSG(!IsConst, "'Set' method called on const BitField portal.");
    using WordType = WordTypePreferred;
    const auto coord = this->GetBitCoordinateFromIndex<WordType>(bitIdx);
    const auto mask = WordType(1) << coord.BitOffset;
    WordType* wordAddr = this->GetWordAddress<WordType>(coord.WordIndex);
    if (val)
    {
      *wordAddr |= mask;
    }
    else
    {
      *wordAddr &= ~mask;
    }
  }

  /// Set the bit at @a bitIdx to @a val using atomic operations. This method
  /// is thread-safe and guarantees, at minimum, "release" memory ordering.
  SVTKM_EXEC_CONT
  void SetBitAtomic(svtkm::Id bitIdx, bool val) const
  {
    SVTKM_STATIC_ASSERT_MSG(!IsConst, "'Set' method called on const BitField portal.");
    using WordType = WordTypePreferred;
    const auto coord = this->GetBitCoordinateFromIndex<WordType>(bitIdx);
    const auto mask = WordType(1) << coord.BitOffset;
    if (val)
    {
      this->OrWordAtomic(coord.WordIndex, mask);
    }
    else
    {
      this->AndWordAtomic(coord.WordIndex, ~mask);
    }
  }

  /// Return whether or not the bit at @a bitIdx is set. Note that this uses
  /// non-atomic loads and thus should not be used for synchronization.
  SVTKM_EXEC_CONT
  bool GetBit(svtkm::Id bitIdx) const noexcept
  {
    using WordType = WordTypePreferred;
    const auto coord = this->GetBitCoordinateFromIndex<WordType>(bitIdx);
    const auto word = this->GetWord<WordType>(coord.WordIndex);
    const auto mask = WordType(1) << coord.BitOffset;
    return (word & mask) != WordType(0);
  }

  /// Return whether or not the bit at @a bitIdx is set using atomic loads.
  /// This method is thread safe and guarantees, at minimum, "acquire" memory
  /// ordering.
  SVTKM_EXEC_CONT
  bool GetBitAtomic(svtkm::Id bitIdx) const
  {
    using WordType = WordTypePreferred;
    const auto coord = this->GetBitCoordinateFromIndex<WordType>(bitIdx);
    const auto word = this->GetWordAtomic<WordType>(coord.WordIndex);
    const auto mask = WordType(1) << coord.BitOffset;
    return (word & mask) != WordType(0);
  }

  /// Set the word (of type @a WordType) at @a wordIdx to @a word using
  /// non-atomic operations.
  template <typename WordType = WordTypePreferred>
  SVTKM_EXEC_CONT void SetWord(svtkm::Id wordIdx, WordType word) const noexcept
  {
    SVTKM_STATIC_ASSERT_MSG(!IsConst, "'Set' method called on const BitField portal.");
    *this->GetWordAddress<WordType>(wordIdx) = word;
  }

  /// Set the word (of type @a WordType) at @a wordIdx to @a word using atomic
  /// operations. The store guarantees, at minimum, "release" memory ordering.
  template <typename WordType = WordTypePreferred>
  SVTKM_EXEC_CONT void SetWordAtomic(svtkm::Id wordIdx, WordType word) const
  {
    SVTKM_STATIC_ASSERT_MSG(!IsConst, "'Set' method called on const BitField portal.");
    SVTKM_STATIC_ASSERT_MSG(IsValidWordTypeAtomic<WordType>::value,
                           "Requested WordType does not support atomic"
                           " operations on target execution platform.");
    AtomicInterface::Store(this->GetWordAddress<WordType>(wordIdx), word);
  }

  /// Get the word (of type @a WordType) at @a wordIdx using non-atomic
  /// operations.
  template <typename WordType = WordTypePreferred>
  SVTKM_EXEC_CONT WordType GetWord(svtkm::Id wordIdx) const noexcept
  {
    return *this->GetWordAddress<WordType>(wordIdx);
  }

  /// Get the word (of type @a WordType) at @ wordIdx using an atomic read with,
  /// at minimum, "acquire" memory ordering.
  template <typename WordType = WordTypePreferred>
  SVTKM_EXEC_CONT WordType GetWordAtomic(svtkm::Id wordIdx) const
  {
    SVTKM_STATIC_ASSERT_MSG(IsValidWordTypeAtomic<WordType>::value,
                           "Requested WordType does not support atomic"
                           " operations on target execution platform.");
    return AtomicInterface::Load(this->GetWordAddress<WordType>(wordIdx));
  }

  /// Toggle the bit at @a bitIdx, returning the original value. This method
  /// uses atomic operations and a full memory barrier.
  SVTKM_EXEC_CONT
  bool NotBitAtomic(svtkm::Id bitIdx) const
  {
    SVTKM_STATIC_ASSERT_MSG(!IsConst, "Attempt to modify const BitField portal.");
    using WordType = WordTypePreferred;
    const auto coord = this->GetBitCoordinateFromIndex<WordType>(bitIdx);
    const auto mask = WordType(1) << coord.BitOffset;
    const auto oldWord = this->XorWordAtomic(coord.WordIndex, mask);
    return (oldWord & mask) != WordType(0);
  }

  /// Perform a bitwise "not" operation on the word at @a wordIdx, returning the
  /// original word. This uses atomic operations and a full memory barrier.
  template <typename WordType = WordTypePreferred>
  SVTKM_EXEC_CONT WordType NotWordAtomic(svtkm::Id wordIdx) const
  {
    SVTKM_STATIC_ASSERT_MSG(!IsConst, "Attempt to modify const BitField portal.");
    SVTKM_STATIC_ASSERT_MSG(IsValidWordTypeAtomic<WordType>::value,
                           "Requested WordType does not support atomic"
                           " operations on target execution platform.");
    WordType* addr = this->GetWordAddress<WordType>(wordIdx);
    return AtomicInterface::Not(addr);
  }

  /// Perform an "and" operation between the bit at @a bitIdx and @a val,
  /// returning the original value at @a bitIdx. This method uses atomic
  /// operations and a full memory barrier.
  SVTKM_EXEC_CONT
  bool AndBitAtomic(svtkm::Id bitIdx, bool val) const
  {
    SVTKM_STATIC_ASSERT_MSG(!IsConst, "Attempt to modify const BitField portal.");
    using WordType = WordTypePreferred;
    const auto coord = this->GetBitCoordinateFromIndex<WordType>(bitIdx);
    const auto bitmask = WordType(1) << coord.BitOffset;
    // wordmask is all 1's, except for BitOffset which is (val ? 1 : 0)
    const auto wordmask = val ? ~WordType(0) : ~bitmask;
    const auto oldWord = this->AndWordAtomic(coord.WordIndex, wordmask);
    return (oldWord & bitmask) != WordType(0);
  }

  /// Perform an "and" operation between the word at @a wordIdx and @a wordMask,
  /// returning the original word at @a wordIdx. This method uses atomic
  /// operations and a full memory barrier.
  template <typename WordType = WordTypePreferred>
  SVTKM_EXEC_CONT WordType AndWordAtomic(svtkm::Id wordIdx, WordType wordmask) const
  {
    SVTKM_STATIC_ASSERT_MSG(!IsConst, "Attempt to modify const BitField portal.");
    SVTKM_STATIC_ASSERT_MSG(IsValidWordTypeAtomic<WordType>::value,
                           "Requested WordType does not support atomic"
                           " operations on target execution platform.");
    WordType* addr = this->GetWordAddress<WordType>(wordIdx);
    return AtomicInterface::And(addr, wordmask);
  }

  /// Perform an "of" operation between the bit at @a bitIdx and @a val,
  /// returning the original value at @a bitIdx. This method uses atomic
  /// operations and a full memory barrier.
  SVTKM_EXEC_CONT
  bool OrBitAtomic(svtkm::Id bitIdx, bool val) const
  {
    SVTKM_STATIC_ASSERT_MSG(!IsConst, "Attempt to modify const BitField portal.");
    using WordType = WordTypePreferred;
    const auto coord = this->GetBitCoordinateFromIndex<WordType>(bitIdx);
    const auto bitmask = WordType(1) << coord.BitOffset;
    // wordmask is all 0's, except for BitOffset which is (val ? 1 : 0)
    const auto wordmask = val ? bitmask : WordType(0);
    const auto oldWord = this->OrWordAtomic(coord.WordIndex, wordmask);
    return (oldWord & bitmask) != WordType(0);
  }

  /// Perform an "or" operation between the word at @a wordIdx and @a wordMask,
  /// returning the original word at @a wordIdx. This method uses atomic
  /// operations and a full memory barrier.
  template <typename WordType = WordTypePreferred>
  SVTKM_EXEC_CONT WordType OrWordAtomic(svtkm::Id wordIdx, WordType wordmask) const
  {
    SVTKM_STATIC_ASSERT_MSG(!IsConst, "Attempt to modify const BitField portal.");
    SVTKM_STATIC_ASSERT_MSG(IsValidWordTypeAtomic<WordType>::value,
                           "Requested WordType does not support atomic"
                           " operations on target execution platform.");
    WordType* addr = this->GetWordAddress<WordType>(wordIdx);
    return AtomicInterface::Or(addr, wordmask);
  }

  /// Perform an "xor" operation between the bit at @a bitIdx and @a val,
  /// returning the original value at @a bitIdx. This method uses atomic
  /// operations and a full memory barrier.
  SVTKM_EXEC_CONT
  bool XorBitAtomic(svtkm::Id bitIdx, bool val) const
  {
    SVTKM_STATIC_ASSERT_MSG(!IsConst, "Attempt to modify const BitField portal.");
    using WordType = WordTypePreferred;
    const auto coord = this->GetBitCoordinateFromIndex<WordType>(bitIdx);
    const auto bitmask = WordType(1) << coord.BitOffset;
    // wordmask is all 0's, except for BitOffset which is (val ? 1 : 0)
    const auto wordmask = val ? bitmask : WordType(0);
    const auto oldWord = this->XorWordAtomic(coord.WordIndex, wordmask);
    return (oldWord & bitmask) != WordType(0);
  }

  /// Perform an "xor" operation between the word at @a wordIdx and @a wordMask,
  /// returning the original word at @a wordIdx. This method uses atomic
  /// operations and a full memory barrier.
  template <typename WordType = WordTypePreferred>
  SVTKM_EXEC_CONT WordType XorWordAtomic(svtkm::Id wordIdx, WordType wordmask) const
  {
    SVTKM_STATIC_ASSERT_MSG(!IsConst, "Attempt to modify const BitField portal.");
    SVTKM_STATIC_ASSERT_MSG(IsValidWordTypeAtomic<WordType>::value,
                           "Requested WordType does not support atomic"
                           " operations on target execution platform.");
    WordType* addr = this->GetWordAddress<WordType>(wordIdx);
    return AtomicInterface::Xor(addr, wordmask);
  }

  /// Perform an atomic compare-and-swap operation on the bit at @a bitIdx.
  /// If the value in memory is equal to @a expectedBit, it is replaced with
  /// the value of @a newBit and the original value of the bit is returned as a
  /// boolean. This method implements a full memory barrier around the atomic
  /// operation.
  SVTKM_EXEC_CONT
  bool CompareAndSwapBitAtomic(svtkm::Id bitIdx, bool newBit, bool expectedBit) const
  {
    SVTKM_STATIC_ASSERT_MSG(!IsConst, "Attempt to modify const BitField portal.");
    using WordType = WordTypePreferred;
    const auto coord = this->GetBitCoordinateFromIndex<WordType>(bitIdx);
    const auto bitmask = WordType(1) << coord.BitOffset;

    WordType oldWord;
    WordType newWord;
    do
    {
      oldWord = this->GetWord<WordType>(coord.WordIndex);
      bool oldBitSet = (oldWord & bitmask) != WordType(0);
      if (oldBitSet != expectedBit)
      { // The bit-of-interest does not match what we expected.
        return oldBitSet;
      }
      else if (oldBitSet == newBit)
      { // The bit hasn't changed, but also already matches newVal. We're done.
        return expectedBit;
      }

      // Compute the new word
      newWord = oldWord ^ bitmask;
    } // CAS loop to resolve any conflicting changes to other bits in the word.
    while (this->CompareAndSwapWordAtomic(coord.WordIndex, newWord, oldWord) != oldWord);

    return expectedBit;
  }

  /// Perform an atomic compare-and-swap operation on the word at @a wordIdx.
  /// If the word in memory is equal to @a expectedWord, it is replaced with
  /// the value of @a newWord and the original word is returned. This method
  /// implements a full memory barrier around the atomic operation.
  template <typename WordType = WordTypePreferred>
  SVTKM_EXEC_CONT WordType CompareAndSwapWordAtomic(svtkm::Id wordIdx,
                                                   WordType newWord,
                                                   WordType expected) const
  {
    SVTKM_STATIC_ASSERT_MSG(!IsConst, "Attempt to modify const BitField portal.");
    SVTKM_STATIC_ASSERT_MSG(IsValidWordTypeAtomic<WordType>::value,
                           "Requested WordType does not support atomic"
                           " operations on target execution platform.");
    WordType* addr = this->GetWordAddress<WordType>(wordIdx);
    return AtomicInterface::CompareAndSwap(addr, newWord, expected);
  }

private:
  template <typename WordType>
  SVTKM_EXEC_CONT MaybeConstPointer<WordType> GetWordAddress(svtkm::Id wordId) const noexcept
  {
    SVTKM_STATIC_ASSERT(IsValidWordType<WordType>::value);
    return reinterpret_cast<MaybeConstPointer<WordType>>(this->Data) + wordId;
  }

  BufferType Data{ nullptr };
  svtkm::Id NumberOfBits{ 0 };
};

template <typename AtomicOps>
using BitPortal = BitPortalBase<AtomicOps, false>;

template <typename AtomicOps>
using BitPortalConst = BitPortalBase<AtomicOps, true>;

} // end namespace detail

class BitField
{
  static constexpr svtkm::Id BlockSize = detail::BitFieldTraits::BlockSize;

public:
  /// The type array handle used to store the bit data internally:
  using ArrayHandleType = ArrayHandle<WordTypeDefault, StorageTagBasic>;

  /// The BitPortal used in the control environment.
  using PortalControl = detail::BitPortal<svtkm::cont::internal::AtomicInterfaceControl>;

  /// A read-only BitPortal used in the control environment.
  using PortalConstControl = detail::BitPortalConst<svtkm::cont::internal::AtomicInterfaceControl>;

  template <typename Device>
  struct ExecutionTypes
  {
    /// The AtomicInterfaceExecution implementation used by the specified device.
    using AtomicInterface = svtkm::cont::internal::AtomicInterfaceExecution<Device>;

    /// The preferred word type used by the specified device.
    using WordTypePreferred = typename AtomicInterface::WordTypePreferred;

    /// A BitPortal that is usable on the specified device.
    using Portal = detail::BitPortal<AtomicInterface>;

    /// A read-only BitPortal that is usable on the specified device.
    using PortalConst = detail::BitPortalConst<AtomicInterface>;
  };

  /// Check whether a word type is valid for non-atomic operations.
  template <typename WordType>
  using IsValidWordType = detail::BitFieldTraits::IsValidWordType<WordType>;

  /// Check whether a word type is valid for atomic operations on a specific
  /// device.
  template <typename WordType, typename Device>
  using IsValidWordTypeAtomic = detail::BitFieldTraits::
    IsValidWordTypeAtomic<WordType, svtkm::cont::internal::AtomicInterfaceExecution<Device>>;

  /// Check whether a word type is valid for atomic operations from the control
  /// environment.
  template <typename WordType>
  using IsValidWordTypeAtomicControl =
    detail::BitFieldTraits::IsValidWordTypeAtomic<WordType,
                                                  svtkm::cont::internal::AtomicInterfaceControl>;

  SVTKM_CONT BitField()
    : Internals{ std::make_shared<InternalStruct>() }
  {
  }
  SVTKM_CONT BitField(const BitField&) = default;
  SVTKM_CONT BitField(BitField&&) noexcept = default;
  SVTKM_CONT ~BitField() = default;
  SVTKM_CONT BitField& operator=(const BitField&) = default;
  SVTKM_CONT BitField& operator=(BitField&&) noexcept = default;

  SVTKM_CONT
  bool operator==(const BitField& rhs) const { return this->Internals == rhs.Internals; }

  SVTKM_CONT
  bool operator!=(const BitField& rhs) const { return this->Internals != rhs.Internals; }

  /// Return the internal ArrayHandle used to store the BitField.
  SVTKM_CONT
  ArrayHandleType& GetData() { return this->Internals->Data; }

  /// Return the internal ArrayHandle used to store the BitField.
  SVTKM_CONT
  const ArrayHandleType& GetData() const { return this->Internals->Data; }

  /// Return the number of bits stored by this BitField.
  SVTKM_CONT
  svtkm::Id GetNumberOfBits() const { return this->Internals->NumberOfBits; }

  /// Return the number of words (of @a WordType) stored in this bit fields.
  ///
  template <typename WordType>
  SVTKM_CONT svtkm::Id GetNumberOfWords() const
  {
    SVTKM_STATIC_ASSERT(IsValidWordType<WordType>::value);
    static constexpr svtkm::Id WordBits = static_cast<svtkm::Id>(sizeof(WordType) * CHAR_BIT);
    return (this->Internals->NumberOfBits + WordBits - 1) / WordBits;
  }

  /// Allocate the requested number of bits.
  SVTKM_CONT
  void Allocate(svtkm::Id numberOfBits)
  {
    const svtkm::Id numWords = this->BitsToAllocatedStorageWords(numberOfBits);

    SVTKM_LOG_F(svtkm::cont::LogLevel::MemCont,
               "BitField Allocation: %llu bits, blocked up to %s.",
               static_cast<unsigned long long>(numberOfBits),
               svtkm::cont::GetSizeString(
                 static_cast<svtkm::UInt64>(static_cast<size_t>(numWords) * sizeof(WordTypeDefault)))
                 .c_str());

    this->Internals->Data.Allocate(numWords);
    this->Internals->NumberOfBits = numberOfBits;
  }

  /// Shrink the bit field to the requested number of bits.
  SVTKM_CONT
  void Shrink(svtkm::Id numberOfBits)
  {
    const svtkm::Id numWords = this->BitsToAllocatedStorageWords(numberOfBits);
    this->Internals->Data.Shrink(numWords);
    this->Internals->NumberOfBits = numberOfBits;
  }

  /// Release all execution-side resources held by this BitField.
  SVTKM_CONT
  void ReleaseResourcesExecution() { this->Internals->Data.ReleaseResourcesExecution(); }

  /// Release all resources held by this BitField and reset to empty.
  SVTKM_CONT
  void ReleaseResources()
  {
    this->Internals->Data.ReleaseResources();
    this->Internals->NumberOfBits = 0;
  }

  /// Force the control array to sync with the last-used device.
  SVTKM_CONT
  void SyncControlArray() const { this->Internals->Data.SyncControlArray(); }

  /// The id of the device where the most up-to-date copy of the data is
  /// currently resident. If the data is on the host, DeviceAdapterTagUndefined
  /// is returned.
  SVTKM_CONT
  DeviceAdapterId GetDeviceAdapterId() const { return this->Internals->Data.GetDeviceAdapterId(); }

  /// Get a portal to the data that is usable from the control environment.
  SVTKM_CONT
  PortalControl GetPortalControl()
  {
    return PortalControl{ this->Internals->Data.GetPortalControl(), this->Internals->NumberOfBits };
  }

  /// Get a read-only portal to the data that is usable from the control
  /// environment.
  SVTKM_CONT
  PortalConstControl GetPortalConstControl() const
  {
    return PortalConstControl{ this->Internals->Data.GetPortalConstControl(),
                               this->Internals->NumberOfBits };
  }

  /// Prepares this BitField to be used as an input to an operation in the
  /// execution environment. If necessary, copies data to the execution
  /// environment. Can throw an exception if this BitField does not yet contain
  /// any data. Returns a portal that can be used in code running in the
  /// execution environment.
  template <typename DeviceAdapterTag>
  SVTKM_CONT typename ExecutionTypes<DeviceAdapterTag>::PortalConst PrepareForInput(
    DeviceAdapterTag device) const
  {
    using PortalType = typename ExecutionTypes<DeviceAdapterTag>::PortalConst;
    return PortalType{ this->Internals->Data.PrepareForInput(device),
                       this->Internals->NumberOfBits };
  }

  /// Prepares (allocates) this BitField to be used as an output from an
  /// operation in the execution environment. The internal state of this class
  /// is set to have valid data in the execution BitField with the assumption
  /// that the array will be filled soon (i.e. before any other methods of this
  /// object are called). Returns a portal that can be used in code running in
  /// the execution environment.
  template <typename DeviceAdapterTag>
  SVTKM_CONT typename ExecutionTypes<DeviceAdapterTag>::Portal PrepareForOutput(
    svtkm::Id numBits,
    DeviceAdapterTag device) const
  {
    using PortalType = typename ExecutionTypes<DeviceAdapterTag>::Portal;
    const svtkm::Id numWords = this->BitsToAllocatedStorageWords(numBits);

    SVTKM_LOG_F(svtkm::cont::LogLevel::MemExec,
               "BitField Allocation: %llu bits, blocked up to %s.",
               static_cast<unsigned long long>(numBits),
               svtkm::cont::GetSizeString(
                 static_cast<svtkm::UInt64>(static_cast<size_t>(numWords) * sizeof(WordTypeDefault)))
                 .c_str());

    auto portal = this->Internals->Data.PrepareForOutput(numWords, device);
    this->Internals->NumberOfBits = numBits;
    return PortalType{ portal, numBits };
  }

  /// Prepares this BitField to be used in an in-place operation (both as input
  /// and output) in the execution environment. If necessary, copies data to
  /// the execution environment. Can throw an exception if this BitField does
  /// not yet contain any data. Returns a portal that can be used in code
  /// running in the execution environment.
  template <typename DeviceAdapterTag>
  SVTKM_CONT typename ExecutionTypes<DeviceAdapterTag>::Portal PrepareForInPlace(
    DeviceAdapterTag device) const
  {
    using PortalType = typename ExecutionTypes<DeviceAdapterTag>::Portal;
    return PortalType{ this->Internals->Data.PrepareForInPlace(device),
                       this->Internals->NumberOfBits };
  }

private:
  /// Returns the number of words, padded out to respect BlockSize.
  SVTKM_CONT
  static svtkm::Id BitsToAllocatedStorageWords(svtkm::Id numBits)
  {
    static constexpr svtkm::Id InternalWordSize = static_cast<svtkm::Id>(sizeof(WordTypeDefault));

    // Round up to BlockSize bytes:
    const svtkm::Id bytesNeeded = (numBits + CHAR_BIT - 1) / CHAR_BIT;
    const svtkm::Id blocksNeeded = (bytesNeeded + BlockSize - 1) / BlockSize;
    const svtkm::Id numBytes = blocksNeeded * BlockSize;
    const svtkm::Id numWords = numBytes / InternalWordSize;
    return numWords;
  }

  struct SVTKM_ALWAYS_EXPORT InternalStruct
  {
    ArrayHandleType Data;
    svtkm::Id NumberOfBits;
  };

  std::shared_ptr<InternalStruct> Internals;
};
}
} // end namespace svtkm::cont

#endif // svtk_m_cont_BitField_h
