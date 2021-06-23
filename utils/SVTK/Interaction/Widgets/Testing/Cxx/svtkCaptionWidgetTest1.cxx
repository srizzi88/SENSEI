#include "svtkCaptionActor2D.h"
#include "svtkCaptionRepresentation.h"
#include "svtkCaptionWidget.h"

#include <cstdlib>
#include <iostream>

#include "WidgetTestingMacros.h"

int svtkCaptionWidgetTest1(int, char*[])
{
  svtkSmartPointer<svtkCaptionWidget> node1 = svtkSmartPointer<svtkCaptionWidget>::New();

  EXERCISE_BASIC_BORDER_METHODS(node1);

  svtkSmartPointer<svtkCaptionRepresentation> rep1 = svtkSmartPointer<svtkCaptionRepresentation>::New();
  node1->SetRepresentation(rep1);

  svtkCaptionActor2D* captionActor = node1->GetCaptionActor2D();
  if (captionActor)
  {
    std::cout << "Caption actor is not null" << std::endl;
  }
  else
  {
    std::cout << "Caption actor is null" << std::endl;
  }
  svtkSmartPointer<svtkCaptionActor2D> captionActor2 = svtkSmartPointer<svtkCaptionActor2D>::New();
  node1->SetCaptionActor2D(captionActor2);
  if (node1->GetCaptionActor2D() != captionActor2)
  {
    std::cerr << "Failed to get back expected caption actor" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
