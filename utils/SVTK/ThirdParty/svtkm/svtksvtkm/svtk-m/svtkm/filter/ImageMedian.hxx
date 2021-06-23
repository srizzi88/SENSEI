//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_ImageMedian_hxx
#define svtk_m_filter_ImageMedian_hxx

#include <svtkm/Swap.h>

namespace svtkm
{
namespace worklet
{

// An implementation of the quickselect/Hoare's selection algorithm to find medians
// inplace, generally fairly fast for reasonable sized data.
//
template <typename T>
SVTKM_EXEC inline T find_median(T* values, std::size_t mid, std::size_t size)
{
  std::size_t begin = 0;
  std::size_t end = size - 1;
  while (begin < end)
  {
    T x = values[mid];
    std::size_t i = begin;
    std::size_t j = end;
    do
    {
      for (; values[i] < x; i++)
      {
      }
      for (; x < values[j]; j--)
      {
      }
      if (i <= j)
      {
        svtkm::Swap(values[i], values[j]);
        i++;
        j--;
      }
    } while (i <= j);

    begin = (j < mid) ? i : begin;
    end = (mid < i) ? j : end;
  }
  return values[mid];
}
struct ImageMedian : public svtkm::worklet::WorkletPointNeighborhood
{
  int Neighborhood;
  ImageMedian(int neighborhoodSize)
    : Neighborhood(neighborhoodSize)
  {
  }
  using ControlSignature = void(CellSetIn, FieldInNeighborhood, FieldOut);
  using ExecutionSignature = void(_2, _3);

  template <typename InNeighborhoodT, typename T>
  SVTKM_EXEC void operator()(const InNeighborhoodT& input, T& out) const
  {
    svtkm::Vec<T, 25> values;
    int index = 0;
    for (int x = -this->Neighborhood; x <= this->Neighborhood; ++x)
    {
      for (int y = -this->Neighborhood; y <= this->Neighborhood; ++y)
      {
        values[index++] = input.Get(x, y, 0);
      }
    }

    std::size_t len =
      static_cast<std::size_t>((this->Neighborhood * 2 + 1) * (this->Neighborhood * 2 + 1));
    std::size_t mid = len / 2;
    out = find_median(&values[0], mid, len);
  }
};
}

namespace filter
{

SVTKM_CONT ImageMedian::ImageMedian()
{
  this->SetOutputFieldName("median");
}

template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet ImageMedian::DoExecute(
  const svtkm::cont::DataSet& input,
  const svtkm::cont::ArrayHandle<T, StorageType>& field,
  const svtkm::filter::FieldMetadata& fieldMetadata,
  const svtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  if (!fieldMetadata.IsPointField())
  {
    throw svtkm::cont::ErrorBadValue("Active field for ImageMedian must be a point field.");
  }

  const svtkm::cont::DynamicCellSet& cells = input.GetCellSet();
  svtkm::cont::ArrayHandle<T> result;
  if (this->Neighborhood == 1 || this->Neighborhood == 2)
  {
    this->Invoke(worklet::ImageMedian{ this->Neighborhood },
                 svtkm::filter::ApplyPolicyCellSetStructured(cells, policy),
                 field,
                 result);
  }
  else
  {
    throw svtkm::cont::ErrorBadValue("ImageMedian only support a 3x3 or 5x5 stencil.");
  }

  std::string name = this->GetOutputFieldName();
  if (name.empty())
  {
    name = fieldMetadata.GetName();
  }

  return CreateResult(input, fieldMetadata.AsField(name, result));
}
}
}

#endif
