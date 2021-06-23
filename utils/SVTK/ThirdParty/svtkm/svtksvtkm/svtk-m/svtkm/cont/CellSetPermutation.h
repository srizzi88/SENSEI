//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_CellSetPermutation_h
#define svtk_m_cont_CellSetPermutation_h

#include <svtkm/CellShape.h>
#include <svtkm/CellTraits.h>
#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandleCast.h>
#include <svtkm/cont/ArrayHandleGroupVecVariable.h>
#include <svtkm/cont/ArrayHandlePermutation.h>
#include <svtkm/cont/CellSet.h>
#include <svtkm/cont/CellSetExplicit.h>
#include <svtkm/cont/Invoker.h>
#include <svtkm/cont/internal/ConnectivityExplicitInternals.h>
#include <svtkm/internal/ConnectivityStructuredInternals.h>
#include <svtkm/worklet/WorkletMapTopology.h>

#include <svtkm/exec/ConnectivityPermuted.h>

#ifndef SVTKM_DEFAULT_CELLSET_PERMUTATION_STORAGE_TAG
#define SVTKM_DEFAULT_CELLSET_PERMUTATION_STORAGE_TAG SVTKM_DEFAULT_STORAGE_TAG
#endif

namespace svtkm
{
namespace cont
{

namespace internal
{

// To generate the reverse connectivity table with the
// ReverseConnectivityBuilder, we need a compact connectivity array that
// contains only the cell definitions from the permuted dataset, and an offsets
// array. These helpers are used to generate these arrays so that they can be
// converted in the reverse conn table.
class RConnTableHelpers
{
public:
  struct WriteNumIndices : public svtkm::worklet::WorkletVisitCellsWithPoints
  {
    using ControlSignature = void(CellSetIn cellset, FieldOutCell numIndices);
    using ExecutionSignature = void(PointCount, _2);
    using InputDomain = _1;

    SVTKM_EXEC void operator()(svtkm::IdComponent pointCount, svtkm::IdComponent& numIndices) const
    {
      numIndices = pointCount;
    }
  };

  struct WriteConnectivity : public svtkm::worklet::WorkletVisitCellsWithPoints
  {
    using ControlSignature = void(CellSetIn cellset, FieldOutCell connectivity);
    using ExecutionSignature = void(PointCount, PointIndices, _2);
    using InputDomain = _1;

    template <typename PointIndicesType, typename OutConnectivityType>
    SVTKM_EXEC void operator()(svtkm::IdComponent pointCount,
                              const PointIndicesType& pointIndices,
                              OutConnectivityType& connectivity) const
    {
      for (svtkm::IdComponent i = 0; i < pointCount; ++i)
      {
        connectivity[i] = pointIndices[i];
      }
    }
  };

public:
  template <typename CellSetPermutationType, typename Device>
  static SVTKM_CONT svtkm::cont::ArrayHandle<svtkm::IdComponent> GetNumIndicesArray(
    const CellSetPermutationType& cs,
    Device)
  {
    svtkm::cont::ArrayHandle<svtkm::IdComponent> numIndices;
    svtkm::cont::Invoker{ Device{} }(WriteNumIndices{}, cs, numIndices);
    return numIndices;
  }

  template <typename NumIndicesStorageType, typename Device>
  static SVTKM_CONT svtkm::cont::ArrayHandle<svtkm::Id> GetOffsetsArray(
    const svtkm::cont::ArrayHandle<svtkm::IdComponent, NumIndicesStorageType>& numIndices,
    svtkm::Id& connectivityLength /* outparam */,
    Device)
  {
    return svtkm::cont::ConvertNumIndicesToOffsets(numIndices, connectivityLength);
  }

  template <typename CellSetPermutationType, typename OffsetsStorageType, typename Device>
  static svtkm::cont::ArrayHandle<svtkm::Id> GetConnectivityArray(
    const CellSetPermutationType& cs,
    const svtkm::cont::ArrayHandle<svtkm::Id, OffsetsStorageType>& offsets,
    svtkm::Id connectivityLength,
    Device)
  {
    svtkm::cont::ArrayHandle<svtkm::Id> connectivity;
    connectivity.Allocate(connectivityLength);
    const auto offsetsTrim =
      svtkm::cont::make_ArrayHandleView(offsets, 0, offsets.GetNumberOfValues() - 1);
    auto connWrap = svtkm::cont::make_ArrayHandleGroupVecVariable(connectivity, offsetsTrim);
    svtkm::cont::Invoker{ Device{} }(WriteConnectivity{}, cs, connWrap);
    return connectivity;
  }
};

// This holds the temporary input arrays for the ReverseConnectivityBuilder
// algorithm.
template <typename ConnectivityStorageTag = SVTKM_DEFAULT_STORAGE_TAG,
          typename OffsetsStorageTag = SVTKM_DEFAULT_STORAGE_TAG,
          typename NumIndicesStorageTag = SVTKM_DEFAULT_STORAGE_TAG>
struct RConnBuilderInputData
{
  using ConnectivityArrayType = svtkm::cont::ArrayHandle<svtkm::Id, ConnectivityStorageTag>;
  using OffsetsArrayType = svtkm::cont::ArrayHandle<svtkm::Id, OffsetsStorageTag>;
  using NumIndicesArrayType = svtkm::cont::ArrayHandle<svtkm::IdComponent, NumIndicesStorageTag>;

  ConnectivityArrayType Connectivity;
  OffsetsArrayType Offsets; // Includes the past-the-end offset.
  NumIndicesArrayType NumIndices;
};

// default for CellSetPermutations of any cell type
template <typename CellSetPermutationType>
class RConnBuilderInput
{
public:
  using ConnectivityArrays = svtkm::cont::internal::RConnBuilderInputData<>;

  template <typename Device>
  static ConnectivityArrays Get(const CellSetPermutationType& cellset, Device)
  {
    using Helper = RConnTableHelpers;
    ConnectivityArrays conn;
    svtkm::Id connectivityLength = 0;

    conn.NumIndices = Helper::GetNumIndicesArray(cellset, Device{});
    conn.Offsets = Helper::GetOffsetsArray(conn.NumIndices, connectivityLength, Device{});
    conn.Connectivity =
      Helper::GetConnectivityArray(cellset, conn.Offsets, connectivityLength, Device{});

    return conn;
  }
};

// Specialization for CellSetExplicit/CellSetSingleType
template <typename InShapesST,
          typename InConnST,
          typename InOffsetsST,
          typename PermutationArrayHandleType>
class RConnBuilderInput<CellSetPermutation<CellSetExplicit<InShapesST, InConnST, InOffsetsST>,
                                           PermutationArrayHandleType>>
{
private:
  using BaseCellSetType = CellSetExplicit<InShapesST, InConnST, InOffsetsST>;
  using CellSetPermutationType = CellSetPermutation<BaseCellSetType, PermutationArrayHandleType>;

  using InShapesArrayType = typename BaseCellSetType::ShapesArrayType;
  using InNumIndicesArrayType = typename BaseCellSetType::NumIndicesArrayType;

  using ConnectivityStorageTag = svtkm::cont::ArrayHandle<svtkm::Id>::StorageTag;
  using OffsetsStorageTag = svtkm::cont::ArrayHandle<svtkm::Id>::StorageTag;
  using NumIndicesStorageTag =
    typename svtkm::cont::ArrayHandlePermutation<PermutationArrayHandleType,
                                                InNumIndicesArrayType>::StorageTag;


public:
  using ConnectivityArrays = svtkm::cont::internal::RConnBuilderInputData<ConnectivityStorageTag,
                                                                         OffsetsStorageTag,
                                                                         NumIndicesStorageTag>;

  template <typename Device>
  static ConnectivityArrays Get(const CellSetPermutationType& cellset, Device)
  {
    using Helper = RConnTableHelpers;

    static constexpr svtkm::TopologyElementTagCell cell{};
    static constexpr svtkm::TopologyElementTagPoint point{};

    auto fullCellSet = cellset.GetFullCellSet();

    svtkm::Id connectivityLength = 0;
    ConnectivityArrays conn;

    fullCellSet.GetOffsetsArray(cell, point);

    // We can use the implicitly generated NumIndices array to save a bit of
    // memory:
    conn.NumIndices = svtkm::cont::make_ArrayHandlePermutation(
      cellset.GetValidCellIds(), fullCellSet.GetNumIndicesArray(cell, point));

    // Need to generate the offsets from scratch so that they're ordered for the
    // lower-bounds binary searches in ReverseConnectivityBuilder.
    conn.Offsets = Helper::GetOffsetsArray(conn.NumIndices, connectivityLength, Device{});

    // Need to create a copy of this containing *only* the permuted cell defs,
    // in order, since the ReverseConnectivityBuilder will process every entry
    // in the connectivity array and we don't want the removed cells to be
    // included.
    conn.Connectivity =
      Helper::GetConnectivityArray(cellset, conn.Offsets, connectivityLength, Device{});

    return conn;
  }
};

// Specialization for CellSetStructured
template <svtkm::IdComponent DIMENSION, typename PermutationArrayHandleType>
class RConnBuilderInput<
  CellSetPermutation<CellSetStructured<DIMENSION>, PermutationArrayHandleType>>
{
private:
  using CellSetPermutationType =
    CellSetPermutation<CellSetStructured<DIMENSION>, PermutationArrayHandleType>;

public:
  using ConnectivityArrays = svtkm::cont::internal::RConnBuilderInputData<
    SVTKM_DEFAULT_STORAGE_TAG,
    typename svtkm::cont::ArrayHandleCounting<svtkm::Id>::StorageTag,
    typename svtkm::cont::ArrayHandleConstant<svtkm::IdComponent>::StorageTag>;

  template <typename Device>
  static ConnectivityArrays Get(const CellSetPermutationType& cellset, Device)
  {
    svtkm::Id numberOfCells = cellset.GetNumberOfCells();
    svtkm::IdComponent numPointsInCell =
      svtkm::internal::ConnectivityStructuredInternals<DIMENSION>::NUM_POINTS_IN_CELL;
    svtkm::Id connectivityLength = numberOfCells * numPointsInCell;

    ConnectivityArrays conn;
    conn.NumIndices = make_ArrayHandleConstant(numPointsInCell, numberOfCells);
    conn.Offsets = ArrayHandleCounting<svtkm::Id>(0, numPointsInCell, numberOfCells + 1);
    conn.Connectivity =
      RConnTableHelpers::GetConnectivityArray(cellset, conn.Offsets, connectivityLength, Device{});

    return conn;
  }
};

template <typename CellSetPermutationType>
struct CellSetPermutationTraits;

template <typename OriginalCellSet_, typename PermutationArrayHandleType_>
struct CellSetPermutationTraits<CellSetPermutation<OriginalCellSet_, PermutationArrayHandleType_>>
{
  using OriginalCellSet = OriginalCellSet_;
  using PermutationArrayHandleType = PermutationArrayHandleType_;
};

template <typename OriginalCellSet_,
          typename OriginalPermutationArrayHandleType,
          typename PermutationArrayHandleType_>
struct CellSetPermutationTraits<
  CellSetPermutation<CellSetPermutation<OriginalCellSet_, OriginalPermutationArrayHandleType>,
                     PermutationArrayHandleType_>>
{
  using PreviousCellSet = CellSetPermutation<OriginalCellSet_, OriginalPermutationArrayHandleType>;
  using PermutationArrayHandleType = svtkm::cont::ArrayHandlePermutation<
    PermutationArrayHandleType_,
    typename CellSetPermutationTraits<PreviousCellSet>::PermutationArrayHandleType>;
  using OriginalCellSet = typename CellSetPermutationTraits<PreviousCellSet>::OriginalCellSet;
  using Superclass = CellSetPermutation<OriginalCellSet, PermutationArrayHandleType>;
};

} // internal

template <typename OriginalCellSetType_,
          typename PermutationArrayHandleType_ =
            svtkm::cont::ArrayHandle<svtkm::Id, SVTKM_DEFAULT_CELLSET_PERMUTATION_STORAGE_TAG>>
class CellSetPermutation : public CellSet
{
  SVTKM_IS_CELL_SET(OriginalCellSetType_);
  SVTKM_IS_ARRAY_HANDLE(PermutationArrayHandleType_);
  SVTKM_STATIC_ASSERT_MSG(
    (std::is_same<svtkm::Id, typename PermutationArrayHandleType_::ValueType>::value),
    "Must use ArrayHandle with value type of Id for permutation array.");

public:
  using OriginalCellSetType = OriginalCellSetType_;
  using PermutationArrayHandleType = PermutationArrayHandleType_;

  SVTKM_CONT
  CellSetPermutation(const PermutationArrayHandleType& validCellIds,
                     const OriginalCellSetType& cellset)
    : CellSet()
    , ValidCellIds(validCellIds)
    , FullCellSet(cellset)
  {
  }

  SVTKM_CONT
  CellSetPermutation()
    : CellSet()
    , ValidCellIds()
    , FullCellSet()
  {
  }

  ~CellSetPermutation() override {}

  CellSetPermutation(const CellSetPermutation& src)
    : CellSet()
    , ValidCellIds(src.ValidCellIds)
    , FullCellSet(src.FullCellSet)
  {
  }


  CellSetPermutation& operator=(const CellSetPermutation& src)
  {
    this->ValidCellIds = src.ValidCellIds;
    this->FullCellSet = src.FullCellSet;
    return *this;
  }

  SVTKM_CONT
  const OriginalCellSetType& GetFullCellSet() const { return this->FullCellSet; }

  SVTKM_CONT
  const PermutationArrayHandleType& GetValidCellIds() const { return this->ValidCellIds; }

  SVTKM_CONT
  svtkm::Id GetNumberOfCells() const override { return this->ValidCellIds.GetNumberOfValues(); }

  SVTKM_CONT
  svtkm::Id GetNumberOfPoints() const override { return this->FullCellSet.GetNumberOfPoints(); }

  SVTKM_CONT
  svtkm::Id GetNumberOfFaces() const override { return -1; }

  SVTKM_CONT
  svtkm::Id GetNumberOfEdges() const override { return -1; }

  SVTKM_CONT
  void ReleaseResourcesExecution() override
  {
    this->ValidCellIds.ReleaseResourcesExecution();
    this->FullCellSet.ReleaseResourcesExecution();
    this->VisitPointsWithCells.ReleaseResourcesExecution();
  }

  SVTKM_CONT
  svtkm::IdComponent GetNumberOfPointsInCell(svtkm::Id cellIndex) const override
  {
    return this->FullCellSet.GetNumberOfPointsInCell(
      this->ValidCellIds.GetPortalConstControl().Get(cellIndex));
  }

  svtkm::UInt8 GetCellShape(svtkm::Id id) const override
  {
    return this->FullCellSet.GetCellShape(this->ValidCellIds.GetPortalConstControl().Get(id));
  }

  void GetCellPointIds(svtkm::Id id, svtkm::Id* ptids) const override
  {
    return this->FullCellSet.GetCellPointIds(this->ValidCellIds.GetPortalConstControl().Get(id),
                                             ptids);
  }

  std::shared_ptr<CellSet> NewInstance() const override
  {
    return std::make_shared<CellSetPermutation>();
  }

  void DeepCopy(const CellSet* src) override
  {
    const auto* other = dynamic_cast<const CellSetPermutation*>(src);
    if (!other)
    {
      throw svtkm::cont::ErrorBadType("CellSetPermutation::DeepCopy types don't match");
    }

    this->FullCellSet.DeepCopy(&(other->GetFullCellSet()));
    svtkm::cont::ArrayCopy(other->GetValidCellIds(), this->ValidCellIds);
  }

  //This is the way you can fill the memory from another system without copying
  SVTKM_CONT
  void Fill(const PermutationArrayHandleType& validCellIds, const OriginalCellSetType& cellset)
  {
    this->ValidCellIds = validCellIds;
    this->FullCellSet = cellset;
  }

  SVTKM_CONT svtkm::Id GetSchedulingRange(svtkm::TopologyElementTagCell) const
  {
    return this->ValidCellIds.GetNumberOfValues();
  }

  SVTKM_CONT svtkm::Id GetSchedulingRange(svtkm::TopologyElementTagPoint) const
  {
    return this->FullCellSet.GetNumberOfPoints();
  }

  template <typename Device, typename VisitTopology, typename IncidentTopology>
  struct ExecutionTypes;

  template <typename Device>
  struct ExecutionTypes<Device, svtkm::TopologyElementTagCell, svtkm::TopologyElementTagPoint>
  {
    SVTKM_IS_DEVICE_ADAPTER_TAG(Device);

    using ExecPortalType =
      typename PermutationArrayHandleType::template ExecutionTypes<Device>::PortalConst;
    using OrigExecObjectType = typename OriginalCellSetType::template ExecutionTypes<
      Device,
      svtkm::TopologyElementTagCell,
      svtkm::TopologyElementTagPoint>::ExecObjectType;

    using ExecObjectType =
      svtkm::exec::ConnectivityPermutedVisitCellsWithPoints<ExecPortalType, OrigExecObjectType>;
  };

  template <typename Device>
  struct ExecutionTypes<Device, svtkm::TopologyElementTagPoint, svtkm::TopologyElementTagCell>
  {
    SVTKM_IS_DEVICE_ADAPTER_TAG(Device);

    using ConnectivityPortalType =
      typename svtkm::cont::ArrayHandle<svtkm::Id>::template ExecutionTypes<Device>::PortalConst;
    using NumIndicesPortalType = typename svtkm::cont::ArrayHandle<
      svtkm::IdComponent>::template ExecutionTypes<Device>::PortalConst;
    using OffsetPortalType =
      typename svtkm::cont::ArrayHandle<svtkm::Id>::template ExecutionTypes<Device>::PortalConst;

    using ExecObjectType =
      svtkm::exec::ConnectivityPermutedVisitPointsWithCells<ConnectivityPortalType,
                                                           OffsetPortalType>;
  };

  template <typename Device>
  SVTKM_CONT typename ExecutionTypes<Device,
                                    svtkm::TopologyElementTagCell,
                                    svtkm::TopologyElementTagPoint>::ExecObjectType
  PrepareForInput(Device device,
                  svtkm::TopologyElementTagCell from,
                  svtkm::TopologyElementTagPoint to) const
  {
    using ConnectivityType = typename ExecutionTypes<Device,
                                                     svtkm::TopologyElementTagCell,
                                                     svtkm::TopologyElementTagPoint>::ExecObjectType;
    return ConnectivityType(this->ValidCellIds.PrepareForInput(device),
                            this->FullCellSet.PrepareForInput(device, from, to));
  }

  template <typename Device>
  SVTKM_CONT typename ExecutionTypes<Device,
                                    svtkm::TopologyElementTagPoint,
                                    svtkm::TopologyElementTagCell>::ExecObjectType
  PrepareForInput(Device device, svtkm::TopologyElementTagPoint, svtkm::TopologyElementTagCell) const
  {
    if (!this->VisitPointsWithCells.ElementsValid)
    {
      auto connTable = internal::RConnBuilderInput<CellSetPermutation>::Get(*this, device);
      internal::ComputeRConnTable(
        this->VisitPointsWithCells, connTable, this->GetNumberOfPoints(), device);
    }

    using ConnectivityType = typename ExecutionTypes<Device,
                                                     svtkm::TopologyElementTagPoint,
                                                     svtkm::TopologyElementTagCell>::ExecObjectType;
    return ConnectivityType(this->VisitPointsWithCells.Connectivity.PrepareForInput(device),
                            this->VisitPointsWithCells.Offsets.PrepareForInput(device));
  }

  SVTKM_CONT
  void PrintSummary(std::ostream& out) const override
  {
    out << "CellSetPermutation of: " << std::endl;
    this->FullCellSet.PrintSummary(out);
    out << "Permutation Array: " << std::endl;
    svtkm::cont::printSummary_ArrayHandle(this->ValidCellIds, out);
  }

private:
  using VisitPointsWithCellsConnectivity = svtkm::cont::internal::ConnectivityExplicitInternals<
    typename ArrayHandleConstant<svtkm::UInt8>::StorageTag>;

  PermutationArrayHandleType ValidCellIds;
  OriginalCellSetType FullCellSet;
  mutable VisitPointsWithCellsConnectivity VisitPointsWithCells;
};

template <typename CellSetType,
          typename PermutationArrayHandleType1,
          typename PermutationArrayHandleType2>
class CellSetPermutation<CellSetPermutation<CellSetType, PermutationArrayHandleType1>,
                         PermutationArrayHandleType2>
  : public internal::CellSetPermutationTraits<
      CellSetPermutation<CellSetPermutation<CellSetType, PermutationArrayHandleType1>,
                         PermutationArrayHandleType2>>::Superclass
{
private:
  using Superclass = typename internal::CellSetPermutationTraits<CellSetPermutation>::Superclass;

public:
  SVTKM_CONT
  CellSetPermutation(const PermutationArrayHandleType2& validCellIds,
                     const CellSetPermutation<CellSetType, PermutationArrayHandleType1>& cellset)
    : Superclass(svtkm::cont::make_ArrayHandlePermutation(validCellIds, cellset.GetValidCellIds()),
                 cellset.GetFullCellSet())
  {
  }

  SVTKM_CONT
  CellSetPermutation()
    : Superclass()
  {
  }

  ~CellSetPermutation() override {}

  SVTKM_CONT
  void Fill(const PermutationArrayHandleType2& validCellIds,
            const CellSetPermutation<CellSetType, PermutationArrayHandleType1>& cellset)
  {
    this->ValidCellIds = make_ArrayHandlePermutation(validCellIds, cellset.GetValidCellIds());
    this->FullCellSet = cellset.GetFullCellSet();
  }

  std::shared_ptr<CellSet> NewInstance() const override
  {
    return std::make_shared<CellSetPermutation>();
  }
};

template <typename OriginalCellSet, typename PermutationArrayHandleType>
svtkm::cont::CellSetPermutation<OriginalCellSet, PermutationArrayHandleType> make_CellSetPermutation(
  const PermutationArrayHandleType& cellIndexMap,
  const OriginalCellSet& cellSet)
{
  SVTKM_IS_CELL_SET(OriginalCellSet);
  SVTKM_IS_ARRAY_HANDLE(PermutationArrayHandleType);

  return svtkm::cont::CellSetPermutation<OriginalCellSet, PermutationArrayHandleType>(cellIndexMap,
                                                                                     cellSet);
}
}
} // namespace svtkm::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace svtkm
{
namespace cont
{

template <typename CSType, typename AHValidCellIds>
struct SerializableTypeString<svtkm::cont::CellSetPermutation<CSType, AHValidCellIds>>
{
  static SVTKM_CONT const std::string& Get()
  {
    static std::string name = "CS_Permutation<" + SerializableTypeString<CSType>::Get() + "," +
      SerializableTypeString<AHValidCellIds>::Get() + ">";
    return name;
  }
};
}
} // svtkm::cont

namespace mangled_diy_namespace
{

template <typename CSType, typename AHValidCellIds>
struct Serialization<svtkm::cont::CellSetPermutation<CSType, AHValidCellIds>>
{
private:
  using Type = svtkm::cont::CellSetPermutation<CSType, AHValidCellIds>;

public:
  static SVTKM_CONT void save(BinaryBuffer& bb, const Type& cs)
  {
    svtkmdiy::save(bb, cs.GetFullCellSet());
    svtkmdiy::save(bb, cs.GetValidCellIds());
  }

  static SVTKM_CONT void load(BinaryBuffer& bb, Type& cs)
  {
    CSType fullCS;
    svtkmdiy::load(bb, fullCS);
    AHValidCellIds validCellIds;
    svtkmdiy::load(bb, validCellIds);

    cs = make_CellSetPermutation(validCellIds, fullCS);
  }
};

} // diy
/// @endcond SERIALIZATION

#endif //svtk_m_cont_CellSetPermutation_h
