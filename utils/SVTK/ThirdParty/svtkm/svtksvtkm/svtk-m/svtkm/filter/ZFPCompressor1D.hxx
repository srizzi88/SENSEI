//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_filter_ZFPCompressor1D_hxx
#define svtk_m_filter_ZFPCompressor1D_hxx

#include <svtkm/cont/CellSetStructured.h>
#include <svtkm/cont/DynamicCellSet.h>
#include <svtkm/cont/ErrorFilterExecution.h>

namespace svtkm
{
namespace filter
{

namespace
{

template <typename CellSetList>
bool IsCellSetStructured(const svtkm::cont::DynamicCellSetBase<CellSetList>& cellset)
{
  if (cellset.template IsType<svtkm::cont::CellSetStructured<1>>())

  {
    return true;
  }
  return false;
}
} // anonymous namespace

//-----------------------------------------------------------------------------
inline SVTKM_CONT ZFPCompressor1D::ZFPCompressor1D()
  : svtkm::filter::FilterField<ZFPCompressor1D>()
  , rate(0)
{
}


//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet ZFPCompressor1D::DoExecute(
  const svtkm::cont::DataSet&,
  const svtkm::cont::ArrayHandle<T, StorageType>& field,
  const svtkm::filter::FieldMetadata&,
  const svtkm::filter::PolicyBase<DerivedPolicy>&)
{
  auto compressed = compressor.Compress(field, rate, field.GetNumberOfValues());

  svtkm::cont::DataSet dataset;
  dataset.AddField(svtkm::cont::make_FieldPoint("compressed", compressed));
  return dataset;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT bool ZFPCompressor1D::DoMapField(svtkm::cont::DataSet&,
                                                  const svtkm::cont::ArrayHandle<T, StorageType>&,
                                                  const svtkm::filter::FieldMetadata&,
                                                  const svtkm::filter::PolicyBase<DerivedPolicy>&)
{
  return false;
}
}
} // namespace svtkm::filter
#endif
