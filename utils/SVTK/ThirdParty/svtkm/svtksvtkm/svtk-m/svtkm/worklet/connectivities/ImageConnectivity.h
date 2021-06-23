//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_connectivity_ImageConnectivity_h
#define svtk_m_worklet_connectivity_ImageConnectivity_h

#include <svtkm/cont/Invoker.h>
#include <svtkm/worklet/WorkletMapField.h>
#include <svtkm/worklet/WorkletPointNeighborhood.h>

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

class ImageGraft : public svtkm::worklet::WorkletPointNeighborhood
{
public:
  using ControlSignature = void(CellSetIn,
                                FieldInNeighborhood compIn,
                                FieldInNeighborhood color,
                                WholeArrayInOut compOut,
                                AtomicArrayInOut changed);

  using ExecutionSignature = void(WorkIndex, _2, _3, _4, _5);

  template <typename Comp>
  SVTKM_EXEC svtkm::Id findRoot(Comp& comp, svtkm::Id index) const
  {
    while (comp.Get(index) != index)
      index = comp.Get(index);
    return index;
  }

  // compOut is an alias of compIn such that we can update component labels
  template <typename NeighborComp, typename NeighborColor, typename CompOut, typename AtomicInOut>
  SVTKM_EXEC void operator()(const svtkm::Id index,
                            const NeighborComp& neighborComp,
                            const NeighborColor& neighborColor,
                            CompOut& compOut,
                            AtomicInOut& updated) const
  {
    svtkm::Id myComp = neighborComp.Get(0, 0, 0);
    auto minComp = myComp;

    auto myColor = neighborColor.Get(0, 0, 0);

    for (int k = -1; k <= 1; k++)
    {
      for (int j = -1; j <= 1; j++)
      {
        for (int i = -1; i <= 1; i++)
        {
          if (myColor == neighborColor.Get(i, j, k))
          {
            minComp = svtkm::Min(minComp, neighborComp.Get(i, j, k));
          }
        }
      }
    }
    // I don't just only want to update the component label of this pixel, I actually
    // want to Union(FindRoot(myComponent), FindRoot(minComp)) and then Flatten the
    // result.
    compOut.Set(index, minComp);

    auto myRoot = findRoot(compOut, myComp);
    auto newRoot = findRoot(compOut, minComp);

    if (myRoot < newRoot)
      compOut.Set(newRoot, myRoot);
    else if (myRoot > newRoot)
      compOut.Set(myRoot, newRoot);

    // mark an update occurred if no other worklets have done so yet
    if (myComp != minComp && updated.Get(0) == 0)
    {
      updated.Set(0, 1);
    }
  }
};
}

class ImageConnectivity
{
public:
  class RunImpl
  {
  public:
    template <int Dimension, typename T, typename StorageT, typename OutputPortalType>
    void operator()(const svtkm::cont::ArrayHandle<T, StorageT>& pixels,
                    const svtkm::cont::CellSetStructured<Dimension>& input,
                    OutputPortalType& components) const
    {
      using Algorithm = svtkm::cont::Algorithm;

      Algorithm::Copy(svtkm::cont::ArrayHandleCounting<svtkm::Id>(0, 1, pixels.GetNumberOfValues()),
                      components);

      //used as an atomic bool, so we use Int32 as it the
      //smallest type that SVTK-m supports as atomics
      svtkm::cont::ArrayHandle<svtkm::Int32> updateRequired;
      updateRequired.Allocate(1);

      svtkm::cont::Invoker invoke;
      do
      {
        updateRequired.GetPortalControl().Set(0, 0); //reset the atomic state
        invoke(detail::ImageGraft{}, input, components, pixels, components, updateRequired);
        invoke(PointerJumping{}, components);
      } while (updateRequired.GetPortalControl().Get(0) > 0);

      // renumber connected component to the range of [0, number of components).
      svtkm::cont::ArrayHandle<svtkm::Id> uniqueComponents;
      Algorithm::Copy(components, uniqueComponents);
      Algorithm::Sort(uniqueComponents);
      Algorithm::Unique(uniqueComponents);

      svtkm::cont::ArrayHandle<svtkm::Id> pixelIds;
      Algorithm::Copy(svtkm::cont::ArrayHandleCounting<svtkm::Id>(0, 1, pixels.GetNumberOfValues()),
                      pixelIds);

      svtkm::cont::ArrayHandle<svtkm::Id> uniqueColor;
      Algorithm::Copy(
        svtkm::cont::ArrayHandleCounting<svtkm::Id>(0, 1, uniqueComponents.GetNumberOfValues()),
        uniqueColor);

      svtkm::cont::ArrayHandle<svtkm::Id> cellColors;
      svtkm::cont::ArrayHandle<svtkm::Id> pixelIdsOut;
      InnerJoin().Run(
        components, pixelIds, uniqueComponents, uniqueColor, cellColors, pixelIdsOut, components);

      Algorithm::SortByKey(pixelIdsOut, components);
    }
  };

  class ResolveDynamicCellSet
  {
  public:
    template <int Dimension, typename T, typename StorageT, typename OutputPortalType>
    void operator()(const svtkm::cont::CellSetStructured<Dimension>& input,
                    const svtkm::cont::ArrayHandle<T, StorageT>& pixels,
                    OutputPortalType& components) const
    {
      svtkm::cont::CastAndCall(pixels, RunImpl(), input, components);
    }
  };

  template <int Dimension, typename T, typename OutputPortalType>
  void Run(const svtkm::cont::CellSetStructured<Dimension>& input,
           const svtkm::cont::VariantArrayHandleBase<T>& pixels,
           OutputPortalType& componentsOut) const
  {
    using Types = svtkm::TypeListScalarAll;
    svtkm::cont::CastAndCall(pixels.ResetTypes(Types{}), RunImpl(), input, componentsOut);
  }

  template <int Dimension, typename T, typename S, typename OutputPortalType>
  void Run(const svtkm::cont::CellSetStructured<Dimension>& input,
           const svtkm::cont::ArrayHandle<T, S>& pixels,
           OutputPortalType& componentsOut) const
  {
    svtkm::cont::CastAndCall(pixels, RunImpl(), input, componentsOut);
  }

  template <typename CellSetTag, typename T, typename S, typename OutputPortalType>
  void Run(const svtkm::cont::DynamicCellSetBase<CellSetTag>& input,
           const svtkm::cont::ArrayHandle<T, S>& pixels,
           OutputPortalType& componentsOut) const
  {
    input.ResetCellSetList(svtkm::cont::CellSetListStructured())
      .CastAndCall(ResolveDynamicCellSet(), pixels, componentsOut);
  }
};
}
}
}

#endif // svtk_m_worklet_connectivity_ImageConnectivity_h
