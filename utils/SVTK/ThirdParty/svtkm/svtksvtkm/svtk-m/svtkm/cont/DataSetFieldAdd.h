//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_DataSetFieldAdd_h
#define svtk_m_cont_DataSetFieldAdd_h

#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/Field.h>

namespace svtkm
{
namespace cont
{

class DataSetFieldAdd
{
public:
  SVTKM_CONT
  DataSetFieldAdd() {}

  //Point centered fields.
  SVTKM_CONT
  static void AddPointField(svtkm::cont::DataSet& dataSet,
                            const std::string& fieldName,
                            const svtkm::cont::VariantArrayHandle& field)
  {
    dataSet.AddField(make_FieldPoint(fieldName, field));
  }

  template <typename T, typename Storage>
  SVTKM_CONT static void AddPointField(svtkm::cont::DataSet& dataSet,
                                      const std::string& fieldName,
                                      const svtkm::cont::ArrayHandle<T, Storage>& field)
  {
    dataSet.AddField(make_FieldPoint(fieldName, field));
  }

  template <typename T>
  SVTKM_CONT static void AddPointField(svtkm::cont::DataSet& dataSet,
                                      const std::string& fieldName,
                                      const std::vector<T>& field)
  {
    dataSet.AddField(
      make_Field(fieldName, svtkm::cont::Field::Association::POINTS, field, svtkm::CopyFlag::On));
  }

  template <typename T>
  SVTKM_CONT static void AddPointField(svtkm::cont::DataSet& dataSet,
                                      const std::string& fieldName,
                                      const T* field,
                                      const svtkm::Id& n)
  {
    dataSet.AddField(
      make_Field(fieldName, svtkm::cont::Field::Association::POINTS, field, n, svtkm::CopyFlag::On));
  }

  //Cell centered field
  SVTKM_CONT
  static void AddCellField(svtkm::cont::DataSet& dataSet,
                           const std::string& fieldName,
                           const svtkm::cont::VariantArrayHandle& field)
  {
    dataSet.AddField(make_FieldCell(fieldName, field));
  }

  template <typename T, typename Storage>
  SVTKM_CONT static void AddCellField(svtkm::cont::DataSet& dataSet,
                                     const std::string& fieldName,
                                     const svtkm::cont::ArrayHandle<T, Storage>& field)
  {
    dataSet.AddField(make_FieldCell(fieldName, field));
  }

  template <typename T>
  SVTKM_CONT static void AddCellField(svtkm::cont::DataSet& dataSet,
                                     const std::string& fieldName,
                                     const std::vector<T>& field)
  {
    dataSet.AddField(
      make_Field(fieldName, svtkm::cont::Field::Association::CELL_SET, field, svtkm::CopyFlag::On));
  }

  template <typename T>
  SVTKM_CONT static void AddCellField(svtkm::cont::DataSet& dataSet,
                                     const std::string& fieldName,
                                     const T* field,
                                     const svtkm::Id& n)
  {
    dataSet.AddField(make_Field(
      fieldName, svtkm::cont::Field::Association::CELL_SET, field, n, svtkm::CopyFlag::On));
  }
};
}
} //namespace svtkm::cont

#endif //svtk_m_cont_DataSetFieldAdd_h
