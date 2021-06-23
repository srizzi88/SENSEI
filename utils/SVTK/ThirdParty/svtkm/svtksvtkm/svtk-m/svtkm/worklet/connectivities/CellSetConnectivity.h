//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_connectivity_CellSetConnectivity_h
#define svtk_m_worklet_connectivity_CellSetConnectivity_h

#include <svtkm/worklet/connectivities/CellSetDualGraph.h>
#include <svtkm/worklet/connectivities/GraphConnectivity.h>

namespace svtkm
{
namespace worklet
{
namespace connectivity
{

class CellSetConnectivity
{
public:
  template <typename CellSetType>
  void Run(const CellSetType& cellSet, svtkm::cont::ArrayHandle<svtkm::Id>& componentArray) const
  {
    svtkm::cont::ArrayHandle<svtkm::Id> numIndicesArray;
    svtkm::cont::ArrayHandle<svtkm::Id> indexOffsetsArray;
    svtkm::cont::ArrayHandle<svtkm::Id> connectivityArray;

    // create cell to cell connectivity graph (dual graph)
    CellSetDualGraph().Run(cellSet, numIndicesArray, indexOffsetsArray, connectivityArray);
    // find the connected component of the dual graph
    GraphConnectivity().Run(numIndicesArray, indexOffsetsArray, connectivityArray, componentArray);
  }
};
}
}
} // svtkm::worklet::connectivity

#endif // svtk_m_worklet_connectivity_CellSetConnectivity_h
