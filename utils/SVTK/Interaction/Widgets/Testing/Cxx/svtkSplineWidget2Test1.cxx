#include "svtkSplineRepresentation.h"
#include "svtkSplineWidget2.h"

#include <cstdlib>
#include <iostream>

#include "WidgetTestingMacros.h"

int svtkSplineWidget2Test1(int, char*[])
{
  svtkSmartPointer<svtkSplineWidget2> node1 = svtkSmartPointer<svtkSplineWidget2>::New();

  EXERCISE_BASIC_ABSTRACT_METHODS(node1);

  svtkSmartPointer<svtkSplineRepresentation> rep1 = svtkSmartPointer<svtkSplineRepresentation>::New();
  node1->SetRepresentation(rep1);

  return EXIT_SUCCESS;
}
