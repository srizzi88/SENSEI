//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_arg_TransportTagTopologyFieldIn_h
#define svtk_m_cont_arg_TransportTagTopologyFieldIn_h

#include <svtkm/TopologyElementTag.h>
#include <svtkm/Types.h>

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/CellSet.h>

#include <svtkm/cont/arg/Transport.h>

namespace svtkm
{
namespace cont
{
namespace arg
{

/// \brief \c Transport tag for input arrays in topology maps.
///
/// \c TransportTagTopologyFieldIn is a tag used with the \c Transport class to
/// transport \c ArrayHandle objects for input data. The transport is templated
/// on a topology element tag and expects a cell set input domain to check the
/// size of the input array.
///
template <typename TopologyElementTag>
struct TransportTagTopologyFieldIn
{
};

namespace detail
{

SVTKM_CONT
inline static svtkm::Id TopologyDomainSize(const svtkm::cont::CellSet& cellSet,
                                          svtkm::TopologyElementTagPoint)
{
  return cellSet.GetNumberOfPoints();
}

SVTKM_CONT
inline static svtkm::Id TopologyDomainSize(const svtkm::cont::CellSet& cellSet,
                                          svtkm::TopologyElementTagCell)
{
  return cellSet.GetNumberOfCells();
}

SVTKM_CONT
inline static svtkm::Id TopologyDomainSize(const svtkm::cont::CellSet& cellSet,
                                          svtkm::TopologyElementTagFace)
{
  return cellSet.GetNumberOfFaces();
}

SVTKM_CONT
inline static svtkm::Id TopologyDomainSize(const svtkm::cont::CellSet& cellSet,
                                          svtkm::TopologyElementTagEdge)
{
  return cellSet.GetNumberOfEdges();
}

} // namespace detail

template <typename TopologyElementTag, typename ContObjectType, typename Device>
struct Transport<svtkm::cont::arg::TransportTagTopologyFieldIn<TopologyElementTag>,
                 ContObjectType,
                 Device>
{
  SVTKM_IS_ARRAY_HANDLE(ContObjectType);


  using ExecObjectType = decltype(std::declval<ContObjectType>().PrepareForInput(Device()));

  SVTKM_CONT
  ExecObjectType operator()(const ContObjectType& object,
                            const svtkm::cont::CellSet& inputDomain,
                            svtkm::Id,
                            svtkm::Id) const
  {
    if (object.GetNumberOfValues() != detail::TopologyDomainSize(inputDomain, TopologyElementTag()))
    {
      throw svtkm::cont::ErrorBadValue("Input array to worklet invocation the wrong size.");
    }

    return object.PrepareForInput(Device());
  }
};
}
}
} // namespace svtkm::cont::arg

#endif //svtk_m_cont_arg_TransportTagTopologyFieldIn_h
