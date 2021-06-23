//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_filter_Contour_hxx
#define svtk_m_filter_Contour_hxx

#include <svtkm/cont/ArrayHandleIndex.h>
#include <svtkm/cont/CellSetSingleType.h>
#include <svtkm/cont/CellSetStructured.h>
#include <svtkm/cont/DynamicCellSet.h>
#include <svtkm/cont/ErrorFilterExecution.h>

#include <svtkm/worklet/SurfaceNormals.h>

namespace svtkm
{
namespace filter
{

namespace
{

template <typename CellSetList>
inline bool IsCellSetStructured(const svtkm::cont::DynamicCellSetBase<CellSetList>& cellset)
{
  if (cellset.template IsType<svtkm::cont::CellSetStructured<1>>() ||
      cellset.template IsType<svtkm::cont::CellSetStructured<2>>() ||
      cellset.template IsType<svtkm::cont::CellSetStructured<3>>())
  {
    return true;
  }
  return false;
}
} // anonymous namespace

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet Contour::DoExecute(
  const svtkm::cont::DataSet& input,
  const svtkm::cont::ArrayHandle<T, StorageType>& field,
  const svtkm::filter::FieldMetadata& fieldMeta,
  svtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  if (fieldMeta.IsPointField() == false)
  {
    throw svtkm::cont::ErrorFilterExecution("Point field expected.");
  }

  if (this->IsoValues.size() == 0)
  {
    throw svtkm::cont::ErrorFilterExecution("No iso-values provided.");
  }

  // Check the fields of the dataset to see what kinds of fields are present so
  // we can free the mapping arrays that won't be needed. A point field must
  // exist for this algorithm, so just check cells.
  const svtkm::Id numFields = input.GetNumberOfFields();
  bool hasCellFields = false;
  for (svtkm::Id fieldIdx = 0; fieldIdx < numFields && !hasCellFields; ++fieldIdx)
  {
    auto f = input.GetField(fieldIdx);
    hasCellFields = f.IsFieldCell();
  }

  //get the cells and coordinates of the dataset
  const svtkm::cont::DynamicCellSet& cells = input.GetCellSet();

  const svtkm::cont::CoordinateSystem& coords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  using Vec3HandleType = svtkm::cont::ArrayHandle<svtkm::Vec3f>;
  Vec3HandleType vertices;
  Vec3HandleType normals;

  svtkm::cont::DataSet output;
  svtkm::cont::CellSetSingleType<> outputCells;

  std::vector<T> ivalues(this->IsoValues.size());
  for (std::size_t i = 0; i < ivalues.size(); ++i)
  {
    ivalues[i] = static_cast<T>(this->IsoValues[i]);
  }

  //not sold on this as we have to generate more signatures for the
  //worklet with the design
  //But I think we should get this to compile before we tinker with
  //a more efficient api

  bool generateHighQualityNormals = IsCellSetStructured(cells)
    ? !this->ComputeFastNormalsForStructured
    : !this->ComputeFastNormalsForUnstructured;
  if (this->GenerateNormals && generateHighQualityNormals)
  {
    outputCells = this->Worklet.Run(&ivalues[0],
                                    static_cast<svtkm::Id>(ivalues.size()),
                                    svtkm::filter::ApplyPolicyCellSet(cells, policy),
                                    coords.GetData(),
                                    field,
                                    vertices,
                                    normals);
  }
  else
  {
    outputCells = this->Worklet.Run(&ivalues[0],
                                    static_cast<svtkm::Id>(ivalues.size()),
                                    svtkm::filter::ApplyPolicyCellSet(cells, policy),
                                    coords.GetData(),
                                    field,
                                    vertices);
  }

  if (this->GenerateNormals)
  {
    if (!generateHighQualityNormals)
    {
      Vec3HandleType faceNormals;
      svtkm::worklet::FacetedSurfaceNormals faceted;
      faceted.Run(outputCells, vertices, faceNormals);

      svtkm::worklet::SmoothSurfaceNormals smooth;
      smooth.Run(outputCells, faceNormals, normals);
    }

    output.AddField(svtkm::cont::make_FieldPoint(this->NormalArrayName, normals));
  }

  if (this->AddInterpolationEdgeIds)
  {
    svtkm::cont::Field interpolationEdgeIdsField(InterpolationEdgeIdsArrayName,
                                                svtkm::cont::Field::Association::POINTS,
                                                this->Worklet.GetInterpolationEdgeIds());
    output.AddField(interpolationEdgeIdsField);
  }

  //assign the connectivity to the cell set
  output.SetCellSet(outputCells);

  //add the coordinates to the output dataset
  svtkm::cont::CoordinateSystem outputCoords("coordinates", vertices);
  output.AddCoordinateSystem(outputCoords);

  if (!hasCellFields)
  {
    this->Worklet.ReleaseCellMapArrays();
  }

  return output;
}
}
} // namespace svtkm::filter

#endif
