//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/io/writer/SVTKDataSetWriter.h>

#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>

namespace
{

#define WRITE_FILE(MakeTestDataMethod)                                                             \
  TestSVTKWriteTestData(#MakeTestDataMethod, tds.MakeTestDataMethod())

void TestSVTKWriteTestData(const std::string& methodName, const svtkm::cont::DataSet& data)
{
  std::cout << "Writing " << methodName << std::endl;
  svtkm::io::writer::SVTKDataSetWriter writer(methodName + ".svtk");
  writer.WriteDataSet(data);
}

void TestSVTKExplicitWrite()
{
  svtkm::cont::testing::MakeTestDataSet tds;

  WRITE_FILE(Make1DExplicitDataSet0);

  WRITE_FILE(Make2DExplicitDataSet0);

  WRITE_FILE(Make3DExplicitDataSet0);
  WRITE_FILE(Make3DExplicitDataSet1);
  WRITE_FILE(Make3DExplicitDataSet2);
  WRITE_FILE(Make3DExplicitDataSet3);
  WRITE_FILE(Make3DExplicitDataSet4);
  WRITE_FILE(Make3DExplicitDataSet5);
  WRITE_FILE(Make3DExplicitDataSet6);
  WRITE_FILE(Make3DExplicitDataSet7);
  WRITE_FILE(Make3DExplicitDataSet8);
  WRITE_FILE(Make3DExplicitDataSetZoo);
  WRITE_FILE(Make3DExplicitDataSetPolygonal);
  WRITE_FILE(Make3DExplicitDataSetCowNose);

  std::cout << "Force writer to output an explicit grid as points" << std::endl;
  svtkm::io::writer::SVTKDataSetWriter writer("Make3DExplicitDataSet0-no-grid.svtk");
  writer.WriteDataSet(tds.Make3DExplicitDataSet0(), true);
}

void TestSVTKUniformWrite()
{
  svtkm::cont::testing::MakeTestDataSet tds;

  WRITE_FILE(Make1DUniformDataSet0);
  WRITE_FILE(Make1DUniformDataSet1);
  WRITE_FILE(Make1DUniformDataSet2);

  WRITE_FILE(Make2DUniformDataSet0);
  WRITE_FILE(Make2DUniformDataSet1);
  WRITE_FILE(Make2DUniformDataSet2);

  WRITE_FILE(Make3DUniformDataSet0);
  WRITE_FILE(Make3DUniformDataSet1);
  // WRITE_FILE(Make3DUniformDataSet2); Skip this one. It's really big.
  WRITE_FILE(Make3DUniformDataSet3);

  WRITE_FILE(Make3DRegularDataSet0);
  WRITE_FILE(Make3DRegularDataSet1);

  std::cout << "Force writer to output a uniform grid as points" << std::endl;
  svtkm::io::writer::SVTKDataSetWriter writer("Make3DUniformDataSet0-no-grid.svtk");
  writer.WriteDataSet(tds.Make3DUniformDataSet0(), true);
}

void TestSVTKRectilinearWrite()
{
  svtkm::cont::testing::MakeTestDataSet tds;

  WRITE_FILE(Make2DRectilinearDataSet0);

  WRITE_FILE(Make3DRectilinearDataSet0);

  std::cout << "Force writer to output a rectilinear grid as points" << std::endl;
  svtkm::io::writer::SVTKDataSetWriter writer("Make3DRectilinearDataSet0-no-grid.svtk");
  writer.WriteDataSet(tds.Make3DRectilinearDataSet0(), true);
}

void TestSVTKWrite()
{
  TestSVTKExplicitWrite();
  TestSVTKUniformWrite();
  TestSVTKRectilinearWrite();
}

} //Anonymous namespace

int UnitTestSVTKDataSetWriter(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestSVTKWrite, argc, argv);
}
