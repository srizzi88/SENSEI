//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/DataSetBuilderExplicit.h>
#include <svtkm/cont/DataSetBuilderUniform.h>
#include <svtkm/cont/DeviceAdapter.h>

#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>
#include <svtkm/io/writer/SVTKDataSetWriter.h>

#include <svtkm/worklet/ExternalFaces.h>

#include <algorithm>
#include <iostream>

namespace
{

svtkm::cont::DataSet RunExternalFaces(svtkm::cont::DataSet& inDataSet)
{
  const svtkm::cont::DynamicCellSet& inCellSet = inDataSet.GetCellSet();

  svtkm::cont::CellSetExplicit<> outCellSet;

  //Run the External Faces worklet
  if (inCellSet.IsSameType(svtkm::cont::CellSetStructured<3>()))
  {
    svtkm::worklet::ExternalFaces().Run(inCellSet.Cast<svtkm::cont::CellSetStructured<3>>(),
                                       inDataSet.GetCoordinateSystem(),
                                       outCellSet);
  }
  else
  {
    svtkm::worklet::ExternalFaces().Run(inCellSet.Cast<svtkm::cont::CellSetExplicit<>>(), outCellSet);
  }

  svtkm::cont::DataSet outDataSet;
  for (svtkm::IdComponent i = 0; i < inDataSet.GetNumberOfCoordinateSystems(); ++i)
  {
    outDataSet.AddCoordinateSystem(inDataSet.GetCoordinateSystem(i));
  }

  outDataSet.SetCellSet(outCellSet);

  return outDataSet;
}

void TestExternalFaces1()
{
  std::cout << "Test 1" << std::endl;

  //--------------Construct a SVTK-m Test Dataset----------------
  const int nVerts = 8; //A cube that is tetrahedralized
  using CoordType = svtkm::Vec3f_32;
  svtkm::cont::ArrayHandle<CoordType> coordinates;
  coordinates.Allocate(nVerts);
  coordinates.GetPortalControl().Set(0, CoordType(0.0f, 0.0f, 0.0f));
  coordinates.GetPortalControl().Set(1, CoordType(1.0f, 0.0f, 0.0f));
  coordinates.GetPortalControl().Set(2, CoordType(1.0f, 1.0f, 0.0f));
  coordinates.GetPortalControl().Set(3, CoordType(0.0f, 1.0f, 0.0f));
  coordinates.GetPortalControl().Set(4, CoordType(0.0f, 0.0f, 1.0f));
  coordinates.GetPortalControl().Set(5, CoordType(1.0f, 0.0f, 1.0f));
  coordinates.GetPortalControl().Set(6, CoordType(1.0f, 1.0f, 1.0f));
  coordinates.GetPortalControl().Set(7, CoordType(0.0f, 1.0f, 1.0f));

  //Construct the SVTK-m shapes and numIndices connectivity arrays
  const int nCells = 6; //The tetrahedrons of the cube
  svtkm::IdComponent cellVerts[nCells][4] = { { 4, 7, 6, 3 }, { 4, 6, 3, 2 }, { 4, 0, 3, 2 },
                                             { 4, 6, 5, 2 }, { 4, 5, 0, 2 }, { 1, 0, 5, 2 } };

  svtkm::cont::ArrayHandle<svtkm::UInt8> shapes;
  svtkm::cont::ArrayHandle<svtkm::IdComponent> numIndices;
  svtkm::cont::ArrayHandle<svtkm::Id> conn;
  shapes.Allocate(static_cast<svtkm::Id>(nCells));
  numIndices.Allocate(static_cast<svtkm::Id>(nCells));
  conn.Allocate(static_cast<svtkm::Id>(4 * nCells));

  int index = 0;
  for (int j = 0; j < nCells; j++)
  {
    shapes.GetPortalControl().Set(j, static_cast<svtkm::UInt8>(svtkm::CELL_SHAPE_TETRA));
    numIndices.GetPortalControl().Set(j, 4);
    for (int k = 0; k < 4; k++)
      conn.GetPortalControl().Set(index++, cellVerts[j][k]);
  }

  svtkm::cont::DataSetBuilderExplicit builder;
  svtkm::cont::DataSet ds = builder.Create(coordinates, shapes, numIndices, conn);

  //Run the External Faces worklet
  svtkm::cont::DataSet new_ds = RunExternalFaces(ds);
  svtkm::cont::CellSetExplicit<> new_cs;
  new_ds.GetCellSet().CopyTo(new_cs);

  svtkm::Id numExtFaces_out = new_cs.GetNumberOfCells();

  //Validate the number of external faces (output) returned by the worklet
  const svtkm::Id numExtFaces_actual = 12;
  SVTKM_TEST_ASSERT(numExtFaces_out == numExtFaces_actual, "Number of External Faces mismatch");

} // TestExternalFaces1

void TestExternalFaces2()
{
  std::cout << "Test 2" << std::endl;

  svtkm::cont::testing::MakeTestDataSet dataSetMaker;
  svtkm::cont::DataSet inDataSet = dataSetMaker.Make3DExplicitDataSet5();

  //  svtkm::io::writer::SVTKDataSetWriter writer("svtkm_explicit_data_5.svtk");
  //  writer.WriteDataSet(inDataSet);

  // Expected faces
  const svtkm::IdComponent MAX_POINTS_PER_FACE = 4;
  const svtkm::Id NUM_FACES = 12;
  const svtkm::Id ExpectedExternalFaces[NUM_FACES][MAX_POINTS_PER_FACE] = {
    { 0, 3, 7, 4 },   { 0, 1, 2, 3 },  { 0, 4, 5, 1 },  { 3, 2, 6, 7 },
    { 1, 5, 8, -1 },  { 6, 2, 8, -1 }, { 2, 1, 8, -1 }, { 8, 10, 6, -1 },
    { 5, 10, 8, -1 }, { 4, 7, 9, -1 }, { 7, 6, 10, 9 }, { 9, 10, 5, 4 }
  };

  svtkm::cont::DataSet outDataSet = RunExternalFaces(inDataSet);
  svtkm::cont::CellSetExplicit<> outCellSet;
  outDataSet.GetCellSet().CopyTo(outCellSet);

  SVTKM_TEST_ASSERT(outCellSet.GetNumberOfCells() == NUM_FACES, "Got wrong number of faces.");

  bool foundFaces[NUM_FACES];
  for (svtkm::Id faceId = 0; faceId < NUM_FACES; faceId++)
  {
    foundFaces[faceId] = false;
  }

  for (svtkm::Id dataFaceId = 0; dataFaceId < NUM_FACES; dataFaceId++)
  {
    svtkm::Vec<svtkm::Id, MAX_POINTS_PER_FACE> dataIndices(-1);
    outCellSet.GetIndices(dataFaceId, dataIndices);
    std::cout << "Looking for face " << dataIndices << std::endl;
    bool foundFace = false;
    for (svtkm::Id expectedFaceId = 0; expectedFaceId < NUM_FACES; expectedFaceId++)
    {
      svtkm::Vec<svtkm::Id, MAX_POINTS_PER_FACE> expectedIndices;
      svtkm::make_VecC(ExpectedExternalFaces[expectedFaceId], 4).CopyInto(expectedIndices);
      if (expectedIndices == dataIndices)
      {
        SVTKM_TEST_ASSERT(!foundFaces[expectedFaceId], "Found face twice.");
        std::cout << "  found" << std::endl;
        foundFace = true;
        foundFaces[expectedFaceId] = true;
        break;
      }
    }
    SVTKM_TEST_ASSERT(foundFace, "Face not found.");
  }
}

void TestExternalFaces3()
{
  std::cout << "Test 3" << std::endl;

  svtkm::cont::DataSetBuilderUniform dataSetBuilder;
  svtkm::cont::DataSet dataSet = dataSetBuilder.Create(svtkm::Id3(6, 6, 5));

  //Run the External Faces worklet
  svtkm::cont::DataSet new_ds = RunExternalFaces(dataSet);
  svtkm::cont::CellSetExplicit<> new_cs;
  new_ds.GetCellSet().CopyTo(new_cs);

  svtkm::Id numExtFaces_out = new_cs.GetNumberOfCells();

  //Validate the number of external faces (output) returned by the worklet
  const svtkm::Id numExtFaces_actual = 130;
  SVTKM_TEST_ASSERT(numExtFaces_out == numExtFaces_actual, "Number of External Faces mismatch");
}

void TestExternalFaces()
{
  TestExternalFaces1();
  TestExternalFaces2();
  TestExternalFaces3();
}
}

int UnitTestExternalFaces(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestExternalFaces, argc, argv);
}
