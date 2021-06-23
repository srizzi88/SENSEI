//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_WorkletMapTopology_h
#define svtk_m_worklet_WorkletMapTopology_h

#include <svtkm/worklet/internal/WorkletBase.h>

#include <svtkm/TopologyElementTag.h>

#include <svtkm/cont/arg/ControlSignatureTagBase.h>
#include <svtkm/cont/arg/TransportTagArrayInOut.h>
#include <svtkm/cont/arg/TransportTagArrayOut.h>
#include <svtkm/cont/arg/TransportTagCellSetIn.h>
#include <svtkm/cont/arg/TransportTagTopologyFieldIn.h>
#include <svtkm/cont/arg/TypeCheckTagArray.h>
#include <svtkm/cont/arg/TypeCheckTagCellSet.h>

#include <svtkm/exec/arg/CellShape.h>
#include <svtkm/exec/arg/FetchTagArrayDirectIn.h>
#include <svtkm/exec/arg/FetchTagArrayDirectInOut.h>
#include <svtkm/exec/arg/FetchTagArrayDirectOut.h>
#include <svtkm/exec/arg/FetchTagArrayTopologyMapIn.h>
#include <svtkm/exec/arg/FetchTagCellSetIn.h>
#include <svtkm/exec/arg/IncidentElementCount.h>
#include <svtkm/exec/arg/IncidentElementIndices.h>
#include <svtkm/exec/arg/ThreadIndicesTopologyMap.h>

namespace svtkm
{
namespace worklet
{

template <typename WorkletType>
class DispatcherMapTopology;

namespace detail
{

struct WorkletMapTopologyBase : svtkm::worklet::internal::WorkletBase
{
  template <typename Worklet>
  using Dispatcher = svtkm::worklet::DispatcherMapTopology<Worklet>;
};

} // namespace detail

/// @brief Base class for worklets that map topology elements onto each other.
///
/// The template parameters for this class must be members of the
/// TopologyElementTag group. The VisitTopology indicates the elements of a
/// cellset that will be visited, and the IncidentTopology will be mapped onto
/// the VisitTopology.
///
/// For instance,
/// `WorkletMapTopology<TopologyElementTagPoint, TopologyElementCell>` will
/// execute one instance per point, and provides convenience methods for
/// gathering information about the cells incident to the current point.
///
template <typename VisitTopology, typename IncidentTopology>
class WorkletMapTopology : public detail::WorkletMapTopologyBase
{
public:
  using VisitTopologyType = VisitTopology;
  using IncidentTopologyType = IncidentTopology;

  /// \brief A control signature tag for input fields from the \em visited
  /// topology.
  ///
  struct FieldInVisit : svtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = svtkm::cont::arg::TypeCheckTagArray;
    using TransportTag = svtkm::cont::arg::TransportTagTopologyFieldIn<VisitTopologyType>;
    using FetchTag = svtkm::exec::arg::FetchTagArrayDirectIn;
  };

  /// \brief A control signature tag for input fields from the \em incident
  /// topology.
  ///
  struct FieldInIncident : svtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = svtkm::cont::arg::TypeCheckTagArray;
    using TransportTag = svtkm::cont::arg::TransportTagTopologyFieldIn<IncidentTopologyType>;
    using FetchTag = svtkm::exec::arg::FetchTagArrayTopologyMapIn;
  };

  /// \brief A control signature tag for output fields.
  ///
  /// This tag takes a template argument that is a type list tag that limits
  /// the possible value types in the array.
  ///
  struct FieldOut : svtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = svtkm::cont::arg::TypeCheckTagArray;
    using TransportTag = svtkm::cont::arg::TransportTagArrayOut;
    using FetchTag = svtkm::exec::arg::FetchTagArrayDirectOut;
  };

  /// \brief A control signature tag for input-output (in-place) fields from
  /// the visited topology.
  ///
  struct FieldInOut : svtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = svtkm::cont::arg::TypeCheckTagArray;
    using TransportTag = svtkm::cont::arg::TransportTagArrayInOut;
    using FetchTag = svtkm::exec::arg::FetchTagArrayDirectInOut;
  };

  /// \brief A control signature tag for input connectivity.
  ///
  struct CellSetIn : svtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = svtkm::cont::arg::TypeCheckTagCellSet;
    using TransportTag =
      svtkm::cont::arg::TransportTagCellSetIn<VisitTopologyType, IncidentTopologyType>;
    using FetchTag = svtkm::exec::arg::FetchTagCellSetIn;
  };

  /// \brief An execution signature tag for getting the cell shape. This only
  /// makes sense when visiting cell topologies.
  ///
  struct CellShape : svtkm::exec::arg::CellShape
  {
  };

  /// \brief An execution signature tag to get the number of \em incident
  /// elements.
  ///
  /// In a topology map, there are \em visited and \em incident topology
  /// elements specified. The scheduling occurs on the \em visited elements,
  /// and for each \em visited element there is some number of incident \em
  /// mapped elements that are accessible. This \c ExecutionSignature tag
  /// provides the number of these \em mapped elements that are accessible.
  ///
  struct IncidentElementCount : svtkm::exec::arg::IncidentElementCount
  {
  };

  /// \brief An execution signature tag to get the indices of from elements.
  ///
  /// In a topology map, there are \em visited and \em incident topology
  /// elements specified. The scheduling occurs on the \em visited elements,
  /// and for each \em visited element there is some number of incident \em
  /// mapped elements that are accessible. This \c ExecutionSignature tag
  /// provides the indices of the \em mapped elements that are incident to the
  /// current \em visited element.
  ///
  struct IncidentElementIndices : svtkm::exec::arg::IncidentElementIndices
  {
  };

  /// Topology map worklets use topology map indices.
  ///
  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutArrayType,
            typename InputDomainType>
  SVTKM_EXEC svtkm::exec::arg::ThreadIndicesTopologyMap<InputDomainType> GetThreadIndices(
    svtkm::Id threadIndex,
    const OutToInArrayType& outToIn,
    const VisitArrayType& visit,
    const ThreadToOutArrayType& threadToOut,
    const InputDomainType& connectivity,
    svtkm::Id globalThreadIndexOffset) const
  {
    const svtkm::Id outIndex = threadToOut.Get(threadIndex);
    return svtkm::exec::arg::ThreadIndicesTopologyMap<InputDomainType>(threadIndex,
                                                                      outToIn.Get(outIndex),
                                                                      visit.Get(outIndex),
                                                                      outIndex,
                                                                      connectivity,
                                                                      globalThreadIndexOffset);
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutArrayType,
            typename InputDomainType>
  SVTKM_EXEC svtkm::exec::arg::ThreadIndicesTopologyMap<InputDomainType> GetThreadIndices(
    const svtkm::Id3& threadIndex,
    const OutToInArrayType& svtkmNotUsed(outToIn),
    const VisitArrayType& svtkmNotUsed(visit),
    const ThreadToOutArrayType& svtkmNotUsed(threadToOut),
    const InputDomainType& connectivity,
    svtkm::Id globalThreadIndexOffset = 0) const
  {
    using ScatterCheck = std::is_same<ScatterType, svtkm::worklet::ScatterIdentity>;
    SVTKM_STATIC_ASSERT_MSG(ScatterCheck::value,
                           "Scheduling on 3D topologies only works with default ScatterIdentity.");
    using MaskCheck = std::is_same<MaskType, svtkm::worklet::MaskNone>;
    SVTKM_STATIC_ASSERT_MSG(MaskCheck::value,
                           "Scheduling on 3D topologies only works with default MaskNone.");

    return svtkm::exec::arg::ThreadIndicesTopologyMap<InputDomainType>(
      threadIndex, connectivity, globalThreadIndexOffset);
  }
};

/// Base class for worklets that map from Points to Cells.
///
class WorkletVisitCellsWithPoints
  : public WorkletMapTopology<svtkm::TopologyElementTagCell, svtkm::TopologyElementTagPoint>
{
public:
  using FieldInPoint = FieldInIncident;

  using FieldInCell = FieldInVisit;

  using FieldOutCell = FieldOut;

  using FieldInOutCell = FieldInOut;

  using PointCount = IncidentElementCount;

  using PointIndices = IncidentElementIndices;
};

/// Base class for worklets that map from Cells to Points.
///
class WorkletVisitPointsWithCells
  : public WorkletMapTopology<svtkm::TopologyElementTagPoint, svtkm::TopologyElementTagCell>
{
public:
  using FieldInCell = FieldInIncident;

  using FieldInPoint = FieldInVisit;

  using FieldOutPoint = FieldOut;

  using FieldInOutPoint = FieldInOut;

  using CellCount = IncidentElementCount;

  using CellIndices = IncidentElementIndices;
};

// Deprecated signatures for legacy support. These will be removed at some
// point.
using WorkletMapCellToPoint = WorkletVisitPointsWithCells;
using WorkletMapPointToCell = WorkletVisitCellsWithPoints;
}
} // namespace svtkm::worklet

#endif //svtk_m_worklet_WorkletMapTopology_h
