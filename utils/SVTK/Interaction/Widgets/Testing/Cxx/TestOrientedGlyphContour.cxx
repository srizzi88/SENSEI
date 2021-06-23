/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOrientedGlyphContour.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the svtkHandleWidget with a 2D representation

// First include the required header files for the SVTK classes we are using.
#include "svtkSmartPointer.h"

#include "svtkActor2D.h"
#include "svtkBoundedPlanePointPlacer.h"
#include "svtkCamera.h"
#include "svtkCommand.h"
#include "svtkContourWidget.h"
#include "svtkCoordinate.h"
#include "svtkEvent.h"
#include "svtkImageActor.h"
#include "svtkImageData.h"
#include "svtkImageMapper3D.h"
#include "svtkImageShiftScale.h"
#include "svtkOrientedGlyphContourRepresentation.h"
#include "svtkPlane.h"
#include "svtkProperty.h"
#include "svtkProperty2D.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"
#include "svtkTesting.h"
#include "svtkVolume16Reader.h"
#include "svtkWidgetEvent.h"
#include "svtkWidgetEventTranslator.h"

int TestOrientedGlyphContour(int argc, char* argv[])
{
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");

  svtkSmartPointer<svtkVolume16Reader> v16 = svtkSmartPointer<svtkVolume16Reader>::New();
  v16->SetDataDimensions(64, 64);
  v16->SetDataByteOrderToLittleEndian();
  v16->SetImageRange(1, 93);
  v16->SetDataSpacing(3.2, 3.2, 1.5);
  v16->SetFilePrefix(fname);
  v16->ReleaseDataFlagOn();
  v16->SetDataMask(0x7fff);
  v16->Update();
  delete[] fname;

  double range[2];
  v16->GetOutput()->GetScalarRange(range);

  svtkSmartPointer<svtkImageShiftScale> shifter = svtkSmartPointer<svtkImageShiftScale>::New();
  shifter->SetShift(-1.0 * range[0]);
  shifter->SetScale(255.0 / (range[1] - range[0]));
  shifter->SetOutputScalarTypeToUnsignedChar();
  shifter->SetInputConnection(v16->GetOutputPort());
  shifter->ReleaseDataFlagOff();
  shifter->Update();

  svtkSmartPointer<svtkImageActor> imageActor = svtkSmartPointer<svtkImageActor>::New();
  imageActor->GetMapper()->SetInputConnection(shifter->GetOutputPort());
  imageActor->VisibilityOn();
  imageActor->SetDisplayExtent(0, 63, 0, 63, 46, 46);
  imageActor->InterpolateOn();

  // Create the RenderWindow, Renderer and both Actors
  //
  svtkSmartPointer<svtkRenderer> ren1 = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->AddRenderer(ren1);

  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  // Add the actors to the renderer, set the background and size
  //
  ren1->SetBackground(0.1, 0.2, 0.4);
  ren1->AddActor(imageActor);
  renWin->SetSize(600, 600);

  // render the image
  //
  ren1->GetActiveCamera()->SetPosition(0, 0, 0);
  ren1->GetActiveCamera()->SetFocalPoint(0, 0, 1);
  ren1->GetActiveCamera()->SetViewUp(0, 1, 0);
  ren1->ResetCamera();
  renWin->Render();

  double bounds[6];
  imageActor->GetBounds(bounds);

  svtkSmartPointer<svtkPlane> p1 = svtkSmartPointer<svtkPlane>::New();
  p1->SetOrigin(bounds[0], bounds[2], bounds[4]);
  p1->SetNormal(1.0, 0.0, 0.0);

  svtkSmartPointer<svtkPlane> p2 = svtkSmartPointer<svtkPlane>::New();
  p2->SetOrigin(bounds[0], bounds[2], bounds[4]);
  p2->SetNormal(0.0, 1.0, 0.0);

  svtkSmartPointer<svtkPlane> p3 = svtkSmartPointer<svtkPlane>::New();
  p3->SetOrigin(bounds[1], bounds[3], bounds[5]);
  p3->SetNormal(-1.0, 0.0, 0.0);

  svtkSmartPointer<svtkPlane> p4 = svtkSmartPointer<svtkPlane>::New();
  p4->SetOrigin(bounds[1], bounds[3], bounds[5]);
  p4->SetNormal(0.0, -1.0, 0.0);

  svtkSmartPointer<svtkOrientedGlyphContourRepresentation> contourRep =
    svtkSmartPointer<svtkOrientedGlyphContourRepresentation>::New();
  svtkSmartPointer<svtkContourWidget> contourWidget = svtkSmartPointer<svtkContourWidget>::New();
  svtkSmartPointer<svtkBoundedPlanePointPlacer> placer =
    svtkSmartPointer<svtkBoundedPlanePointPlacer>::New();

  contourWidget->SetInteractor(iren);
  contourWidget->SetRepresentation(contourRep);

  // Change bindings.
  svtkWidgetEventTranslator* eventTranslator = contourWidget->GetEventTranslator();
  eventTranslator->RemoveTranslation(svtkCommand::RightButtonPressEvent);
  eventTranslator->SetTranslation(
    svtkCommand::KeyPressEvent, svtkEvent::NoModifier, 103, 0, "g", svtkWidgetEvent::AddFinalPoint);
  eventTranslator->SetTranslation(svtkCommand::RightButtonPressEvent, svtkWidgetEvent::Translate);
  contourWidget->On();

  contourRep->SetPointPlacer(placer);
  //  contourRep->AlwaysOnTopOn();

  placer->SetProjectionNormalToZAxis();
  placer->SetProjectionPosition(imageActor->GetCenter()[2]);

  placer->AddBoundingPlane(p1);
  placer->AddBoundingPlane(p2);
  placer->AddBoundingPlane(p3);
  placer->AddBoundingPlane(p4);

  iren->Initialize();
  renWin->Render();
  int retVal = svtkTesting::InteractorEventLoop(argc, argv, iren);

  return retVal;
}
