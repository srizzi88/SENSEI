//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_filter_SurfaceNormals_hxx
#define svtk_m_filter_SurfaceNormals_hxx

#include <svtkm/cont/ErrorFilterExecution.h>

#include <svtkm/worklet/OrientNormals.h>
#include <svtkm/worklet/SurfaceNormals.h>
#include <svtkm/worklet/TriangleWinding.h>

namespace svtkm
{
namespace filter
{

namespace internal
{

inline std::string ComputePointNormalsName(const SurfaceNormals* filter)
{
  if (!filter->GetPointNormalsName().empty())
  {
    return filter->GetPointNormalsName();
  }
  else if (!filter->GetOutputFieldName().empty())
  {
    return filter->GetOutputFieldName();
  }
  else
  {
    return "Normals";
  }
}

inline std::string ComputeCellNormalsName(const SurfaceNormals* filter)
{
  if (!filter->GetCellNormalsName().empty())
  {
    return filter->GetCellNormalsName();
  }
  else if (!filter->GetGeneratePointNormals() && !filter->GetOutputFieldName().empty())
  {
    return filter->GetOutputFieldName();
  }
  else
  {
    return "Normals";
  }
}

} // internal

inline SurfaceNormals::SurfaceNormals()
  : GenerateCellNormals(false)
  , NormalizeCellNormals(true)
  , GeneratePointNormals(true)
  , AutoOrientNormals(false)
  , FlipNormals(false)
  , Consistency(true)
{
  this->SetUseCoordinateSystemAsField(true);
}

template <typename T, typename StorageType, typename DerivedPolicy>
inline svtkm::cont::DataSet SurfaceNormals::DoExecute(
  const svtkm::cont::DataSet& input,
  const svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>, StorageType>& points,
  const svtkm::filter::FieldMetadata& fieldMeta,
  svtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  SVTKM_ASSERT(fieldMeta.IsPointField());

  if (!this->GenerateCellNormals && !this->GeneratePointNormals)
  {
    throw svtkm::cont::ErrorFilterExecution("No normals selected.");
  }

  const auto cellset = svtkm::filter::ApplyPolicyCellSetUnstructured(input.GetCellSet(), policy);
  const auto& coords = input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()).GetData();

  svtkm::cont::ArrayHandle<svtkm::Vec3f> faceNormals;
  svtkm::worklet::FacetedSurfaceNormals faceted;
  faceted.SetNormalize(this->NormalizeCellNormals);
  faceted.Run(cellset, points, faceNormals);

  svtkm::cont::DataSet result;
  svtkm::cont::ArrayHandle<svtkm::Vec3f> pointNormals;
  if (this->GeneratePointNormals)
  {
    svtkm::worklet::SmoothSurfaceNormals smooth;
    smooth.Run(cellset, faceNormals, pointNormals);


    result = CreateResultFieldPoint(input, pointNormals, internal::ComputePointNormalsName(this));
    if (this->GenerateCellNormals)
    {
      result.AddField(
        svtkm::cont::make_FieldCell(internal::ComputeCellNormalsName(this), faceNormals));
    }
  }
  else
  {
    result = CreateResultFieldCell(input, faceNormals, internal::ComputeCellNormalsName(this));
  }

  if (this->AutoOrientNormals)
  {
    using Orient = svtkm::worklet::OrientNormals;

    if (this->GenerateCellNormals && this->GeneratePointNormals)
    {
      Orient::RunPointAndCellNormals(cellset, coords, pointNormals, faceNormals);
    }
    else if (this->GenerateCellNormals)
    {
      Orient::RunCellNormals(cellset, coords, faceNormals);
    }
    else if (this->GeneratePointNormals)
    {
      Orient::RunPointNormals(cellset, coords, pointNormals);
    }

    if (this->FlipNormals)
    {
      if (this->GenerateCellNormals)
      {
        Orient::RunFlipNormals(faceNormals);
      }
      if (this->GeneratePointNormals)
      {
        Orient::RunFlipNormals(pointNormals);
      }
    }
  }

  if (this->Consistency && this->GenerateCellNormals)
  {
    auto newCells = svtkm::worklet::TriangleWinding::Run(cellset, coords, faceNormals);
    result.SetCellSet(newCells); // Overwrite the cellset in the result
  }

  return result;
}
}
} // svtkm::filter
#endif
