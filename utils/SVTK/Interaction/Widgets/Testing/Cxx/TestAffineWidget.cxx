/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAffineWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the svtkAffineWidget.

// First include the required header files for the SVTK classes we are using.
#include "svtkSmartPointer.h"

#include "svtkAffineRepresentation2D.h"
#include "svtkAffineWidget.h"
#include "svtkCommand.h"
#include "svtkImageActor.h"
#include "svtkImageData.h"
#include "svtkImageMapper3D.h"
#include "svtkImageShiftScale.h"
#include "svtkInteractorEventRecorder.h"
#include "svtkInteractorStyleImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"
#include "svtkTransform.h"
#include "svtkVolume16Reader.h"

// This callback is responsible for adjusting the point position.
// It looks in the region around the point and finds the maximum or
// minimum value.
class svtkAffineCallback : public svtkCommand
{
public:
  static svtkAffineCallback* New() { return new svtkAffineCallback; }
  void Execute(svtkObject* caller, unsigned long, void*) override;
  svtkAffineCallback()
    : ImageActor(nullptr)
    , AffineRep(nullptr)
  {
    this->Transform = svtkTransform::New();
  }
  ~svtkAffineCallback() override { this->Transform->Delete(); }
  svtkSmartPointer<svtkImageActor> ImageActor;
  svtkAffineRepresentation2D* AffineRep;
  svtkTransform* Transform;
};

// Method re-positions the points using random perturbation
void svtkAffineCallback::Execute(svtkObject*, unsigned long, void*)
{
  this->AffineRep->GetTransform(this->Transform);
  this->ImageActor->SetUserTransform(this->Transform);
}

int TestAffineWidget(int argc, char* argv[])
{
  // Create the pipeline
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

  double bounds[6];
  imageActor->GetBounds(bounds);

  // Create the RenderWindow, Renderer and both Actors
  //
  svtkSmartPointer<svtkRenderer> ren1 = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->AddRenderer(ren1);

  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  svtkSmartPointer<svtkInteractorStyleImage> style = svtkSmartPointer<svtkInteractorStyleImage>::New();
  iren->SetInteractorStyle(style);

  // SVTK widgets consist of two parts: the widget part that handles event processing;
  // and the widget representation that defines how the widget appears in the scene
  // (i.e., matters pertaining to geometry).
  svtkSmartPointer<svtkAffineRepresentation2D> rep =
    svtkSmartPointer<svtkAffineRepresentation2D>::New();
  rep->SetBoxWidth(100);
  rep->SetCircleWidth(75);
  rep->SetAxesWidth(60);
  rep->DisplayTextOn();
  rep->PlaceWidget(bounds);

  svtkSmartPointer<svtkAffineWidget> widget = svtkSmartPointer<svtkAffineWidget>::New();
  widget->SetInteractor(iren);
  widget->SetRepresentation(rep);

  svtkSmartPointer<svtkAffineCallback> acbk = svtkSmartPointer<svtkAffineCallback>::New();
  acbk->AffineRep = rep;
  acbk->ImageActor = imageActor;
  widget->AddObserver(svtkCommand::InteractionEvent, acbk);
  widget->AddObserver(svtkCommand::EndInteractionEvent, acbk);

  // Add the actors to the renderer, set the background and size
  ren1->AddActor(imageActor);
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // record events
  svtkSmartPointer<svtkInteractorEventRecorder> recorder =
    svtkSmartPointer<svtkInteractorEventRecorder>::New();
  recorder->SetInteractor(iren);
  recorder->SetFileName("c:/record.log");
  //  recorder->Record();
  //  recorder->ReadFromInputStringOn();
  //  recorder->SetInputString(eventLog);

  // render the image
  //
  iren->Initialize();
  renWin->Render();
  //  recorder->Play();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  iren->Start();

  return EXIT_SUCCESS;
}
