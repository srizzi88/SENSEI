#include "svtkCaptionRepresentation.h"

#include <cstdlib>
#include <iostream>

#include "svtkImageData.h"
#include "svtkProperty2D.h"
#include "svtkTextProperty.h"

#include "WidgetTestingMacros.h"

#include "svtkCaptionActor2D.h"
#include "svtkPointHandleRepresentation3D.h"

int svtkCaptionRepresentationTest1(int, char*[])
{
  svtkSmartPointer<svtkCaptionRepresentation> node1 =
    svtkSmartPointer<svtkCaptionRepresentation>::New();

  EXERCISE_BASIC_BORDER_REPRESENTATION_METHODS(svtkCaptionRepresentation, node1);

  double pos[3] = { -99.0, 100.0, 50.0 };
  node1->SetAnchorPosition(pos);
  double pos2[3];
  node1->GetAnchorPosition(pos2);
  if (pos2[0] != pos[0] || pos2[1] != pos[1] || pos2[2] != pos[2])
  {
    std::cerr << "Failure in Get/Set AnchorPosition, expected " << pos[0] << ", " << pos[1] << ", "
              << pos[2] << ", instead got " << pos2[0] << ", " << pos2[1] << ", " << pos2[2]
              << std::endl;
    return EXIT_FAILURE;
  }

  svtkSmartPointer<svtkCaptionActor2D> captionActor = svtkSmartPointer<svtkCaptionActor2D>::New();
  node1->SetCaptionActor2D(captionActor);
  if (node1->GetCaptionActor2D() != captionActor)
  {
    std::cerr << "Failure in Get/Set CaptionActor2D." << std::endl;
    return EXIT_FAILURE;
  }

  svtkSmartPointer<svtkPointHandleRepresentation3D> handleRep =
    svtkSmartPointer<svtkPointHandleRepresentation3D>::New();
  node1->SetAnchorRepresentation(handleRep);
  if (node1->GetAnchorRepresentation() != handleRep)
  {
    std::cerr << "Failure in Get/Set AnchorRepresentation." << std::endl;
    return EXIT_FAILURE;
  }
  TEST_SET_GET_DOUBLE_RANGE(node1, FontFactor, 1.1, 9.0);

  return EXIT_SUCCESS;
}
