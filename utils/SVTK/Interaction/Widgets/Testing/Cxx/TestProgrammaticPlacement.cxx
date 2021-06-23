/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDistanceWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the placement of svtkDistanceWidget, svtkAngleWidget,
// svtkBiDimensionalWidget

// First include the required header files for the SVTK classes we are using.
#include "svtkActor.h"
#include "svtkAngleRepresentation2D.h"
#include "svtkAngleWidget.h"
#include "svtkAxisActor2D.h"
#include "svtkBiDimensionalRepresentation2D.h"
#include "svtkBiDimensionalWidget.h"
#include "svtkCommand.h"
#include "svtkCoordinate.h"
#include "svtkDebugLeaks.h"
#include "svtkDistanceRepresentation2D.h"
#include "svtkDistanceRepresentation3D.h"
#include "svtkDistanceWidget.h"
#include "svtkHandleWidget.h"
#include "svtkInteractorEventRecorder.h"
#include "svtkMath.h"
#include "svtkPointHandleRepresentation2D.h"
#include "svtkPointHandleRepresentation3D.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkProperty2D.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

// The actual test function
int TestProgrammaticPlacement(int argc, char* argv[])
{
  // Create the RenderWindow, Renderer and both Actors
  //
  SVTK_CREATE(svtkRenderer, ren1);
  SVTK_CREATE(svtkRenderWindow, renWin);
  renWin->AddRenderer(ren1);

  SVTK_CREATE(svtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(renWin);

  // Create a test pipeline
  //
  SVTK_CREATE(svtkSphereSource, ss);
  SVTK_CREATE(svtkPolyDataMapper, mapper);
  mapper->SetInputConnection(ss->GetOutputPort());
  SVTK_CREATE(svtkActor, actor);
  actor->SetMapper(mapper);

  // Create the widget and its representation
  SVTK_CREATE(svtkPointHandleRepresentation2D, handle);
  handle->GetProperty()->SetColor(1, 0, 0);

  SVTK_CREATE(svtkDistanceRepresentation2D, dRep);
  dRep->SetHandleRepresentation(handle);
  dRep->InstantiateHandleRepresentation();
  dRep->GetAxis()->SetNumberOfMinorTicks(4);
  dRep->GetAxis()->SetTickLength(9);
  dRep->GetAxis()->SetTitlePosition(0.2);
  dRep->RulerModeOn();
  dRep->SetRulerDistance(0.25);

  SVTK_CREATE(svtkDistanceWidget, dWidget);
  dWidget->SetInteractor(iren);
  dWidget->SetRepresentation(dRep);
  dWidget->SetWidgetStateToManipulate();

  // Create the widget and its representation
  SVTK_CREATE(svtkPointHandleRepresentation3D, handle2);
  handle2->GetProperty()->SetColor(1, 1, 0);

  SVTK_CREATE(svtkDistanceRepresentation3D, dRep2);
  dRep2->SetHandleRepresentation(handle2);
  dRep2->InstantiateHandleRepresentation();
  dRep2->RulerModeOn();
  dRep2->SetRulerDistance(0.25);

  SVTK_CREATE(svtkDistanceWidget, dWidget2);
  dWidget2->SetInteractor(iren);
  dWidget2->SetRepresentation(dRep2);
  dWidget2->SetWidgetStateToManipulate();

  // Add the actors to the renderer, set the background and size
  //
  ren1->AddActor(actor);
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // render the image
  //
  iren->Initialize();
  renWin->Render();
  dWidget->On();
  dWidget2->On();

  double p[3];
  p[0] = 25;
  p[1] = 50;
  p[2] = 0;
  dRep->SetPoint1DisplayPosition(p);
  p[0] = 275;
  p[1] = 250;
  p[2] = 0;
  dRep->SetPoint2DisplayPosition(p);

  p[0] = -0.75;
  p[1] = 0.75;
  p[2] = 0;
  dRep2->SetPoint1WorldPosition(p);
  p[0] = 0.75;
  p[1] = -0.75;
  p[2] = 0;
  dRep2->SetPoint2WorldPosition(p);

  renWin->Render();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  dWidget->Off();

  return !retVal;
}
