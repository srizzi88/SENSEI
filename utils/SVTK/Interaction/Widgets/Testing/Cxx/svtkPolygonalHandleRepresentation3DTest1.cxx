#include "svtkPolygonalHandleRepresentation3D.h"

#include <cstdlib>
#include <iostream>

#include "WidgetTestingMacros.h"

int svtkPolygonalHandleRepresentation3DTest1(int, char*[])
{
  svtkSmartPointer<svtkPolygonalHandleRepresentation3D> node1 =
    svtkSmartPointer<svtkPolygonalHandleRepresentation3D>::New();

  EXERCISE_BASIC_ABSTRACT_POLYGONAL_HANDLE_REPRESENTATION3D_METHODS(
    svtkPolygonalHandleRepresentation3D, node1);

  TEST_SET_GET_VECTOR3_DOUBLE_RANGE(node1, Offset, -10.0, 10.0);

  return EXIT_SUCCESS;
}
