//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_filter_ThresholdPoints_hxx
#define svtk_m_filter_ThresholdPoints_hxx

#include <svtkm/cont/ErrorFilterExecution.h>

namespace
{
// Predicate for values less than minimum
class ValuesBelow
{
public:
  SVTKM_CONT
  ValuesBelow(const svtkm::Float64& value)
    : Value(value)
  {
  }

  template <typename ScalarType>
  SVTKM_EXEC bool operator()(const ScalarType& value) const
  {
    return value <= static_cast<ScalarType>(this->Value);
  }

private:
  svtkm::Float64 Value;
};

// Predicate for values greater than maximum
class ValuesAbove
{
public:
  SVTKM_CONT
  ValuesAbove(const svtkm::Float64& value)
    : Value(value)
  {
  }

  template <typename ScalarType>
  SVTKM_EXEC bool operator()(const ScalarType& value) const
  {
    return value >= static_cast<ScalarType>(this->Value);
  }

private:
  svtkm::Float64 Value;
};

// Predicate for values between minimum and maximum

class ValuesBetween
{
public:
  SVTKM_CONT
  ValuesBetween(const svtkm::Float64& lower, const svtkm::Float64& upper)
    : Lower(lower)
    , Upper(upper)
  {
  }

  template <typename ScalarType>
  SVTKM_EXEC bool operator()(const ScalarType& value) const
  {
    return value >= static_cast<ScalarType>(this->Lower) &&
      value <= static_cast<ScalarType>(this->Upper);
  }

private:
  svtkm::Float64 Lower;
  svtkm::Float64 Upper;
};
}

namespace svtkm
{
namespace filter
{

const int THRESHOLD_BELOW = 0;
const int THRESHOLD_ABOVE = 1;
const int THRESHOLD_BETWEEN = 2;

//-----------------------------------------------------------------------------
inline SVTKM_CONT ThresholdPoints::ThresholdPoints()
  : svtkm::filter::FilterDataSetWithField<ThresholdPoints>()
  , LowerValue(0)
  , UpperValue(0)
  , ThresholdType(THRESHOLD_BETWEEN)
  , CompactPoints(false)
{
}

//-----------------------------------------------------------------------------
inline SVTKM_CONT void ThresholdPoints::SetThresholdBelow(const svtkm::Float64 value)
{
  this->SetLowerThreshold(value);
  this->SetUpperThreshold(value);
  this->ThresholdType = THRESHOLD_BELOW;
}

inline SVTKM_CONT void ThresholdPoints::SetThresholdAbove(const svtkm::Float64 value)
{
  this->SetLowerThreshold(value);
  this->SetUpperThreshold(value);
  this->ThresholdType = THRESHOLD_ABOVE;
}

inline SVTKM_CONT void ThresholdPoints::SetThresholdBetween(const svtkm::Float64 value1,
                                                           const svtkm::Float64 value2)
{
  this->SetLowerThreshold(value1);
  this->SetUpperThreshold(value2);
  this->ThresholdType = THRESHOLD_BETWEEN;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet ThresholdPoints::DoExecute(
  const svtkm::cont::DataSet& input,
  const svtkm::cont::ArrayHandle<T, StorageType>& field,
  const svtkm::filter::FieldMetadata& fieldMeta,
  svtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  // extract the input cell set
  const svtkm::cont::DynamicCellSet& cells = input.GetCellSet();

  // field to threshold on must be a point field
  if (fieldMeta.IsPointField() == false)
  {
    throw svtkm::cont::ErrorFilterExecution("Point field expected.");
  }

  // run the worklet on the cell set and input field
  svtkm::cont::CellSetSingleType<> outCellSet;
  svtkm::worklet::ThresholdPoints worklet;

  switch (this->ThresholdType)
  {
    case THRESHOLD_BELOW:
    {
      outCellSet = worklet.Run(svtkm::filter::ApplyPolicyCellSet(cells, policy),
                               field,
                               ValuesBelow(this->GetLowerThreshold()));
      break;
    }
    case THRESHOLD_ABOVE:
    {
      outCellSet = worklet.Run(svtkm::filter::ApplyPolicyCellSet(cells, policy),
                               field,
                               ValuesAbove(this->GetUpperThreshold()));
      break;
    }
    case THRESHOLD_BETWEEN:
    default:
    {
      outCellSet = worklet.Run(svtkm::filter::ApplyPolicyCellSet(cells, policy),
                               field,
                               ValuesBetween(this->GetLowerThreshold(), this->GetUpperThreshold()));
      break;
    }
  }

  // create the output dataset
  svtkm::cont::DataSet output;
  output.SetCellSet(outCellSet);
  output.AddCoordinateSystem(input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()));

  // compact the unused points in the output dataset
  if (this->CompactPoints)
  {
    this->Compactor.SetCompactPointFields(true);
    this->Compactor.SetMergePoints(true);
    return this->Compactor.Execute(output, PolicyDefault{});
  }
  else
  {
    return output;
  }
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT bool ThresholdPoints::DoMapField(
  svtkm::cont::DataSet& result,
  const svtkm::cont::ArrayHandle<T, StorageType>& input,
  const svtkm::filter::FieldMetadata& fieldMeta,
  svtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  // point data is copied as is because it was not collapsed
  if (fieldMeta.IsPointField())
  {
    if (this->CompactPoints)
    {
      return this->Compactor.DoMapField(result, input, fieldMeta, policy);
    }
    else
    {
      result.AddField(fieldMeta.AsField(input));
      return true;
    }
  }

  // cell data does not apply
  return false;
}
}
}
#endif
