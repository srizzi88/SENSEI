//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_Gradient_hxx
#define svtk_m_filter_Gradient_hxx

#include <svtkm/cont/DynamicCellSet.h>
#include <svtkm/cont/ErrorFilterExecution.h>

#include <svtkm/worklet/Gradient.h>

namespace
{
//-----------------------------------------------------------------------------
template <typename T, typename S>
inline void transpose_3x3(svtkm::cont::ArrayHandle<svtkm::Vec<svtkm::Vec<T, 3>, 3>, S>& field)
{
  svtkm::worklet::gradient::Transpose3x3<T> transpose;
  transpose.Run(field);
}

//-----------------------------------------------------------------------------
template <typename T, typename S>
inline void transpose_3x3(svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>, S>&)
{ //This is not a 3x3 matrix so no transpose needed
}

} //namespace

namespace svtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline svtkm::cont::DataSet Gradient::DoExecute(
  const svtkm::cont::DataSet& input,
  const svtkm::cont::ArrayHandle<T, StorageType>& inField,
  const svtkm::filter::FieldMetadata& fieldMetadata,
  const svtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  if (!fieldMetadata.IsPointField())
  {
    throw svtkm::cont::ErrorFilterExecution("Point field expected.");
  }

  const svtkm::cont::DynamicCellSet& cells = input.GetCellSet();
  const svtkm::cont::CoordinateSystem& coords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  std::string outputName = this->GetOutputFieldName();
  if (outputName.empty())
  {
    outputName = this->GradientsName;
  }

  //todo: we need to ask the policy what storage type we should be using
  //If the input is implicit, we should know what to fall back to
  svtkm::worklet::GradientOutputFields<T> gradientfields(this->GetComputeGradient(),
                                                        this->GetComputeDivergence(),
                                                        this->GetComputeVorticity(),
                                                        this->GetComputeQCriterion());
  svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>> outArray;
  if (this->ComputePointGradient)
  {
    svtkm::worklet::PointGradient gradient;
    outArray = gradient.Run(
      svtkm::filter::ApplyPolicyCellSet(cells, policy), coords, inField, gradientfields);
  }
  else
  {
    svtkm::worklet::CellGradient gradient;
    outArray = gradient.Run(
      svtkm::filter::ApplyPolicyCellSet(cells, policy), coords, inField, gradientfields);
  }
  if (!this->RowOrdering)
  {
    transpose_3x3(outArray);
  }

  constexpr bool isVector = std::is_same<typename svtkm::VecTraits<T>::HasMultipleComponents,
                                         svtkm::VecTraitsTagMultipleComponents>::value;

  svtkm::cont::Field::Association fieldAssociation(this->ComputePointGradient
                                                    ? svtkm::cont::Field::Association::POINTS
                                                    : svtkm::cont::Field::Association::CELL_SET);
  svtkm::cont::DataSet result;
  result.CopyStructure(input);
  result.AddField(svtkm::cont::Field{ outputName, fieldAssociation, outArray });

  if (this->GetComputeDivergence() && isVector)
  {
    result.AddField(
      svtkm::cont::Field{ this->GetDivergenceName(), fieldAssociation, gradientfields.Divergence });
  }
  if (this->GetComputeVorticity() && isVector)
  {
    result.AddField(
      svtkm::cont::Field{ this->GetVorticityName(), fieldAssociation, gradientfields.Vorticity });
  }
  if (this->GetComputeQCriterion() && isVector)
  {
    result.AddField(
      svtkm::cont::Field{ this->GetQCriterionName(), fieldAssociation, gradientfields.QCriterion });
  }
  return result;
}
}
} // namespace svtkm::filter

#endif
