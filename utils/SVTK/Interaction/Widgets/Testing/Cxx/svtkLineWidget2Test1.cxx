#include "svtkLineRepresentation.h"
#include "svtkLineWidget2.h"

#include <cstdlib>
#include <iostream>

#include "WidgetTestingMacros.h"

int svtkLineWidget2Test1(int, char*[])
{
  svtkSmartPointer<svtkLineWidget2> node1 = svtkSmartPointer<svtkLineWidget2>::New();

  EXERCISE_BASIC_ABSTRACT_METHODS(node1);

  node1->SetProcessEvents(0);
  node1->SetProcessEvents(1);

  svtkSmartPointer<svtkLineRepresentation> rep1 = svtkSmartPointer<svtkLineRepresentation>::New();
  node1->SetRepresentation(rep1);

  return EXIT_SUCCESS;
}
