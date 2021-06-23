//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_arg_TransportTagCellSetIn_h
#define svtk_m_cont_arg_TransportTagCellSetIn_h

#include <svtkm/Types.h>

#include <svtkm/cont/CellSet.h>

#include <svtkm/cont/arg/Transport.h>

namespace svtkm
{
namespace cont
{
namespace arg
{

/// \brief \c Transport tag for input arrays.
///
/// \c TransportTagCellSetIn is a tag used with the \c Transport class to
/// transport topology objects for input data.
///
template <typename VisitTopology, typename IncidentTopology>
struct TransportTagCellSetIn
{
};

template <typename VisitTopology,
          typename IncidentTopology,
          typename ContObjectType,
          typename Device>
struct Transport<svtkm::cont::arg::TransportTagCellSetIn<VisitTopology, IncidentTopology>,
                 ContObjectType,
                 Device>
{
  SVTKM_IS_CELL_SET(ContObjectType);

  using ExecObjectType = decltype(
    std::declval<ContObjectType>().PrepareForInput(Device(), VisitTopology(), IncidentTopology()));

  template <typename InputDomainType>
  SVTKM_CONT ExecObjectType
  operator()(const ContObjectType& object, const InputDomainType&, svtkm::Id, svtkm::Id) const
  {
    return object.PrepareForInput(Device(), VisitTopology(), IncidentTopology());
  }
};
}
}
} // namespace svtkm::cont::arg

#endif //svtk_m_cont_arg_TransportTagCellSetIn_h
