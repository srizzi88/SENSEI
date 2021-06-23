#include "svtkAngleRepresentation2D.h"

#include <cstdlib>
#include <iostream>

#include "WidgetTestingMacros.h"

int svtkAngleRepresentation2DTest1(int, char*[])
{
  svtkSmartPointer<svtkAngleRepresentation2D> node1 =
    svtkSmartPointer<svtkAngleRepresentation2D>::New();

  EXERCISE_BASIC_ANGLE_REPRESENTATION_METHODS(svtkAngleRepresentation2D, node1);

  svtkLeaderActor2D* actor = node1->GetRay1();
  if (actor == nullptr)
  {
    std::cout << "Ray 1 is null." << std::endl;
  }
  actor = node1->GetRay2();
  if (actor == nullptr)
  {
    std::cout << "Ray 2 is null." << std::endl;
  }
  actor = node1->GetArc();
  if (actor == nullptr)
  {
    std::cout << "Arc is null." << std::endl;
  }
  return EXIT_SUCCESS;
}
