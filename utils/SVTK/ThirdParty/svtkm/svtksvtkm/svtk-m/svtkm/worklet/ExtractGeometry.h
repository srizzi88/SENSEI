//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtkm_m_worklet_ExtractGeometry_h
#define svtkm_m_worklet_ExtractGeometry_h

#include <svtkm/worklet/DispatcherMapTopology.h>
#include <svtkm/worklet/WorkletMapTopology.h>

#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/CellSetPermutation.h>
#include <svtkm/cont/CoordinateSystem.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/ImplicitFunctionHandle.h>

namespace svtkm
{
namespace worklet
{

class ExtractGeometry
{
public:
  ////////////////////////////////////////////////////////////////////////////////////
  // Worklet to identify cells within volume of interest
  class ExtractCellsByVOI : public svtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn cellset,
                                  WholeArrayIn coordinates,
                                  ExecObject implicitFunction,
                                  FieldOutCell passFlags);
    using ExecutionSignature = _4(PointCount, PointIndices, _2, _3);

    ExtractCellsByVOI() = default;

    SVTKM_CONT
    ExtractCellsByVOI(bool extractInside, bool extractBoundaryCells, bool extractOnlyBoundaryCells)
      : ExtractInside(extractInside)
      , ExtractBoundaryCells(extractBoundaryCells)
      , ExtractOnlyBoundaryCells(extractOnlyBoundaryCells)
    {
    }

    template <typename ConnectivityInVec, typename InVecFieldPortalType>
    SVTKM_EXEC bool operator()(svtkm::Id numIndices,
                              const ConnectivityInVec& connectivityIn,
                              const InVecFieldPortalType& coordinates,
                              const svtkm::ImplicitFunction* function) const
    {
      // Count points inside/outside volume of interest
      svtkm::IdComponent inCnt = 0;
      svtkm::IdComponent outCnt = 0;
      svtkm::Id indx;
      for (indx = 0; indx < numIndices; indx++)
      {
        svtkm::Id ptId = connectivityIn[static_cast<svtkm::IdComponent>(indx)];
        svtkm::Vec<FloatDefault, 3> coordinate = coordinates.Get(ptId);
        svtkm::FloatDefault value = function->Value(coordinate);
        if (value <= 0)
          inCnt++;
        if (value >= 0)
          outCnt++;
      }

      // Decide if cell is extracted
      bool passFlag = false;
      if (inCnt == numIndices && ExtractInside && !ExtractOnlyBoundaryCells)
      {
        passFlag = true;
      }
      else if (outCnt == numIndices && !ExtractInside && !ExtractOnlyBoundaryCells)
      {
        passFlag = true;
      }
      else if (inCnt > 0 && outCnt > 0 && (ExtractBoundaryCells || ExtractOnlyBoundaryCells))
      {
        passFlag = true;
      }
      return passFlag;
    }

  private:
    bool ExtractInside;
    bool ExtractBoundaryCells;
    bool ExtractOnlyBoundaryCells;
  };

  class AddPermutationCellSet
  {
    svtkm::cont::DynamicCellSet* Output;
    svtkm::cont::ArrayHandle<svtkm::Id>* ValidIds;

  public:
    AddPermutationCellSet(svtkm::cont::DynamicCellSet& cellOut,
                          svtkm::cont::ArrayHandle<svtkm::Id>& validIds)
      : Output(&cellOut)
      , ValidIds(&validIds)
    {
    }

    template <typename CellSetType>
    void operator()(const CellSetType& cellset) const
    {
      svtkm::cont::CellSetPermutation<CellSetType> permCellSet(*this->ValidIds, cellset);
      *this->Output = permCellSet;
    }
  };

  ////////////////////////////////////////////////////////////////////////////////////
  // Extract cells by ids permutes input data
  template <typename CellSetType>
  svtkm::cont::CellSetPermutation<CellSetType> Run(const CellSetType& cellSet,
                                                  const svtkm::cont::ArrayHandle<svtkm::Id>& cellIds)
  {
    using OutputType = svtkm::cont::CellSetPermutation<CellSetType>;

    svtkm::cont::ArrayCopy(cellIds, this->ValidCellIds);

    return OutputType(this->ValidCellIds, cellSet);
  }

  ////////////////////////////////////////////////////////////////////////////////////
  // Extract cells by implicit function permutes input data
  template <typename CellSetType>
  svtkm::cont::CellSetPermutation<CellSetType> Run(
    const CellSetType& cellSet,
    const svtkm::cont::CoordinateSystem& coordinates,
    const svtkm::cont::ImplicitFunctionHandle& implicitFunction,
    bool extractInside,
    bool extractBoundaryCells,
    bool extractOnlyBoundaryCells)
  {
    // Worklet output will be a boolean passFlag array
    svtkm::cont::ArrayHandle<bool> passFlags;

    ExtractCellsByVOI worklet(extractInside, extractBoundaryCells, extractOnlyBoundaryCells);
    DispatcherMapTopology<ExtractCellsByVOI> dispatcher(worklet);
    dispatcher.Invoke(cellSet, coordinates, implicitFunction, passFlags);

    svtkm::cont::ArrayHandleCounting<svtkm::Id> indices =
      svtkm::cont::make_ArrayHandleCounting(svtkm::Id(0), svtkm::Id(1), passFlags.GetNumberOfValues());
    svtkm::cont::Algorithm::CopyIf(indices, passFlags, this->ValidCellIds);

    // generate the cellset
    return svtkm::cont::CellSetPermutation<CellSetType>(this->ValidCellIds, cellSet);
  }

  template <typename ValueType, typename StorageTagIn>
  svtkm::cont::ArrayHandle<ValueType> ProcessCellField(
    const svtkm::cont::ArrayHandle<ValueType, StorageTagIn>& input)
  {
    // Use a temporary permutation array to simplify the mapping:
    auto tmp = svtkm::cont::make_ArrayHandlePermutation(this->ValidCellIds, input);

    // Copy into an array with default storage:
    svtkm::cont::ArrayHandle<ValueType> result;
    svtkm::cont::ArrayCopy(tmp, result);

    return result;
  }

private:
  svtkm::cont::ArrayHandle<svtkm::Id> ValidCellIds;
};
}
} // namespace svtkm::worklet

#endif // svtkm_m_worklet_ExtractGeometry_h
