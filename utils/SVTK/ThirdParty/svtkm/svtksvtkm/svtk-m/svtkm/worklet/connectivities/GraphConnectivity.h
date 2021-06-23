//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_connectivity_graph_connectivity_h
#define svtk_m_worklet_connectivity_graph_connectivity_h

#include <svtkm/cont/Invoker.h>
#include <svtkm/worklet/connectivities/CellSetDualGraph.h>
#include <svtkm/worklet/connectivities/InnerJoin.h>
#include <svtkm/worklet/connectivities/UnionFind.h>

namespace svtkm
{
namespace worklet
{
namespace connectivity
{
namespace detail
{
class Graft : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn start,
                                FieldIn degree,
                                WholeArrayIn ids,
                                WholeArrayInOut comp);

  using ExecutionSignature = void(WorkIndex, _1, _2, _3, _4);

  using InputDomain = _1;

  // TODO: Use Scatter?
  template <typename InPortalType, typename InOutPortalType>
  SVTKM_EXEC void operator()(svtkm::Id index,
                            svtkm::Id start,
                            svtkm::Id degree,
                            const InPortalType& conn,
                            InOutPortalType& comp) const
  {
    for (svtkm::Id offset = start; offset < start + degree; offset++)
    {
      svtkm::Id neighbor = conn.Get(offset);
      if ((comp.Get(index) == comp.Get(comp.Get(index))) && (comp.Get(neighbor) < comp.Get(index)))
      {
        comp.Set(comp.Get(index), comp.Get(neighbor));
      }
    }
  }
};
}

class GraphConnectivity
{
public:
  using Algorithm = svtkm::cont::Algorithm;

  template <typename InputPortalType, typename OutputPortalType>
  void Run(const InputPortalType& numIndicesArray,
           const InputPortalType& indexOffsetsArray,
           const InputPortalType& connectivityArray,
           OutputPortalType& componentsOut) const
  {
    bool everythingIsAStar = false;
    svtkm::cont::ArrayHandle<svtkm::Id> components;
    Algorithm::Copy(
      svtkm::cont::ArrayHandleCounting<svtkm::Id>(0, 1, numIndicesArray.GetNumberOfValues()),
      components);

    //used as an atomic bool, so we use Int32 as it the
    //smallest type that SVTK-m supports as atomics
    svtkm::cont::ArrayHandle<svtkm::Int32> allStars;
    allStars.Allocate(1);

    svtkm::cont::Invoker invoke;

    do
    {
      allStars.GetPortalControl().Set(0, 1); //reset the atomic state
      invoke(detail::Graft{}, indexOffsetsArray, numIndicesArray, connectivityArray, components);

      // Detection of allStars has to come before pointer jumping. Don't try to rearrange it.
      invoke(IsStar{}, components, allStars);
      everythingIsAStar = (allStars.GetPortalControl().Get(0) == 1);
      invoke(PointerJumping{}, components);
    } while (!everythingIsAStar);

    // renumber connected component to the range of [0, number of components).
    svtkm::cont::ArrayHandle<svtkm::Id> uniqueComponents;
    Algorithm::Copy(components, uniqueComponents);
    Algorithm::Sort(uniqueComponents);
    Algorithm::Unique(uniqueComponents);

    svtkm::cont::ArrayHandle<svtkm::Id> cellIds;
    Algorithm::Copy(
      svtkm::cont::ArrayHandleCounting<svtkm::Id>(0, 1, numIndicesArray.GetNumberOfValues()),
      cellIds);

    svtkm::cont::ArrayHandle<svtkm::Id> uniqueColor;
    Algorithm::Copy(
      svtkm::cont::ArrayHandleCounting<svtkm::Id>(0, 1, uniqueComponents.GetNumberOfValues()),
      uniqueColor);
    svtkm::cont::ArrayHandle<svtkm::Id> cellColors;
    svtkm::cont::ArrayHandle<svtkm::Id> cellIdsOut;
    InnerJoin().Run(
      components, cellIds, uniqueComponents, uniqueColor, cellColors, cellIdsOut, componentsOut);

    Algorithm::SortByKey(cellIdsOut, componentsOut);
  }
};
}
}
}
#endif //svtk_m_worklet_connectivity_graph_connectivity_h
