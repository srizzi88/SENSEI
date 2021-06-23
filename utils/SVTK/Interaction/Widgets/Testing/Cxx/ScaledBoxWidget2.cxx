/*=========================================================================

  Program:   Visualization Toolkit
  Module:    BoxWidget2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkAppendPolyData.h"
#include "svtkBoxRepresentation.h"
#include "svtkBoxWidget2.h"
#include "svtkCamera.h"
#include "svtkCommand.h"
#include "svtkConeSource.h"
#include "svtkGlyph3D.h"
#include "svtkInteractorEventRecorder.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"
#include "svtkTransform.h"
#include "svtkTransformFilter.h"

// Callback for the interaction
class svtkSBWCallback2 : public svtkCommand
{
public:
  static svtkSBWCallback2* New() { return new svtkSBWCallback2; }
  virtual void Execute(svtkObject* caller, unsigned long, void*)
  {
    svtkBoxWidget2* boxWidget = reinterpret_cast<svtkBoxWidget2*>(caller);
    svtkBoxRepresentation* boxRep =
      reinterpret_cast<svtkBoxRepresentation*>(boxWidget->GetRepresentation());
    boxRep->GetTransform(this->Transform);

    svtkCamera* camera = boxRep->GetRenderer()->GetActiveCamera();
    //    this->Actor->SetUserTransform(this->Transform);
  }
  svtkSBWCallback2()
    : Transform(0)
    , Actor(0)
  {
  }
  svtkTransform* Transform;
  svtkActor* Actor;
};

char ScaledBoxWidgetEventLog2[] = "# StreamVersion 1\n"
                                  "CharEvent 187 242 0 0 105 1 i\n"
                                  "KeyReleaseEvent 187 242 0 0 105 1 i\n";

int ScaledBoxWidget2(int, char*[])
{
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->AddRenderer(renderer);
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  svtkSmartPointer<svtkConeSource> cone = svtkSmartPointer<svtkConeSource>::New();
  cone->SetResolution(6);
  svtkSmartPointer<svtkSphereSource> sphere = svtkSmartPointer<svtkSphereSource>::New();
  sphere->SetThetaResolution(8);
  sphere->SetPhiResolution(8);
  svtkSmartPointer<svtkGlyph3D> glyph = svtkSmartPointer<svtkGlyph3D>::New();
  glyph->SetInputConnection(sphere->GetOutputPort());
  glyph->SetSource(cone->GetOutput());
  glyph->SetVectorModeToUseNormal();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.25);
  glyph->Update();

  svtkSmartPointer<svtkAppendPolyData> append = svtkSmartPointer<svtkAppendPolyData>::New();
  append->AddInput(glyph->GetOutput());
  append->AddInput(sphere->GetOutput());

  svtkSmartPointer<svtkTransform> dataTransform = svtkSmartPointer<svtkTransform>::New();
  dataTransform->Identity();
  dataTransform->Scale(1, 2, 1);

  svtkSmartPointer<svtkTransformFilter> tf = svtkSmartPointer<svtkTransformFilter>::New();
  tf->SetTransform(dataTransform);
  tf->SetInputConnection(append->GetOutputPort());
  tf->Update();

  svtkSmartPointer<svtkPolyDataMapper> maceMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  maceMapper->SetInputConnection(tf->GetOutputPort());

  svtkSmartPointer<svtkActor> maceActor = svtkSmartPointer<svtkActor>::New();
  maceActor->SetMapper(maceMapper);

  // Configure the box widget including callbacks
  svtkSmartPointer<svtkTransform> t = svtkSmartPointer<svtkTransform>::New();
  svtkSmartPointer<svtkSBWCallback2> myCallback = svtkSmartPointer<svtkSBWCallback2>::New();
  myCallback->Transform = t;
  myCallback->Actor = maceActor;

  svtkSmartPointer<svtkBoxRepresentation> boxRep = svtkSmartPointer<svtkBoxRepresentation>::New();
  boxRep->SetPlaceFactor(1.25);
  boxRep->PlaceWidget(tf->GetOutput()->GetBounds());

  svtkSmartPointer<svtkBoxWidget2> boxWidget = svtkSmartPointer<svtkBoxWidget2>::New();
  boxWidget->SetInteractor(iren);
  boxWidget->SetRepresentation(boxRep);
  boxWidget->AddObserver(svtkCommand::InteractionEvent, myCallback);
  boxWidget->SetPriority(1);

  renderer->AddActor(maceActor);
  renderer->SetBackground(0, 0, 0);
  renWin->SetSize(1024, 768);

  // Introduce scale to test out calculation of clipping range
  // by svtkRenderer.
  svtkSmartPointer<svtkTransform> scaleTransform = svtkSmartPointer<svtkTransform>::New();
  scaleTransform->SetInput(dataTransform);

  svtkCamera* camera = renderer->GetActiveCamera();
  camera->SetModelTransformMatrix(scaleTransform->GetMatrix());

  // record events
  svtkSmartPointer<svtkInteractorEventRecorder> recorder =
    svtkSmartPointer<svtkInteractorEventRecorder>::New();
  recorder->SetInteractor(iren);
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(ScaledBoxWidgetEventLog2);

  // interact with data
  // render the image
  //
  iren->Initialize();
  renWin->Render();
  recorder->Play();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  boxRep->SetPlaceFactor(1.0);
  boxRep->HandlesOff();

  boxRep->SetPlaceFactor(1.25);
  boxRep->HandlesOn();

  renderer->ResetCamera();
  iren->Start();

  return EXIT_SUCCESS;
}
