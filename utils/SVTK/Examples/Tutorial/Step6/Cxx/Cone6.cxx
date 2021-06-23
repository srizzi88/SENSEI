/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Cone6.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example introduces 3D widgets. 3D widgets take advantage of the
// event/observer design pattern introduced previously. They typically
// have a particular representation in the scene which can be interactively
// selected and manipulated using the mouse and keyboard. As the widgets
// are manipulated, they in turn invoke events such as StartInteractionEvent,
// InteractionEvent, and EndInteractionEvent which can be used to manipulate
// the scene that the widget is embedded in. 3D widgets work in the context
// of the event loop which was set up in the previous example.
//
// Note: there are more 3D widget examples in SVTK/Examples/GUI/.
//

// First include the required header files for the SVTK classes we are using.
#include "svtkActor.h"
#include "svtkBoxWidget.h"
#include "svtkCamera.h"
#include "svtkCommand.h"
#include "svtkConeSource.h"
#include "svtkInteractorStyleTrackballCamera.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTransform.h"

//
// Similar to Cone2.cxx, we define a callback for interaction.
//
class svtkMyCallback : public svtkCommand
{
public:
  static svtkMyCallback* New() { return new svtkMyCallback; }
  void Execute(svtkObject* caller, unsigned long, void*) override
  {
    svtkTransform* t = svtkTransform::New();
    svtkBoxWidget* widget = reinterpret_cast<svtkBoxWidget*>(caller);
    widget->GetTransform(t);
    widget->GetProp3D()->SetUserTransform(t);
    t->Delete();
  }
};

int main()
{
  //
  // Next we create an instance of svtkConeSource and set some of its
  // properties. The instance of svtkConeSource "cone" is part of a
  // visualization pipeline (it is a source process object); it produces data
  // (output type is svtkPolyData) which other filters may process.
  //
  svtkConeSource* cone = svtkConeSource::New();
  cone->SetHeight(3.0);
  cone->SetRadius(1.0);
  cone->SetResolution(10);

  //
  // In this example we terminate the pipeline with a mapper process object.
  // (Intermediate filters such as svtkShrinkPolyData could be inserted in
  // between the source and the mapper.)  We create an instance of
  // svtkPolyDataMapper to map the polygonal data into graphics primitives. We
  // connect the output of the cone source to the input of this mapper.
  //
  svtkPolyDataMapper* coneMapper = svtkPolyDataMapper::New();
  coneMapper->SetInputConnection(cone->GetOutputPort());

  //
  // Create an actor to represent the cone. The actor orchestrates rendering
  // of the mapper's graphics primitives. An actor also refers to properties
  // via a svtkProperty instance, and includes an internal transformation
  // matrix. We set this actor's mapper to be coneMapper which we created
  // above.
  //
  svtkActor* coneActor = svtkActor::New();
  coneActor->SetMapper(coneMapper);

  //
  // Create the Renderer and assign actors to it. A renderer is like a
  // viewport. It is part or all of a window on the screen and it is
  // responsible for drawing the actors it has.  We also set the background
  // color here.
  //
  svtkRenderer* ren1 = svtkRenderer::New();
  ren1->AddActor(coneActor);
  ren1->SetBackground(0.1, 0.2, 0.4);

  //
  // Finally we create the render window which will show up on the screen.
  // We put our renderer into the render window using AddRenderer. We also
  // set the size to be 300 pixels by 300.
  //
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  renWin->AddRenderer(ren1);
  renWin->SetSize(300, 300);

  //
  // The svtkRenderWindowInteractor class watches for events (e.g., keypress,
  // mouse) in the svtkRenderWindow. These events are translated into
  // event invocations that SVTK understands (see SVTK/Common/svtkCommand.h
  // for all events that SVTK processes). Then observers of these SVTK
  // events can process them as appropriate.
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  //
  // By default the svtkRenderWindowInteractor instantiates an instance
  // of svtkInteractorStyle. svtkInteractorStyle translates a set of events
  // it observes into operations on the camera, actors, and/or properties
  // in the svtkRenderWindow associated with the svtkRenderWinodwInteractor.
  // Here we specify a particular interactor style.
  svtkInteractorStyleTrackballCamera* style = svtkInteractorStyleTrackballCamera::New();
  iren->SetInteractorStyle(style);

  //
  // Here we use a svtkBoxWidget to transform the underlying coneActor (by
  // manipulating its transformation matrix). Many other types of widgets
  // are available for use, see the documentation for more details.
  //
  // The SetInteractor method is how 3D widgets are associated with the render
  // window interactor. Internally, SetInteractor sets up a bunch of callbacks
  // using the Command/Observer mechanism (AddObserver()). The place factor
  // controls the initial size of the widget with respect to the bounding box
  // of the input to the widget.
  svtkBoxWidget* boxWidget = svtkBoxWidget::New();
  boxWidget->SetInteractor(iren);
  boxWidget->SetPlaceFactor(1.25);

  //
  // Place the interactor initially. The input to a 3D widget is used to
  // initially position and scale the widget. The EndInteractionEvent is
  // observed which invokes the SelectPolygons callback.
  //
  boxWidget->SetProp3D(coneActor);
  boxWidget->PlaceWidget();
  svtkMyCallback* callback = svtkMyCallback::New();
  boxWidget->AddObserver(svtkCommand::InteractionEvent, callback);

  //
  // Normally the user presses the "i" key to bring a 3D widget to life. Here
  // we will manually enable it so it appears with the cone.
  //
  boxWidget->On();

  //
  // Start the event loop.
  //
  iren->Initialize();
  iren->Start();

  //
  // Free up any objects we created. All instances in SVTK are deleted by
  // using the Delete() method.
  //
  cone->Delete();
  coneMapper->Delete();
  coneActor->Delete();
  callback->Delete();
  boxWidget->Delete();
  ren1->Delete();
  renWin->Delete();
  iren->Delete();
  style->Delete();

  return 0;
}
