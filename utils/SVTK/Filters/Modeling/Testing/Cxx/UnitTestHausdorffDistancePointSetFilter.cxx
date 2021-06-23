/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UnitTestHausdorffDistancePointSetFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkHausdorffDistancePointSetFilter.h"
#include "svtkMathUtilities.h"
#include "svtkMinimalStandardRandomSequence.h"
#include "svtkPolyData.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"

int UnitTestHausdorffDistancePointSetFilter(int, char*[])
{
  int status = 0;

  // Random numbers for radius
  auto rng = svtkSmartPointer<svtkMinimalStandardRandomSequence>::New();

  // Create two spheres, both with the same center
  auto sphereA = svtkSmartPointer<svtkSphereSource>::New();
  sphereA->SetPhiResolution(21);
  sphereA->SetThetaResolution(21);
  auto sphereB = svtkSmartPointer<svtkSphereSource>::New();
  sphereB->SetPhiResolution(21);
  sphereB->SetThetaResolution(21);

  auto numberOfRandomRuns = 50;
  for (auto j = 0; j < 2; ++j)
  {
    for (auto i = 0; i < numberOfRandomRuns; ++i)
    {
      auto hausdorffDistance = svtkSmartPointer<svtkHausdorffDistancePointSetFilter>::New();
      // Generate random radii for the two spheres
      hausdorffDistance->SetTargetDistanceMethod(j);
      rng->Next();
      sphereA->SetRadius(rng->GetRangeValue(1.0, 1000.0));
      rng->Next();
      sphereB->SetRadius(rng->GetRangeValue(1.0, sphereA->GetRadius()));
      sphereB->SetRadius(rng->GetRangeValue(1.0, 1000.0));

      hausdorffDistance->SetInputConnection(0, sphereA->GetOutputPort());
      hausdorffDistance->SetInputConnection(1, sphereB->GetOutputPort());

      hausdorffDistance->Update();
      auto deltaRadius = std::fabs(sphereA->GetRadius() - sphereB->GetRadius());
      if (!svtkMathUtilities::FuzzyCompare(
            hausdorffDistance->GetRelativeDistance()[0], deltaRadius, 1.e-3) &&
        !svtkMathUtilities::FuzzyCompare(
          hausdorffDistance->GetRelativeDistance()[1], deltaRadius, 1.e-3))
      {
        std::cout << "ERROR: "
                  << "Wrong distance..." << std::endl;
        std::cout << "RadiusOuter: " << sphereA->GetRadius() << std::endl;
        std::cout << "RadiusInner: " << sphereB->GetRadius() << std::endl;
        std::cout << "RelativeDistance: " << hausdorffDistance->GetRelativeDistance()[0] << ", "
                  << hausdorffDistance->GetRelativeDistance()[1] << std::endl;
        std::cout << "deltaRadius: " << deltaRadius << std::endl;
        ++status;
      }
      if (i == numberOfRandomRuns - 1)
      {
        hausdorffDistance->Print(std::cout);
      }
    }
  }
  // Now test some error conditions
  auto emptyPoints = svtkSmartPointer<svtkPolyData>::New();
  {
    auto hausdorffDistance = svtkSmartPointer<svtkHausdorffDistancePointSetFilter>::New();
    hausdorffDistance->Update();
    hausdorffDistance->SetInputData(0, emptyPoints);
  }
  {
    auto hausdorffDistance = svtkSmartPointer<svtkHausdorffDistancePointSetFilter>::New();
    hausdorffDistance->Update();
    hausdorffDistance->SetInputData(1, emptyPoints);
  }
  // Exercise some standard methods
  {
    auto hausdorffDistance = svtkSmartPointer<svtkHausdorffDistancePointSetFilter>::New();
    auto newHaus = hausdorffDistance->NewInstance();
    if (!newHaus->IsA("svtkHausdorffDistancePointSetFilter"))
    {
      ++status;
      std::cout << "ERROR: IsA should be svtkHausdorffDistancePointSetFilter, but is "
                << newHaus->GetClassName() << std::endl;
    }
    if (!newHaus->IsTypeOf("svtkPointSetAlgorithm"))
    {
      ++status;
      std::cout << "ERROR: " << newHaus->GetClassName()
                << " is not a subclass of svtkPointSetAlgorithm" << std::endl;
    }
    newHaus->Delete();
  }
  {
    auto hausdorffDistance = svtkSmartPointer<svtkHausdorffDistancePointSetFilter>::New();
    hausdorffDistance->SetInputConnection(0, sphereA->GetOutputPort());
    hausdorffDistance->SetInputConnection(1, sphereB->GetOutputPort());
    hausdorffDistance->Update();

    double relativeDistance[2];
    hausdorffDistance->GetRelativeDistance(relativeDistance);
    double rel1, rel2;
    hausdorffDistance->GetRelativeDistance(rel1, rel2);
    if (rel1 != relativeDistance[0] || rel2 != relativeDistance[1])
    {
      ++status;
      std::cout << "GetRelativeDistance(" << rel1 << "," << rel2 << ")"
                << " does not match GetRelativeDistance(relativeDistance) where "
                << " relativeDistance[0] = " << relativeDistance[0] << " and "
                << " relativeDistance[1] = " << relativeDistance[1] << std::endl;
    }
  }

  if (status)
  {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
