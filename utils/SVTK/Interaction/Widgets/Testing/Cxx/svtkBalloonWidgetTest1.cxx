#include "svtkAbstractPropPicker.h"
#include "svtkActor.h"
#include "svtkBalloonRepresentation.h"
#include "svtkBalloonWidget.h"
#include "svtkCellPicker.h"
#include "svtkImageData.h"
#include "svtkProp.h"
#include "svtkStdString.h"

#include <cstdlib>
#include <iostream>

#include "WidgetTestingMacros.h"

int svtkBalloonWidgetTest1(int, char*[])
{
  svtkSmartPointer<svtkBalloonWidget> node1 = svtkSmartPointer<svtkBalloonWidget>::New();
  // failing at all of these
  //  EXERCISE_BASIC_HOVER_METHODS (node1 );
  //  EXERCISE_BASIC_ABSTRACT_METHODS (node1);
  //  EXERCISE_BASIC_INTERACTOR_OBSERVER_METHODS(node1);
  EXERCISE_BASIC_OBJECT_METHODS(node1);
  svtkSmartPointer<svtkBalloonRepresentation> rep1 = svtkSmartPointer<svtkBalloonRepresentation>::New();
  node1->SetRepresentation(rep1);

  svtkSmartPointer<svtkActor> prop1 = svtkSmartPointer<svtkActor>::New();
  svtkSmartPointer<svtkImageData> imageData = svtkSmartPointer<svtkImageData>::New();
  svtkStdString stdString = "something with a space";
  const char* cstr = "string1";
  const char* retstr = nullptr;

  node1->AddBalloon(prop1, stdString, imageData);
  retstr = node1->GetBalloonString(prop1);
  if (!retstr)
  {
    std::cerr << "1. Get null return string." << std::endl;
    return EXIT_FAILURE;
  }
  if (stdString.compare(retstr) != 0)
  {
    std::cerr << "1. Expected " << stdString << ", got " << retstr << std::endl;
    return EXIT_FAILURE;
  }

  node1->AddBalloon(prop1, cstr, imageData);
  retstr = node1->GetBalloonString(prop1);
  if (!retstr)
  {
    std::cerr << "2. Get null return string." << std::endl;
    return EXIT_FAILURE;
  }
  if (strcmp(retstr, cstr) != 0)
  {
    std::cerr << "2. Expected " << cstr << ", got " << retstr << std::endl;
    return EXIT_FAILURE;
  }

  node1->AddBalloon(prop1, "string2", imageData);
  // check the image data first, since adding other balloons resets it
  svtkImageData* retImageData = node1->GetBalloonImage(prop1);
  if (retImageData != imageData)
  {
    std::cerr << "Didn't get back expected image data" << std::endl;
    return EXIT_FAILURE;
  }
  retstr = node1->GetBalloonString(prop1);
  if (!retstr)
  {
    std::cerr << "3. Get null return string." << std::endl;
    return EXIT_FAILURE;
  }
  if (strcmp(retstr, "string2") != 0)
  {
    std::cerr << "3. Expected 'string2', got " << retstr << std::endl;
    return EXIT_FAILURE;
  }

  node1->AddBalloon(prop1, cstr);
  retstr = node1->GetBalloonString(prop1);
  if (!retstr)
  {
    std::cerr << "4. Get null return string." << std::endl;
    return EXIT_FAILURE;
  }
  if (strcmp(retstr, cstr) != 0)
  {
    std::cerr << "4. Expected " << cstr << ", got " << retstr << std::endl;
    return EXIT_FAILURE;
  }

  node1->AddBalloon(prop1, "string3");
  retstr = node1->GetBalloonString(prop1);
  if (!retstr)
  {
    std::cerr << "5. Get null return string." << std::endl;
    return EXIT_FAILURE;
  }
  if (strcmp(retstr, "string3") != 0)
  {
    std::cerr << "5. Expected 'string3', got " << retstr << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
