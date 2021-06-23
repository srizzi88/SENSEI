//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_filter_ZFPCompressor3D_hxx
#define svtk_m_filter_ZFPCompressor3D_hxx

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
bool IsCellSet3DStructured(const svtkm::cont::DynamicCellSetBase<CellSetList>& cellset)
{
  if (cellset.template IsType<svtkm::cont::CellSetStructured<3>>())
  {
    return true;
  }
  return false;
}
} // anonymous namespace

//-----------------------------------------------------------------------------
inline SVTKM_CONT ZFPCompressor3D::ZFPCompressor3D()
  : svtkm::filter::FilterField<ZFPCompressor3D>()
  , rate(0)
{
}


//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet ZFPCompressor3D::DoExecute(
  const svtkm::cont::DataSet& input,
  const svtkm::cont::ArrayHandle<T, StorageType>& field,
  const svtkm::filter::FieldMetadata&,
  const svtkm::filter::PolicyBase<DerivedPolicy>&)
{
  svtkm::cont::CellSetStructured<3> cellSet;
  input.GetCellSet().CopyTo(cellSet);
  svtkm::Id3 pointDimensions = cellSet.GetPointDimensions();


  auto compressed = compressor.Compress(field, rate, pointDimensions);

  svtkm::cont::DataSet dataset;
  dataset.SetCellSet(cellSet);
  dataset.AddField(svtkm::cont::make_FieldPoint("compressed", compressed));

  return dataset;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT bool ZFPCompressor3D::DoMapField(svtkm::cont::DataSet&,
                                                  const svtkm::cont::ArrayHandle<T, StorageType>&,
                                                  const svtkm::filter::FieldMetadata&,
                                                  const svtkm::filter::PolicyBase<DerivedPolicy>&)
{
  return false;
}
}
} // namespace svtkm::filter
#endif
