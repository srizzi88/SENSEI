#include "svtkAngleRepresentation2D.h"
#include "svtkAngleRepresentation3D.h"
#include "svtkAngleWidget.h"

#include <cstdlib>
#include <iostream>

#include "WidgetTestingMacros.h"

int svtkAngleWidgetTest1(int, char*[])
{
  svtkSmartPointer<svtkAngleWidget> node1 = svtkSmartPointer<svtkAngleWidget>::New();

  EXERCISE_BASIC_ABSTRACT_METHODS(node1);

  std::cout << "Angle Valid = " << node1->IsAngleValid() << std::endl;

  node1->SetProcessEvents(1);
  node1->SetProcessEvents(0);

  svtkSmartPointer<svtkAngleRepresentation2D> rep2d =
    svtkSmartPointer<svtkAngleRepresentation2D>::New();
  node1->SetRepresentation(rep2d);

  svtkSmartPointer<svtkAngleRepresentation3D> rep3d =
    svtkSmartPointer<svtkAngleRepresentation3D>::New();
  node1->SetRepresentation(rep3d);

  std::cout << "Can't get at WidgetState, CurrentHandle, subwidgets" << std::endl;

  return EXIT_SUCCESS;
}
