/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Cone2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example shows how to add an observer to a C++ program. It extends
// the Step1/Cxx/Cone.cxx C++ example (see that example for information on
// the basic setup).
//
// SVTK uses a command/observer design pattern. That is, observers watch for
// particular events that any svtkObject (or subclass) may invoke on
// itself. For example, the svtkRenderer invokes a "StartEvent" as it begins
// to render. Here we add an observer that invokes a command when this event
// is observed.
//

// first include the required header files for the svtk classes we are using
#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCommand.h"
#include "svtkConeSource.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"

// Callback for the interaction
class svtkMyCallback : public svtkCommand
{
public:
  static svtkMyCallback* New() { return new svtkMyCallback; }
  void Execute(svtkObject* caller, unsigned long, void*) override
  {
    svtkRenderer* renderer = reinterpret_cast<svtkRenderer*>(caller);
    cout << renderer->GetActiveCamera()->GetPosition()[0] << " "
         << renderer->GetActiveCamera()->GetPosition()[1] << " "
         << renderer->GetActiveCamera()->GetPosition()[2] << "\n";
  }
};

int main()
{
  //
  // The pipeline creation is documented in Step1
  //
  svtkConeSource* cone = svtkConeSource::New();
  cone->SetHeight(3.0);
  cone->SetRadius(1.0);
  cone->SetResolution(10);

  svtkPolyDataMapper* coneMapper = svtkPolyDataMapper::New();
  coneMapper->SetInputConnection(cone->GetOutputPort());
  svtkActor* coneActor = svtkActor::New();
  coneActor->SetMapper(coneMapper);

  svtkRenderer* ren1 = svtkRenderer::New();
  ren1->AddActor(coneActor);
  ren1->SetBackground(0.1, 0.2, 0.4);
  ren1->ResetCamera();

  svtkRenderWindow* renWin = svtkRenderWindow::New();
  renWin->AddRenderer(ren1);
  renWin->SetSize(300, 300);

  // Here is where we setup the observer, we do a new and ren1 will
  // eventually free the observer
  svtkMyCallback* mo1 = svtkMyCallback::New();
  ren1->AddObserver(svtkCommand::StartEvent, mo1);
  mo1->Delete();

  //
  // now we loop over 360 degrees and render the cone each time
  //
  int i;
  for (i = 0; i < 360; ++i)
  {
    // render the image
    renWin->Render();
    // rotate the active camera by one degree
    ren1->GetActiveCamera()->Azimuth(1);
  }

  //
  // Free up any objects we created
  //
  cone->Delete();
  coneMapper->Delete();
  coneActor->Delete();
  ren1->Delete();
  renWin->Delete();

  return 0;
}
