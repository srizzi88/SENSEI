/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestVector.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Charts includes
#include "svtkChartXY.h"
#include "svtkColorTransferControlPointsItem.h"
#include "svtkColorTransferFunction.h"
#include "svtkContextInteractorStyle.h"
#include "svtkContextScene.h"
#include "svtkControlPointsItem.h"

// Common includes"
#include "svtkIdTypeArray.h"
#include "svtkInteractorEventRecorder.h"
#include "svtkNew.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkSmartPointer.h"

// STD includes
#include <iostream>
#include <map>

//----------------------------------------------------------------------------
class svtkTFCallback : public svtkCommand
{
public:
  static svtkTFCallback* New() { return new svtkTFCallback; }

  svtkTFCallback() = default;

  void Execute(svtkObject* caller, unsigned long event, void* svtkNotUsed(callData)) override
  {
    svtkColorTransferFunction* self = reinterpret_cast<svtkColorTransferFunction*>(caller);
    if (!self)
    {
      return;
    }
    if (this->EventSpy.count(event) == 0)
    {
      this->EventSpy[event] = 0;
    }
    ++this->EventSpy[event];
    std::cout << "InvokedEvent: " << event << this->EventSpy[event] << std::endl;
  }
  std::map<unsigned long, int> EventSpy;
};

//----------------------------------------------------------------------------
int TestControlPointsItemEvents(int, char*[])
{
  svtkNew<svtkColorTransferFunction> transferFunction;
  transferFunction->AddHSVSegment(50., 0., 1., 1., 85., 0.3333, 1., 1.);
  transferFunction->AddHSVSegment(85., 0.3333, 1., 1., 170., 0.6666, 1., 1.);
  transferFunction->AddHSVSegment(170., 0.6666, 1., 1., 200., 0., 1., 1.);

  svtkNew<svtkTFCallback> cbk;
  transferFunction->AddObserver(svtkCommand::StartEvent, cbk);
  transferFunction->AddObserver(svtkCommand::ModifiedEvent, cbk);
  transferFunction->AddObserver(svtkCommand::EndEvent, cbk);
  transferFunction->AddObserver(svtkCommand::StartInteractionEvent, cbk);
  transferFunction->AddObserver(svtkCommand::InteractionEvent, cbk);
  transferFunction->AddObserver(svtkCommand::EndInteractionEvent, cbk);

  svtkNew<svtkColorTransferControlPointsItem> controlPoints;
  controlPoints->SetColorTransferFunction(transferFunction);

  svtkNew<svtkChartXY> chart;
  chart->AddPlot(controlPoints);

  svtkNew<svtkContextScene> scene;
  scene->AddItem(controlPoints);

  svtkNew<svtkContextInteractorStyle> interactorStyle;
  interactorStyle->SetScene(scene);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetInteractorStyle(interactorStyle);

  svtkNew<svtkInteractorEventRecorder> recorder;
  recorder->SetInteractor(iren);
  recorder->ReadFromInputStringOn();

  // Add a point at (60, 0.5) and move it to (62, 0.5)
  const char addAndDragEvents[] = "# StreamVersion 1\n"
                                  "LeftButtonPressEvent 60 1 0 0 0 0 0\n"
                                  "MouseMoveEvent 62 1 0 0 0 0 0\n"
                                  "LeftButtonReleaseEvent 62 1 0 0 0 0 0\n";
  recorder->SetInputString(addAndDragEvents);
  recorder->Play();

  // 1 ModifiedEvent for adding a point
  // 1 ModifiedEvent for moving the point
  if (cbk->EventSpy[svtkCommand::ModifiedEvent] != 2 ||
    cbk->EventSpy[svtkCommand::StartInteractionEvent] != 1 ||
    cbk->EventSpy[svtkCommand::InteractionEvent] != 1 ||
    cbk->EventSpy[svtkCommand::EndInteractionEvent] != 1 ||
    cbk->EventSpy[svtkCommand::StartEvent] != 2 || cbk->EventSpy[svtkCommand::EndEvent] != 2)
  {
    std::cerr << "Wrong number of fired events : " << cbk->EventSpy[svtkCommand::ModifiedEvent]
              << " " << cbk->EventSpy[svtkCommand::StartInteractionEvent] << " "
              << cbk->EventSpy[svtkCommand::InteractionEvent] << " "
              << cbk->EventSpy[svtkCommand::EndInteractionEvent] << " "
              << cbk->EventSpy[svtkCommand::StartEvent] << " " << cbk->EventSpy[svtkCommand::EndEvent]
              << std::endl;
    return EXIT_FAILURE;
  }
  cbk->EventSpy.clear();
  // Move all the points to the right.
  controlPoints->MovePoints(svtkVector2f(5, 0.));
  // One ModifiedEvent for each moved point

  if (cbk->EventSpy[svtkCommand::ModifiedEvent] > controlPoints->GetNumberOfPoints() ||
    cbk->EventSpy[svtkCommand::StartInteractionEvent] != 0 ||
    cbk->EventSpy[svtkCommand::InteractionEvent] != 0 ||
    cbk->EventSpy[svtkCommand::EndInteractionEvent] != 0 ||
    cbk->EventSpy[svtkCommand::StartEvent] != 1 || cbk->EventSpy[svtkCommand::EndEvent] != 1)
  {
    std::cerr << "Wrong number of fired events : " << cbk->EventSpy[svtkCommand::ModifiedEvent]
              << " " << cbk->EventSpy[svtkCommand::StartInteractionEvent] << " "
              << cbk->EventSpy[svtkCommand::InteractionEvent] << " "
              << cbk->EventSpy[svtkCommand::EndInteractionEvent] << " "
              << cbk->EventSpy[svtkCommand::StartEvent] << " " << cbk->EventSpy[svtkCommand::EndEvent]
              << std::endl;
    return EXIT_FAILURE;
  }

  cbk->EventSpy.clear();

  const char dblClickEvents[] =
    "# StreamVersion 1\n"
    "MouseMoveEvent 56 1 0 0 0 0 0\n"       // shouldn't move the point
    "LeftButtonPressEvent 55 1 0 0 0 0 0\n" // select the first point
    "LeftButtonReleaseEvent 55 1 0 0 0 0 0\n"
    "LeftButtonPressEvent 55 1 0 0 0 1 0\n"   // dbl click
    "LeftButtonReleaseEvent 55 1 0 0 0 0 0\n" // must be followed by release
    ;

  recorder->SetInputString(dblClickEvents);
  recorder->Play();

  if (cbk->EventSpy[svtkCommand::ModifiedEvent] != 0 ||
    cbk->EventSpy[svtkCommand::StartInteractionEvent] != 0 ||
    cbk->EventSpy[svtkCommand::InteractionEvent] != 0 ||
    cbk->EventSpy[svtkCommand::EndInteractionEvent] != 0 ||
    cbk->EventSpy[svtkCommand::StartEvent] != 0 || cbk->EventSpy[svtkCommand::EndEvent] != 0)
  {
    std::cerr << "Wrong number of fired events : " << cbk->EventSpy[svtkCommand::ModifiedEvent]
              << " " << cbk->EventSpy[svtkCommand::StartInteractionEvent] << " "
              << cbk->EventSpy[svtkCommand::InteractionEvent] << " "
              << cbk->EventSpy[svtkCommand::EndInteractionEvent] << " "
              << cbk->EventSpy[svtkCommand::StartEvent] << " " << cbk->EventSpy[svtkCommand::EndEvent]
              << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
