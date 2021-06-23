#include "svtkOrientedPolygonalHandleRepresentation3D.h"

#include <cstdlib>
#include <iostream>

#include "WidgetTestingMacros.h"

#include "svtkProperty2D.h"

int svtkOrientedPolygonalHandleRepresentation3DTest1(int, char*[])
{
  svtkSmartPointer<svtkOrientedPolygonalHandleRepresentation3D> node1 =
    svtkSmartPointer<svtkOrientedPolygonalHandleRepresentation3D>::New();

  EXERCISE_BASIC_ABSTRACT_POLYGONAL_HANDLE_REPRESENTATION3D_METHODS(
    svtkOrientedPolygonalHandleRepresentation3D, node1);

  return EXIT_SUCCESS;
}
