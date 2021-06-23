//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_filter_CreateResult_h
#define svtk_m_filter_CreateResult_h

#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/ErrorBadValue.h>
#include <svtkm/cont/Field.h>

#include <svtkm/filter/FieldMetadata.h>

namespace svtkm
{
namespace filter
{
//@{
/// These are utility functions defined to use in filters when creating an
/// output dataset to return from `DoExecute` methods. The various overloads
/// provides different ways of creating the output dataset (copying the input
/// without any of the fields) and adding additional field(s).

/// Use this if you have built a \c Field object. An output
/// \c DataSet will be created by adding the field to the input.
inline SVTKM_CONT svtkm::cont::DataSet CreateResult(const svtkm::cont::DataSet& inDataSet,
                                                  const svtkm::cont::Field& field)
{
  svtkm::cont::DataSet clone;
  clone.CopyStructure(inDataSet);
  clone.AddField(field);
  SVTKM_ASSERT(!field.GetName().empty());
  SVTKM_ASSERT(clone.HasField(field.GetName(), field.GetAssociation()));
  return clone;
}

/// Use this function if you have an ArrayHandle that holds the data for
/// the field. You also need to specify a name for the field.
template <typename T, typename Storage>
inline SVTKM_CONT svtkm::cont::DataSet CreateResult(
  const svtkm::cont::DataSet& inDataSet,
  const svtkm::cont::ArrayHandle<T, Storage>& fieldArray,
  const std::string& fieldName,
  const svtkm::filter::FieldMetadata& metaData)
{
  SVTKM_ASSERT(!fieldName.empty());

  svtkm::cont::DataSet clone;
  clone.CopyStructure(inDataSet);
  clone.AddField(metaData.AsField(fieldName, fieldArray));

  // Sanity check.
  SVTKM_ASSERT(clone.HasField(fieldName, metaData.GetAssociation()));
  return clone;
}

/// Use this function if you have a VariantArrayHandle that holds the data
/// for the field.
inline SVTKM_CONT svtkm::cont::DataSet CreateResult(const svtkm::cont::DataSet& inDataSet,
                                                  const svtkm::cont::VariantArrayHandle& fieldArray,
                                                  const std::string& fieldName,
                                                  const svtkm::filter::FieldMetadata& metaData)
{
  SVTKM_ASSERT(!fieldName.empty());

  svtkm::cont::DataSet clone;
  clone.CopyStructure(inDataSet);
  clone.AddField(metaData.AsField(fieldName, fieldArray));

  // Sanity check.
  SVTKM_ASSERT(clone.HasField(fieldName, metaData.GetAssociation()));
  return clone;
}

/// Use this function if you want to explicit construct a Cell field and have a ArrayHandle
/// that holds the data for the field.
template <typename T, typename Storage>
inline SVTKM_CONT svtkm::cont::DataSet CreateResultFieldCell(
  const svtkm::cont::DataSet& inDataSet,
  const svtkm::cont::ArrayHandle<T, Storage>& fieldArray,
  const std::string& fieldName)
{
  SVTKM_ASSERT(!fieldName.empty());

  svtkm::cont::DataSet clone;
  clone.CopyStructure(inDataSet);
  clone.AddField(svtkm::cont::make_FieldCell(fieldName, fieldArray));

  // Sanity check.
  SVTKM_ASSERT(clone.HasCellField(fieldName));
  return clone;
}

/// Use this function if you want to explicit construct a Cell field and have a VariantArrayHandle
/// that holds the data for the field.
inline SVTKM_CONT svtkm::cont::DataSet CreateResultFieldCell(
  const svtkm::cont::DataSet& inDataSet,
  const svtkm::cont::VariantArrayHandle& fieldArray,
  const std::string& fieldName)
{
  SVTKM_ASSERT(!fieldName.empty());

  svtkm::cont::DataSet clone;
  clone.CopyStructure(inDataSet);
  clone.AddField(svtkm::cont::make_FieldCell(fieldName, fieldArray));

  // Sanity check.
  SVTKM_ASSERT(clone.HasCellField(fieldName));
  return clone;
}

/// Use this function if you want to explicit construct a Point field and have a ArrayHandle
/// that holds the data for the field.
template <typename T, typename Storage>
inline SVTKM_CONT svtkm::cont::DataSet CreateResultFieldPoint(
  const svtkm::cont::DataSet& inDataSet,
  const svtkm::cont::ArrayHandle<T, Storage>& fieldArray,
  const std::string& fieldName)
{
  SVTKM_ASSERT(!fieldName.empty());

  svtkm::cont::DataSet clone;
  clone.CopyStructure(inDataSet);
  clone.AddField(svtkm::cont::make_FieldPoint(fieldName, fieldArray));

  // Sanity check.
  SVTKM_ASSERT(clone.HasPointField(fieldName));
  return clone;
}

/// Use this function if you want to explicit construct a Point field and have a VariantArrayHandle
/// that holds the data for the field.
inline SVTKM_CONT svtkm::cont::DataSet CreateResultFieldPoint(
  const svtkm::cont::DataSet& inDataSet,
  const svtkm::cont::VariantArrayHandle& fieldArray,
  const std::string& fieldName)
{
  SVTKM_ASSERT(!fieldName.empty());

  svtkm::cont::DataSet clone;
  clone.CopyStructure(inDataSet);
  clone.AddField(svtkm::cont::make_FieldPoint(fieldName, fieldArray));

  // Sanity check.
  SVTKM_ASSERT(clone.HasPointField(fieldName));
  return clone;
}

//@}
} // namespace svtkm::filter
} // namespace svtkm

#endif
