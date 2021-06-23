//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_CellSetExtrude_h
#define svtk_m_cont_CellSetExtrude_h

#include <svtkm/TopologyElementTag.h>
#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/cont/ArrayHandleExtrudeCoords.h>
#include <svtkm/cont/CellSet.h>
#include <svtkm/cont/Invoker.h>
#include <svtkm/exec/ConnectivityExtrude.h>
#include <svtkm/exec/arg/ThreadIndicesExtrude.h>
#include <svtkm/worklet/WorkletMapField.h>

namespace svtkm
{
namespace cont
{

class SVTKM_CONT_EXPORT CellSetExtrude : public CellSet
{
public:
  SVTKM_CONT CellSetExtrude();

  SVTKM_CONT CellSetExtrude(const svtkm::cont::ArrayHandle<svtkm::Int32>& conn,
                           svtkm::Int32 numberOfPointsPerPlane,
                           svtkm::Int32 numberOfPlanes,
                           const svtkm::cont::ArrayHandle<svtkm::Int32>& nextNode,
                           bool periodic);

  SVTKM_CONT CellSetExtrude(const CellSetExtrude& src);
  SVTKM_CONT CellSetExtrude(CellSetExtrude&& src) noexcept;

  SVTKM_CONT CellSetExtrude& operator=(const CellSetExtrude& src);
  SVTKM_CONT CellSetExtrude& operator=(CellSetExtrude&& src) noexcept;

  virtual ~CellSetExtrude() override;

  svtkm::Int32 GetNumberOfPlanes() const;

  svtkm::Id GetNumberOfCells() const override;

  svtkm::Id GetNumberOfPoints() const override;

  svtkm::Id GetNumberOfFaces() const override;

  svtkm::Id GetNumberOfEdges() const override;

  SVTKM_CONT svtkm::Id2 GetSchedulingRange(svtkm::TopologyElementTagCell) const;

  SVTKM_CONT svtkm::Id2 GetSchedulingRange(svtkm::TopologyElementTagPoint) const;

  svtkm::UInt8 GetCellShape(svtkm::Id id) const override;
  svtkm::IdComponent GetNumberOfPointsInCell(svtkm::Id id) const override;
  void GetCellPointIds(svtkm::Id id, svtkm::Id* ptids) const override;

  std::shared_ptr<CellSet> NewInstance() const override;
  void DeepCopy(const CellSet* src) override;

  void PrintSummary(std::ostream& out) const override;
  void ReleaseResourcesExecution() override;

  const svtkm::cont::ArrayHandle<svtkm::Int32>& GetConnectivityArray() const
  {
    return this->Connectivity;
  }

  svtkm::Int32 GetNumberOfPointsPerPlane() const { return this->NumberOfPointsPerPlane; }

  const svtkm::cont::ArrayHandle<svtkm::Int32>& GetNextNodeArray() const { return this->NextNode; }

  bool GetIsPeriodic() const { return this->IsPeriodic; }

  template <typename DeviceAdapter>
  using ConnectivityP2C = svtkm::exec::ConnectivityExtrude<DeviceAdapter>;
  template <typename DeviceAdapter>
  using ConnectivityC2P = svtkm::exec::ReverseConnectivityExtrude<DeviceAdapter>;

  template <typename DeviceAdapter, typename VisitTopology, typename IncidentTopology>
  struct ExecutionTypes;

  template <typename DeviceAdapter>
  struct ExecutionTypes<DeviceAdapter, svtkm::TopologyElementTagCell, svtkm::TopologyElementTagPoint>
  {
    using ExecObjectType = ConnectivityP2C<DeviceAdapter>;
  };

  template <typename DeviceAdapter>
  struct ExecutionTypes<DeviceAdapter, svtkm::TopologyElementTagPoint, svtkm::TopologyElementTagCell>
  {
    using ExecObjectType = ConnectivityC2P<DeviceAdapter>;
  };

  template <typename Device>
  ConnectivityP2C<Device> PrepareForInput(Device,
                                          svtkm::TopologyElementTagCell,
                                          svtkm::TopologyElementTagPoint) const;

  template <typename Device>
  ConnectivityC2P<Device> PrepareForInput(Device,
                                          svtkm::TopologyElementTagPoint,
                                          svtkm::TopologyElementTagCell) const;

private:
  template <typename Device>
  void BuildReverseConnectivity(Device);

  bool IsPeriodic;

  svtkm::Int32 NumberOfPointsPerPlane;
  svtkm::Int32 NumberOfCellsPerPlane;
  svtkm::Int32 NumberOfPlanes;
  svtkm::cont::ArrayHandle<svtkm::Int32> Connectivity;
  svtkm::cont::ArrayHandle<svtkm::Int32> NextNode;

  bool ReverseConnectivityBuilt;
  svtkm::cont::ArrayHandle<svtkm::Int32> RConnectivity;
  svtkm::cont::ArrayHandle<svtkm::Int32> ROffsets;
  svtkm::cont::ArrayHandle<svtkm::Int32> RCounts;
  svtkm::cont::ArrayHandle<svtkm::Int32> PrevNode;
};

template <typename T>
CellSetExtrude make_CellSetExtrude(const svtkm::cont::ArrayHandle<svtkm::Int32>& conn,
                                   const svtkm::cont::ArrayHandleExtrudeCoords<T>& coords,
                                   const svtkm::cont::ArrayHandle<svtkm::Int32>& nextNode,
                                   bool periodic = true)
{
  return CellSetExtrude{
    conn, coords.GetNumberOfPointsPerPlane(), coords.GetNumberOfPlanes(), nextNode, periodic
  };
}

template <typename T>
CellSetExtrude make_CellSetExtrude(const std::vector<svtkm::Int32>& conn,
                                   const svtkm::cont::ArrayHandleExtrudeCoords<T>& coords,
                                   const std::vector<svtkm::Int32>& nextNode,
                                   bool periodic = true)
{
  return CellSetExtrude{ svtkm::cont::make_ArrayHandle(conn),
                         static_cast<svtkm::Int32>(coords.GetNumberOfPointsPerPlane()),
                         coords.GetNumberOfPlanes(),
                         svtkm::cont::make_ArrayHandle(nextNode),
                         periodic };
}
}
} // svtkm::cont


//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace svtkm
{
namespace cont
{

template <>
struct SerializableTypeString<svtkm::cont::CellSetExtrude>
{
  static SVTKM_CONT const std::string& Get()
  {
    static std::string name = "CS_Extrude";
    return name;
  }
};
}
} // svtkm::cont

namespace mangled_diy_namespace
{

template <>
struct Serialization<svtkm::cont::CellSetExtrude>
{
private:
  using Type = svtkm::cont::CellSetExtrude;

public:
  static SVTKM_CONT void save(BinaryBuffer& bb, const Type& cs)
  {
    svtkmdiy::save(bb, cs.GetNumberOfPointsPerPlane());
    svtkmdiy::save(bb, cs.GetNumberOfPlanes());
    svtkmdiy::save(bb, cs.GetIsPeriodic());
    svtkmdiy::save(bb, cs.GetConnectivityArray());
    svtkmdiy::save(bb, cs.GetNextNodeArray());
  }

  static SVTKM_CONT void load(BinaryBuffer& bb, Type& cs)
  {
    svtkm::Int32 numberOfPointsPerPlane;
    svtkm::Int32 numberOfPlanes;
    bool isPeriodic;
    svtkm::cont::ArrayHandle<svtkm::Int32> conn;
    svtkm::cont::ArrayHandle<svtkm::Int32> nextNode;

    svtkmdiy::load(bb, numberOfPointsPerPlane);
    svtkmdiy::load(bb, numberOfPlanes);
    svtkmdiy::load(bb, isPeriodic);
    svtkmdiy::load(bb, conn);
    svtkmdiy::load(bb, nextNode);

    cs = Type{ conn, numberOfPointsPerPlane, numberOfPlanes, nextNode, isPeriodic };
  }
};

} // diy
/// @endcond SERIALIZATION

#include <svtkm/cont/CellSetExtrude.hxx>

#endif // svtk_m_cont_CellSetExtrude.h
