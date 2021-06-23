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
#include "ArrayConverters.hxx"

#include "svtkmDataArray.h"
#include "svtkmFilterPolicy.h"

#include "svtkmlib/PortalTraits.h"

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/CoordinateSystem.hxx>
#include <svtkm/cont/DataSet.h>

#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkDataObject.h"
#include "svtkDataSet.h"
#include "svtkPointData.h"
#include "svtkPoints.h"

namespace tosvtkm
{

void ProcessFields(svtkDataSet* input, svtkm::cont::DataSet& dataset, tosvtkm::FieldsFlag fields)
{
  if ((fields & tosvtkm::FieldsFlag::Points) != tosvtkm::FieldsFlag::None)
  {
    svtkPointData* pd = input->GetPointData();
    for (int i = 0; i < pd->GetNumberOfArrays(); i++)
    {
      svtkDataArray* array = pd->GetArray(i);
      if (array == nullptr)
      {
        continue;
      }

      svtkm::cont::Field pfield = tosvtkm::Convert(array, svtkDataObject::FIELD_ASSOCIATION_POINTS);
      dataset.AddField(pfield);
    }
  }

  if ((fields & tosvtkm::FieldsFlag::Cells) != tosvtkm::FieldsFlag::None)
  {
    svtkCellData* cd = input->GetCellData();
    for (int i = 0; i < cd->GetNumberOfArrays(); i++)
    {
      svtkDataArray* array = cd->GetArray(i);
      if (array == nullptr)
      {
        continue;
      }

      svtkm::cont::Field cfield = tosvtkm::Convert(array, svtkDataObject::FIELD_ASSOCIATION_CELLS);
      dataset.AddField(cfield);
    }
  }
}

template <typename T>
svtkm::cont::Field Convert(svtkmDataArray<T>* input, int association)
{
  // we need to switch on if we are a cell or point field first!
  // The problem is that the constructor signature for fields differ based
  // on if they are a cell or point field.
  if (association == svtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    return svtkm::cont::make_FieldPoint(input->GetName(), input->GetVtkmVariantArrayHandle());
  }
  else if (association == svtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    return svtkm::cont::make_FieldCell(input->GetName(), input->GetVtkmVariantArrayHandle());
  }

  return svtkm::cont::Field();
}

// determine the type and call the proper Convert routine
svtkm::cont::Field Convert(svtkDataArray* input, int association)
{
  // The association will tell us if we have a cell or point field

  // We need to properly deduce the ValueType of the array. This means
  // that we need to switch on Float/Double/Int, and then figure out the
  // number of components. The upside is that the Convert Method can internally
  // figure out the number of components, and not have to generate a lot
  // of template to do so

  // Investigate using svtkArrayDispatch, AOS for all types, and than SOA for
  // just
  // float/double
  svtkm::cont::Field field;
  switch (input->GetDataType())
  {
    svtkTemplateMacro(
      svtkAOSDataArrayTemplate<SVTK_TT>* typedIn1 =
        svtkAOSDataArrayTemplate<SVTK_TT>::FastDownCast(input);
      if (typedIn1) { field = Convert(typedIn1, association); } else {
        svtkSOADataArrayTemplate<SVTK_TT>* typedIn2 =
          svtkSOADataArrayTemplate<SVTK_TT>::FastDownCast(input);
        if (typedIn2)
        {
          field = Convert(typedIn2, association);
        }
        else
        {
          svtkmDataArray<SVTK_TT>* typedIn3 = svtkmDataArray<SVTK_TT>::SafeDownCast(input);
          if (typedIn3)
          {
            field = Convert(typedIn3, association);
          }
        }
      });
    // end svtkTemplateMacro
  }
  return field;
}

} // namespace tosvtkm

namespace fromsvtkm
{

namespace
{

struct ArrayConverter
{
public:
  mutable svtkDataArray* Data;

  ArrayConverter()
    : Data(nullptr)
  {
  }

  // CastAndCall always passes a const array handle. Just shallow copy to a
  // local array handle by taking by value.

  template <typename T, typename S>
  void operator()(svtkm::cont::ArrayHandle<T, S> handle) const
  {
    this->Data = make_svtkmDataArray(handle);
  }

  template <typename T>
  void operator()(svtkm::cont::ArrayHandle<T, svtkm::cont::StorageTagVirtual> handle) const
  {
    using BasicHandle = svtkm::cont::ArrayHandle<T, svtkm::cont::StorageTagBasic>;
    if (svtkm::cont::IsType<BasicHandle>(handle))
    {
      this->operator()(svtkm::cont::Cast<BasicHandle>(handle));
    }
    else
    {
      this->Data = make_svtkmDataArray(handle);
    }
  }

  template <typename T, int N>
  void operator()(
    svtkm::cont::ArrayHandle<svtkm::Vec<T, N>, svtkm::cont::StorageTagVirtual> handle) const
  {
    using SOAHandle = svtkm::cont::ArrayHandle<svtkm::Vec<T, N>, svtkm::cont::StorageTagSOA>;
    using BasicHandle = svtkm::cont::ArrayHandle<svtkm::Vec<T, N>, svtkm::cont::StorageTagBasic>;
    if (svtkm::cont::IsType<SOAHandle>(handle))
    {
      this->operator()(svtkm::cont::Cast<SOAHandle>(handle));
    }
    else if (svtkm::cont::IsType<BasicHandle>(handle))
    {
      this->operator()(svtkm::cont::Cast<BasicHandle>(handle));
    }
    else
    {
      this->Data = make_svtkmDataArray(handle);
    }
  }

  template <typename T>
  void operator()(svtkm::cont::ArrayHandle<T, svtkm::cont::StorageTagBasic> handle) const
  {
    // we can steal this array!
    using Traits = tosvtkm::svtkPortalTraits<T>; // Handles Vec<Vec<T,N,N> properly
    using ValueType = typename Traits::ComponentType;
    using SVTKArrayType = svtkAOSDataArrayTemplate<ValueType>;

    SVTKArrayType* array = SVTKArrayType::New();
    array->SetNumberOfComponents(Traits::NUM_COMPONENTS);

    handle.SyncControlArray();
    const svtkm::Id size = handle.GetNumberOfValues() * Traits::NUM_COMPONENTS;

    // SVTK-m allocations are aligned or done with cuda uvm memory so we need to propagate
    // the proper free function to SVTK
    auto stolenState = handle.GetStorage().StealArray();
    auto stolenMemory = reinterpret_cast<ValueType*>(stolenState.first);
    array->SetVoidArray(stolenMemory, size, 0, svtkAbstractArray::SVTK_DATA_ARRAY_USER_DEFINED);
    array->SetArrayFreeFunction(stolenState.second);

    this->Data = array;
  }

  template <typename T>
  void operator()(svtkm::cont::ArrayHandle<T, svtkm::cont::StorageTagSOA> handle) const
  {
    // we can steal this array!
    using Traits = tosvtkm::svtkPortalTraits<T>; // Handles Vec<Vec<T,N,N> properly
    using ValueType = typename Traits::ComponentType;
    using SVTKArrayType = svtkSOADataArrayTemplate<ValueType>;

    SVTKArrayType* array = SVTKArrayType::New();
    array->SetNumberOfComponents(Traits::NUM_COMPONENTS);

    handle.SyncControlArray();
    auto storage = handle.GetStorage();
    const svtkm::Id size = handle.GetNumberOfValues() * Traits::NUM_COMPONENTS;
    for (svtkm::IdComponent i = 0; i < Traits::NUM_COMPONENTS; ++i)
    {
      // steal each component array.
      auto stolenState = storage.GetArray(i).GetStorage().StealArray();
      auto stolenMemory = reinterpret_cast<ValueType*>(stolenState.first);
      array->SetArray(
        i, stolenMemory, size, true, 0, svtkAbstractArray::SVTK_DATA_ARRAY_USER_DEFINED);
      array->SetArrayFreeFunction(i, stolenState.second);
    }
  }
};
} // anonymous namespace

// Though the following conversion routines take const-ref parameters as input,
// the underlying storage will be stolen, whenever possible, instead of
// performing a full copy.
// Therefore, these routines should be treated as "moves" and the state of the
// input is undeterminisitic.

svtkDataArray* Convert(const svtkm::cont::Field& input)
{
  // We need to do the conversion from Field to a known svtkm::cont::ArrayHandle
  // after that we need to fill the svtkDataArray
  svtkmOutputFilterPolicy policy;
  svtkDataArray* data = nullptr;
  ArrayConverter aConverter;

  try
  {
    svtkm::cont::CastAndCall(svtkm::filter::ApplyPolicyFieldNotActive(input, policy), aConverter);
    data = aConverter.Data;
    if (data)
    {
      data->SetName(input.GetName().c_str());
    }
  }
  catch (svtkm::cont::Error&)
  {
  }
  return data;
}

svtkPoints* Convert(const svtkm::cont::CoordinateSystem& input)
{
  ArrayConverter aConverter;
  svtkPoints* points = nullptr;
  try
  {
    svtkm::cont::CastAndCall(input, aConverter);
    svtkDataArray* pdata = aConverter.Data;
    points = svtkPoints::New();
    points->SetData(pdata);
    pdata->FastDelete();
  }
  catch (svtkm::cont::Error& e)
  {
    svtkGenericWarningMacro("Converting svtkm::cont::CoordinateSystem to "
                           "svtkPoints failed: "
      << e.what());
  }
  return points;
}

bool ConvertArrays(const svtkm::cont::DataSet& input, svtkDataSet* output)
{
  svtkPointData* pd = output->GetPointData();
  svtkCellData* cd = output->GetCellData();

  svtkm::IdComponent numFields = input.GetNumberOfFields();
  for (svtkm::IdComponent i = 0; i < numFields; ++i)
  {
    const svtkm::cont::Field& f = input.GetField(i);
    svtkDataArray* vfield = Convert(f);
    if (vfield && f.GetAssociation() == svtkm::cont::Field::Association::POINTS)
    {
      pd->AddArray(vfield);
      vfield->FastDelete();
    }
    else if (vfield && f.GetAssociation() == svtkm::cont::Field::Association::CELL_SET)
    {
      cd->AddArray(vfield);
      vfield->FastDelete();
    }
    else if (vfield)
    {
      vfield->Delete();
    }
  }
  return true;
}
}
