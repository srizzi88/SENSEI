/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSVTKMClip.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkmCoordinateSystemTransform.h"

#include "svtkDoubleArray.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkSmartPointer.h"

namespace
{
const double pi = 3.14159265358979323846264338327950288;
const double twoPi = 6.28318530717958647692528676655900576;
const int dim = 5;
const double eps = 0.00001;
const double tolerance = 0.0001;

bool ArePointsWithinTolerance(double v1, double v2)
{
  if (v1 == v2 || fabs(v1) + fabs(v2) < tolerance)
  {
    return true;
  }

  if (v1 == 0.0)
  {
    if (fabs(v2) < tolerance)
    {
      return true;
    }
    return false;
  }
  if (fabs(fabs(v1) - fabs(v2)) < tolerance)
  {
    return true;
  }
  return false;
}

enum struct CoordinateType
{
  CART = 0,
  CYL,
  SPH
};

void MakeTestDataSet(svtkPolyData* pd, const CoordinateType& coordType)
{
  svtkSmartPointer<svtkDoubleArray> pcoords = svtkSmartPointer<svtkDoubleArray>::New();
  pcoords->SetNumberOfComponents(3);
  pcoords->SetNumberOfTuples(dim * dim);

  if (coordType == CoordinateType::CART)
  {
    for (svtkIdType i = 0; i < dim; ++i)
    {
      double z = static_cast<double>(i) / static_cast<double>(dim - 1);
      for (svtkIdType j = 0; j < dim; ++j)
      {
        double x = static_cast<double>(j) / static_cast<double>(dim - 1);
        double y = (x * x + z * z) / 2.0f;
        pcoords->SetTuple3(i * dim + j, x + 0, y + 0, z + 0);
      }
    }
  }
  else if (coordType == CoordinateType::CYL)
  {
    double R = 1.0f;
    for (svtkIdType i = 0; i < dim; i++)
    {
      double Z = static_cast<double>(i) / static_cast<double>(dim - 1);
      for (svtkIdType j = 0; j < dim; j++)
      {
        double Theta = twoPi * (static_cast<double>(j) / static_cast<double>(dim - 1));
        pcoords->SetTuple3(i * dim + j, R, Theta, Z);
      }
    }
  }
  else if (coordType == CoordinateType::SPH)
  {
    // Spherical coordinates have some degenerate cases, so provide some good cases.
    double R = 1.0f;
    std::vector<double> Thetas = { eps, pi / 4, pi / 3, pi / 2, pi - eps };
    std::vector<double> Phis = { eps, twoPi / 4, twoPi / 3, twoPi / 2, twoPi - eps };
    for (std::size_t i = 0; i < Thetas.size(); i++)
    {
      for (std::size_t j = 0; j < Phis.size(); j++)
      {
        pcoords->SetTuple3(static_cast<svtkIdType>(i * dim + j), R, Thetas[i], Phis[j]);
      }
    }
  }
  pd->GetPoints()->SetData(pcoords);
}

void ValidateCoordTransform(svtkPolyData* pd, svtkPolyData* pdTrans, const std::vector<bool>& isAngle)
{
  svtkPoints* pdPoints = pd->GetPoints();
  svtkPoints* pdTransPoints = pdTrans->GetPoints();
  assert(pdPoints->GetNumberOfPoints() == pdTransPoints->GetNumberOfPoints());
  for (svtkIdType i = 0; i < pdPoints->GetNumberOfPoints(); i++)
  {
    double* point = pdPoints->GetPoint(i);
    double* pointTrans = pdTransPoints->GetPoint(i);
    bool isEqual = true;
    for (size_t j = 0; j < 3; j++)
    {
      if (isAngle[j])
      {
        isEqual &= (ArePointsWithinTolerance(point[j], pointTrans[j]) ||
          ArePointsWithinTolerance(point[j] + static_cast<double>(twoPi), pointTrans[j]) ||
          ArePointsWithinTolerance(point[j], pointTrans[j] + static_cast<double>(twoPi)));
      }
      else
      {
        isEqual &= ArePointsWithinTolerance(point[j], pointTrans[j]);
      }
      if (isEqual == false)
      {
        std::cerr << "i=" << i << " is wrong! result value=" << pointTrans[j]
                  << " target value=" << point[j] << std::endl;
      }
    }
    assert(isEqual == true);
  }
}
}

int TestSVTKMCoordinateSystemTransform(int, char*[])
{
  // Test cartesian to cylindrical
  svtkSmartPointer<svtkPolyData> pdCart = svtkSmartPointer<svtkPolyData>::New();
  svtkSmartPointer<svtkPoints> pdCartPoints = svtkSmartPointer<svtkPoints>::New();
  pdCart->SetPoints(pdCartPoints);
  MakeTestDataSet(pdCart, CoordinateType::CART);

  svtkSmartPointer<svtkmCoordinateSystemTransform> cstFilter =
    svtkSmartPointer<svtkmCoordinateSystemTransform>::New();
  {
    cstFilter->SetInputData(pdCart);
    cstFilter->SetCartesianToCylindrical();
    cstFilter->Update();
    svtkPolyData* pdCarToCyl = svtkPolyData::SafeDownCast(cstFilter->GetOutput());

    svtkSmartPointer<svtkPolyData> pdCarToCylCopy = svtkSmartPointer<svtkPolyData>::New();
    pdCarToCylCopy->ShallowCopy(pdCarToCyl);
    cstFilter->SetInputData(pdCarToCylCopy);
    cstFilter->SetCylindricalToCartesian();
    cstFilter->Update();
    svtkPolyData* pdCylToCar = svtkPolyData::SafeDownCast(cstFilter->GetOutput());
    ValidateCoordTransform(pdCart, pdCylToCar, { false, false, false });
  }

  // Test cylindrical to cartesian
  svtkSmartPointer<svtkPolyData> pdCyl = svtkSmartPointer<svtkPolyData>::New();
  svtkSmartPointer<svtkPoints> pdCylPoints = svtkSmartPointer<svtkPoints>::New();
  pdCyl->SetPoints(pdCylPoints);
  MakeTestDataSet(pdCyl, CoordinateType::CYL);

  {
    cstFilter->SetInputData(pdCyl);
    cstFilter->SetCylindricalToCartesian();
    cstFilter->Update();
    svtkPolyData* pdCylToCal = svtkPolyData::SafeDownCast(cstFilter->GetOutput());

    svtkSmartPointer<svtkPolyData> pdCylToCarCopy = svtkSmartPointer<svtkPolyData>::New();
    pdCylToCarCopy->ShallowCopy(pdCylToCal);
    cstFilter->SetInputData(pdCylToCarCopy);
    cstFilter->SetCartesianToCylindrical();
    cstFilter->Update();
    svtkPolyData* pdCarToCyl = svtkPolyData::SafeDownCast(cstFilter->GetOutput());
    ValidateCoordTransform(pdCyl, pdCarToCyl, { true, true, false });
  }

  // Test cartesian to spherical
  {
    cstFilter->SetInputData(pdCart);
    cstFilter->SetCartesianToSpherical();
    cstFilter->Update();
    svtkPolyData* pdCarToSph = svtkPolyData::SafeDownCast(cstFilter->GetOutput());

    svtkSmartPointer<svtkPolyData> pdCarToSphCopy = svtkSmartPointer<svtkPolyData>::New();
    pdCarToSphCopy->ShallowCopy(pdCarToSph);
    cstFilter->SetInputData(pdCarToSphCopy);
    cstFilter->SetSphericalToCartesian();
    cstFilter->Update();
    svtkPolyData* pdSphToCar = svtkPolyData::SafeDownCast(cstFilter->GetOutput());
    ValidateCoordTransform(pdCart, pdSphToCar, { false, false, false });
  }

  // Test spherical to cartesian
  svtkSmartPointer<svtkPolyData> pdSph = svtkSmartPointer<svtkPolyData>::New();
  svtkSmartPointer<svtkPoints> pdSphPoints = svtkSmartPointer<svtkPoints>::New();
  pdSph->SetPoints(pdSphPoints);
  MakeTestDataSet(pdSph, CoordinateType::SPH);
  {
    cstFilter->SetInputData(pdSph);
    cstFilter->SetSphericalToCartesian();
    cstFilter->Update();
    svtkPolyData* pdSphToCar = svtkPolyData::SafeDownCast(cstFilter->GetOutput());

    svtkSmartPointer<svtkPolyData> pdSphToCarCopy = svtkSmartPointer<svtkPolyData>::New();
    pdSphToCarCopy->ShallowCopy(pdSphToCar);
    cstFilter->SetInputData(pdSphToCarCopy);
    cstFilter->SetCartesianToSpherical();
    cstFilter->Update();
    svtkPolyData* pdCarToSph = svtkPolyData::SafeDownCast(cstFilter->GetOutput());
    ValidateCoordTransform(pdSph, pdCarToSph, { false, true, true });
  }
  return 0;
}
