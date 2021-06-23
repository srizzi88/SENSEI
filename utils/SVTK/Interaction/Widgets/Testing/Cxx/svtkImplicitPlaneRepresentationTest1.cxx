#include "svtkImplicitPlaneRepresentation.h"

#include <cstdlib>
#include <iostream>

#include "WidgetTestingMacros.h"

#include "svtkPlane.h"
#include "svtkPolyData.h"
#include "svtkPolyDataAlgorithm.h"
#include "svtkProperty.h"

int svtkImplicitPlaneRepresentationTest1(int, char*[])
{
  svtkSmartPointer<svtkImplicitPlaneRepresentation> node1 =
    svtkSmartPointer<svtkImplicitPlaneRepresentation>::New();

  EXERCISE_BASIC_IMPLICIT_PLANE_REPRESENTATION_METHODS(svtkImplicitPlaneRepresentation, node1);

  svtkSmartPointer<svtkPolyData> pd;
  node1->GetPolyData(pd);
  if (pd == nullptr)
  {
    std::cout << "Polydata is null" << std::endl;
  }

  svtkSmartPointer<svtkPolyDataAlgorithm> pda = node1->GetPolyDataAlgorithm();
  if (pda == nullptr)
  {
    std::cout << "Polydata algorithm is null" << std::endl;
  }

  svtkSmartPointer<svtkPlane> plane = svtkSmartPointer<svtkPlane>::New();
  node1->GetPlane(plane);
  if (!plane)
  {
    std::cout << "Plane is null" << std::endl;
  }

  node1->UpdatePlacement();

  svtkSmartPointer<svtkProperty> prop = node1->GetNormalProperty();
  if (prop == nullptr)
  {
    std::cout << "Normal Property is nullptr." << std::endl;
  }
  prop = node1->GetSelectedNormalProperty();
  if (prop == nullptr)
  {
    std::cout << "Selected Normal Property is nullptr." << std::endl;
  }

  prop = node1->GetPlaneProperty();
  if (prop == nullptr)
  {
    std::cout << "Plane Property is nullptr." << std::endl;
  }
  prop = node1->GetSelectedPlaneProperty();
  if (prop == nullptr)
  {
    std::cout << "Selected Plane Property is nullptr." << std::endl;
  }

  prop = node1->GetOutlineProperty();
  if (prop == nullptr)
  {
    std::cout << "Outline Property is nullptr." << std::endl;
  }
  prop = node1->GetSelectedOutlineProperty();
  if (prop == nullptr)
  {
    std::cout << "Selected Outline Property is nullptr." << std::endl;
  }

  prop = node1->GetEdgesProperty();
  if (prop == nullptr)
  {
    std::cout << "Edges Property is nullptr." << std::endl;
  }

  // clamped 0-7
  TEST_SET_GET_INT_RANGE(node1, InteractionState, 1, 6);

  TEST_SET_GET_INT_RANGE(node1, RepresentationState, 0, 5);
  return EXIT_SUCCESS;
}
