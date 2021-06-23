//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_FieldToColors_h
#define svtk_m_filter_FieldToColors_h

#include <svtkm/cont/ColorTable.h>
#include <svtkm/filter/FilterField.h>

namespace svtkm
{
namespace filter
{

/// \brief  Convert an arbitrary field to an RGB or RGBA field
///
class FieldToColors : public svtkm::filter::FilterField<FieldToColors>
{
public:
  SVTKM_CONT
  FieldToColors(const svtkm::cont::ColorTable& table = svtkm::cont::ColorTable());

  enum FieldToColorsInputMode
  {
    SCALAR,
    MAGNITUDE,
    COMPONENT
  };

  enum FieldToColorsOutputMode
  {
    RGB,
    RGBA
  };

  void SetColorTable(const svtkm::cont::ColorTable& table)
  {
    this->Table = table;
    this->ModifiedCount = -1;
  }
  const svtkm::cont::ColorTable& GetColorTable() const { return this->Table; }

  void SetMappingMode(FieldToColorsInputMode mode) { this->InputMode = mode; }
  void SetMappingToScalar() { this->InputMode = FieldToColorsInputMode::SCALAR; }
  void SetMappingToMagnitude() { this->InputMode = FieldToColorsInputMode::MAGNITUDE; }
  void SetMappingToComponent() { this->InputMode = FieldToColorsInputMode::COMPONENT; }
  FieldToColorsInputMode GetMappingMode() const { return this->InputMode; }
  bool IsMappingScalar() const { return this->InputMode == FieldToColorsInputMode::SCALAR; }
  bool IsMappingMagnitude() const { return this->InputMode == FieldToColorsInputMode::MAGNITUDE; }
  bool IsMappingComponent() const { return this->InputMode == FieldToColorsInputMode::COMPONENT; }

  void SetMappingComponent(svtkm::IdComponent comp) { this->Component = comp; }
  svtkm::IdComponent GetMappingComponent() const { return this->Component; }

  void SetOutputMode(FieldToColorsOutputMode mode) { this->OutputMode = mode; }
  void SetOutputToRGB() { this->OutputMode = FieldToColorsOutputMode::RGB; }
  void SetOutputToRGBA() { this->OutputMode = FieldToColorsOutputMode::RGBA; }
  FieldToColorsOutputMode GetOutputMode() const { return this->OutputMode; }
  bool IsOutputRGB() const { return this->OutputMode == FieldToColorsOutputMode::RGB; }
  bool IsOutputRGBA() const { return this->OutputMode == FieldToColorsOutputMode::RGBA; }


  void SetNumberOfSamplingPoints(svtkm::Int32 count);
  svtkm::Int32 GetNumberOfSamplingPoints() const { return this->SampleCount; }

  template <typename T, typename StorageType, typename DerivedPolicy>
  SVTKM_CONT svtkm::cont::DataSet DoExecute(const svtkm::cont::DataSet& input,
                                          const svtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const svtkm::filter::FieldMetadata& fieldMeta,
                                          svtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  svtkm::cont::ColorTable Table;
  FieldToColorsInputMode InputMode;
  FieldToColorsOutputMode OutputMode;
  svtkm::cont::ColorTableSamplesRGB SamplesRGB;
  svtkm::cont::ColorTableSamplesRGBA SamplesRGBA;
  svtkm::IdComponent Component;
  svtkm::Int32 SampleCount;
  svtkm::Id ModifiedCount;
};
}
} // namespace svtkm::filter

#include <svtkm/filter/FieldToColors.hxx>

#endif // svtk_m_filter_FieldToColors_h
