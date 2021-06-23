//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_ExtractGeometry_hxx
#define svtk_m_filter_ExtractGeometry_hxx

#include <svtkm/cont/ArrayHandlePermutation.h>
#include <svtkm/cont/CellSetPermutation.h>
#include <svtkm/cont/CoordinateSystem.h>
#include <svtkm/cont/DynamicCellSet.h>

namespace
{

struct CallWorker
{
  svtkm::cont::DynamicCellSet& Output;
  svtkm::worklet::ExtractGeometry& Worklet;
  const svtkm::cont::CoordinateSystem& Coords;
  const svtkm::cont::ImplicitFunctionHandle& Function;
  bool ExtractInside;
  bool ExtractBoundaryCells;
  bool ExtractOnlyBoundaryCells;

  CallWorker(svtkm::cont::DynamicCellSet& output,
             svtkm::worklet::ExtractGeometry& worklet,
             const svtkm::cont::CoordinateSystem& coords,
             const svtkm::cont::ImplicitFunctionHandle& function,
             bool extractInside,
             bool extractBoundaryCells,
             bool extractOnlyBoundaryCells)
    : Output(output)
    , Worklet(worklet)
    , Coords(coords)
    , Function(function)
    , ExtractInside(extractInside)
    , ExtractBoundaryCells(extractBoundaryCells)
    , ExtractOnlyBoundaryCells(extractOnlyBoundaryCells)
  {
  }

  template <typename CellSetType>
  void operator()(const CellSetType& cellSet) const
  {
    this->Output = this->Worklet.Run(cellSet,
                                     this->Coords,
                                     this->Function,
                                     this->ExtractInside,
                                     this->ExtractBoundaryCells,
                                     this->ExtractOnlyBoundaryCells);
  }
};

} // end anon namespace

namespace svtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline SVTKM_CONT ExtractGeometry::ExtractGeometry()
  : svtkm::filter::FilterDataSet<ExtractGeometry>()
  , ExtractInside(true)
  , ExtractBoundaryCells(false)
  , ExtractOnlyBoundaryCells(false)
{
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet ExtractGeometry::DoExecute(
  const svtkm::cont::DataSet& input,
  const svtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  // extract the input cell set and coordinates
  const svtkm::cont::DynamicCellSet& cells = input.GetCellSet();
  const svtkm::cont::CoordinateSystem& coords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  svtkm::cont::DynamicCellSet outCells;
  CallWorker worker(outCells,
                    this->Worklet,
                    coords,
                    this->Function,
                    this->ExtractInside,
                    this->ExtractBoundaryCells,
                    this->ExtractOnlyBoundaryCells);
  svtkm::filter::ApplyPolicyCellSet(cells, policy).CastAndCall(worker);

  // create the output dataset
  svtkm::cont::DataSet output;
  output.AddCoordinateSystem(input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()));
  output.SetCellSet(outCells);
  return output;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT bool ExtractGeometry::DoMapField(
  svtkm::cont::DataSet& result,
  const svtkm::cont::ArrayHandle<T, StorageType>& input,
  const svtkm::filter::FieldMetadata& fieldMeta,
  const svtkm::filter::PolicyBase<DerivedPolicy>&)
{
  svtkm::cont::VariantArrayHandle output;

  if (fieldMeta.IsPointField())
  {
    output = input; // pass through, points aren't changed.
  }
  else if (fieldMeta.IsCellField())
  {
    output = this->Worklet.ProcessCellField(input);
  }
  else
  {
    return false;
  }

  // use the same meta data as the input so we get the same field name, etc.
  result.AddField(fieldMeta.AsField(output));
  return true;
}
}
}

#endif
