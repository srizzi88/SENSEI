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

#include "svtkChartXY.h"
#include "svtkColorTransferFunction.h"
#include "svtkContextInteractorStyle.h"
#include "svtkContextScene.h"
#include "svtkInteractorEventRecorder.h"
#include "svtkNew.h"
#include "svtkRangeHandlesItem.h"
#include "svtkRenderWindowInteractor.h"

#include <iostream>
#include <map>

//----------------------------------------------------------------------------
class svtkRangeHandlesCallBack : public svtkCommand
{
public:
  static svtkRangeHandlesCallBack* New() { return new svtkRangeHandlesCallBack; }

  void Execute(svtkObject* caller, unsigned long event, void* svtkNotUsed(callData)) override
  {
    svtkRangeHandlesItem* self = svtkRangeHandlesItem::SafeDownCast(caller);
    if (!self)
    {
      return;
    }
    if (event == svtkCommand::EndInteractionEvent)
    {
      self->GetHandlesRange(this->Range);
    }
    if (this->EventSpy.count(event) == 0)
    {
      this->EventSpy[event] = 0;
    }
    ++this->EventSpy[event];
    std::cout << "InvokedEvent: " << event << this->EventSpy[event] << std::endl;
  }
  std::map<unsigned long, int> EventSpy;
  double Range[2];
};

//----------------------------------------------------------------------------
int TestRangeHandlesItemEvents(int, char*[])
{
  svtkNew<svtkColorTransferFunction> transferFunction;
  transferFunction->AddHSVSegment(50., 0., 1., 1., 85., 0.3333, 1., 1.);
  transferFunction->AddHSVSegment(85., 0.3333, 1., 1., 170., 0.6666, 1., 1.);
  transferFunction->AddHSVSegment(170., 0.6666, 1., 1., 200., 0., 1., 1.);

  svtkNew<svtkRangeHandlesItem> rangeHandles;
  rangeHandles->SetColorTransferFunction(transferFunction);
  rangeHandles->ComputeHandlesDrawRange();
  double range[2];
  rangeHandles->GetHandlesRange(range);
  if (range[0] != 50 || range[1] != 200)
  {
    std::cerr << "Unexepected range in range handle : [" << range[0] << ", " << range[1]
              << "]. Expecting : [50, 200]." << std::endl;
    return EXIT_FAILURE;
  }

  svtkNew<svtkRangeHandlesCallBack> cbk;
  rangeHandles->AddObserver(svtkCommand::StartInteractionEvent, cbk);
  rangeHandles->AddObserver(svtkCommand::InteractionEvent, cbk);
  rangeHandles->AddObserver(svtkCommand::EndInteractionEvent, cbk);

  svtkNew<svtkChartXY> chart;
  chart->AddPlot(rangeHandles);

  svtkNew<svtkContextScene> scene;
  scene->AddItem(rangeHandles);

  svtkNew<svtkContextInteractorStyle> interactorStyle;
  interactorStyle->SetScene(scene);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetInteractorStyle(interactorStyle);

  svtkNew<svtkInteractorEventRecorder> recorder;
  recorder->SetInteractor(iren);
  recorder->ReadFromInputStringOn();

  // Move left handle
  const char leftEvents[] = "# StreamVersion 1\n"
                            "LeftButtonPressEvent 51 1 0 0 0 0 0\n"
                            "MouseMoveEvent 70 1 0 0 0 0 0\n"
                            "LeftButtonReleaseEvent 70 1 0 0 0 0 0\n";
  recorder->SetInputString(leftEvents);
  recorder->Play();

  if (cbk->EventSpy[svtkCommand::StartInteractionEvent] != 1 ||
    cbk->EventSpy[svtkCommand::InteractionEvent] != 1 ||
    cbk->EventSpy[svtkCommand::EndInteractionEvent] != 1)
  {
    std::cerr << "Wrong number of fired events : "
              << cbk->EventSpy[svtkCommand::StartInteractionEvent] << " "
              << cbk->EventSpy[svtkCommand::InteractionEvent] << " "
              << cbk->EventSpy[svtkCommand::EndInteractionEvent] << std::endl;
    return EXIT_FAILURE;
  }

  if (cbk->Range[0] != 69.25 || cbk->Range[1] != 200)
  {
    std::cerr << "Unexepected range in range handle : [" << cbk->Range[0] << ", " << cbk->Range[1]
              << "]. Expecting : [69.25, 200]." << std::endl;
    return EXIT_FAILURE;
  }

  cbk->EventSpy.clear();

  // Move right handle
  const char rightEvents[] = "# StreamVersion 1\n"
                             "LeftButtonPressEvent 199 1 0 0 0 0 0\n"
                             "MouseMoveEvent 120 1 0 0 0 0 0\n"
                             "LeftButtonReleaseEvent 120 1 0 0 0 0 0\n";
  recorder->SetInputString(rightEvents);
  recorder->Play();

  if (cbk->EventSpy[svtkCommand::StartInteractionEvent] != 1 ||
    cbk->EventSpy[svtkCommand::InteractionEvent] != 1 ||
    cbk->EventSpy[svtkCommand::EndInteractionEvent] != 1)
  {
    std::cerr << "Wrong number of fired events : "
              << cbk->EventSpy[svtkCommand::StartInteractionEvent] << " "
              << cbk->EventSpy[svtkCommand::InteractionEvent] << " "
              << cbk->EventSpy[svtkCommand::EndInteractionEvent] << std::endl;
    return EXIT_FAILURE;
  }

  if (cbk->Range[0] != 50 || cbk->Range[1] != 120.75)
  {
    std::cerr << "Unexepected range in range handle : [" << cbk->Range[0] << ", " << cbk->Range[1]
              << "]. Expecting : [50, 120.75]." << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
