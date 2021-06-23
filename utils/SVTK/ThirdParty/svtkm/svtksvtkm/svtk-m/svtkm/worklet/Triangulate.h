//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtkm_m_worklet_Triangulate_h
#define svtkm_m_worklet_Triangulate_h

#include <svtkm/worklet/triangulate/TriangulateExplicit.h>
#include <svtkm/worklet/triangulate/TriangulateStructured.h>

namespace svtkm
{
namespace worklet
{

class Triangulate
{
public:
  //
  // Distribute multiple copies of cell data depending on cells create from original
  //
  struct DistributeCellData : public svtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn inIndices, FieldOut outIndices);

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

  Triangulate()
    : OutCellsPerCell()
  {
  }

  // Triangulate explicit data set, save number of triangulated cells per input
  template <typename CellSetType>
  svtkm::cont::CellSetSingleType<> Run(const CellSetType& cellSet)
  {
    TriangulateExplicit worklet;
    return worklet.Run(cellSet, this->OutCellsPerCell);
  }

  // Triangulate structured data set, save number of triangulated cells per input
  svtkm::cont::CellSetSingleType<> Run(const svtkm::cont::CellSetStructured<2>& cellSet)
  {
    TriangulateStructured worklet;
    return worklet.Run(cellSet, this->OutCellsPerCell);
  }

  svtkm::cont::CellSetSingleType<> Run(const svtkm::cont::CellSetStructured<3>&)
  {
    throw svtkm::cont::ErrorBadType("CellSetStructured<3> can't be triangulated");
  }

  // Using the saved input to output cells, expand cell data
  template <typename ValueType, typename StorageType>
  svtkm::cont::ArrayHandle<ValueType> ProcessCellField(
    const svtkm::cont::ArrayHandle<ValueType, StorageType>& input) const
  {
    svtkm::cont::ArrayHandle<ValueType> output;

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

#endif // svtkm_m_worklet_Triangulate_h
