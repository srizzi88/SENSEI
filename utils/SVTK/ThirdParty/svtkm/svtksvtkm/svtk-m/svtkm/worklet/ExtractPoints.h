//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtkm_m_worklet_ExtractPoints_h
#define svtkm_m_worklet_ExtractPoints_h

#include <svtkm/worklet/DispatcherMapTopology.h>
#include <svtkm/worklet/WorkletMapTopology.h>

#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/CoordinateSystem.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/ImplicitFunctionHandle.h>

namespace svtkm
{
namespace worklet
{

class ExtractPoints
{
public:
  ////////////////////////////////////////////////////////////////////////////////////
  // Worklet to identify points within volume of interest
  class ExtractPointsByVOI : public svtkm::worklet::WorkletVisitPointsWithCells
  {
  public:
    using ControlSignature = void(CellSetIn cellset,
                                  FieldInPoint coordinates,
                                  ExecObject function,
                                  FieldOutPoint passFlags);
    using ExecutionSignature = _4(_2, _3);

    SVTKM_CONT
    ExtractPointsByVOI(bool extractInside)
      : passValue(extractInside)
      , failValue(!extractInside)
    {
    }

    SVTKM_EXEC
    bool operator()(const svtkm::Vec3f_64& coordinate, const svtkm::ImplicitFunction* function) const
    {
      bool pass = passValue;
      svtkm::Float64 value = function->Value(coordinate);
      if (value > 0)
      {
        pass = failValue;
      }
      return pass;
    }

  private:
    bool passValue;
    bool failValue;
  };

  ////////////////////////////////////////////////////////////////////////////////////
  // Extract points by id creates new cellset of vertex cells
  template <typename CellSetType>
  svtkm::cont::CellSetSingleType<> Run(const CellSetType& cellSet,
                                      const svtkm::cont::ArrayHandle<svtkm::Id>& pointIds)
  {
    svtkm::cont::ArrayCopy(pointIds, this->ValidPointIds);

    // Make CellSetSingleType with VERTEX at each point id
    svtkm::cont::CellSetSingleType<> outCellSet;
    outCellSet.Fill(
      cellSet.GetNumberOfPoints(), svtkm::CellShapeTagVertex::Id, 1, this->ValidPointIds);

    return outCellSet;
  }

  ////////////////////////////////////////////////////////////////////////////////////
  // Extract points by implicit function
  template <typename CellSetType, typename CoordinateType>
  svtkm::cont::CellSetSingleType<> Run(const CellSetType& cellSet,
                                      const CoordinateType& coordinates,
                                      const svtkm::cont::ImplicitFunctionHandle& implicitFunction,
                                      bool extractInside)
  {
    // Worklet output will be a boolean passFlag array
    svtkm::cont::ArrayHandle<bool> passFlags;

    ExtractPointsByVOI worklet(extractInside);
    DispatcherMapTopology<ExtractPointsByVOI> dispatcher(worklet);
    dispatcher.Invoke(cellSet, coordinates, implicitFunction, passFlags);

    svtkm::cont::ArrayHandleCounting<svtkm::Id> indices =
      svtkm::cont::make_ArrayHandleCounting(svtkm::Id(0), svtkm::Id(1), passFlags.GetNumberOfValues());
    svtkm::cont::Algorithm::CopyIf(indices, passFlags, this->ValidPointIds);

    // Make CellSetSingleType with VERTEX at each point id
    svtkm::cont::CellSetSingleType<> outCellSet;
    outCellSet.Fill(
      cellSet.GetNumberOfPoints(), svtkm::CellShapeTagVertex::Id, 1, this->ValidPointIds);

    return outCellSet;
  }

private:
  svtkm::cont::ArrayHandle<svtkm::Id> ValidPointIds;
};
}
} // namespace svtkm::worklet

#endif // svtkm_m_worklet_ExtractPoints_h
