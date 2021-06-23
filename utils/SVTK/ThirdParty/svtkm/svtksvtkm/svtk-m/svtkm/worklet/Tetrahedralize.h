//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtkm_m_worklet_Tetrahedralize_h
#define svtkm_m_worklet_Tetrahedralize_h

#include <svtkm/worklet/tetrahedralize/TetrahedralizeExplicit.h>
#include <svtkm/worklet/tetrahedralize/TetrahedralizeStructured.h>

namespace svtkm
{
namespace worklet
{

class Tetrahedralize
{
public:
  //
  // Distribute multiple copies of cell data depending on cells create from original
  //
  struct DistributeCellData : public svtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn inIndices, FieldOut outIndices);
    using ExecutionSignature = void(_1, _2);

    using ScatterType = svtkm::worklet::ScatterCounting;

    template <typename CountArrayType>
    SVTKM_CONT static ScatterType MakeScatter(const CountArrayType& countArray)
    {
      return ScatterType(countArray);
    }

    template <typename T>
    SVTKM_EXEC void operator()(T inputIndex, T& outputIndex) const
    {
      outputIndex = inputIndex;
    }
  };

  Tetrahedralize()
    : OutCellsPerCell()
  {
  }

  // Tetrahedralize explicit data set, save number of tetra cells per input
  template <typename CellSetType>
  svtkm::cont::CellSetSingleType<> Run(const CellSetType& cellSet)
  {
    TetrahedralizeExplicit worklet;
    return worklet.Run(cellSet, this->OutCellsPerCell);
  }

  // Tetrahedralize structured data set, save number of tetra cells per input
  svtkm::cont::CellSetSingleType<> Run(const svtkm::cont::CellSetStructured<3>& cellSet)
  {
    TetrahedralizeStructured worklet;
    return worklet.Run(cellSet, this->OutCellsPerCell);
  }

  svtkm::cont::CellSetSingleType<> Run(const svtkm::cont::CellSetStructured<2>&)
  {
    throw svtkm::cont::ErrorBadType("CellSetStructured<2> can't be tetrahedralized");
  }

  // Using the saved input to output cells, expand cell data
  template <typename T, typename StorageType>
  svtkm::cont::ArrayHandle<T> ProcessCellField(
    const svtkm::cont::ArrayHandle<T, StorageType>& input) const
  {
    svtkm::cont::ArrayHandle<T> output;

    svtkm::worklet::DispatcherMapField<DistributeCellData> dispatcher(
      DistributeCellData::MakeScatter(this->OutCellsPerCell));
    dispatcher.Invoke(input, output);

    return output;
  }

private:
  svtkm::cont::ArrayHandle<svtkm::IdComponent> OutCellsPerCell;
};
}
} // namespace svtkm::worklet

#endif // svtkm_m_worklet_Tetrahedralize_h
