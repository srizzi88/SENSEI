//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_FieldMetadata_h
#define svtk_m_filter_FieldMetadata_h

#include <svtkm/cont/CoordinateSystem.h>
#include <svtkm/cont/Field.h>

namespace svtkm
{
namespace filter
{

class FieldMetadata
{
public:
  SVTKM_CONT
  FieldMetadata()
    : Name()
    , Association(svtkm::cont::Field::Association::ANY)
  {
  }

  SVTKM_CONT
  FieldMetadata(const svtkm::cont::Field& f)
    : Name(f.GetName())
    , Association(f.GetAssociation())
  {
  }

  SVTKM_CONT
  FieldMetadata(const svtkm::cont::CoordinateSystem& sys)
    : Name(sys.GetName())
    , Association(sys.GetAssociation())
  {
  }

  SVTKM_CONT
  bool IsPointField() const { return this->Association == svtkm::cont::Field::Association::POINTS; }

  SVTKM_CONT
  bool IsCellField() const { return this->Association == svtkm::cont::Field::Association::CELL_SET; }

  SVTKM_CONT
  const std::string& GetName() const { return this->Name; }

  SVTKM_CONT
  svtkm::cont::Field::Association GetAssociation() const { return this->Association; }

  /// Construct a new field with the same association as stored in this FieldMetaData
  /// but with a new name
  template <typename T, typename StorageTag>
  SVTKM_CONT svtkm::cont::Field AsField(const std::string& name,
                                      const svtkm::cont::ArrayHandle<T, StorageTag>& handle) const
  {
    return svtkm::cont::Field(name, this->Association, handle);
  }
  /// Construct a new field with the same association as stored in this FieldMetaData
  /// but with a new name
  SVTKM_CONT
  svtkm::cont::Field AsField(const std::string& name,
                            const svtkm::cont::VariantArrayHandle& handle) const
  {
    return svtkm::cont::Field(name, this->Association, handle);
  }

  /// Construct a new field with the same association and name as stored in this FieldMetaData
  template <typename T, typename StorageTag>
  SVTKM_CONT svtkm::cont::Field AsField(const svtkm::cont::ArrayHandle<T, StorageTag>& handle) const
  {
    return this->AsField(this->Name, handle);
  }
  /// Construct a new field with the same association and name as stored in this FieldMetaData
  SVTKM_CONT svtkm::cont::Field AsField(const svtkm::cont::VariantArrayHandle& handle) const
  {
    return this->AsField(this->Name, handle);
  }

private:
  std::string Name; ///< name of field
  svtkm::cont::Field::Association Association;
};
}
}

#endif //svtk_m_filter_FieldMetadata_h
