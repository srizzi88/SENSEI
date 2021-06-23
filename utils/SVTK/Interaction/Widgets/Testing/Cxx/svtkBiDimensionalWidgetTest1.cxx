#include "svtkBiDimensionalRepresentation2D.h"
#include "svtkBiDimensionalWidget.h"

#include <cstdlib>
#include <iostream>

#include "WidgetTestingMacros.h"

int svtkBiDimensionalWidgetTest1(int, char*[])
{
  svtkSmartPointer<svtkBiDimensionalWidget> node1 = svtkSmartPointer<svtkBiDimensionalWidget>::New();

  EXERCISE_BASIC_ABSTRACT_METHODS(node1);

  std::cout << "Measure Valid = " << node1->IsMeasureValid() << std::endl;

  node1->SetProcessEvents(1);
  node1->SetProcessEvents(0);

  svtkSmartPointer<svtkBiDimensionalRepresentation2D> rep1 =
    svtkSmartPointer<svtkBiDimensionalRepresentation2D>::New();
  node1->SetRepresentation(rep1);

  return EXIT_SUCCESS;
}
