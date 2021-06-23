#include "svtkTextActor.h"
#include "svtkTextRepresentation.h"
#include "svtkTextWidget.h"

#include <cstdlib>
#include <iostream>

#include "WidgetTestingMacros.h"

int svtkTextWidgetTest1(int, char*[])
{
  svtkSmartPointer<svtkTextWidget> node1 = svtkSmartPointer<svtkTextWidget>::New();

  EXERCISE_BASIC_BORDER_METHODS(node1);

  svtkSmartPointer<svtkTextRepresentation> rep1 = svtkSmartPointer<svtkTextRepresentation>::New();
  node1->SetRepresentation(rep1);

  svtkTextActor* textActor = node1->GetTextActor();
  if (textActor)
  {
    std::cout << "Text actor is not null" << std::endl;
  }
  else
  {
    std::cout << "Text actor is null" << std::endl;
  }
  svtkSmartPointer<svtkTextActor> textActor2 = svtkSmartPointer<svtkTextActor>::New();
  node1->SetTextActor(textActor2);
  if (node1->GetTextActor() != textActor2)
  {
    std::cerr << "Failed to get back expected text actor" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
