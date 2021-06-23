#include "svtkHandleWidget.h"
#include "svtkSeedRepresentation.h"
#include "svtkSeedWidget.h"

#include <cstdlib>
#include <iostream>

#include "WidgetTestingMacros.h"
#include "svtkPointHandleRepresentation2D.h"

int svtkSeedWidgetTest1(int, char*[])
{
  svtkSmartPointer<svtkSeedWidget> node1 = svtkSmartPointer<svtkSeedWidget>::New();

  EXERCISE_BASIC_ABSTRACT_METHODS(node1);

  node1->SetProcessEvents(0);
  node1->SetProcessEvents(1);

  svtkSmartPointer<svtkSeedRepresentation> rep1 = svtkSmartPointer<svtkSeedRepresentation>::New();
  node1->SetRepresentation(rep1);

  node1->CompleteInteraction();
  node1->RestartInteraction();

  // have to create a handle rep before create new handle
  svtkSmartPointer<svtkPointHandleRepresentation2D> handle =
    svtkSmartPointer<svtkPointHandleRepresentation2D>::New();
  handle->GetProperty()->SetColor(1, 0, 0);
  rep1->SetHandleRepresentation(handle);

  svtkSmartPointer<svtkHandleWidget> handleWidget = node1->CreateNewHandle();
  if (handleWidget == nullptr)
  {
    std::cerr << "Failed to CreateNewHandle." << std::endl;
    return EXIT_FAILURE;
  }
  svtkSmartPointer<svtkHandleWidget> handleWidget2 = node1->GetSeed(0);
  if (handleWidget2 != handleWidget)
  {
    std::cerr << "Failed to get seed 0 handle" << std::endl;
    return EXIT_FAILURE;
  }

  // try deleting a seed that doesn't exist
  node1->DeleteSeed(100);
  // now delete the one that we added
  node1->DeleteSeed(0);

  return EXIT_SUCCESS;
}
