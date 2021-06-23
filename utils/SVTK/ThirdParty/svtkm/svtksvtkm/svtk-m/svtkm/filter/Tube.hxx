//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_filter_Tube_hxx
#define svtk_m_filter_Tube_hxx

#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandleIndex.h>
#include <svtkm/cont/ErrorFilterExecution.h>

#include <svtkm/filter/PolicyBase.h>

namespace svtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline SVTKM_CONT Tube::Tube()
  : svtkm::filter::FilterDataSet<Tube>()
  , Worklet()
{
}

//-----------------------------------------------------------------------------
template <typename Policy>
inline SVTKM_CONT svtkm::cont::DataSet Tube::DoExecute(const svtkm::cont::DataSet& input,
                                                     svtkm::filter::PolicyBase<Policy> policy)
{
  this->Worklet.SetCapping(this->Capping);
  this->Worklet.SetNumberOfSides(this->NumberOfSides);
  this->Worklet.SetRadius(this->Radius);

  auto originalPoints = svtkm::filter::ApplyPolicyFieldOfType<svtkm::Vec3f>(
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()), policy, *this);
  svtkm::cont::ArrayHandle<svtkm::Vec3f> newPoints;
  svtkm::cont::CellSetSingleType<> newCells;
  this->Worklet.Run(originalPoints, input.GetCellSet(), newPoints, newCells);

  svtkm::cont::DataSet outData;
  svtkm::cont::CoordinateSystem outCoords("coordinates", newPoints);
  outData.SetCellSet(newCells);
  outData.AddCoordinateSystem(outCoords);
  return outData;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT bool Tube::DoMapField(svtkm::cont::DataSet& result,
                                       const svtkm::cont::ArrayHandle<T, StorageType>& input,
                                       const svtkm::filter::FieldMetadata& fieldMeta,
                                       svtkm::filter::PolicyBase<DerivedPolicy>)
{
  svtkm::cont::ArrayHandle<T> fieldArray;

  if (fieldMeta.IsPointField())
    fieldArray = this->Worklet.ProcessPointField(input);
  else if (fieldMeta.IsCellField())
    fieldArray = this->Worklet.ProcessCellField(input);
  else
    return false;

  //use the same meta data as the input so we get the same field name, etc.
  result.AddField(fieldMeta.AsField(fieldArray));
  return true;
}
}
} // namespace svtkm::filter
#endif
