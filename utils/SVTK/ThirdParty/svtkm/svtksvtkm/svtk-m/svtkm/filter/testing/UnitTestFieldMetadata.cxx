//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>
#include <svtkm/filter/CellAverage.h>

namespace
{

svtkm::cont::Field makePointField()
{
  return svtkm::cont::Field(
    "foo", svtkm::cont::Field::Association::POINTS, svtkm::cont::ArrayHandle<svtkm::Float32>());
}

void TestFieldTypesUnknown()
{
  svtkm::filter::FieldMetadata defaultMD;
  SVTKM_TEST_ASSERT(defaultMD.IsPointField() == false, "default is not point or cell");
  SVTKM_TEST_ASSERT(defaultMD.IsCellField() == false, "default is not point or cell");

  //verify the field helper works properly
  svtkm::cont::Field field1;
  svtkm::filter::FieldMetadata makeMDFromField(field1);
  SVTKM_TEST_ASSERT(makeMDFromField.IsPointField() == false, "makeMDFromField is not point or cell");
  SVTKM_TEST_ASSERT(makeMDFromField.IsCellField() == false, "makeMDFromField is not point or cell");
}

void TestFieldTypesPoint()
{
  svtkm::filter::FieldMetadata helperMD(makePointField());
  SVTKM_TEST_ASSERT(helperMD.IsPointField() == true, "point should be a point field");
  SVTKM_TEST_ASSERT(helperMD.IsCellField() == false, "point can't be a cell field");

  //verify the field helper works properly
  svtkm::Float32 vars[6] = { 10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f };
  auto field = svtkm::cont::make_Field("pointvar", svtkm::cont::Field::Association::POINTS, vars, 6);
  svtkm::filter::FieldMetadata makeMDFromField(field);
  SVTKM_TEST_ASSERT(makeMDFromField.IsPointField() == true, "point should be a point field");
  SVTKM_TEST_ASSERT(makeMDFromField.IsCellField() == false, "point can't be a cell field");
}

void TestFieldTypesCell()
{
  svtkm::filter::FieldMetadata defaultMD;
  svtkm::filter::FieldMetadata helperMD(
    svtkm::cont::make_FieldCell("foo", svtkm::cont::ArrayHandle<svtkm::Float32>()));
  SVTKM_TEST_ASSERT(helperMD.IsPointField() == false, "cell can't be a point field");
  SVTKM_TEST_ASSERT(helperMD.IsCellField() == true, "cell should be a cell field");

  //verify the field helper works properly
  svtkm::Float32 vars[6] = { 10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f };
  auto field = svtkm::cont::make_FieldCell("pointvar", svtkm::cont::make_ArrayHandle(vars, 6));
  svtkm::filter::FieldMetadata makeMDFromField(field);
  SVTKM_TEST_ASSERT(makeMDFromField.IsPointField() == false, "cell can't be a point field");
  SVTKM_TEST_ASSERT(makeMDFromField.IsCellField() == true, "cell should be a cell field");
}

void TestFieldMetadata()
{
  TestFieldTypesUnknown();
  TestFieldTypesPoint();
  TestFieldTypesCell();
}
}

int UnitTestFieldMetadata(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestFieldMetadata, argc, argv);
}
