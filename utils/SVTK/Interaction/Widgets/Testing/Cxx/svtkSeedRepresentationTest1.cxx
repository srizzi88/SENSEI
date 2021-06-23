/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSeedRepresentationTest1.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSeedRepresentation.h"

#include <cstdlib>
#include <iostream>

#include "svtkImageData.h"
#include "svtkProperty2D.h"
#include "svtkTextProperty.h"

#include "WidgetTestingMacros.h"
#include "svtkTestErrorObserver.h"

#include "svtkPointHandleRepresentation3D.h"
#include "svtkTextActor.h"

int svtkSeedRepresentationTest1(int, char*[])
{
  svtkSmartPointer<svtkSeedRepresentation> node1 = svtkSmartPointer<svtkSeedRepresentation>::New();

  EXERCISE_BASIC_REPRESENTATION_METHODS(svtkSeedRepresentation, node1);

  std::cout << "Number of Seeds = " << node1->GetNumberOfSeeds() << std::endl;

  double pos[3] = { 1.0, 2.0, -3.0 };
  double pos2[3];
  int s = 0;

  // set/get display and world should fail without seeds
  svtkSmartPointer<svtkTest::ErrorObserver> errorObserver =
    svtkSmartPointer<svtkTest::ErrorObserver>::New();
  node1->AddObserver(svtkCommand::ErrorEvent, errorObserver);
  node1->SetSeedDisplayPosition(s, pos);

  int status = errorObserver->CheckErrorMessage("Trying to access non-existent handle");
  node1->GetSeedWorldPosition(s, pos2);
  status += errorObserver->CheckErrorMessage("Trying to access non-existent handle");
  node1->GetSeedDisplayPosition(s, pos);
  status += errorObserver->CheckErrorMessage("Trying to access non-existent handle");

  // set/get display and world position will fail without seeds having been
  // created, so add some and then do the testing of return values.

  // have to set rep first
  svtkSmartPointer<svtkPointHandleRepresentation3D> handleRep =
    svtkSmartPointer<svtkPointHandleRepresentation3D>::New();
  node1->SetHandleRepresentation(handleRep);

  double e[2] = { 10.0, 10.0 };
  int numSeeds = 10;
  for (int n = 0; n < numSeeds; n++)
  {
    int handleNum = node1->CreateHandle(e);
    std::cout << "Created handle number " << handleNum << std::endl;
    e[0] -= 1.0;
    e[1] += 1.0;
  }
  std::cout << "Number of Seeds = " << node1->GetNumberOfSeeds() << std::endl;

  node1->SetSeedDisplayPosition(s, pos);
  node1->GetSeedDisplayPosition(s, pos2);
  if (pos2[0] != pos[0] || pos2[1] != pos[1])
  {
    std::cerr << "Error in Set/Get Seed display position " << s << ", expected " << pos[0] << ", "
              << pos[1] << ", instead got " << pos2[0] << ", " << pos2[1] << std::endl;
    return EXIT_FAILURE;
  }

  node1->GetSeedWorldPosition(s, pos2);
  std::cout << "Get Seed world position " << s << " = " << pos2[0] << ", " << pos2[1] << ", "
            << pos2[2] << std::endl;

  svtkSmartPointer<svtkPointHandleRepresentation3D> handleRep2;
  handleRep2 = svtkPointHandleRepresentation3D::SafeDownCast(node1->GetHandleRepresentation());
  if (handleRep2 == nullptr || handleRep2 != handleRep)
  {
    std::cerr << "Error in Set/Get handle rep at top level." << std::endl;
    return EXIT_FAILURE;
  }
  handleRep2 = svtkPointHandleRepresentation3D::SafeDownCast(node1->GetHandleRepresentation(0));
  if (handleRep2 == nullptr)
  {
    std::cerr << "Error in Set/Get handle rep 0." << std::endl;
    return EXIT_FAILURE;
  }

  // clamped 1,100
  TEST_SET_GET_INT_RANGE(node1, Tolerance, 2, 99);

  int activeHandle = node1->GetActiveHandle();
  std::cout << "Active Handle = " << activeHandle << std::endl;

  node1->RemoveLastHandle();
  node1->RemoveActiveHandle();

  node1->RemoveHandle(0);

  return EXIT_SUCCESS;
}
