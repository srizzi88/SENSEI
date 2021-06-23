//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/CellSetExplicit.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/testing/Testing.h>
#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/PointTransform.h>

#include <random>
#include <vector>

namespace
{
std::mt19937 randGenerator;

svtkm::cont::DataSet MakePointTransformTestDataSet()
{
  svtkm::cont::DataSet dataSet;

  std::vector<svtkm::Vec3f> coordinates;
  const svtkm::Id dim = 5;
  for (svtkm::Id j = 0; j < dim; ++j)
  {
    svtkm::FloatDefault z =
      static_cast<svtkm::FloatDefault>(j) / static_cast<svtkm::FloatDefault>(dim - 1);
    for (svtkm::Id i = 0; i < dim; ++i)
    {
      svtkm::FloatDefault x =
        static_cast<svtkm::FloatDefault>(i) / static_cast<svtkm::FloatDefault>(dim - 1);
      svtkm::FloatDefault y = (x * x + z * z) / 2.0f;
      coordinates.push_back(svtkm::make_Vec(x, y, z));
    }
  }

  svtkm::Id numCells = (dim - 1) * (dim - 1);
  dataSet.AddCoordinateSystem(
    svtkm::cont::make_CoordinateSystem("coordinates", coordinates, svtkm::CopyFlag::On));

  svtkm::cont::CellSetExplicit<> cellSet;
  cellSet.PrepareToAddCells(numCells, numCells * 4);
  for (svtkm::Id j = 0; j < dim - 1; ++j)
  {
    for (svtkm::Id i = 0; i < dim - 1; ++i)
    {
      cellSet.AddCell(svtkm::CELL_SHAPE_QUAD,
                      4,
                      svtkm::make_Vec<svtkm::Id>(
                        j * dim + i, j * dim + i + 1, (j + 1) * dim + i + 1, (j + 1) * dim + i));
    }
  }
  cellSet.CompleteAddingCells(svtkm::Id(coordinates.size()));

  dataSet.SetCellSet(cellSet);
  return dataSet;
}

void ValidatePointTransform(const svtkm::cont::CoordinateSystem& coords,
                            const svtkm::cont::ArrayHandle<svtkm::Vec3f>& result,
                            const svtkm::Matrix<svtkm::FloatDefault, 4, 4>& matrix)
{
  auto points = coords.GetData();
  SVTKM_TEST_ASSERT(points.GetNumberOfValues() == result.GetNumberOfValues(),
                   "Incorrect number of points in point transform");

  auto pointsPortal = points.GetPortalConstControl();
  auto resultsPortal = result.GetPortalConstControl();

  for (svtkm::Id i = 0; i < points.GetNumberOfValues(); i++)
    SVTKM_TEST_ASSERT(
      test_equal(resultsPortal.Get(i), svtkm::Transform3DPoint(matrix, pointsPortal.Get(i))),
      "Wrong result for PointTransform worklet");
}


void TestPointTransformTranslation(const svtkm::cont::DataSet& ds, const svtkm::Vec3f& trans)
{
  svtkm::cont::ArrayHandle<svtkm::Vec3f> result;
  svtkm::worklet::PointTransform<svtkm::FloatDefault> worklet;

  worklet.SetTranslation(trans);
  svtkm::worklet::DispatcherMapField<svtkm::worklet::PointTransform<svtkm::FloatDefault>> dispatcher(
    worklet);
  dispatcher.Invoke(ds.GetCoordinateSystem(), result);

  ValidatePointTransform(ds.GetCoordinateSystem(), result, Transform3DTranslate(trans));
}

void TestPointTransformScale(const svtkm::cont::DataSet& ds, const svtkm::Vec3f& scale)
{
  svtkm::cont::ArrayHandle<svtkm::Vec3f> result;
  svtkm::worklet::PointTransform<svtkm::FloatDefault> worklet;

  worklet.SetScale(scale);
  svtkm::worklet::DispatcherMapField<svtkm::worklet::PointTransform<svtkm::FloatDefault>> dispatcher(
    worklet);
  dispatcher.Invoke(ds.GetCoordinateSystem(), result);

  ValidatePointTransform(ds.GetCoordinateSystem(), result, Transform3DScale(scale));
}

void TestPointTransformRotation(const svtkm::cont::DataSet& ds,
                                const svtkm::FloatDefault& angle,
                                const svtkm::Vec3f& axis)
{
  svtkm::cont::ArrayHandle<svtkm::Vec3f> result;
  svtkm::worklet::PointTransform<svtkm::FloatDefault> worklet;

  worklet.SetRotation(angle, axis);
  svtkm::worklet::DispatcherMapField<svtkm::worklet::PointTransform<svtkm::FloatDefault>> dispatcher(
    worklet);
  dispatcher.Invoke(ds.GetCoordinateSystem(), result);

  ValidatePointTransform(ds.GetCoordinateSystem(), result, Transform3DRotate(angle, axis));
}
}

void TestPointTransform()
{
  std::cout << "Testing PointTransform Worklet" << std::endl;

  svtkm::cont::DataSet ds = MakePointTransformTestDataSet();
  int N = 41;

  //Test translation
  TestPointTransformTranslation(ds, svtkm::Vec3f(0, 0, 0));
  TestPointTransformTranslation(ds, svtkm::Vec3f(1, 1, 1));
  TestPointTransformTranslation(ds, svtkm::Vec3f(-1, -1, -1));

  std::uniform_real_distribution<svtkm::FloatDefault> transDist(-100, 100);
  for (int i = 0; i < N; i++)
    TestPointTransformTranslation(
      ds,
      svtkm::Vec3f(transDist(randGenerator), transDist(randGenerator), transDist(randGenerator)));

  //Test scaling
  TestPointTransformScale(ds, svtkm::Vec3f(1, 1, 1));
  TestPointTransformScale(ds, svtkm::Vec3f(.23f, .23f, .23f));
  TestPointTransformScale(ds, svtkm::Vec3f(1, 2, 3));
  TestPointTransformScale(ds, svtkm::Vec3f(3.23f, 9.23f, 4.23f));

  std::uniform_real_distribution<svtkm::FloatDefault> scaleDist(0.0001f, 100);
  for (int i = 0; i < N; i++)
  {
    TestPointTransformScale(ds, svtkm::Vec3f(scaleDist(randGenerator)));
    TestPointTransformScale(
      ds,
      svtkm::Vec3f(scaleDist(randGenerator), scaleDist(randGenerator), scaleDist(randGenerator)));
  }

  //Test rotation
  std::vector<svtkm::FloatDefault> angles;
  std::uniform_real_distribution<svtkm::FloatDefault> angleDist(0, 360);
  for (int i = 0; i < N; i++)
    angles.push_back(angleDist(randGenerator));

  std::vector<svtkm::Vec3f> axes;
  axes.push_back(svtkm::Vec3f(1, 0, 0));
  axes.push_back(svtkm::Vec3f(0, 1, 0));
  axes.push_back(svtkm::Vec3f(0, 0, 1));
  axes.push_back(svtkm::Vec3f(1, 1, 1));
  axes.push_back(-axes[0]);
  axes.push_back(-axes[1]);
  axes.push_back(-axes[2]);
  axes.push_back(-axes[3]);

  std::uniform_real_distribution<svtkm::FloatDefault> axisDist(-1, 1);
  for (int i = 0; i < N; i++)
    axes.push_back(
      svtkm::Vec3f(axisDist(randGenerator), axisDist(randGenerator), axisDist(randGenerator)));

  for (std::size_t i = 0; i < angles.size(); i++)
    for (std::size_t j = 0; j < axes.size(); j++)
      TestPointTransformRotation(ds, angles[i], axes[j]);
}

int UnitTestPointTransform(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestPointTransform, argc, argv);
}
