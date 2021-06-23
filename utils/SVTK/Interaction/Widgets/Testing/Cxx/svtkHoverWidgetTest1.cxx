#include "svtkHoverWidget.h"

#include <cstdlib>
#include <iostream>

#include "WidgetTestingMacros.h"

int svtkHoverWidgetTest1(int, char*[])
{
  svtkSmartPointer<svtkHoverWidget> node1 = svtkSmartPointer<svtkHoverWidget>::New();

  EXERCISE_BASIC_HOVER_METHODS(node1);

  return EXIT_SUCCESS;
}
