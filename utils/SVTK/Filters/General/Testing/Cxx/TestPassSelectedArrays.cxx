/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPassSelectedArrays.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <svtkCellData.h>
#include <svtkDataArraySelection.h>
#include <svtkDoubleArray.h>
#include <svtkLogger.h>
#include <svtkNew.h>
#include <svtkPassSelectedArrays.h>
#include <svtkPointData.h>
#include <svtkPolyData.h>
#include <svtkSmartPointer.h>
#include <svtkSphereSource.h>

namespace
{
svtkSmartPointer<svtkPolyData> GetData()
{
  svtkNew<svtkSphereSource> sphere;
  sphere->Update();
  auto pd = sphere->GetOutput();
  pd->GetPointData()->Initialize();
  pd->GetCellData()->Initialize();
  pd->GetFieldData()->Initialize();
  return pd;
}

void AddArray(svtkPolyData* pd, const char* name, int assoc, svtkIdType numElems = 0)
{
  if (assoc != svtkDataObject::FIELD_ASSOCIATION_NONE && numElems == 0)
  {
    numElems = pd->GetNumberOfElements(assoc);
  }

  svtkNew<svtkDoubleArray> array;
  array->SetName(name);
  array->SetNumberOfTuples(numElems);
  array->FillValue(0);
  pd->GetAttributesAsFieldData(assoc)->AddArray(array);
}

int GetArrayCount(svtkDataObject* dobj, int assoc)
{
  if (auto fd = dobj->GetAttributesAsFieldData(assoc))
  {
    return fd->GetNumberOfArrays();
  }
  return 0;
}
}

int TestPassSelectedArrays(int, char*[])
{
  auto data = GetData();
  AddArray(data, "Temp", svtkDataObject::FIELD_ASSOCIATION_POINTS);
  AddArray(data, "Press", svtkDataObject::FIELD_ASSOCIATION_POINTS);
  AddArray(data, "PointVar0", svtkDataObject::FIELD_ASSOCIATION_POINTS);
  AddArray(data, "PointVar1", svtkDataObject::FIELD_ASSOCIATION_POINTS);
  data->GetPointData()->SetActiveScalars("Temp");

  AddArray(data, "CellVar0", svtkDataObject::FIELD_ASSOCIATION_CELLS);
  AddArray(data, "CellVar1", svtkDataObject::FIELD_ASSOCIATION_CELLS);
  AddArray(data, "CellVar2", svtkDataObject::FIELD_ASSOCIATION_CELLS);
  data->GetCellData()->SetActiveScalars("CellVar0");

  AddArray(data, "FieldVar0", svtkDataObject::FIELD_ASSOCIATION_NONE, 10);
  AddArray(data, "FieldVar1", svtkDataObject::FIELD_ASSOCIATION_NONE, 5);

  svtkNew<svtkPassSelectedArrays> passArrays;
  passArrays->SetInputData(data);

  // case 1: pass nothing.
  passArrays->Update();

  data = svtkPolyData::SafeDownCast(passArrays->GetOutputDataObject(0));
  if (GetArrayCount(data, svtkDataObject::FIELD_ASSOCIATION_POINTS) != 0 ||
    GetArrayCount(data, svtkDataObject::FIELD_ASSOCIATION_CELLS) != 0 ||
    GetArrayCount(data, svtkDataObject::FIELD_ASSOCIATION_NONE) != 0)
  {
    svtkLogF(ERROR, "no arrays should have been passed through!");
    return EXIT_FAILURE;
  }

  // case 2: pass all point arrays only.
  passArrays->GetPointDataArraySelection()->SetUnknownArraySetting(1);
  passArrays->Update();
  data = svtkPolyData::SafeDownCast(passArrays->GetOutputDataObject(0));
  if (GetArrayCount(data, svtkDataObject::FIELD_ASSOCIATION_POINTS) == 0 ||
    GetArrayCount(data, svtkDataObject::FIELD_ASSOCIATION_CELLS) != 0 ||
    GetArrayCount(data, svtkDataObject::FIELD_ASSOCIATION_NONE) != 0)
  {
    svtkLogF(ERROR, "expecting point array only!");
    return EXIT_FAILURE;
  }
  passArrays->GetPointDataArraySelection()->SetUnknownArraySetting(0);

  // case 3: pass chosen arrays.
  passArrays->GetPointDataArraySelection()->EnableArray("Temp");

  passArrays->GetCellDataArraySelection()->SetUnknownArraySetting(1);
  passArrays->GetCellDataArraySelection()->DisableArray("CellVar0");
  passArrays->GetCellDataArraySelection()->DisableArray("CellVar1");

  passArrays->GetFieldDataArraySelection()->EnableArray("FieldVar1");

  passArrays->Update();
  data = svtkPolyData::SafeDownCast(passArrays->GetOutputDataObject(0));
  if (GetArrayCount(data, svtkDataObject::FIELD_ASSOCIATION_POINTS) != 1 ||
    GetArrayCount(data, svtkDataObject::FIELD_ASSOCIATION_CELLS) != 1 ||
    GetArrayCount(data, svtkDataObject::FIELD_ASSOCIATION_NONE) != 1)
  {
    svtkLogF(ERROR, "expecting exactly 1 array of each type!");
    return EXIT_FAILURE;
  }

  // ensure attribute type is getting preserved too.
  if (data->GetPointData()->GetScalars() == nullptr || data->GetCellData()->GetScalars() != nullptr)
  {
    svtkLogF(ERROR, "incorrect attribute type preserved.");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
