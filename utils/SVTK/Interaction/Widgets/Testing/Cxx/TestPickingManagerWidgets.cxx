/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPickingManagerWidgets.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*==============================================================================

  Library: MSSVTK

  Copyright (c) Kitware Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

//
// This example tests the PickingManager using different widgets and associated
// pickers:
// * svtkBalloonWidget
// * svtkBoxWidget
// * svtkImplicitPlaneWidget2
// By default the Picking Manager is enabled.
// Press 'Ctrl' to switch the activation of the Picking Manager.
// Press 'o' to enable/disable the Optimization on render events.

#include "svtkActor.h"
#include "svtkAppendPolyData.h"
#include "svtkBalloonRepresentation.h"
#include "svtkBalloonWidget.h"
#include "svtkBoxWidget.h"
#include "svtkCamera.h"
#include "svtkClipPolyData.h"
#include "svtkCommand.h"
#include "svtkConeSource.h"
#include "svtkCylinderSource.h"
#include "svtkGlyph3D.h"
#include "svtkImplicitPlaneRepresentation.h"
#include "svtkImplicitPlaneWidget2.h"
#include "svtkInteractorStyleTrackballCamera.h"
#include "svtkLODActor.h"
#include "svtkNew.h"
#include "svtkPickingManager.h"
#include "svtkPlane.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkPropPicker.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"

//------------------------------------------------------------------------------
class svtkBalloonPickCallback : public svtkCommand
{
public:
  static svtkBalloonPickCallback* New() { return new svtkBalloonPickCallback; }
  void Execute(svtkObject* caller, unsigned long, void*) override
  {
    svtkPropPicker* picker = reinterpret_cast<svtkPropPicker*>(caller);
    svtkProp* prop = picker->GetViewProp();
    if (prop != nullptr)
    {
      this->BalloonWidget->UpdateBalloonString(prop, "Picked");
    }
  }

  svtkBalloonWidget* BalloonWidget;
};

//------------------------------------------------------------------------------
// Updates the svtkPlane implicit function.
// This in turn causes the pipeline to update and clip the object.
// Callback for the interaction
class svtkTIPW2Callback : public svtkCommand
{
public:
  static svtkTIPW2Callback* New() { return new svtkTIPW2Callback; }
  void Execute(svtkObject* caller, unsigned long, void*) override
  {
    svtkImplicitPlaneWidget2* planeWidget = reinterpret_cast<svtkImplicitPlaneWidget2*>(caller);
    svtkImplicitPlaneRepresentation* rep =
      reinterpret_cast<svtkImplicitPlaneRepresentation*>(planeWidget->GetRepresentation());
    rep->GetPlane(this->Plane);
    this->Actor->VisibilityOn();
  }

  svtkTIPW2Callback()
    : Plane(nullptr)
    , Actor(nullptr)
  {
  }
  svtkPlane* Plane;
  svtkActor* Actor;
};

//------------------------------------------------------------------------------
// Press 'Ctrl' to switch the activation of the Picking Manager.
// Press 'o' to switch the activation of the optimization based on the render
// events.
class svtkEnableManagerCallback : public svtkCommand
{
public:
  static svtkEnableManagerCallback* New() { return new svtkEnableManagerCallback; }

  void Execute(svtkObject* caller, unsigned long, void*) override
  {
    svtkRenderWindowInteractor* iren = static_cast<svtkRenderWindowInteractor*>(caller);

    if ((svtkStdString(iren->GetKeySym()) == "Control_L" ||
          svtkStdString(iren->GetKeySym()) == "Control_R") &&
      iren->GetPickingManager())
    {
      if (!iren->GetPickingManager()->GetEnabled())
      {
        std::cout << "PickingManager ON !" << std::endl;
        iren->GetPickingManager()->EnabledOn();
      }
      else
      {
        std::cout << "PickingManager OFF !" << std::endl;
        iren->GetPickingManager()->EnabledOff();
      }
    }
    // Enable/Disable the Optimization on render events.
    else if (svtkStdString(iren->GetKeySym()) == "o" && iren->GetPickingManager())
    {
      if (!iren->GetPickingManager()->GetOptimizeOnInteractorEvents())
      {
        std::cout << "Optimization on Interactor events ON !" << std::endl;
        iren->GetPickingManager()->SetOptimizeOnInteractorEvents(1);
      }
      else
      {
        std::cout << "Optimization on Interactor events OFF !" << std::endl;
        iren->GetPickingManager()->SetOptimizeOnInteractorEvents(0);
      }
    }
  }

  svtkEnableManagerCallback() = default;
};

//------------------------------------------------------------------------------
// Test Picking Manager with a several widgets
//------------------------------------------------------------------------------
int TestPickingManagerWidgets(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  // Create the RenderWindow, Renderer and both Actors
  //
  svtkNew<svtkRenderer> ren1;
  svtkNew<svtkRenderWindow> renWin;
  renWin->AddRenderer(ren1);

  svtkNew<svtkRenderWindowInteractor> iren;
  svtkNew<svtkInteractorStyleTrackballCamera> irenStyle;
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(irenStyle);

  // Instantiate a picker and link it to the ballonWidgetCallback
  svtkNew<svtkPropPicker> picker;
  svtkNew<svtkBalloonPickCallback> pcbk;
  picker->AddObserver(svtkCommand::PickEvent, pcbk);
  iren->SetPicker(picker);

  /*--------------------------------------------------------------------------*/
  // PICKING MANAGER
  /*--------------------------------------------------------------------------*/
  // Callback to switch between the managed and non-managed mode of the
  // Picking Manager
  svtkNew<svtkEnableManagerCallback> callMode;
  iren->AddObserver(svtkCommand::KeyPressEvent, callMode);

  /*--------------------------------------------------------------------------*/
  // BALLOON WIDGET
  /*--------------------------------------------------------------------------*/
  // Create a test pipeline
  svtkNew<svtkSphereSource> ss;
  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(ss->GetOutputPort());
  svtkNew<svtkActor> sph;
  sph->SetMapper(mapper);

  svtkNew<svtkCylinderSource> cs;
  svtkNew<svtkPolyDataMapper> csMapper;
  csMapper->SetInputConnection(cs->GetOutputPort());
  svtkNew<svtkActor> cyl;
  cyl->SetMapper(csMapper);
  cyl->AddPosition(5, 0, 0);

  svtkNew<svtkConeSource> coneSource;
  svtkNew<svtkPolyDataMapper> coneMapper;
  coneMapper->SetInputConnection(coneSource->GetOutputPort());
  svtkNew<svtkActor> cone;
  cone->SetMapper(coneMapper);
  cone->AddPosition(0, 5, 0);

  // Create the widget
  svtkNew<svtkBalloonRepresentation> rep;
  rep->SetBalloonLayoutToImageRight();

  svtkNew<svtkBalloonWidget> widget;
  widget->SetInteractor(iren);
  widget->SetRepresentation(rep);
  widget->AddBalloon(sph, "This is a sphere", nullptr);
  widget->AddBalloon(cyl, "This is a\ncylinder", nullptr);
  widget->AddBalloon(cone, "This is a\ncone,\na really big.", nullptr);
  pcbk->BalloonWidget = widget;

  /*--------------------------------------------------------------------------*/
  // BOX WIDGET
  /*--------------------------------------------------------------------------*/
  svtkNew<svtkBoxWidget> boxWidget;
  boxWidget->SetInteractor(iren);
  boxWidget->SetPlaceFactor(1.25);

  // Create the mass actor
  svtkNew<svtkConeSource> cone1;
  cone1->SetResolution(6);
  svtkNew<svtkSphereSource> sphere;
  sphere->SetThetaResolution(8);
  sphere->SetPhiResolution(8);
  sphere->SetCenter(5, 5, 0);
  svtkNew<svtkGlyph3D> glyph;
  glyph->SetInputConnection(sphere->GetOutputPort());
  glyph->SetSourceData(cone1->GetOutput());
  glyph->SetVectorModeToUseNormal();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.25);

  svtkNew<svtkAppendPolyData> append;
  append->AddInputData(glyph->GetOutput());
  append->AddInputData(sphere->GetOutput());

  svtkNew<svtkPolyDataMapper> maceMapper;
  maceMapper->SetInputConnection(append->GetOutputPort());

  svtkNew<svtkActor> maceActor;
  maceActor->SetMapper(maceMapper);

  /*--------------------------------------------------------------------------*/
  // Multiple ImplicitPlane Widgets
  /*--------------------------------------------------------------------------*/
  // Create a mace out of filters.
  //
  svtkNew<svtkSphereSource> sphereImpPlane;
  svtkNew<svtkConeSource> coneImpPlane;
  svtkNew<svtkGlyph3D> glyphImpPlane;
  glyphImpPlane->SetInputConnection(sphereImpPlane->GetOutputPort());
  glyphImpPlane->SetSourceConnection(coneImpPlane->GetOutputPort());
  glyphImpPlane->SetVectorModeToUseNormal();
  glyphImpPlane->SetScaleModeToScaleByVector();
  glyphImpPlane->SetScaleFactor(0.25);
  glyphImpPlane->Update();

  // The sphere and spikes are appended into a single polydata.
  // This just makes things simpler to manage.
  svtkNew<svtkAppendPolyData> apdImpPlane;
  apdImpPlane->AddInputData(glyphImpPlane->GetOutput());
  apdImpPlane->AddInputData(sphereImpPlane->GetOutput());

  svtkNew<svtkPolyDataMapper> maceMapperImpPlane;
  maceMapperImpPlane->SetInputConnection(apdImpPlane->GetOutputPort());

  svtkNew<svtkActor> maceActorImpPlane;
  maceActorImpPlane->SetMapper(maceMapperImpPlane);
  maceActorImpPlane->AddPosition(0, 0, 0);
  maceActorImpPlane->VisibilityOn();

  // This portion of the code clips the mace with the svtkPlanes
  // implicit function. The clipped region is colored green.
  svtkNew<svtkPlane> plane;
  svtkNew<svtkClipPolyData> clipper;
  clipper->SetInputConnection(apdImpPlane->GetOutputPort());
  clipper->SetClipFunction(plane);
  clipper->InsideOutOn();

  svtkNew<svtkPolyDataMapper> selectMapper;
  selectMapper->SetInputConnection(clipper->GetOutputPort());

  svtkNew<svtkActor> selectActor;
  selectActor->SetMapper(selectMapper);
  selectActor->GetProperty()->SetColor(0, 1, 0);
  selectActor->VisibilityOff();
  selectActor->AddPosition(0, 0, 0);
  selectActor->SetScale(1.01, 1.01, 1.01);

  // The SetInteractor method is how 3D widgets are associated with the render
  // window interactor. Internally, SetInteractor sets up a bunch of callbacks
  // using the Command/Observer mechanism (AddObserver()).
  svtkNew<svtkTIPW2Callback> impPlaneCallback;
  impPlaneCallback->Plane = plane;
  impPlaneCallback->Actor = selectActor;

  // First ImplicitPlaneWidget (Green)
  svtkNew<svtkImplicitPlaneRepresentation> impPlaneRep;
  impPlaneRep->SetPlaceFactor(1.);
  impPlaneRep->SetOutlineTranslation(0);
  impPlaneRep->SetScaleEnabled(0);
  impPlaneRep->PlaceWidget(glyphImpPlane->GetOutput()->GetBounds());
  impPlaneRep->SetEdgeColor(0., 1., 0.);
  impPlaneRep->SetNormal(1, 0, 1);

  svtkNew<svtkImplicitPlaneWidget2> planeWidget;
  planeWidget->SetInteractor(iren);
  planeWidget->SetRepresentation(impPlaneRep);
  planeWidget->On();

  planeWidget->AddObserver(svtkCommand::InteractionEvent, impPlaneCallback);
  planeWidget->AddObserver(svtkCommand::UpdateEvent, impPlaneCallback);

  // Second ImplicitPlaneWidget (Red)
  svtkNew<svtkImplicitPlaneRepresentation> impPlaneRep2;
  impPlaneRep2->SetOutlineTranslation(0);
  impPlaneRep2->SetScaleEnabled(0);
  impPlaneRep2->SetPlaceFactor(1.);
  impPlaneRep2->PlaceWidget(glyphImpPlane->GetOutput()->GetBounds());
  impPlaneRep2->SetEdgeColor(1., 0., 0.);

  svtkNew<svtkImplicitPlaneWidget2> planeWidget2;
  planeWidget2->SetInteractor(iren);
  planeWidget2->SetRepresentation(impPlaneRep2);
  planeWidget2->On();

  /*--------------------------------------------------------------------------*/
  // Rendering
  /*--------------------------------------------------------------------------*/
  // Add the actors to the renderer, set the background and size
  ren1->AddActor(sph);
  ren1->AddActor(cyl);
  ren1->AddActor(cone);
  ren1->AddActor(maceActorImpPlane);
  ren1->AddActor(selectActor);
  ren1->AddActor(maceActor);
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(600, 600);

  // Configure the box widget
  boxWidget->SetProp3D(maceActor);
  boxWidget->PlaceWidget();

  // render the image
  iren->Initialize();
  double extent[6] = { -2, 7, -2, 7, -1, 1 };
  ren1->ResetCamera(extent);
  renWin->Render();
  widget->On();
  boxWidget->On();
  iren->Start();

  return EXIT_SUCCESS;
}
