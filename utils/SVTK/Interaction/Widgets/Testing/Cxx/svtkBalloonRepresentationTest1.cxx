#include "svtkBalloonRepresentation.h"

#include <cstdlib>
#include <iostream>

#include "svtkImageData.h"
#include "svtkProperty2D.h"
#include "svtkTextProperty.h"

#include "WidgetTestingMacros.h"

int svtkBalloonRepresentationTest1(int, char*[])
{
  svtkSmartPointer<svtkBalloonRepresentation> node1 =
    svtkSmartPointer<svtkBalloonRepresentation>::New();

  EXERCISE_BASIC_REPRESENTATION_METHODS(svtkBalloonRepresentation, node1);

  svtkSmartPointer<svtkImageData> imageData = svtkSmartPointer<svtkImageData>::New();
  node1->SetBalloonImage(imageData);
  svtkSmartPointer<svtkImageData> imageData2 = node1->GetBalloonImage();
  if (imageData2 != imageData)
  {
    std::cerr << "Error in Set/Get ImageData" << std::endl;
    return EXIT_FAILURE;
  }
  TEST_SET_GET_STRING(node1, BalloonText);

  TEST_SET_GET_VECTOR2_INT_RANGE(node1, ImageSize, 0, 100);

  svtkSmartPointer<svtkTextProperty> textProp = svtkSmartPointer<svtkTextProperty>::New();
  node1->SetTextProperty(textProp);
  if (node1->GetTextProperty() != textProp)
  {
    std::cerr << "Failure in Set/Get TextProperty" << std::endl;
    return EXIT_FAILURE;
  }

  svtkSmartPointer<svtkProperty2D> frameProp = svtkSmartPointer<svtkProperty2D>::New();
  node1->SetFrameProperty(frameProp);
  if (node1->GetFrameProperty() != frameProp)
  {
    std::cerr << "Failure in Set/Get FrameProperty" << std::endl;
    return EXIT_FAILURE;
  }

  svtkSmartPointer<svtkProperty2D> imageProp = svtkSmartPointer<svtkProperty2D>::New();
  node1->SetImageProperty(imageProp);
  if (node1->GetImageProperty() != imageProp)
  {
    std::cerr << "Failure in Set/Get ImageProperty" << std::endl;
    return EXIT_FAILURE;
  }

  TEST_SET_GET_INT_RANGE(node1, BalloonLayout, 0, 3);
  node1->SetBalloonLayoutToImageLeft();
  node1->SetBalloonLayoutToImageRight();
  node1->SetBalloonLayoutToImageBottom();
  node1->SetBalloonLayoutToImageTop();
  node1->SetBalloonLayoutToTextLeft();
  node1->SetBalloonLayoutToTextRight();
  node1->SetBalloonLayoutToTextTop();
  node1->SetBalloonLayoutToTextBottom();

  TEST_SET_GET_VECTOR2_INT_RANGE(node1, Offset, -1, 1);
  TEST_SET_GET_INT_RANGE(node1, Padding, 1, 99);

  return EXIT_SUCCESS;
}
