//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_ArrayHandleBitField_h
#define svtk_m_cont_ArrayHandleBitField_h

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/BitField.h>
#include <svtkm/cont/Storage.h>

namespace svtkm
{
namespace cont
{

namespace internal
{

template <typename BitPortalType>
class ArrayPortalBitField
{
public:
  using ValueType = bool;

  SVTKM_EXEC_CONT
  explicit ArrayPortalBitField(const BitPortalType& portal) noexcept : BitPortal{ portal } {}

  SVTKM_EXEC_CONT
  explicit ArrayPortalBitField(BitPortalType&& portal) noexcept : BitPortal{ std::move(portal) } {}

  ArrayPortalBitField() noexcept = default;
  ArrayPortalBitField(const ArrayPortalBitField&) noexcept = default;
  ArrayPortalBitField(ArrayPortalBitField&&) noexcept = default;
  ArrayPortalBitField& operator=(const ArrayPortalBitField&) noexcept = default;
  ArrayPortalBitField& operator=(ArrayPortalBitField&&) noexcept = default;

  SVTKM_EXEC_CONT
  svtkm::Id GetNumberOfValues() const noexcept { return this->BitPortal.GetNumberOfBits(); }

  SVTKM_EXEC_CONT
  ValueType Get(svtkm::Id index) const noexcept { return this->BitPortal.GetBit(index); }

  SVTKM_EXEC_CONT
  void Set(svtkm::Id index, ValueType value) const
  {
    // Use an atomic set so we don't clash with other threads writing nearby
    // bits.
    this->BitPortal.SetBitAtomic(index, value);
  }

private:
  BitPortalType BitPortal;
};

struct SVTKM_ALWAYS_EXPORT StorageTagBitField
{
};

template <>
class Storage<bool, StorageTagBitField>
{
  using BitPortalType = svtkm::cont::detail::BitPortal<svtkm::cont::internal::AtomicInterfaceControl>;
  using BitPortalConstType =
    svtkm::cont::detail::BitPortalConst<svtkm::cont::internal::AtomicInterfaceControl>;

public:
  using ValueType = bool;
  using PortalType = svtkm::cont::internal::ArrayPortalBitField<BitPortalType>;
  using PortalConstType = svtkm::cont::internal::ArrayPortalBitField<BitPortalConstType>;

  explicit SVTKM_CONT Storage(const svtkm::cont::BitField& data)
    : Data{ data }
  {
  }

  explicit SVTKM_CONT Storage(svtkm::cont::BitField&& data) noexcept : Data{ std::move(data) } {}

  SVTKM_CONT Storage() = default;
  SVTKM_CONT Storage(const Storage&) = default;
  SVTKM_CONT Storage(Storage&&) noexcept = default;
  SVTKM_CONT Storage& operator=(const Storage&) = default;
  SVTKM_CONT Storage& operator=(Storage&&) noexcept = default;

  SVTKM_CONT
  PortalType GetPortal() { return PortalType{ this->Data.GetPortalControl() }; }

  SVTKM_CONT
  PortalConstType GetPortalConst() { return PortalConstType{ this->Data.GetPortalConstControl() }; }

  SVTKM_CONT svtkm::Id GetNumberOfValues() const { return this->Data.GetNumberOfBits(); }
  SVTKM_CONT void Allocate(svtkm::Id numberOfValues) { this->Data.Allocate(numberOfValues); }
  SVTKM_CONT void Shrink(svtkm::Id numberOfValues) { this->Data.Shrink(numberOfValues); }
  SVTKM_CONT void ReleaseResources() { this->Data.ReleaseResources(); }

  SVTKM_CONT svtkm::cont::BitField GetBitField() const { return this->Data; }

private:
  svtkm::cont::BitField Data;
};

template <typename Device>
class ArrayTransfer<bool, StorageTagBitField, Device>
{
  using AtomicInterface = AtomicInterfaceExecution<Device>;
  using StorageType = Storage<bool, StorageTagBitField>;
  using BitPortalExecution = svtkm::cont::detail::BitPortal<AtomicInterface>;
  using BitPortalConstExecution = svtkm::cont::detail::BitPortalConst<AtomicInterface>;

public:
  using ValueType = bool;
  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;
  using PortalExecution = svtkm::cont::internal::ArrayPortalBitField<BitPortalExecution>;
  using PortalConstExecution = svtkm::cont::internal::ArrayPortalBitField<BitPortalConstExecution>;

  SVTKM_CONT
  explicit ArrayTransfer(StorageType* storage)
    : Data{ storage->GetBitField() }
  {
  }

  SVTKM_CONT
  svtkm::Id GetNumberOfValues() const { return this->Data.GetNumberOfBits(); }

  SVTKM_CONT
  PortalConstExecution PrepareForInput(bool svtkmNotUsed(updateData))
  {
    return PortalConstExecution{ this->Data.PrepareForInput(Device{}) };
  }

  SVTKM_CONT
  PortalExecution PrepareForInPlace(bool svtkmNotUsed(updateData))
  {
    return PortalExecution{ this->Data.PrepareForInPlace(Device{}) };
  }

  SVTKM_CONT
  PortalExecution PrepareForOutput(svtkm::Id numberOfValues)
  {
    return PortalExecution{ this->Data.PrepareForOutput(numberOfValues, Device{}) };
  }

  SVTKM_CONT
  void RetrieveOutputData(StorageType* svtkmNotUsed(storage)) const
  {
    // Implementation of this method should be unnecessary. The internal
    // bitfield should automatically retrieve the output data as necessary.
  }

  SVTKM_CONT
  void Shrink(svtkm::Id numberOfValues) { this->Data.Shrink(numberOfValues); }

  SVTKM_CONT
  void ReleaseResources() { this->Data.ReleaseResources(); }

private:
  svtkm::cont::BitField Data;
};

} // end namespace internal


/// The ArrayHandleBitField class is a boolean-valued ArrayHandle that is backed
/// by a BitField.
///
class ArrayHandleBitField : public ArrayHandle<bool, internal::StorageTagBitField>
{
public:
  SVTKM_ARRAY_HANDLE_SUBCLASS_NT(ArrayHandleBitField,
                                (ArrayHandle<bool, internal::StorageTagBitField>));

  SVTKM_CONT
  explicit ArrayHandleBitField(const svtkm::cont::BitField& bitField)
    : Superclass{ StorageType{ bitField } }
  {
  }

  SVTKM_CONT
  explicit ArrayHandleBitField(svtkm::cont::BitField&& bitField) noexcept
    : Superclass{ StorageType{ std::move(bitField) } }
  {
  }

  SVTKM_CONT
  svtkm::cont::BitField GetBitField() const { return this->GetStorage().GetBitField(); }
};

SVTKM_CONT inline svtkm::cont::ArrayHandleBitField make_ArrayHandleBitField(
  const svtkm::cont::BitField& bitField)
{
  return ArrayHandleBitField{ bitField };
}

SVTKM_CONT inline svtkm::cont::ArrayHandleBitField make_ArrayHandleBitField(
  svtkm::cont::BitField&& bitField) noexcept
{
  return ArrayHandleBitField{ std::move(bitField) };
}
}
} // end namespace svtkm::cont

#endif // svtk_m_cont_ArrayHandleBitField_h
