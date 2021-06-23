/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UnitTestKernels.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkDoubleArray.h"
#include "svtkEllipsoidalGaussianKernel.h"
#include "svtkGaussianKernel.h"
#include "svtkLinearKernel.h"
#include "svtkMath.h"
#include "svtkMathUtilities.h"
#include "svtkPointData.h"
#include "svtkPointSource.h"
#include "svtkPoints.h"
#include "svtkProbabilisticVoronoiKernel.h"
#include "svtkShepardKernel.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"
#include "svtkStaticPointLocator.h"
#include "svtkVoronoiKernel.h"

#include <cmath>
#include <sstream>
#include <string>

template <typename T>
int TestProbabilisticKernel(svtkSmartPointer<T> kernel, svtkIdType numberOfPoints,
  const std::string& description = "", bool useProbs = true);
template <typename T>
int TestKernel(
  svtkSmartPointer<T> kernel, svtkIdType numberOfPoints, const std::string& description = "");

//-----------------------------------------------------------------------------
int UnitTestKernels(int, char*[])
{
  const svtkIdType numberOfPoints = 100000;
  int status = 0;
  {
    svtkSmartPointer<svtkGaussianKernel> kernel = svtkSmartPointer<svtkGaussianKernel>::New();
    std::ostringstream emptyPrint;
    kernel->Print(emptyPrint);

    kernel->RequiresInitializationOff();
    kernel->SetKernelFootprintToNClosest();
    kernel->SetNumberOfPoints(100);
    kernel->SetSharpness(5.0);
    kernel->NormalizeWeightsOn();
    status += TestProbabilisticKernel<svtkGaussianKernel>(
      kernel, numberOfPoints, "GaussianKernel: NClosest(100): Sharpness(5.0)");
  }
  {
    svtkSmartPointer<svtkGaussianKernel> kernel = svtkSmartPointer<svtkGaussianKernel>::New();
    std::ostringstream emptyPrint;
    kernel->Print(emptyPrint);

    kernel->RequiresInitializationOff();
    kernel->SetKernelFootprintToRadius();
    kernel->SetRadius(.05);
    status += TestProbabilisticKernel<svtkGaussianKernel>(
      kernel, numberOfPoints, "GaussianKernel: Radius(.05)");
  }
  {
    svtkSmartPointer<svtkShepardKernel> kernel = svtkSmartPointer<svtkShepardKernel>::New();
    std::ostringstream emptyPrint;
    kernel->Print(emptyPrint);

    kernel->RequiresInitializationOff();
    kernel->SetKernelFootprintToNClosest();
    kernel->SetNumberOfPoints(100);
    status += TestProbabilisticKernel<svtkShepardKernel>(
      kernel, numberOfPoints, "ShepardKernel: NClosest(100)");
  }
  {
    svtkSmartPointer<svtkShepardKernel> kernel = svtkSmartPointer<svtkShepardKernel>::New();
    std::ostringstream emptyPrint;
    kernel->Print(emptyPrint);

    kernel->RequiresInitializationOff();
    kernel->SetKernelFootprintToRadius();
    kernel->SetRadius(.05);
    status += TestProbabilisticKernel<svtkShepardKernel>(
      kernel, numberOfPoints, "ShepardKernel: Radius(.05)");
  }
  {
    svtkSmartPointer<svtkShepardKernel> kernel = svtkSmartPointer<svtkShepardKernel>::New();
    std::ostringstream emptyPrint;
    kernel->Print(emptyPrint);

    kernel->RequiresInitializationOff();
    kernel->SetKernelFootprintToRadius();
    kernel->SetPowerParameter(10.0);
    kernel->SetRadius(.05);
    status += TestProbabilisticKernel<svtkShepardKernel>(
      kernel, numberOfPoints, "ShepardKernel: Radius(.05) PowerParameter(10)");
  }
  {
    svtkSmartPointer<svtkShepardKernel> kernel = svtkSmartPointer<svtkShepardKernel>::New();
    std::ostringstream emptyPrint;
    kernel->Print(emptyPrint);

    kernel->RequiresInitializationOff();
    kernel->SetKernelFootprintToRadius();
    kernel->SetPowerParameter(1.0);
    kernel->SetRadius(.05);
    status += TestProbabilisticKernel<svtkShepardKernel>(
      kernel, numberOfPoints, "ShepardKernel: Radius(.05) PowerParameter(1)");
  }
  {
    svtkSmartPointer<svtkProbabilisticVoronoiKernel> kernel =
      svtkSmartPointer<svtkProbabilisticVoronoiKernel>::New();
    std::ostringstream emptyPrint;
    kernel->Print(emptyPrint);

    kernel->RequiresInitializationOff();
    kernel->SetKernelFootprintToNClosest();
    kernel->SetNumberOfPoints(100);
    status += TestProbabilisticKernel<svtkProbabilisticVoronoiKernel>(
      kernel, numberOfPoints, "ProbabilisticVoronoiKernel: NClosest(100)");
  }
  {
    svtkSmartPointer<svtkProbabilisticVoronoiKernel> kernel =
      svtkSmartPointer<svtkProbabilisticVoronoiKernel>::New();
    std::ostringstream emptyPrint;
    kernel->Print(emptyPrint);

    kernel->RequiresInitializationOff();
    kernel->SetKernelFootprintToRadius();
    kernel->SetRadius(.05);
    status += TestProbabilisticKernel<svtkProbabilisticVoronoiKernel>(
      kernel, numberOfPoints, "ProbabilisticVoronoiKernel: Radius(.05)");
  }
  {
    svtkSmartPointer<svtkLinearKernel> kernel = svtkSmartPointer<svtkLinearKernel>::New();
    std::ostringstream emptyPrint;
    kernel->Print(emptyPrint);

    kernel->RequiresInitializationOff();
    kernel->SetKernelFootprintToNClosest();
    kernel->SetNumberOfPoints(100);
    status += TestProbabilisticKernel<svtkLinearKernel>(
      kernel, numberOfPoints, "LinearKernel: NClosest(100)");
  }
  {
    svtkSmartPointer<svtkLinearKernel> kernel = svtkSmartPointer<svtkLinearKernel>::New();
    std::ostringstream emptyPrint;
    kernel->Print(emptyPrint);

    kernel->RequiresInitializationOff();
    kernel->SetKernelFootprintToRadius();
    kernel->SetRadius(.05);
    status +=
      TestProbabilisticKernel<svtkLinearKernel>(kernel, numberOfPoints, "LinearKernel: Radius(.05)");
  }
  {
    svtkSmartPointer<svtkLinearKernel> kernel = svtkSmartPointer<svtkLinearKernel>::New();
    std::ostringstream emptyPrint;
    kernel->Print(emptyPrint);

    kernel->RequiresInitializationOff();
    kernel->SetKernelFootprintToRadius();
    kernel->SetRadius(.05);
    status += TestProbabilisticKernel<svtkLinearKernel>(
      kernel, numberOfPoints, "LinearKernel: Radius(.05), No Probabilities", false);
  }
  {
    svtkSmartPointer<svtkEllipsoidalGaussianKernel> kernel =
      svtkSmartPointer<svtkEllipsoidalGaussianKernel>::New();
    std::ostringstream emptyPrint;
    kernel->Print(emptyPrint);
    std::ostringstream superPrint;
    kernel->Superclass::Print(superPrint);

    kernel->UseNormalsOff();
    kernel->UseScalarsOn();
    kernel->SetScaleFactor(2.0);

    kernel->SetScalarsArrayName("TestDistances");
    kernel->RequiresInitializationOff();
    kernel->SetRadius(.05);
    status += TestKernel<svtkEllipsoidalGaussianKernel>(
      kernel, numberOfPoints, "EllipsoidalGaussianKernel: Radius(.05)");
  }
  {
    svtkSmartPointer<svtkEllipsoidalGaussianKernel> kernel =
      svtkSmartPointer<svtkEllipsoidalGaussianKernel>::New();
    std::ostringstream emptyPrint;
    kernel->Print(emptyPrint);

    kernel->RequiresInitializationOff();
    kernel->UseNormalsOn();
    kernel->SetNormalsArrayName("TestNormals");
    kernel->UseScalarsOff();
    kernel->SetRadius(.05);
    kernel->SetSharpness(5.0);
    status += TestKernel<svtkEllipsoidalGaussianKernel>(
      kernel, numberOfPoints, "EllipsoidalGaussianKernel: Radius(.05) Sharpness(5.0)");
  }
  {
    svtkSmartPointer<svtkEllipsoidalGaussianKernel> kernel =
      svtkSmartPointer<svtkEllipsoidalGaussianKernel>::New();
    std::ostringstream emptyPrint;
    kernel->Print(emptyPrint);

    kernel->RequiresInitializationOff();
    kernel->SetRadius(.05);
    kernel->SetEccentricity(.1);
    status += TestKernel<svtkEllipsoidalGaussianKernel>(
      kernel, numberOfPoints, "EllipsoidalGaussianKernel: Radius(.05) Eccentricity(.1)");
  }
  {
    svtkSmartPointer<svtkEllipsoidalGaussianKernel> kernel =
      svtkSmartPointer<svtkEllipsoidalGaussianKernel>::New();
    std::ostringstream emptyPrint;
    kernel->Print(emptyPrint);

    kernel->RequiresInitializationOff();
    kernel->SetRadius(.05);
    kernel->SetEccentricity(10.0);
    status += TestKernel<svtkEllipsoidalGaussianKernel>(
      kernel, numberOfPoints, "EllipsoidalGaussianKernel: Radius(.05) Eccentricity(10.0)");
  }
  {
    svtkSmartPointer<svtkVoronoiKernel> kernel = svtkSmartPointer<svtkVoronoiKernel>::New();
    std::ostringstream emptyPrint;
    kernel->Print(emptyPrint);

    kernel->RequiresInitializationOff();
    status += TestKernel<svtkVoronoiKernel>(kernel, numberOfPoints, "VoronoiKernel");
  }
  return status;
}

template <typename T>
int TestProbabilisticKernel(svtkSmartPointer<T> kernel, svtkIdType numberOfPoints,
  const std::string& description, bool useProbs)
{
  int status = EXIT_SUCCESS;

  std::cout << "Testing " << description;

  if (!kernel->IsTypeOf("svtkGeneralizedKernel"))
  {
    std::cout << " ERROR: " << kernel->GetClassName()
              << " is not a subclass of svtkGeneralizedKernel";
    std::cout << " FAILED" << std::endl;
    status = EXIT_FAILURE;
  }
  if (!kernel->IsTypeOf("svtkInterpolationKernel"))
  {
    std::cout << " ERROR: " << kernel->GetClassName()
              << " is not a subclass of svtkInterpolationKernel";
    std::cout << " FAILED" << std::endl;
    status = EXIT_FAILURE;
  }

  svtkSmartPointer<svtkSphereSource> sphere = svtkSmartPointer<svtkSphereSource>::New();
  sphere->SetPhiResolution(11);
  sphere->SetThetaResolution(21);
  sphere->SetRadius(.5);
  sphere->Update();

  svtkSmartPointer<svtkPointSource> randomSphere = svtkSmartPointer<svtkPointSource>::New();
  randomSphere->SetRadius(sphere->GetRadius() * 2.0);
  randomSphere->SetNumberOfPoints(numberOfPoints);
  randomSphere->Update();
  svtkSmartPointer<svtkDoubleArray> distances = svtkSmartPointer<svtkDoubleArray>::New();
  distances->SetNumberOfTuples(randomSphere->GetOutput()->GetNumberOfPoints());

  double refPt[3];
  refPt[0] = 0.0;
  refPt[1] = 0.0;
  refPt[2] = 0.0;
  for (svtkIdType id = 0; id < randomSphere->GetOutput()->GetNumberOfPoints(); ++id)
  {
    double distance;
    double pt[3];

    randomSphere->GetOutput()->GetPoint(id, pt);
    distance = std::sqrt(svtkMath::Distance2BetweenPoints(refPt, pt));
    distances->SetTuple1(id, distance);
  }
  distances->SetName("Distances");

  randomSphere->GetOutput()->GetPointData()->SetScalars(distances);

  svtkSmartPointer<svtkStaticPointLocator> locator = svtkSmartPointer<svtkStaticPointLocator>::New();
  locator->SetDataSet(randomSphere->GetOutput());
  double meanProbe = 0.0;
  kernel->Initialize(locator, randomSphere->GetOutput(), randomSphere->GetOutput()->GetPointData());

  std::ostringstream fullPrint;
  kernel->Print(fullPrint);

  for (svtkIdType id = 0; id < sphere->GetOutput()->GetNumberOfPoints(); ++id)
  {
    double point[3];
    sphere->GetOutput()->GetPoints()->GetPoint(id, point);
    svtkSmartPointer<svtkIdList> ptIds = svtkSmartPointer<svtkIdList>::New();
    svtkSmartPointer<svtkDoubleArray> weights = svtkSmartPointer<svtkDoubleArray>::New();
    kernel->ComputeBasis(point, ptIds);
    svtkSmartPointer<svtkDoubleArray> probabilities = svtkSmartPointer<svtkDoubleArray>::New();
    probabilities->SetNumberOfTuples(ptIds->GetNumberOfIds());
    for (svtkIdType p = 0; p < ptIds->GetNumberOfIds(); ++p)
    {
      double distance;
      double pt[3];
      randomSphere->GetOutput()->GetPoint(p, pt);
      distance = std::sqrt(svtkMath::Distance2BetweenPoints(refPt, pt));
      probabilities->SetTuple1(p, (2.0 - distance) / 2.0);
    }
    if (useProbs)
    {
      kernel->ComputeWeights(point, ptIds, probabilities, weights);
    }
    else
    {
      kernel->ComputeWeights(point, ptIds, nullptr, weights);
    }
    double scalar;
    randomSphere->GetOutput()->GetPointData()->GetArray("Distances")->GetTuple(id, &scalar);
    double probe = 0.0;
    if (id == 0)
    {
      std::cout << " # points: " << ptIds->GetNumberOfIds();
    }
    for (svtkIdType p = 0; p < ptIds->GetNumberOfIds(); ++p)
    {
      double value;
      randomSphere->GetOutput()
        ->GetPointData()
        ->GetArray("Distances")
        ->GetTuple(ptIds->GetId(p), &value);
      double weight;
      weights->GetTuple(p, &weight);
      probe += weight * value;
    }
    meanProbe += probe;
  }
  meanProbe /= static_cast<double>(sphere->GetOutput()->GetNumberOfPoints());
  std::cout << " Mean probe:" << meanProbe;

  if (!svtkMathUtilities::FuzzyCompare(meanProbe, .5, .01))
  {
    std::cout << " ERROR: Mean of the probes: " << meanProbe
              << " is not within .01 of the radius .5";
    std::cout << " FAILED" << std::endl;
    status = EXIT_FAILURE;
  }

  // Test for exact points
  svtkSmartPointer<svtkStaticPointLocator> exactLocator =
    svtkSmartPointer<svtkStaticPointLocator>::New();
  exactLocator->SetDataSet(sphere->GetOutput());
  svtkSmartPointer<svtkDoubleArray> radii = svtkSmartPointer<svtkDoubleArray>::New();
  radii->SetNumberOfTuples(sphere->GetOutput()->GetNumberOfPoints());
  radii->FillComponent(0, .5);
  sphere->GetOutput()->GetPointData()->SetScalars(radii);
  kernel->Initialize(exactLocator, sphere->GetOutput(), sphere->GetOutput()->GetPointData());
  for (svtkIdType id = 0; id < sphere->GetOutput()->GetNumberOfPoints(); ++id)
  {
    double point[3];
    sphere->GetOutput()->GetPoints()->GetPoint(id, point);
    svtkSmartPointer<svtkIdList> ptIds = svtkSmartPointer<svtkIdList>::New();
    kernel->ComputeBasis(point, ptIds);
    svtkSmartPointer<svtkDoubleArray> weights = svtkSmartPointer<svtkDoubleArray>::New();
    kernel->ComputeWeights(point, ptIds, nullptr, weights);

    double probe = 0.0;
    for (svtkIdType p = 0; p < ptIds->GetNumberOfIds(); ++p)
    {
      double value;
      sphere->GetOutput()->GetPointData()->GetScalars()->GetTuple(ptIds->GetId(p), &value);
      double weight;
      weights->GetTuple(p, &weight);
      probe += weight * value;
    }
    if (!svtkMathUtilities::FuzzyCompare(probe, .5, std::numeric_limits<double>::epsilon() * 256.0))
    {
      status = EXIT_FAILURE;
      std::cout << "Expected .5 but got " << probe << std::endl;
    }
  }

  if (status == EXIT_SUCCESS)
  {
    std::cout << " PASSED" << std::endl;
  }
  return status;
}

template <typename T>
int TestKernel(svtkSmartPointer<T> kernel, svtkIdType numberOfPoints, const std::string& description)
{
  int status = 0;
  std::cout << "Testing " << description;
  svtkSmartPointer<svtkSphereSource> sphere = svtkSmartPointer<svtkSphereSource>::New();
  sphere->SetPhiResolution(21);
  sphere->SetThetaResolution(21);
  sphere->SetRadius(.5);
  sphere->Update();

  svtkSmartPointer<svtkPointSource> randomSphere = svtkSmartPointer<svtkPointSource>::New();
  randomSphere->SetRadius(sphere->GetRadius() * 2.0);
  randomSphere->SetNumberOfPoints(numberOfPoints);
  randomSphere->Update();
  svtkSmartPointer<svtkDoubleArray> distances = svtkSmartPointer<svtkDoubleArray>::New();
  distances->SetNumberOfTuples(randomSphere->GetOutput()->GetNumberOfPoints());
  svtkSmartPointer<svtkDoubleArray> normals = svtkSmartPointer<svtkDoubleArray>::New();
  normals->SetNumberOfComponents(3);
  normals->SetNumberOfTuples(randomSphere->GetOutput()->GetNumberOfPoints());

  double refPt[3];
  refPt[0] = 0.0;
  refPt[1] = 0.0;
  refPt[2] = 0.0;
  for (svtkIdType id = 0; id < randomSphere->GetOutput()->GetNumberOfPoints(); ++id)
  {
    double distance;
    double pt[3];
    randomSphere->GetOutput()->GetPoint(id, pt);
    distance = std::sqrt(svtkMath::Distance2BetweenPoints(refPt, pt));
    distances->SetTuple1(id, distance);
    double normal[3];
    normal[0] = pt[0];
    normal[1] = pt[1];
    normal[2] = pt[2];
    normals->SetTuple3(id, normal[0], normal[1], normal[2]);
  }
  distances->SetName("TestDistances");
  normals->SetName("TestNormals");

  randomSphere->GetOutput()->GetPointData()->AddArray(distances);
  randomSphere->GetOutput()->GetPointData()->AddArray(normals);

  svtkSmartPointer<svtkStaticPointLocator> locator = svtkSmartPointer<svtkStaticPointLocator>::New();
  locator->SetDataSet(randomSphere->GetOutput());
  double meanProbe = 0.0;
  kernel->Initialize(locator, randomSphere->GetOutput(), randomSphere->GetOutput()->GetPointData());

  std::ostringstream fullPrint;
  kernel->Print(fullPrint);
  for (svtkIdType id = 0; id < sphere->GetOutput()->GetNumberOfPoints(); ++id)
  {
    double point[3];
    sphere->GetOutput()->GetPoints()->GetPoint(id, point);
    svtkSmartPointer<svtkIdList> ptIds = svtkSmartPointer<svtkIdList>::New();
    svtkSmartPointer<svtkDoubleArray> weights = svtkSmartPointer<svtkDoubleArray>::New();
    kernel->ComputeBasis(point, ptIds);
    kernel->ComputeWeights(point, ptIds, weights);
    if (id == 0)
    {
      std::cout << " # points: " << ptIds->GetNumberOfIds();
    }
    double scalar;
    randomSphere->GetOutput()->GetPointData()->GetArray("TestDistances")->GetTuple(id, &scalar);
    double probe = 0.0;
    for (svtkIdType p = 0; p < ptIds->GetNumberOfIds(); ++p)
    {
      double value;
      randomSphere->GetOutput()
        ->GetPointData()
        ->GetArray("TestDistances")
        ->GetTuple(ptIds->GetId(p), &value);
      double weight;
      weights->GetTuple(p, &weight);
      probe += weight * value;
    }
    meanProbe += probe;
  }
  meanProbe /= static_cast<double>(sphere->GetOutput()->GetNumberOfPoints());
  std::cout << " Mean probe:" << meanProbe;
  if (!svtkMathUtilities::FuzzyCompare(meanProbe, .5, .01))
  {
    std::cout << "ERROR: Mean of the probes: " << meanProbe
              << " is not within .01 of the radius .5";
    std::cout << " FAILED" << std::endl;
    status = EXIT_FAILURE;
  }

  // Test for exact points
  svtkSmartPointer<svtkStaticPointLocator> exactLocator =
    svtkSmartPointer<svtkStaticPointLocator>::New();
  exactLocator->SetDataSet(sphere->GetOutput());
  svtkSmartPointer<svtkDoubleArray> radii = svtkSmartPointer<svtkDoubleArray>::New();
  radii->SetNumberOfTuples(sphere->GetOutput()->GetNumberOfPoints());
  radii->FillComponent(0, .5);
  sphere->GetOutput()->GetPointData()->SetScalars(radii);
  kernel->Initialize(exactLocator, sphere->GetOutput(), sphere->GetOutput()->GetPointData());
  for (svtkIdType id = 0; id < sphere->GetOutput()->GetNumberOfPoints(); ++id)
  {
    double point[3];
    sphere->GetOutput()->GetPoints()->GetPoint(id, point);
    svtkSmartPointer<svtkIdList> ptIds = svtkSmartPointer<svtkIdList>::New();
    svtkSmartPointer<svtkDoubleArray> weights = svtkSmartPointer<svtkDoubleArray>::New();
    kernel->ComputeBasis(point, ptIds);
    kernel->ComputeWeights(point, ptIds, weights);

    double probe = 0.0;
    for (svtkIdType p = 0; p < ptIds->GetNumberOfIds(); ++p)
    {
      double value;
      sphere->GetOutput()->GetPointData()->GetScalars()->GetTuple(ptIds->GetId(p), &value);
      double weight;
      weights->GetTuple(p, &weight);
      probe += weight * value;
    }
    if (!svtkMathUtilities::FuzzyCompare(probe, .5, std::numeric_limits<double>::epsilon() * 256.0))
    {
      status = EXIT_FAILURE;
      std::cout << "Expected .5 but got " << probe << std::endl;
    }
  }

  if (status == EXIT_SUCCESS)
  {
    std::cout << " PASSED" << std::endl;
  }
  return status;
}
