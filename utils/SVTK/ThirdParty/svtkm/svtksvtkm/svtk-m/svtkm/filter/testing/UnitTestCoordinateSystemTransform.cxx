//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/testing/Testing.h>
#include <svtkm/filter/CoordinateSystemTransform.h>

#include <string>
#include <vector>

namespace
{

enum CoordinateType
{
  CART = 0,
  CYL,
  SPH
};

svtkm::cont::DataSet MakeTestDataSet(const CoordinateType& cType)
{
  svtkm::cont::DataSet dataSet;

  std::vector<svtkm::Vec3f> coordinates;
  const svtkm::Id dim = 5;
  if (cType == CART)
  {
    for (svtkm::Id j = 0; j < dim; ++j)
    {
      svtkm::FloatDefault z =
        static_cast<svtkm::FloatDefault>(j) / static_cast<svtkm::FloatDefault>(dim - 1);
      for (svtkm::Id i = 0; i < dim; ++i)
      {
        svtkm::FloatDefault x =
          static_cast<svtkm::FloatDefault>(i) / static_cast<svtkm::FloatDefault>(dim - 1);
        svtkm::FloatDefault y = (x * x + z * z) / 2.0f;
        coordinates.push_back(svtkm::make_Vec(x + 0, y + 0, z + 0));
      }
    }
  }
  else if (cType == CYL)
  {
    svtkm::FloatDefault R = 1.0f;
    for (svtkm::Id j = 0; j < dim; j++)
    {
      svtkm::FloatDefault Z =
        static_cast<svtkm::FloatDefault>(j) / static_cast<svtkm::FloatDefault>(dim - 1);
      for (svtkm::Id i = 0; i < dim; i++)
      {
        svtkm::FloatDefault Theta = svtkm::TwoPif() *
          (static_cast<svtkm::FloatDefault>(i) / static_cast<svtkm::FloatDefault>(dim - 1));
        coordinates.push_back(svtkm::make_Vec(R, Theta, Z));
      }
    }
  }
  else if (cType == SPH)
  {
    //Spherical coordinates have some degenerate cases, so provide some good cases.
    svtkm::FloatDefault R = 1.0f;
    svtkm::FloatDefault eps = svtkm::Epsilon<float>();
    std::vector<svtkm::FloatDefault> Thetas = {
      eps, svtkm::Pif() / 4, svtkm::Pif() / 3, svtkm::Pif() / 2, svtkm::Pif() - eps
    };
    std::vector<svtkm::FloatDefault> Phis = {
      eps, svtkm::TwoPif() / 4, svtkm::TwoPif() / 3, svtkm::TwoPif() / 2, svtkm::TwoPif() - eps
    };
    for (std::size_t i = 0; i < Thetas.size(); i++)
      for (std::size_t j = 0; j < Phis.size(); j++)
        coordinates.push_back(svtkm::make_Vec(R, Thetas[i], Phis[j]));
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

void ValidateCoordTransform(const svtkm::cont::DataSet& ds,
                            const svtkm::cont::DataSet& dsTrn,
                            const std::vector<bool>& isAngle)
{
  auto points = ds.GetCoordinateSystem().GetData();
  auto pointsTrn = dsTrn.GetCoordinateSystem().GetData();
  SVTKM_TEST_ASSERT(points.GetNumberOfValues() == pointsTrn.GetNumberOfValues(),
                   "Incorrect number of points in point transform");

  auto pointsPortal = points.GetPortalConstControl();
  auto pointsTrnPortal = pointsTrn.GetPortalConstControl();

  for (svtkm::Id i = 0; i < points.GetNumberOfValues(); i++)
  {
    svtkm::Vec3f p = pointsPortal.Get(i);
    svtkm::Vec3f r = pointsTrnPortal.Get(i);
    bool isEqual = true;
    for (svtkm::IdComponent j = 0; j < 3; j++)
    {
      if (isAngle[static_cast<std::size_t>(j)])
        isEqual &= (test_equal(p[j], r[j]) || test_equal(p[j] + svtkm::TwoPif(), r[j]) ||
                    test_equal(p[j], r[j] + svtkm::TwoPif()));
      else
        isEqual &= test_equal(p[j], r[j]);
    }
    SVTKM_TEST_ASSERT(isEqual, "Wrong result for PointTransform worklet");
  }
}
}

void TestCoordinateSystemTransform()
{
  std::cout << "Testing CylindricalCoordinateTransform Filter" << std::endl;

  //Test cartesian to cyl
  svtkm::cont::DataSet dsCart = MakeTestDataSet(CART);
  svtkm::filter::CylindricalCoordinateTransform cylTrn;

  cylTrn.SetOutputFieldName("cylindricalCoords");
  cylTrn.SetUseCoordinateSystemAsField(true);
  cylTrn.SetCartesianToCylindrical();
  svtkm::cont::DataSet carToCylDataSet = cylTrn.Execute(dsCart);

  cylTrn.SetCylindricalToCartesian();
  cylTrn.SetUseCoordinateSystemAsField(true);
  cylTrn.SetOutputFieldName("cartesianCoords");
  svtkm::cont::DataSet cylToCarDataSet = cylTrn.Execute(carToCylDataSet);
  ValidateCoordTransform(dsCart, cylToCarDataSet, { false, false, false });

  //Test cyl to cart.
  svtkm::cont::DataSet dsCyl = MakeTestDataSet(CYL);
  cylTrn.SetCylindricalToCartesian();
  cylTrn.SetUseCoordinateSystemAsField(true);
  cylTrn.SetOutputFieldName("cartesianCoords");
  cylToCarDataSet = cylTrn.Execute(dsCyl);

  cylTrn.SetCartesianToCylindrical();
  cylTrn.SetUseCoordinateSystemAsField(true);
  cylTrn.SetOutputFieldName("cylindricalCoords");
  carToCylDataSet = cylTrn.Execute(cylToCarDataSet);
  ValidateCoordTransform(dsCyl, carToCylDataSet, { false, true, false });

  std::cout << "Testing SphericalCoordinateTransform Filter" << std::endl;

  svtkm::filter::SphericalCoordinateTransform sphTrn;
  sphTrn.SetOutputFieldName("sphericalCoords");
  sphTrn.SetUseCoordinateSystemAsField(true);
  sphTrn.SetCartesianToSpherical();
  svtkm::cont::DataSet carToSphDataSet = sphTrn.Execute(dsCart);

  sphTrn.SetOutputFieldName("cartesianCoords");
  sphTrn.SetUseCoordinateSystemAsField(true);
  sphTrn.SetSphericalToCartesian();
  svtkm::cont::DataSet sphToCarDataSet = sphTrn.Execute(carToSphDataSet);
  ValidateCoordTransform(dsCart, sphToCarDataSet, { false, true, true });

  svtkm::cont::DataSet dsSph = MakeTestDataSet(SPH);
  sphTrn.SetSphericalToCartesian();
  sphTrn.SetUseCoordinateSystemAsField(true);
  sphTrn.SetOutputFieldName("sphericalCoords");
  sphToCarDataSet = sphTrn.Execute(dsSph);

  sphTrn.SetCartesianToSpherical();
  sphTrn.SetUseCoordinateSystemAsField(true);
  sphTrn.SetOutputFieldName("sphericalCoords");
  carToSphDataSet = cylTrn.Execute(sphToCarDataSet);
  ValidateCoordTransform(dsSph, carToSphDataSet, { false, true, true });
}


int UnitTestCoordinateSystemTransform(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestCoordinateSystemTransform, argc, argv);
}
