//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_CellSetStructured_hxx
#define svtk_m_cont_CellSetStructured_hxx

namespace svtkm
{
namespace cont
{

template <svtkm::IdComponent DIMENSION>
template <typename TopologyElement>
typename CellSetStructured<DIMENSION>::SchedulingRangeType
  CellSetStructured<DIMENSION>::GetSchedulingRange(TopologyElement) const
{
  SVTKM_IS_TOPOLOGY_ELEMENT_TAG(TopologyElement);
  return this->Structure.GetSchedulingRange(TopologyElement());
}

template <svtkm::IdComponent DIMENSION>
template <typename DeviceAdapter, typename VisitTopology, typename IncidentTopology>
typename CellSetStructured<DIMENSION>::template ExecutionTypes<DeviceAdapter,
                                                               VisitTopology,
                                                               IncidentTopology>::ExecObjectType
  CellSetStructured<DIMENSION>::PrepareForInput(DeviceAdapter,
                                                VisitTopology,
                                                IncidentTopology) const
{
  using ConnectivityType =
    typename ExecutionTypes<DeviceAdapter, VisitTopology, IncidentTopology>::ExecObjectType;
  return ConnectivityType(this->Structure);
}

template <svtkm::IdComponent DIMENSION>
void CellSetStructured<DIMENSION>::PrintSummary(std::ostream& out) const
{
  out << "  StructuredCellSet: " << std::endl;
  this->Structure.PrintSummary(out);
}
}
}
#endif
