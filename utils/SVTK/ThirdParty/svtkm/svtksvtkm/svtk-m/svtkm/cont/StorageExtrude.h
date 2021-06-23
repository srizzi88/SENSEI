//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtkm_internal_StorageExtrude_h
#define svtkm_internal_StorageExtrude_h

#include <svtkm/internal/IndicesExtrude.h>

#include <svtkm/VecTraits.h>

#include <svtkm/cont/ErrorBadType.h>

#include <svtkm/cont/serial/DeviceAdapterSerial.h>
#include <svtkm/cont/tbb/DeviceAdapterTBB.h>

namespace svtkm
{
namespace exec
{

template <typename PortalType>
struct SVTKM_ALWAYS_EXPORT ArrayPortalExtrudePlane
{
  using ValueType = typename PortalType::ValueType;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  ArrayPortalExtrudePlane()
    : Portal()
    , NumberOfPlanes(0){};

  ArrayPortalExtrudePlane(const PortalType& p, svtkm::Int32 numOfPlanes)
    : Portal(p)
    , NumberOfPlanes(numOfPlanes)
  {
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  svtkm::Id GetNumberOfValues() const
  {
    return this->Portal.GetNumberOfValues() * static_cast<svtkm::Id>(NumberOfPlanes);
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  ValueType Get(svtkm::Id index) const { return this->Portal.Get(index % this->NumberOfPlanes); }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  ValueType Get(svtkm::Id2 index) const { return this->Portal.Get(index[0]); }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  svtkm::Vec<ValueType, 6> GetWedge(const IndicesExtrude& index) const
  {
    svtkm::Vec<ValueType, 6> result;
    result[0] = this->Portal.Get(index.PointIds[0][0]);
    result[1] = this->Portal.Get(index.PointIds[0][1]);
    result[2] = this->Portal.Get(index.PointIds[0][2]);
    result[3] = this->Portal.Get(index.PointIds[1][0]);
    result[4] = this->Portal.Get(index.PointIds[1][1]);
    result[5] = this->Portal.Get(index.PointIds[1][2]);

    return result;
  }

  PortalType Portal;
  svtkm::Int32 NumberOfPlanes;
};
}
} // svtkm::exec

namespace svtkm
{
namespace cont
{
namespace internal
{

struct SVTKM_ALWAYS_EXPORT StorageTagExtrudePlane
{
};

template <typename T>
class SVTKM_ALWAYS_EXPORT Storage<T, internal::StorageTagExtrudePlane>
{
  using HandleType = svtkm::cont::ArrayHandle<T>;

public:
  using ValueType = T;

  using PortalConstType =
    svtkm::exec::ArrayPortalExtrudePlane<typename HandleType::PortalConstControl>;

  // Note that this array is read only, so you really should only be getting the const
  // version of the portal. If you actually try to write to this portal, you will
  // get an error.
  using PortalType = PortalConstType;

  Storage()
    : Array()
    , NumberOfPlanes(0)
  {
  }

  Storage(const HandleType& array, svtkm::Int32 numberOfPlanes)
    : Array(array)
    , NumberOfPlanes(numberOfPlanes)
  {
  }

  PortalType GetPortal()
  {
    throw svtkm::cont::ErrorBadType(
      "Extrude ArrayHandles are read only. Cannot get writable portal.");
  }

  PortalConstType GetPortalConst() const
  {
    return PortalConstType(this->Array.GetPortalConstControl(), this->NumberOfPlanes);
  }

  svtkm::Id GetNumberOfValues() const
  {
    return this->Array.GetNumberOfValues() * static_cast<svtkm::Id>(this->NumberOfPlanes);
  }

  svtkm::Int32 GetNumberOfValuesPerPlane() const
  {
    return static_cast<svtkm::Int32>(this->Array->GetNumberOfValues());
  }

  svtkm::Int32 GetNumberOfPlanes() const { return this->NumberOfPlanes; }

  void Allocate(svtkm::Id svtkmNotUsed(numberOfValues))
  {
    throw svtkm::cont::ErrorBadType("ArrayPortalExtrudePlane is read only. It cannot be allocated.");
  }

  void Shrink(svtkm::Id svtkmNotUsed(numberOfValues))
  {
    throw svtkm::cont::ErrorBadType("ArrayPortalExtrudePlane is read only. It cannot shrink.");
  }

  void ReleaseResources()
  {
    // This request is ignored since we don't own the memory that was past
    // to us
  }

private:
  svtkm::cont::ArrayHandle<T> Array;
  svtkm::Int32 NumberOfPlanes;
};

template <typename T, typename Device>
class SVTKM_ALWAYS_EXPORT ArrayTransfer<T, internal::StorageTagExtrudePlane, Device>
{
public:
  using ValueType = T;
  using StorageType = svtkm::cont::internal::Storage<T, internal::StorageTagExtrudePlane>;

  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;

  //meant to be an invalid writeable execution portal
  using PortalExecution = typename StorageType::PortalType;
  using PortalConstExecution = svtkm::exec::ArrayPortalExtrudePlane<decltype(
    svtkm::cont::ArrayHandle<T>{}.PrepareForInput(Device{}))>;

  SVTKM_CONT
  ArrayTransfer(StorageType* storage)
    : ControlData(storage)
  {
  }

  svtkm::Id GetNumberOfValues() const { return this->ControlData->GetNumberOfValues(); }

  SVTKM_CONT
  PortalConstExecution PrepareForInput(bool svtkmNotUsed(updateData))
  {
    return PortalConstExecution(this->ControlData->Array.PrepareForInput(Device()),
                                this->ControlData->NumberOfPlanes);
  }

  SVTKM_CONT
  PortalExecution PrepareForInPlace(bool& svtkmNotUsed(updateData))
  {
    throw svtkm::cont::ErrorBadType("ArrayPortalExtrudePlane read only. "
                                   "Cannot be used for in-place operations.");
  }

  SVTKM_CONT
  PortalExecution PrepareForOutput(svtkm::Id svtkmNotUsed(numberOfValues))
  {
    throw svtkm::cont::ErrorBadType("ArrayPortalExtrudePlane read only. Cannot be used as output.");
  }

  SVTKM_CONT
  void RetrieveOutputData(StorageType* svtkmNotUsed(storage)) const
  {
    throw svtkm::cont::ErrorInternal(
      "ArrayPortalExtrudePlane read only. "
      "There should be no occurance of the ArrayHandle trying to pull "
      "data from the execution environment.");
  }

  SVTKM_CONT
  void Shrink(svtkm::Id svtkmNotUsed(numberOfValues))
  {
    throw svtkm::cont::ErrorBadType("ArrayPortalExtrudePlane read only. Cannot shrink.");
  }

  SVTKM_CONT
  void ReleaseResources()
  {
    // This request is ignored since we don't own the memory that was past
    // to us
  }

private:
  const StorageType* const ControlData;
};
}
}
} // svtkm::cont::internal

namespace svtkm
{
namespace exec
{

template <typename PortalType>
struct SVTKM_ALWAYS_EXPORT ArrayPortalExtrude
{
  using ValueType = svtkm::Vec<typename PortalType::ValueType, 3>;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  ArrayPortalExtrude()
    : Portal()
    , NumberOfValues(0)
    , NumberOfPlanes(0)
    , UseCylindrical(false){};

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  ArrayPortalExtrude(const PortalType& p,
                     svtkm::Int32 numOfValues,
                     svtkm::Int32 numOfPlanes,
                     bool cylindrical = false)
    : Portal(p)
    , NumberOfValues(numOfValues)
    , NumberOfPlanes(numOfPlanes)
    , UseCylindrical(cylindrical)
  {
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  svtkm::Id GetNumberOfValues() const
  {
    return ((NumberOfValues / 2) * static_cast<svtkm::Id>(NumberOfPlanes));
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  ValueType Get(svtkm::Id index) const;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  ValueType Get(svtkm::Id2 index) const;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  svtkm::Vec<ValueType, 6> GetWedge(const IndicesExtrude& index) const;

  PortalType Portal;
  svtkm::Int32 NumberOfValues;
  svtkm::Int32 NumberOfPlanes;
  bool UseCylindrical;
};
template <typename PortalType>
typename ArrayPortalExtrude<PortalType>::ValueType
ArrayPortalExtrude<PortalType>::ArrayPortalExtrude::Get(svtkm::Id index) const
{
  using CompType = typename ValueType::ComponentType;

  const svtkm::Id realIdx = (index * 2) % this->NumberOfValues;
  const svtkm::Id whichPlane = (index * 2) / this->NumberOfValues;
  const auto phi = static_cast<CompType>(whichPlane * (svtkm::TwoPi() / this->NumberOfPlanes));

  auto r = this->Portal.Get(realIdx);
  auto z = this->Portal.Get(realIdx + 1);
  if (this->UseCylindrical)
  {
    return ValueType(r, phi, z);
  }
  else
  {
    return ValueType(r * svtkm::Cos(phi), r * svtkm::Sin(phi), z);
  }
}

template <typename PortalType>
typename ArrayPortalExtrude<PortalType>::ValueType
ArrayPortalExtrude<PortalType>::ArrayPortalExtrude::Get(svtkm::Id2 index) const
{
  using CompType = typename ValueType::ComponentType;

  const svtkm::Id realIdx = (index[0] * 2);
  const svtkm::Id whichPlane = index[1];
  const auto phi = static_cast<CompType>(whichPlane * (svtkm::TwoPi() / this->NumberOfPlanes));

  auto r = this->Portal.Get(realIdx);
  auto z = this->Portal.Get(realIdx + 1);
  if (this->UseCylindrical)
  {
    return ValueType(r, phi, z);
  }
  else
  {
    return ValueType(r * svtkm::Cos(phi), r * svtkm::Sin(phi), z);
  }
}

template <typename PortalType>
svtkm::Vec<typename ArrayPortalExtrude<PortalType>::ValueType, 6>
ArrayPortalExtrude<PortalType>::ArrayPortalExtrude::GetWedge(const IndicesExtrude& index) const
{
  using CompType = typename ValueType::ComponentType;

  svtkm::Vec<ValueType, 6> result;
  for (int j = 0; j < 2; ++j)
  {
    const auto phi =
      static_cast<CompType>(index.Planes[j] * (svtkm::TwoPi() / this->NumberOfPlanes));
    for (int i = 0; i < 3; ++i)
    {
      const svtkm::Id realIdx = index.PointIds[j][i] * 2;
      auto r = this->Portal.Get(realIdx);
      auto z = this->Portal.Get(realIdx + 1);
      result[3 * j + i] = this->UseCylindrical
        ? ValueType(r, phi, z)
        : ValueType(r * svtkm::Cos(phi), r * svtkm::Sin(phi), z);
    }
  }

  return result;
}
}
}

namespace svtkm
{
namespace cont
{
namespace internal
{
struct SVTKM_ALWAYS_EXPORT StorageTagExtrude
{
};

template <typename T>
class Storage<T, internal::StorageTagExtrude>
{
  using BaseT = typename VecTraits<T>::BaseComponentType;
  using HandleType = svtkm::cont::ArrayHandle<BaseT>;
  using TPortalType = typename HandleType::PortalConstControl;

public:
  using ValueType = T;

  using PortalConstType = exec::ArrayPortalExtrude<TPortalType>;

  // Note that this array is read only, so you really should only be getting the const
  // version of the portal. If you actually try to write to this portal, you will
  // get an error.
  using PortalType = PortalConstType;

  Storage()
    : Array()
    , NumberOfPlanes(0)
  {
  }

  // Create with externally managed memory
  Storage(const BaseT* array, svtkm::Id arrayLength, svtkm::Int32 numberOfPlanes, bool cylindrical)
    : Array(svtkm::cont::make_ArrayHandle(array, arrayLength))
    , NumberOfPlanes(numberOfPlanes)
    , UseCylindrical(cylindrical)
  {
    SVTKM_ASSERT(this->Array.GetNumberOfValues() >= 0);
  }

  Storage(const HandleType& array, svtkm::Int32 numberOfPlanes, bool cylindrical)
    : Array(array)
    , NumberOfPlanes(numberOfPlanes)
    , UseCylindrical(cylindrical)
  {
    SVTKM_ASSERT(this->Array.GetNumberOfValues() >= 0);
  }

  PortalType GetPortal()
  {
    throw svtkm::cont::ErrorBadType(
      "Extrude ArrayHandles are read only. Cannot get writable portal.");
  }

  PortalConstType GetPortalConst() const
  {
    SVTKM_ASSERT(this->Array.GetNumberOfValues() >= 0);
    return PortalConstType(this->Array.GetPortalConstControl(),
                           static_cast<svtkm::Int32>(this->Array.GetNumberOfValues()),
                           this->NumberOfPlanes,
                           this->UseCylindrical);
  }

  svtkm::Id GetNumberOfValues() const
  {
    SVTKM_ASSERT(this->Array.GetNumberOfValues() >= 0);
    return (this->Array.GetNumberOfValues() / 2) * static_cast<svtkm::Id>(this->NumberOfPlanes);
  }

  svtkm::Id GetLength() const { return this->Array.GetNumberOfValues(); }

  svtkm::Int32 GetNumberOfPlanes() const { return NumberOfPlanes; }

  bool GetUseCylindrical() const { return UseCylindrical; }
  void Allocate(svtkm::Id svtkmNotUsed(numberOfValues))
  {
    throw svtkm::cont::ErrorBadType("StorageTagExtrude is read only. It cannot be allocated.");
  }

  void Shrink(svtkm::Id svtkmNotUsed(numberOfValues))
  {
    throw svtkm::cont::ErrorBadType("StoraageTagExtrue is read only. It cannot shrink.");
  }

  void ReleaseResources()
  {
    // This request is ignored since we don't own the memory that was past
    // to us
  }


  svtkm::cont::ArrayHandle<BaseT> Array;

private:
  svtkm::Int32 NumberOfPlanes;
  bool UseCylindrical;
};

template <typename T, typename Device>
class SVTKM_ALWAYS_EXPORT ArrayTransfer<T, internal::StorageTagExtrude, Device>
{
  using BaseT = typename VecTraits<T>::BaseComponentType;
  using TPortalType = decltype(svtkm::cont::ArrayHandle<BaseT>{}.PrepareForInput(Device{}));

public:
  using ValueType = T;
  using StorageType = svtkm::cont::internal::Storage<T, internal::StorageTagExtrude>;

  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;

  //meant to be an invalid writeable execution portal
  using PortalExecution = typename StorageType::PortalType;

  using PortalConstExecution = svtkm::exec::ArrayPortalExtrude<TPortalType>;

  SVTKM_CONT
  ArrayTransfer(StorageType* storage)
    : ControlData(storage)
  {
  }
  svtkm::Id GetNumberOfValues() const { return this->ControlData->GetNumberOfValues(); }

  SVTKM_CONT
  PortalConstExecution PrepareForInput(bool svtkmNotUsed(updateData))
  {
    return PortalConstExecution(
      this->ControlData->Array.PrepareForInput(Device()),
      static_cast<svtkm::Int32>(this->ControlData->Array.GetNumberOfValues()),
      this->ControlData->GetNumberOfPlanes(),
      this->ControlData->GetUseCylindrical());
  }

  SVTKM_CONT
  PortalExecution PrepareForInPlace(bool& svtkmNotUsed(updateData))
  {
    throw svtkm::cont::ErrorBadType("StorageExtrude read only. "
                                   "Cannot be used for in-place operations.");
  }

  SVTKM_CONT
  PortalExecution PrepareForOutput(svtkm::Id svtkmNotUsed(numberOfValues))
  {
    throw svtkm::cont::ErrorBadType("StorageExtrude read only. Cannot be used as output.");
  }

  SVTKM_CONT
  void RetrieveOutputData(StorageType* svtkmNotUsed(storage)) const
  {
    throw svtkm::cont::ErrorInternal(
      "ArrayHandleExrPointCoordinates read only. "
      "There should be no occurance of the ArrayHandle trying to pull "
      "data from the execution environment.");
  }

  SVTKM_CONT
  void Shrink(svtkm::Id svtkmNotUsed(numberOfValues))
  {
    throw svtkm::cont::ErrorBadType("StorageExtrude read only. Cannot shrink.");
  }

  SVTKM_CONT
  void ReleaseResources()
  {
    // This request is ignored since we don't own the memory that was past
    // to us
  }

private:
  const StorageType* const ControlData;
};
}
}
}

#endif
