#include "svtkBorderRepresentation.h"
#include "svtkBorderWidget.h"

#include <cstdlib>
#include <iostream>

#include "WidgetTestingMacros.h"

int svtkBorderWidgetTest1(int, char*[])
{
  svtkSmartPointer<svtkBorderWidget> node1 = svtkSmartPointer<svtkBorderWidget>::New();

  EXERCISE_BASIC_BORDER_METHODS(node1);

  svtkSmartPointer<svtkBorderRepresentation> rep1 = svtkSmartPointer<svtkBorderRepresentation>::New();
  node1->SetRepresentation(rep1);

  return EXIT_SUCCESS;
}
