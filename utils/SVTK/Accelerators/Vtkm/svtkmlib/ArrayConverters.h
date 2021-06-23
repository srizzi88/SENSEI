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

#ifndef svtkmlib_ArrayConverters_h
#define svtkmlib_ArrayConverters_h

#include "svtkAcceleratorsSVTKmModule.h" //required for correct implementation
#include "svtkmConfig.h"                //required for general svtkm setup

#include "svtkAOSDataArrayTemplate.h"
#include "svtkSOADataArrayTemplate.h"

#include <svtkm/cont/ArrayHandleSOA.h>
#include <svtkm/cont/Field.h>

#include <type_traits> // for std::underlying_type

class svtkDataArray;
class svtkDataSet;
class svtkPoints;

namespace svtkm
{
namespace cont
{
class DataSet;
class CoordinateSystem;
}
}

namespace tosvtkm
{

template <typename DataArrayType, svtkm::IdComponent NumComponents>
struct DataArrayToArrayHandle;

template <typename T, svtkm::IdComponent NumComponents>
struct DataArrayToArrayHandle<svtkAOSDataArrayTemplate<T>, NumComponents>
{
  using ValueType =
    typename std::conditional<NumComponents == 1, T, svtkm::Vec<T, NumComponents> >::type;
  using StorageType = svtkm::cont::internal::Storage<ValueType, svtkm::cont::StorageTagBasic>;
  using ArrayHandleType = svtkm::cont::ArrayHandle<ValueType, svtkm::cont::StorageTagBasic>;

  static ArrayHandleType Wrap(svtkAOSDataArrayTemplate<T>* input)
  {
    return svtkm::cont::make_ArrayHandle(
      reinterpret_cast<ValueType*>(input->GetPointer(0)), input->GetNumberOfTuples());
  }
};

template <typename T, svtkm::IdComponent NumComponents>
struct DataArrayToArrayHandle<svtkSOADataArrayTemplate<T>, NumComponents>
{
  using ValueType = svtkm::Vec<T, NumComponents>;
  using StorageType = svtkm::cont::internal::Storage<ValueType, svtkm::cont::StorageTagSOA>;
  using ArrayHandleType = svtkm::cont::ArrayHandle<ValueType, svtkm::cont::StorageTagSOA>;

  static ArrayHandleType Wrap(svtkSOADataArrayTemplate<T>* input)
  {
    svtkm::Id numValues = input->GetNumberOfTuples();
    svtkm::cont::internal::Storage<ValueType, svtkm::cont::StorageTagSOA> storage;
    for (svtkm::IdComponent i = 0; i < NumComponents; ++i)
    {
      storage.SetArray(i,
        svtkm::cont::make_ArrayHandle<T>(
          reinterpret_cast<T*>(input->GetComponentArrayPointer(i)), numValues));
    }

    return svtkm::cont::ArrayHandleSOA<ValueType>(storage);
  }
};

template <typename T>
struct DataArrayToArrayHandle<svtkSOADataArrayTemplate<T>, 1>
{
  using StorageType = svtkm::cont::internal::Storage<T, svtkm::cont::StorageTagBasic>;
  using ArrayHandleType = svtkm::cont::ArrayHandle<T, svtkm::cont::StorageTagBasic>;

  static ArrayHandleType Wrap(svtkSOADataArrayTemplate<T>* input)
  {
    return svtkm::cont::make_ArrayHandle(
      input->GetComponentArrayPointer(0), input->GetNumberOfTuples());
  }
};

enum class FieldsFlag
{
  None = 0x0,
  Points = 0x1,
  Cells = 0x2,

  PointsAndCells = Points | Cells
};

SVTKACCELERATORSSVTKM_EXPORT
void ProcessFields(svtkDataSet* input, svtkm::cont::DataSet& dataset, tosvtkm::FieldsFlag fields);

// determine the type and call the proper Convert routine
SVTKACCELERATORSSVTKM_EXPORT
svtkm::cont::Field Convert(svtkDataArray* input, int association);
}

namespace fromsvtkm
{

SVTKACCELERATORSSVTKM_EXPORT
svtkDataArray* Convert(const svtkm::cont::Field& input);

SVTKACCELERATORSSVTKM_EXPORT
svtkPoints* Convert(const svtkm::cont::CoordinateSystem& input);

SVTKACCELERATORSSVTKM_EXPORT
bool ConvertArrays(const svtkm::cont::DataSet& input, svtkDataSet* output);
}

inline tosvtkm::FieldsFlag operator&(const tosvtkm::FieldsFlag& a, const tosvtkm::FieldsFlag& b)
{
  using T = std::underlying_type<tosvtkm::FieldsFlag>::type;
  return static_cast<tosvtkm::FieldsFlag>(static_cast<T>(a) & static_cast<T>(b));
}

inline tosvtkm::FieldsFlag operator|(const tosvtkm::FieldsFlag& a, const tosvtkm::FieldsFlag& b)
{
  using T = std::underlying_type<tosvtkm::FieldsFlag>::type;
  return static_cast<tosvtkm::FieldsFlag>(static_cast<T>(a) | static_cast<T>(b));
}

#endif // svtkmlib_ArrayConverters_h
