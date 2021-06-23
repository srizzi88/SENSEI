//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_CellSetExplicit_h
#define svtk_m_cont_CellSetExplicit_h

#include <svtkm/CellShape.h>
#include <svtkm/TopologyElementTag.h>
#include <svtkm/cont/ArrayGetValues.h>
#include <svtkm/cont/ArrayHandleCast.h>
#include <svtkm/cont/ArrayHandleConstant.h>
#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/cont/ArrayHandleDecorator.h>
#include <svtkm/cont/CellSet.h>
#include <svtkm/cont/internal/ConnectivityExplicitInternals.h>
#include <svtkm/exec/ConnectivityExplicit.h>

#include <svtkm/cont/svtkm_cont_export.h>

namespace svtkm
{
namespace cont
{

namespace detail
{

template <typename CellSetType, typename VisitTopology, typename IncidentTopology>
struct CellSetExplicitConnectivityChooser
{
  using ConnectivityType = svtkm::cont::internal::ConnectivityExplicitInternals<>;
};

// Used with ArrayHandleDecorator to recover the NumIndices array from the
// offsets.
struct NumIndicesDecorator
{
  template <typename OffsetsPortal>
  struct Functor
  {
    OffsetsPortal Offsets;

    SVTKM_EXEC_CONT
    svtkm::IdComponent operator()(svtkm::Id cellId) const
    {
      return static_cast<svtkm::IdComponent>(this->Offsets.Get(cellId + 1) -
                                            this->Offsets.Get(cellId));
    }
  };

  template <typename OffsetsPortal>
  static SVTKM_CONT Functor<typename std::decay<OffsetsPortal>::type> CreateFunctor(
    OffsetsPortal&& portal)
  {
    return { std::forward<OffsetsPortal>(portal) };
  }
};

} // namespace detail

#ifndef SVTKM_DEFAULT_SHAPES_STORAGE_TAG
#define SVTKM_DEFAULT_SHAPES_STORAGE_TAG SVTKM_DEFAULT_STORAGE_TAG
#endif

#ifndef SVTKM_DEFAULT_CONNECTIVITY_STORAGE_TAG
#define SVTKM_DEFAULT_CONNECTIVITY_STORAGE_TAG SVTKM_DEFAULT_STORAGE_TAG
#endif

#ifndef SVTKM_DEFAULT_OFFSETS_STORAGE_TAG
#define SVTKM_DEFAULT_OFFSETS_STORAGE_TAG SVTKM_DEFAULT_STORAGE_TAG
#endif

template <typename S1, typename S2>
void ConvertNumIndicesToOffsets(const svtkm::cont::ArrayHandle<svtkm::Id, S1>& numIndices,
                                svtkm::cont::ArrayHandle<svtkm::Id, S2>& offsets)
{
  svtkm::cont::Algorithm::ScanExtended(numIndices, offsets);
}

template <typename T, typename S1, typename S2>
void ConvertNumIndicesToOffsets(const svtkm::cont::ArrayHandle<T, S1>& numIndices,
                                svtkm::cont::ArrayHandle<svtkm::Id, S2>& offsets)
{
  const auto castCounts = svtkm::cont::make_ArrayHandleCast<svtkm::Id>(numIndices);
  ConvertNumIndicesToOffsets(castCounts, offsets);
}

template <typename T, typename S1, typename S2>
void ConvertNumIndicesToOffsets(const svtkm::cont::ArrayHandle<T, S1>& numIndices,
                                svtkm::cont::ArrayHandle<svtkm::Id, S2>& offsets,
                                svtkm::Id& connectivitySize /* outparam */)
{
  ConvertNumIndicesToOffsets(numIndices, offsets);
  connectivitySize = svtkm::cont::ArrayGetValue(offsets.GetNumberOfValues() - 1, offsets);
}

template <typename T, typename S>
svtkm::cont::ArrayHandle<svtkm::Id> ConvertNumIndicesToOffsets(
  const svtkm::cont::ArrayHandle<T, S>& numIndices)
{
  svtkm::cont::ArrayHandle<svtkm::Id> offsets;
  ConvertNumIndicesToOffsets(numIndices, offsets);
  return offsets;
}

template <typename T, typename S>
svtkm::cont::ArrayHandle<svtkm::Id> ConvertNumIndicesToOffsets(
  const svtkm::cont::ArrayHandle<T, S>& numIndices,
  svtkm::Id& connectivityLength /* outparam */)
{
  svtkm::cont::ArrayHandle<svtkm::Id> offsets;
  ConvertNumIndicesToOffsets(numIndices, offsets, connectivityLength);
  return offsets;
}

template <typename ShapesStorageTag = SVTKM_DEFAULT_SHAPES_STORAGE_TAG,
          typename ConnectivityStorageTag = SVTKM_DEFAULT_CONNECTIVITY_STORAGE_TAG,
          typename OffsetsStorageTag = SVTKM_DEFAULT_OFFSETS_STORAGE_TAG>
class SVTKM_ALWAYS_EXPORT CellSetExplicit : public CellSet
{
  using Thisclass = CellSetExplicit<ShapesStorageTag, ConnectivityStorageTag, OffsetsStorageTag>;

  template <typename VisitTopology, typename IncidentTopology>
  struct ConnectivityChooser
  {
  private:
    using Chooser = typename detail::CellSetExplicitConnectivityChooser<Thisclass,
                                                                        VisitTopology,
                                                                        IncidentTopology>;

  public:
    using ConnectivityType = typename Chooser::ConnectivityType;
    using ShapesArrayType = typename ConnectivityType::ShapesArrayType;
    using ConnectivityArrayType = typename ConnectivityType::ConnectivityArrayType;
    using OffsetsArrayType = typename ConnectivityType::OffsetsArrayType;

    using NumIndicesArrayType =
      svtkm::cont::ArrayHandleDecorator<detail::NumIndicesDecorator, OffsetsArrayType>;
  };

  using ConnTypes =
    ConnectivityChooser<svtkm::TopologyElementTagCell, svtkm::TopologyElementTagPoint>;
  using RConnTypes =
    ConnectivityChooser<svtkm::TopologyElementTagPoint, svtkm::TopologyElementTagCell>;

  using CellPointIdsType = typename ConnTypes::ConnectivityType;
  using PointCellIdsType = typename RConnTypes::ConnectivityType;

public:
  using SchedulingRangeType = svtkm::Id;

  using ShapesArrayType = typename CellPointIdsType::ShapesArrayType;
  using ConnectivityArrayType = typename CellPointIdsType::ConnectivityArrayType;
  using OffsetsArrayType = typename CellPointIdsType::OffsetsArrayType;
  using NumIndicesArrayType = typename ConnTypes::NumIndicesArrayType;

  SVTKM_CONT CellSetExplicit();
  SVTKM_CONT CellSetExplicit(const Thisclass& src);
  SVTKM_CONT CellSetExplicit(Thisclass&& src) noexcept;

  SVTKM_CONT Thisclass& operator=(const Thisclass& src);
  SVTKM_CONT Thisclass& operator=(Thisclass&& src) noexcept;

  SVTKM_CONT virtual ~CellSetExplicit() override;

  SVTKM_CONT svtkm::Id GetNumberOfCells() const override;
  SVTKM_CONT svtkm::Id GetNumberOfPoints() const override;
  SVTKM_CONT svtkm::Id GetNumberOfFaces() const override;
  SVTKM_CONT svtkm::Id GetNumberOfEdges() const override;
  SVTKM_CONT void PrintSummary(std::ostream& out) const override;

  SVTKM_CONT void ReleaseResourcesExecution() override;

  SVTKM_CONT std::shared_ptr<CellSet> NewInstance() const override;
  SVTKM_CONT void DeepCopy(const CellSet* src) override;

  SVTKM_CONT svtkm::Id GetSchedulingRange(svtkm::TopologyElementTagCell) const;
  SVTKM_CONT svtkm::Id GetSchedulingRange(svtkm::TopologyElementTagPoint) const;

  SVTKM_CONT svtkm::IdComponent GetNumberOfPointsInCell(svtkm::Id cellid) const override;
  SVTKM_CONT void GetCellPointIds(svtkm::Id id, svtkm::Id* ptids) const override;

  SVTKM_CONT svtkm::UInt8 GetCellShape(svtkm::Id cellid) const override;

  template <svtkm::IdComponent NumIndices>
  SVTKM_CONT void GetIndices(svtkm::Id index, svtkm::Vec<svtkm::Id, NumIndices>& ids) const;

  SVTKM_CONT void GetIndices(svtkm::Id index, svtkm::cont::ArrayHandle<svtkm::Id>& ids) const;

  /// First method to add cells -- one at a time.
  SVTKM_CONT void PrepareToAddCells(svtkm::Id numCells, svtkm::Id connectivityMaxLen);

  template <typename IdVecType>
  SVTKM_CONT void AddCell(svtkm::UInt8 cellType, svtkm::IdComponent numVertices, const IdVecType& ids);

  SVTKM_CONT void CompleteAddingCells(svtkm::Id numPoints);

  /// Second method to add cells -- all at once.
  /// Assigns the array handles to the explicit connectivity. This is
  /// the way you can fill the memory from another system without copying
  SVTKM_CONT
  void Fill(svtkm::Id numPoints,
            const svtkm::cont::ArrayHandle<svtkm::UInt8, ShapesStorageTag>& cellTypes,
            const svtkm::cont::ArrayHandle<svtkm::Id, ConnectivityStorageTag>& connectivity,
            const svtkm::cont::ArrayHandle<svtkm::Id, OffsetsStorageTag>& offsets);

  template <typename Device, typename VisitTopology, typename IncidentTopology>
  struct ExecutionTypes
  {
  private:
    SVTKM_IS_DEVICE_ADAPTER_TAG(Device);
    SVTKM_IS_TOPOLOGY_ELEMENT_TAG(VisitTopology);
    SVTKM_IS_TOPOLOGY_ELEMENT_TAG(IncidentTopology);

    using Chooser = ConnectivityChooser<VisitTopology, IncidentTopology>;

    using ShapesAT = typename Chooser::ShapesArrayType;
    using ConnAT = typename Chooser::ConnectivityArrayType;
    using OffsetsAT = typename Chooser::OffsetsArrayType;

    using ShapesET = typename ShapesAT::template ExecutionTypes<Device>;
    using ConnET = typename ConnAT::template ExecutionTypes<Device>;
    using OffsetsET = typename OffsetsAT::template ExecutionTypes<Device>;

  public:
    using ShapesPortalType = typename ShapesET::PortalConst;
    using ConnectivityPortalType = typename ConnET::PortalConst;
    using OffsetsPortalType = typename OffsetsET::PortalConst;

    using ExecObjectType =
      svtkm::exec::ConnectivityExplicit<ShapesPortalType, ConnectivityPortalType, OffsetsPortalType>;
  };

  template <typename Device, typename VisitTopology, typename IncidentTopology>
  SVTKM_CONT typename ExecutionTypes<Device, VisitTopology, IncidentTopology>::ExecObjectType
    PrepareForInput(Device, VisitTopology, IncidentTopology) const;

  template <typename VisitTopology, typename IncidentTopology>
  SVTKM_CONT const typename ConnectivityChooser<VisitTopology, IncidentTopology>::ShapesArrayType&
    GetShapesArray(VisitTopology, IncidentTopology) const;

  template <typename VisitTopology, typename IncidentTopology>
  SVTKM_CONT const typename ConnectivityChooser<VisitTopology,
                                               IncidentTopology>::ConnectivityArrayType&
    GetConnectivityArray(VisitTopology, IncidentTopology) const;

  template <typename VisitTopology, typename IncidentTopology>
  SVTKM_CONT const typename ConnectivityChooser<VisitTopology, IncidentTopology>::OffsetsArrayType&
    GetOffsetsArray(VisitTopology, IncidentTopology) const;

  template <typename VisitTopology, typename IncidentTopology>
  SVTKM_CONT typename ConnectivityChooser<VisitTopology, IncidentTopology>::NumIndicesArrayType
    GetNumIndicesArray(VisitTopology, IncidentTopology) const;

  template <typename VisitTopology, typename IncidentTopology>
  SVTKM_CONT bool HasConnectivity(VisitTopology visit, IncidentTopology incident) const
  {
    return this->HasConnectivityImpl(visit, incident);
  }

  // Can be used to reset a connectivity table, mostly useful for benchmarking.
  template <typename VisitTopology, typename IncidentTopology>
  SVTKM_CONT void ResetConnectivity(VisitTopology visit, IncidentTopology incident)
  {
    this->ResetConnectivityImpl(visit, incident);
  }

protected:
  SVTKM_CONT void BuildConnectivity(svtkm::cont::DeviceAdapterId,
                                   svtkm::TopologyElementTagCell,
                                   svtkm::TopologyElementTagPoint) const;

  SVTKM_CONT void BuildConnectivity(svtkm::cont::DeviceAdapterId,
                                   svtkm::TopologyElementTagPoint,
                                   svtkm::TopologyElementTagCell) const;

  SVTKM_CONT bool HasConnectivityImpl(svtkm::TopologyElementTagCell,
                                     svtkm::TopologyElementTagPoint) const
  {
    return this->Data->CellPointIds.ElementsValid;
  }

  SVTKM_CONT bool HasConnectivityImpl(svtkm::TopologyElementTagPoint,
                                     svtkm::TopologyElementTagCell) const
  {
    return this->Data->PointCellIds.ElementsValid;
  }

  SVTKM_CONT void ResetConnectivityImpl(svtkm::TopologyElementTagCell, svtkm::TopologyElementTagPoint)
  {
    // Reset entire cell set
    this->Data->CellPointIds = CellPointIdsType{};
    this->Data->PointCellIds = PointCellIdsType{};
    this->Data->ConnectivityAdded = -1;
    this->Data->NumberOfCellsAdded = -1;
    this->Data->NumberOfPoints = 0;
  }

  SVTKM_CONT void ResetConnectivityImpl(svtkm::TopologyElementTagPoint, svtkm::TopologyElementTagCell)
  {
    this->Data->PointCellIds = PointCellIdsType{};
  }

  // Store internals in a shared pointer so shallow copies stay consistent.
  // See #2268.
  struct Internals
  {
    CellPointIdsType CellPointIds;
    PointCellIdsType PointCellIds;

    // These are used in the AddCell and related methods to incrementally add
    // cells. They need to be protected as subclasses of CellSetExplicit
    // need to set these values when implementing Fill()
    svtkm::Id ConnectivityAdded;
    svtkm::Id NumberOfCellsAdded;
    svtkm::Id NumberOfPoints;

    SVTKM_CONT
    Internals()
      : ConnectivityAdded(-1)
      , NumberOfCellsAdded(-1)
      , NumberOfPoints(0)
    {
    }
  };

  std::shared_ptr<Internals> Data;

private:
  SVTKM_CONT
  const CellPointIdsType& GetConnectivity(svtkm::TopologyElementTagCell,
                                          svtkm::TopologyElementTagPoint) const
  {
    return this->Data->CellPointIds;
  }

  SVTKM_CONT
  const CellPointIdsType& GetConnectivity(svtkm::TopologyElementTagCell,
                                          svtkm::TopologyElementTagPoint)
  {
    return this->Data->CellPointIds;
  }

  SVTKM_CONT
  const PointCellIdsType& GetConnectivity(svtkm::TopologyElementTagPoint,
                                          svtkm::TopologyElementTagCell) const
  {
    return this->Data->PointCellIds;
  }

  SVTKM_CONT
  const PointCellIdsType& GetConnectivity(svtkm::TopologyElementTagPoint,
                                          svtkm::TopologyElementTagCell)
  {
    return this->Data->PointCellIds;
  }
};

namespace detail
{

template <typename Storage1, typename Storage2, typename Storage3>
struct CellSetExplicitConnectivityChooser<svtkm::cont::CellSetExplicit<Storage1, Storage2, Storage3>,
                                          svtkm::TopologyElementTagCell,
                                          svtkm::TopologyElementTagPoint>
{
  using ConnectivityType =
    svtkm::cont::internal::ConnectivityExplicitInternals<Storage1, Storage2, Storage3>;
};

template <typename CellSetType>
struct CellSetExplicitConnectivityChooser<CellSetType,
                                          svtkm::TopologyElementTagPoint,
                                          svtkm::TopologyElementTagCell>
{
  //only specify the shape type as it will be constant as everything
  //is a vertex. otherwise use the defaults.
  using ConnectivityType = svtkm::cont::internal::ConnectivityExplicitInternals<
    typename ArrayHandleConstant<svtkm::UInt8>::StorageTag>;
};

} // namespace detail

/// \cond
/// Make doxygen ignore this section
#ifndef svtk_m_cont_CellSetExplicit_cxx
extern template class SVTKM_CONT_TEMPLATE_EXPORT CellSetExplicit<>; // default
extern template class SVTKM_CONT_TEMPLATE_EXPORT CellSetExplicit<
  typename svtkm::cont::ArrayHandleConstant<svtkm::UInt8>::StorageTag,
  SVTKM_DEFAULT_CONNECTIVITY_STORAGE_TAG,
  typename svtkm::cont::ArrayHandleCounting<svtkm::Id>::StorageTag>; // CellSetSingleType base
#endif
/// \endcond
}
} // namespace svtkm::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace svtkm
{
namespace cont
{

template <typename SST, typename CST, typename OST>
struct SerializableTypeString<svtkm::cont::CellSetExplicit<SST, CST, OST>>
{
  static SVTKM_CONT const std::string& Get()
  {
    static std::string name = "CS_Explicit<" +
      SerializableTypeString<svtkm::cont::ArrayHandle<svtkm::UInt8, SST>>::Get() + "_ST," +
      SerializableTypeString<svtkm::cont::ArrayHandle<svtkm::Id, CST>>::Get() + "_ST," +
      SerializableTypeString<svtkm::cont::ArrayHandle<svtkm::Id, OST>>::Get() + "_ST>";

    return name;
  }
};
}
} // svtkm::cont

namespace mangled_diy_namespace
{

template <typename SST, typename CST, typename OST>
struct Serialization<svtkm::cont::CellSetExplicit<SST, CST, OST>>
{
private:
  using Type = svtkm::cont::CellSetExplicit<SST, CST, OST>;

public:
  static SVTKM_CONT void save(BinaryBuffer& bb, const Type& cs)
  {
    svtkmdiy::save(bb, cs.GetNumberOfPoints());
    svtkmdiy::save(
      bb, cs.GetShapesArray(svtkm::TopologyElementTagCell{}, svtkm::TopologyElementTagPoint{}));
    svtkmdiy::save(
      bb, cs.GetConnectivityArray(svtkm::TopologyElementTagCell{}, svtkm::TopologyElementTagPoint{}));
    svtkmdiy::save(
      bb, cs.GetOffsetsArray(svtkm::TopologyElementTagCell{}, svtkm::TopologyElementTagPoint{}));
  }

  static SVTKM_CONT void load(BinaryBuffer& bb, Type& cs)
  {
    svtkm::Id numberOfPoints = 0;
    svtkmdiy::load(bb, numberOfPoints);
    svtkm::cont::ArrayHandle<svtkm::UInt8, SST> shapes;
    svtkmdiy::load(bb, shapes);
    svtkm::cont::ArrayHandle<svtkm::Id, CST> connectivity;
    svtkmdiy::load(bb, connectivity);
    svtkm::cont::ArrayHandle<svtkm::Id, OST> offsets;
    svtkmdiy::load(bb, offsets);

    cs = Type{};
    cs.Fill(numberOfPoints, shapes, connectivity, offsets);
  }
};

} // diy
/// @endcond SERIALIZATION

#include <svtkm/cont/CellSetExplicit.hxx>

#endif //svtk_m_cont_CellSetExplicit_h
