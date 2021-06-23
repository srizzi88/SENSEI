#include "svtkPolyLineRepresentation.h"
#include "svtkPolyLineWidget.h"

#include <cstdlib>
#include <iostream>

#include "WidgetTestingMacros.h"

int svtkPolyLineWidgetTest1(int, char*[])
{
  svtkSmartPointer<svtkPolyLineWidget> node1 = svtkSmartPointer<svtkPolyLineWidget>::New();

  EXERCISE_BASIC_ABSTRACT_METHODS(node1);

  svtkSmartPointer<svtkPolyLineRepresentation> rep1 =
    svtkSmartPointer<svtkPolyLineRepresentation>::New();
  node1->SetRepresentation(rep1);

  EXERCISE_BASIC_INTERACTOR_OBSERVER_METHODS(node1);

  return EXIT_SUCCESS;
}
