#include "svtkImplicitPlaneRepresentation.h"
#include "svtkImplicitPlaneWidget2.h"

#include <cstdlib>
#include <iostream>

#include "WidgetTestingMacros.h"

int svtkImplicitPlaneWidget2Test1(int, char*[])
{
  svtkSmartPointer<svtkImplicitPlaneWidget2> node1 = svtkSmartPointer<svtkImplicitPlaneWidget2>::New();

  EXERCISE_BASIC_ABSTRACT_METHODS(node1);

  svtkSmartPointer<svtkImplicitPlaneRepresentation> rep1 =
    svtkSmartPointer<svtkImplicitPlaneRepresentation>::New();
  node1->SetRepresentation(rep1);

  return EXIT_SUCCESS;
}
