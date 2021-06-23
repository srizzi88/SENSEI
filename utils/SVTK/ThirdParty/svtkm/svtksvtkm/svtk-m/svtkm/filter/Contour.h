//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_Contour_h
#define svtk_m_filter_Contour_h

#include <svtkm/filter/svtkm_filter_export.h>

#include <svtkm/filter/FilterDataSetWithField.h>
#include <svtkm/worklet/Contour.h>

namespace svtkm
{
namespace filter
{
/// \brief generate isosurface(s) from a Volume

/// Takes as input a volume (e.g., 3D structured point set) and generates on
/// output one or more isosurfaces.
/// Multiple contour values must be specified to generate the isosurfaces.
/// @warning
/// This filter is currently only supports 3D volumes.
class SVTKM_ALWAYS_EXPORT Contour : public svtkm::filter::FilterDataSetWithField<Contour>
{
public:
  using SupportedTypes = svtkm::List<svtkm::UInt8, svtkm::Int8, svtkm::Float32, svtkm::Float64>;

  SVTKM_FILTER_EXPORT
  Contour();

  SVTKM_FILTER_EXPORT
  void SetNumberOfIsoValues(svtkm::Id num);

  SVTKM_FILTER_EXPORT
  svtkm::Id GetNumberOfIsoValues() const;

  SVTKM_FILTER_EXPORT
  void SetIsoValue(svtkm::Float64 v) { this->SetIsoValue(0, v); }

  SVTKM_FILTER_EXPORT
  void SetIsoValue(svtkm::Id index, svtkm::Float64);

  SVTKM_FILTER_EXPORT
  void SetIsoValues(const std::vector<svtkm::Float64>& values);

  SVTKM_FILTER_EXPORT
  svtkm::Float64 GetIsoValue(svtkm::Id index) const;

  /// Set/Get whether the points generated should be unique for every triangle
  /// or will duplicate points be merged together. Duplicate points are identified
  /// by the unique edge it was generated from.
  ///
  SVTKM_CONT
  void SetMergeDuplicatePoints(bool on) { this->Worklet.SetMergeDuplicatePoints(on); }

  SVTKM_CONT
  bool GetMergeDuplicatePoints() const { return this->Worklet.GetMergeDuplicatePoints(); }

  /// Set/Get whether normals should be generated. Off by default. If enabled,
  /// the default behaviour is to generate high quality normals for structured
  /// datasets, using gradients, and generate fast normals for unstructured
  /// datasets based on the result triangle mesh.
  ///
  SVTKM_CONT
  void SetGenerateNormals(bool on) { this->GenerateNormals = on; }
  SVTKM_CONT
  bool GetGenerateNormals() const { return this->GenerateNormals; }

  /// Set/Get whether to append the ids of the intersected edges to the vertices of the isosurface triangles. Off by default.
  SVTKM_CONT
  void SetAddInterpolationEdgeIds(bool on) { this->AddInterpolationEdgeIds = on; }
  SVTKM_CONT
  bool GetAddInterpolationEdgeIds() const { return this->AddInterpolationEdgeIds; }

  /// Set/Get whether the fast path should be used for normals computation for
  /// structured datasets. Off by default.
  SVTKM_CONT
  void SetComputeFastNormalsForStructured(bool on) { this->ComputeFastNormalsForStructured = on; }
  SVTKM_CONT
  bool GetComputeFastNormalsForStructured() const { return this->ComputeFastNormalsForStructured; }

  /// Set/Get whether the fast path should be used for normals computation for
  /// unstructured datasets. On by default.
  SVTKM_CONT
  void SetComputeFastNormalsForUnstructured(bool on)
  {
    this->ComputeFastNormalsForUnstructured = on;
  }
  SVTKM_CONT
  bool GetComputeFastNormalsForUnstructured() const
  {
    return this->ComputeFastNormalsForUnstructured;
  }

  SVTKM_CONT
  void SetNormalArrayName(const std::string& name) { this->NormalArrayName = name; }

  SVTKM_CONT
  const std::string& GetNormalArrayName() const { return this->NormalArrayName; }

  template <typename T, typename StorageType, typename DerivedPolicy>
  svtkm::cont::DataSet DoExecute(const svtkm::cont::DataSet& input,
                                const svtkm::cont::ArrayHandle<T, StorageType>& field,
                                const svtkm::filter::FieldMetadata& fieldMeta,
                                svtkm::filter::PolicyBase<DerivedPolicy> policy);

  //Map a new field onto the resulting dataset after running the filter
  //this call is only valid
  template <typename T, typename StorageType, typename DerivedPolicy>
  SVTKM_CONT bool DoMapField(svtkm::cont::DataSet& result,
                            const svtkm::cont::ArrayHandle<T, StorageType>& input,
                            const svtkm::filter::FieldMetadata& fieldMeta,
                            svtkm::filter::PolicyBase<DerivedPolicy>)
  {
    svtkm::cont::ArrayHandle<T> fieldArray;

    if (fieldMeta.IsPointField())
    {
      fieldArray = this->Worklet.ProcessPointField(input);
    }
    else if (fieldMeta.IsCellField())
    {
      fieldArray = this->Worklet.ProcessCellField(input);
    }
    else
    {
      return false;
    }

    //use the same meta data as the input so we get the same field name, etc.
    result.AddField(fieldMeta.AsField(fieldArray));
    return true;
  }

private:
  std::vector<svtkm::Float64> IsoValues;
  bool GenerateNormals;
  bool AddInterpolationEdgeIds;
  bool ComputeFastNormalsForStructured;
  bool ComputeFastNormalsForUnstructured;
  std::string NormalArrayName;
  std::string InterpolationEdgeIdsArrayName;
  svtkm::worklet::Contour Worklet;
};

#ifndef svtkm_filter_Contour_cxx
SVTKM_FILTER_EXPORT_EXECUTE_METHOD(Contour);
#endif
}
} // namespace svtkm::filter

#include <svtkm/filter/Contour.hxx>

#endif // svtk_m_filter_Contour_h
