//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#define svtkm_filter_Contour_cxx
#include <svtkm/filter/Contour.h>

namespace svtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
SVTKM_FILTER_EXPORT Contour::Contour()
  : svtkm::filter::FilterDataSetWithField<Contour>()
  , IsoValues()
  , GenerateNormals(false)
  , AddInterpolationEdgeIds(false)
  , ComputeFastNormalsForStructured(false)
  , ComputeFastNormalsForUnstructured(true)
  , NormalArrayName("normals")
  , InterpolationEdgeIdsArrayName("edgeIds")
  , Worklet()
{
  // todo: keep an instance of marching cubes worklet as a member variable
}

//-----------------------------------------------------------------------------
SVTKM_FILTER_EXPORT void Contour::SetNumberOfIsoValues(svtkm::Id num)
{
  if (num >= 0)
  {
    this->IsoValues.resize(static_cast<std::size_t>(num));
  }
}

//-----------------------------------------------------------------------------
SVTKM_FILTER_EXPORT svtkm::Id Contour::GetNumberOfIsoValues() const
{
  return static_cast<svtkm::Id>(this->IsoValues.size());
}

//-----------------------------------------------------------------------------
SVTKM_FILTER_EXPORT void Contour::SetIsoValue(svtkm::Id index, svtkm::Float64 v)
{
  std::size_t i = static_cast<std::size_t>(index);
  if (i >= this->IsoValues.size())
  {
    this->IsoValues.resize(i + 1);
  }
  this->IsoValues[i] = v;
}

//-----------------------------------------------------------------------------
SVTKM_FILTER_EXPORT void Contour::SetIsoValues(const std::vector<svtkm::Float64>& values)
{
  this->IsoValues = values;
}

//-----------------------------------------------------------------------------
SVTKM_FILTER_EXPORT svtkm::Float64 Contour::GetIsoValue(svtkm::Id index) const
{
  return this->IsoValues[static_cast<std::size_t>(index)];
}

//-----------------------------------------------------------------------------
SVTKM_FILTER_INSTANTIATE_EXECUTE_METHOD(Contour);
}
}
