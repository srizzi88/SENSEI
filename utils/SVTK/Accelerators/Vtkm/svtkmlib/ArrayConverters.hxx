//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================

#include "ArrayConverters.h"

#include <svtkm/cont/ArrayHandleGroupVecVariable.h>

#include "svtkDataArray.h"
#include "svtkDataObject.h"

namespace tosvtkm
{

template <typename DataArrayType>
svtkm::cont::VariantArrayHandle svtkDataArrayToVariantArrayHandle(DataArrayType* input)
{
  int numComps = input->GetNumberOfComponents();
  switch (numComps)
  {
    case 1:
      return svtkm::cont::VariantArrayHandle(DataArrayToArrayHandle<DataArrayType, 1>::Wrap(input));
    case 2:
      return svtkm::cont::VariantArrayHandle(DataArrayToArrayHandle<DataArrayType, 2>::Wrap(input));
    case 3:
      return svtkm::cont::VariantArrayHandle(DataArrayToArrayHandle<DataArrayType, 3>::Wrap(input));
    case 4:
      return svtkm::cont::VariantArrayHandle(DataArrayToArrayHandle<DataArrayType, 4>::Wrap(input));
    case 6:
      return svtkm::cont::VariantArrayHandle(DataArrayToArrayHandle<DataArrayType, 6>::Wrap(input));
    case 9:
      return svtkm::cont::VariantArrayHandle(DataArrayToArrayHandle<DataArrayType, 9>::Wrap(input));
    default:
    {
      svtkm::Id numTuples = input->GetNumberOfTuples();
      auto subHandle = DataArrayToArrayHandle<DataArrayType, 1>::Wrap(input);
      auto offsets =
        svtkm::cont::ArrayHandleCounting<svtkm::Id>(svtkm::Id(0), svtkm::Id(numComps), numTuples);
      auto handle = svtkm::cont::make_ArrayHandleGroupVecVariable(subHandle, offsets);
      return svtkm::cont::VariantArrayHandle(handle);
    }
  }
}

template <typename DataArrayType>
svtkm::cont::Field ConvertPointField(DataArrayType* input)
{
  auto vhandle = svtkDataArrayToVariantArrayHandle(input);
  return svtkm::cont::make_FieldPoint(input->GetName(), vhandle);
}

template <typename DataArrayType>
svtkm::cont::Field ConvertCellField(DataArrayType* input)
{
  auto vhandle = svtkDataArrayToVariantArrayHandle(input);
  return svtkm::cont::make_FieldCell(input->GetName(), vhandle);
}

template <typename DataArrayType>
svtkm::cont::Field Convert(DataArrayType* input, int association)
{
  // we need to switch on if we are a cell or point field first!
  // The problem is that the constructor signature for fields differ based
  // on if they are a cell or point field.
  if (association == svtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    return ConvertPointField(input);
  }
  else if (association == svtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    return ConvertCellField(input);
  }

  return svtkm::cont::Field();
}

#if !defined(svtkmlib_ArrayConverterExport_cxx)
#define SVTK_EXPORT_ARRAY_CONVERSION_TO_SVTKM_DETAIL(ArrayType, ValueType)                           \
  extern template svtkm::cont::Field Convert<ArrayType<ValueType> >(                                \
    ArrayType<ValueType> * input, int association);
#else
#define SVTK_EXPORT_ARRAY_CONVERSION_TO_SVTKM_DETAIL(ArrayType, ValueType)                           \
  template svtkm::cont::Field Convert<ArrayType<ValueType> >(                                       \
    ArrayType<ValueType> * input, int association);
#endif

#define SVTK_EXPORT_SIGNED_ARRAY_CONVERSION_TO_SVTKM(ArrayType)                                      \
  SVTK_EXPORT_ARRAY_CONVERSION_TO_SVTKM_DETAIL(ArrayType, char)                                      \
  SVTK_EXPORT_ARRAY_CONVERSION_TO_SVTKM_DETAIL(ArrayType, int)                                       \
  SVTK_EXPORT_ARRAY_CONVERSION_TO_SVTKM_DETAIL(ArrayType, long)                                      \
  SVTK_EXPORT_ARRAY_CONVERSION_TO_SVTKM_DETAIL(ArrayType, long long)                                 \
  SVTK_EXPORT_ARRAY_CONVERSION_TO_SVTKM_DETAIL(ArrayType, short)                                     \
  SVTK_EXPORT_ARRAY_CONVERSION_TO_SVTKM_DETAIL(ArrayType, signed char)

#define SVTK_EXPORT_UNSIGNED_ARRAY_CONVERSION_TO_SVTKM(ArrayType)                                    \
  SVTK_EXPORT_ARRAY_CONVERSION_TO_SVTKM_DETAIL(ArrayType, unsigned char)                             \
  SVTK_EXPORT_ARRAY_CONVERSION_TO_SVTKM_DETAIL(ArrayType, unsigned int)                              \
  SVTK_EXPORT_ARRAY_CONVERSION_TO_SVTKM_DETAIL(ArrayType, unsigned long)                             \
  SVTK_EXPORT_ARRAY_CONVERSION_TO_SVTKM_DETAIL(ArrayType, unsigned long long)                        \
  SVTK_EXPORT_ARRAY_CONVERSION_TO_SVTKM_DETAIL(ArrayType, unsigned short)

#define SVTK_EXPORT_REAL_ARRAY_CONVERSION_TO_SVTKM(ArrayType)                                        \
  SVTK_EXPORT_ARRAY_CONVERSION_TO_SVTKM_DETAIL(ArrayType, double)                                    \
  SVTK_EXPORT_ARRAY_CONVERSION_TO_SVTKM_DETAIL(ArrayType, float)

#if !defined(svtkmlib_ArrayConverterExport_cxx)
#define SVTK_EXPORT_ARRAY_CONVERSION_TO_SVTKM(ArrayType)                                             \
  SVTK_EXPORT_SIGNED_ARRAY_CONVERSION_TO_SVTKM(ArrayType)                                            \
  SVTK_EXPORT_UNSIGNED_ARRAY_CONVERSION_TO_SVTKM(ArrayType)                                          \
  SVTK_EXPORT_REAL_ARRAY_CONVERSION_TO_SVTKM(ArrayType)

SVTK_EXPORT_ARRAY_CONVERSION_TO_SVTKM(svtkAOSDataArrayTemplate)
SVTK_EXPORT_ARRAY_CONVERSION_TO_SVTKM(svtkSOADataArrayTemplate)

#undef SVTK_EXPORT_ARRAY_CONVERSION_TO_SVTKM
#undef SVTK_EXPORT_ARRAY_CONVERSION_TO_SVTKM_DETAIL

#endif // !defined(ArrayConverterExport_cxx)

} // tosvtkm
