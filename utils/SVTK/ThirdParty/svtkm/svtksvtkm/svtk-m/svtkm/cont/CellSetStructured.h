//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_CellSetStructured_h
#define svtk_m_cont_CellSetStructured_h

#include <svtkm/cont/svtkm_cont_export.h>

#include <svtkm/TopologyElementTag.h>
#include <svtkm/cont/CellSet.h>
#include <svtkm/cont/DeviceAdapter.h>
#include <svtkm/exec/ConnectivityStructured.h>
#include <svtkm/internal/ConnectivityStructuredInternals.h>

namespace svtkm
{
namespace cont
{

template <svtkm::IdComponent DIMENSION>
class SVTKM_ALWAYS_EXPORT CellSetStructured final : public CellSet
{
private:
  using Thisclass = svtkm::cont::CellSetStructured<DIMENSION>;
  using InternalsType = svtkm::internal::ConnectivityStructuredInternals<DIMENSION>;

public:
  static const svtkm::IdComponent Dimension = DIMENSION;

  using SchedulingRangeType = typename InternalsType::SchedulingRangeType;

  svtkm::Id GetNumberOfCells() const override { return this->Structure.GetNumberOfCells(); }

  svtkm::Id GetNumberOfPoints() const override { return this->Structure.GetNumberOfPoints(); }

  svtkm::Id GetNumberOfFaces() const override { return -1; }

  svtkm::Id GetNumberOfEdges() const override { return -1; }

  // Since the entire topology is defined by by three integers, nothing to do here.
  void ReleaseResourcesExecution() override {}

  void SetPointDimensions(SchedulingRangeType dimensions)
  {
    this->Structure.SetPointDimensions(dimensions);
  }

  void SetGlobalPointIndexStart(SchedulingRangeType start)
  {
    this->Structure.SetGlobalPointIndexStart(start);
  }

  SchedulingRangeType GetPointDimensions() const { return this->Structure.GetPointDimensions(); }

  SchedulingRangeType GetCellDimensions() const { return this->Structure.GetCellDimensions(); }

  SchedulingRangeType GetGlobalPointIndexStart() const
  {
    return this->Structure.GetGlobalPointIndexStart();
  }

  svtkm::IdComponent GetNumberOfPointsInCell(svtkm::Id svtkmNotUsed(cellIndex) = 0) const override
  {
    return this->Structure.GetNumberOfPointsInCell();
  }

  svtkm::UInt8 GetCellShape(svtkm::Id svtkmNotUsed(cellIndex) = 0) const override
  {
    return static_cast<svtkm::UInt8>(this->Structure.GetCellShape());
  }

  void GetCellPointIds(svtkm::Id id, svtkm::Id* ptids) const override
  {
    auto asVec = this->Structure.GetPointsOfCell(id);
    for (svtkm::IdComponent i = 0; i < InternalsType::NUM_POINTS_IN_CELL; ++i)
    {
      ptids[i] = asVec[i];
    }
  }

  std::shared_ptr<CellSet> NewInstance() const override
  {
    return std::make_shared<CellSetStructured>();
  }

  void DeepCopy(const CellSet* src) override
  {
    const auto* other = dynamic_cast<const CellSetStructured*>(src);
    if (!other)
    {
      throw svtkm::cont::ErrorBadType("CellSetStructured::DeepCopy types don't match");
    }

    this->Structure = other->Structure;
  }

  template <typename TopologyElement>
  SchedulingRangeType GetSchedulingRange(TopologyElement) const;

  template <typename DeviceAdapter, typename VisitTopology, typename IncidentTopology>
  struct ExecutionTypes
  {
    SVTKM_IS_DEVICE_ADAPTER_TAG(DeviceAdapter);
    SVTKM_IS_TOPOLOGY_ELEMENT_TAG(VisitTopology);
    SVTKM_IS_TOPOLOGY_ELEMENT_TAG(IncidentTopology);
    using ExecObjectType =
      svtkm::exec::ConnectivityStructured<VisitTopology, IncidentTopology, Dimension>;
  };

  template <typename DeviceAdapter, typename VisitTopology, typename IncidentTopology>
  typename ExecutionTypes<DeviceAdapter, VisitTopology, IncidentTopology>::ExecObjectType
    PrepareForInput(DeviceAdapter, VisitTopology, IncidentTopology) const;

  void PrintSummary(std::ostream& out) const override;

  // Cannot use the default implementation of the destructor because the CUDA compiler
  // will attempt to create it for device, and then it will fail when it tries to call
  // the destructor of the superclass.
  ~CellSetStructured() override {}

  CellSetStructured() = default;

  CellSetStructured(const CellSetStructured& src)
    : CellSet()
    , Structure(src.Structure)
  {
  }

  CellSetStructured& operator=(const CellSetStructured& src)
  {
    this->Structure = src.Structure;
    return *this;
  }

  CellSetStructured(CellSetStructured&& src) noexcept : CellSet(),
                                                        Structure(std::move(src.Structure))
  {
  }

  CellSetStructured& operator=(CellSetStructured&& src) noexcept
  {
    this->Structure = std::move(src.Structure);
    return *this;
  }

private:
  InternalsType Structure;
};

#ifndef svtkm_cont_CellSetStructured_cxx
extern template class SVTKM_CONT_TEMPLATE_EXPORT CellSetStructured<1>;
extern template class SVTKM_CONT_TEMPLATE_EXPORT CellSetStructured<2>;
extern template class SVTKM_CONT_TEMPLATE_EXPORT CellSetStructured<3>;
#endif
}
} // namespace svtkm::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace svtkm
{
namespace cont
{

template <svtkm::IdComponent DIMENSION>
struct SerializableTypeString<svtkm::cont::CellSetStructured<DIMENSION>>
{
  static SVTKM_CONT const std::string& Get()
  {
    static std::string name = "CS_Structured<" + std::to_string(DIMENSION) + ">";
    return name;
  }
};
}
} // svtkm::cont

namespace mangled_diy_namespace
{

template <svtkm::IdComponent DIMENSION>
struct Serialization<svtkm::cont::CellSetStructured<DIMENSION>>
{
private:
  using Type = svtkm::cont::CellSetStructured<DIMENSION>;

public:
  static SVTKM_CONT void save(BinaryBuffer& bb, const Type& cs)
  {
    svtkmdiy::save(bb, cs.GetPointDimensions());
    svtkmdiy::save(bb, cs.GetGlobalPointIndexStart());
  }

  static SVTKM_CONT void load(BinaryBuffer& bb, Type& cs)
  {
    typename Type::SchedulingRangeType dims, start;
    svtkmdiy::load(bb, dims);
    svtkmdiy::load(bb, start);

    cs = Type{};
    cs.SetPointDimensions(dims);
    cs.SetGlobalPointIndexStart(start);
  }
};

} // diy
/// @endcond SERIALIZATION

#include <svtkm/cont/CellSetStructured.hxx>

#endif //svtk_m_cont_CellSetStructured_h
