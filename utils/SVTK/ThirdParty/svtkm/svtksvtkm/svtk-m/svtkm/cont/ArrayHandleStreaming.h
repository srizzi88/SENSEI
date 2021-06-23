//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_ArrayHandleStreaming_h
#define svtk_m_cont_ArrayHandleStreaming_h

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayPortal.h>

namespace svtkm
{
namespace cont
{
namespace internal
{

template <typename P>
class SVTKM_ALWAYS_EXPORT ArrayPortalStreaming
{
  using Writable = svtkm::internal::PortalSupportsSets<P>;

public:
  using PortalType = P;
  using ValueType = typename PortalType::ValueType;

  SVTKM_EXEC_CONT
  ArrayPortalStreaming()
    : InputPortal()
    , BlockIndex(0)
    , BlockSize(0)
    , CurBlockSize(0)
  {
  }

  SVTKM_EXEC_CONT
  ArrayPortalStreaming(const PortalType& inputPortal,
                       svtkm::Id blockIndex,
                       svtkm::Id blockSize,
                       svtkm::Id curBlockSize)
    : InputPortal(inputPortal)
    , BlockIndex(blockIndex)
    , BlockSize(blockSize)
    , CurBlockSize(curBlockSize)
  {
  }

  template <typename OtherP>
  SVTKM_EXEC_CONT ArrayPortalStreaming(const ArrayPortalStreaming<OtherP>& src)
    : InputPortal(src.GetPortal())
    , BlockIndex(src.GetBlockIndex())
    , BlockSize(src.GetBlockSize())
    , CurBlockSize(src.GetCurBlockSize())
  {
  }

  SVTKM_EXEC_CONT
  svtkm::Id GetNumberOfValues() const { return this->CurBlockSize; }

  SVTKM_EXEC_CONT
  ValueType Get(svtkm::Id index) const
  {
    return this->InputPortal.Get(this->BlockIndex * this->BlockSize + index);
  }

  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  SVTKM_EXEC_CONT void Set(svtkm::Id index, const ValueType& value) const
  {
    this->InputPortal.Set(this->BlockIndex * this->BlockSize + index, value);
  }

  SVTKM_EXEC_CONT
  const PortalType& GetPortal() const { return this->InputPortal; }

  SVTKM_EXEC_CONT
  void SetBlockSize(svtkm::Id blockSize) { this->BlockSize = blockSize; }

  SVTKM_EXEC_CONT
  void SetBlockIndex(svtkm::Id blockIndex) { this->BlockIndex = blockIndex; }

  SVTKM_EXEC_CONT
  void SetCurBlockSize(svtkm::Id curBlockSize) { this->CurBlockSize = curBlockSize; }

  SVTKM_EXEC_CONT
  svtkm::Id GetBlockSize() { return this->BlockSize; }

  SVTKM_EXEC_CONT
  svtkm::Id GetBlockIndex() { return this->BlockIndex; }

  SVTKM_EXEC_CONT
  svtkm::Id GetCurBlockSize() { return this->CurBlockSize; }

private:
  PortalType InputPortal;
  svtkm::Id BlockIndex;
  svtkm::Id BlockSize;
  svtkm::Id CurBlockSize;
};

} // internal

template <typename ArrayHandleInputType>
struct SVTKM_ALWAYS_EXPORT StorageTagStreaming
{
};

namespace internal
{

template <typename ArrayHandleInputType>
class Storage<typename ArrayHandleInputType::ValueType, StorageTagStreaming<ArrayHandleInputType>>
{
public:
  using ValueType = typename ArrayHandleInputType::ValueType;

  using PortalType =
    svtkm::cont::internal::ArrayPortalStreaming<typename ArrayHandleInputType::PortalControl>;
  using PortalConstType =
    svtkm::cont::internal::ArrayPortalStreaming<typename ArrayHandleInputType::PortalConstControl>;

  SVTKM_CONT
  Storage()
    : Valid(false)
  {
  }

  SVTKM_CONT
  Storage(const ArrayHandleInputType inputArray,
          svtkm::Id blockSize,
          svtkm::Id blockIndex,
          svtkm::Id curBlockSize)
    : InputArray(inputArray)
    , BlockSize(blockSize)
    , BlockIndex(blockIndex)
    , CurBlockSize(curBlockSize)
    , Valid(true)
  {
  }

  SVTKM_CONT
  PortalType GetPortal()
  {
    SVTKM_ASSERT(this->Valid);
    return PortalType(this->InputArray.GetPortalControl(), BlockSize, BlockIndex, CurBlockSize);
  }

  SVTKM_CONT
  PortalConstType GetPortalConst() const
  {
    SVTKM_ASSERT(this->Valid);
    return PortalConstType(
      this->InputArray.GetPortalConstControl(), BlockSize, BlockIndex, CurBlockSize);
  }

  SVTKM_CONT
  svtkm::Id GetNumberOfValues() const
  {
    SVTKM_ASSERT(this->Valid);
    return CurBlockSize;
  }

  SVTKM_CONT
  void Allocate(svtkm::Id numberOfValues) const
  {
    (void)numberOfValues;
    // Do nothing, since we only allocate a streaming array once at the beginning
  }

  SVTKM_CONT
  void AllocateFullArray(svtkm::Id numberOfValues)
  {
    SVTKM_ASSERT(this->Valid);
    this->InputArray.Allocate(numberOfValues);
  }

  SVTKM_CONT
  void Shrink(svtkm::Id numberOfValues)
  {
    SVTKM_ASSERT(this->Valid);
    this->InputArray.Shrink(numberOfValues);
  }

  SVTKM_CONT
  void ReleaseResources()
  {
    SVTKM_ASSERT(this->Valid);
    this->InputArray.ReleaseResources();
  }

  SVTKM_CONT
  const ArrayHandleInputType& GetArray() const
  {
    SVTKM_ASSERT(this->Valid);
    return this->InputArray;
  }

private:
  ArrayHandleInputType InputArray;
  svtkm::Id BlockSize;
  svtkm::Id BlockIndex;
  svtkm::Id CurBlockSize;
  bool Valid;
};
}
}
}

namespace svtkm
{
namespace cont
{

template <typename ArrayHandleInputType>
class ArrayHandleStreaming
  : public svtkm::cont::ArrayHandle<typename ArrayHandleInputType::ValueType,
                                   StorageTagStreaming<ArrayHandleInputType>>
{
public:
  SVTKM_ARRAY_HANDLE_SUBCLASS(ArrayHandleStreaming,
                             (ArrayHandleStreaming<ArrayHandleInputType>),
                             (svtkm::cont::ArrayHandle<typename ArrayHandleInputType::ValueType,
                                                      StorageTagStreaming<ArrayHandleInputType>>));

private:
  using StorageType = svtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
  SVTKM_CONT
  ArrayHandleStreaming(const ArrayHandleInputType& inputArray,
                       const svtkm::Id blockIndex,
                       const svtkm::Id blockSize,
                       const svtkm::Id curBlockSize)
    : Superclass(StorageType(inputArray, blockIndex, blockSize, curBlockSize))
  {
    this->GetPortalConstControl().SetBlockIndex(blockIndex);
    this->GetPortalConstControl().SetBlockSize(blockSize);
    this->GetPortalConstControl().SetCurBlockSize(curBlockSize);
  }

  SVTKM_CONT
  void AllocateFullArray(svtkm::Id numberOfValues)
  {
    auto lock = this->GetLock();

    this->ReleaseResourcesExecutionInternal(lock);
    this->Internals->GetControlArray(lock)->AllocateFullArray(numberOfValues);
    this->Internals->SetControlArrayValid(lock, true);
  }
};
}
}

#endif //svtk_m_cont_ArrayHandleStreaming_h
