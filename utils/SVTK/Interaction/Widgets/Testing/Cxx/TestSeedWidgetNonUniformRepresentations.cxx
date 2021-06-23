/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSeedWidgetNonUniformRepresentations.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the svtkSeedWidget by instantiating it with handles
// composed of varied geometric representations and constraints.
// There are 4 handles. They are composed of heterogeneous representations.
// One of them is passive and does not respond to user interaction.
// It also shows how they are placed in a non-interacitve mode (ie
// programmatically).

#include "svtkActor.h"
#include "svtkAxisActor2D.h"
#include "svtkCommand.h"
#include "svtkCoordinate.h"
#include "svtkGlyphSource2D.h"
#include "svtkHandleWidget.h"
#include "svtkInteractorEventRecorder.h"
#include "svtkMath.h"
#include "svtkOrientedPolygonalHandleRepresentation3D.h"
#include "svtkPointHandleRepresentation3D.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkProperty2D.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSeedRepresentation.h"
#include "svtkSeedWidget.h"
#include "svtkSphereSource.h"

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

// This callback is responsible for setting the seed label.
class svtkSeedNonUniformRepresentationCallback : public svtkCommand
{
public:
  static svtkSeedNonUniformRepresentationCallback* New()
  {
    return new svtkSeedNonUniformRepresentationCallback;
  }
  void Execute(svtkObject* o, unsigned long event, void*) override
  {
    svtkSeedWidget* sw = svtkSeedWidget::SafeDownCast(o);
    if (sw && event == svtkCommand::PlacePointEvent)
    {
      std::cout << "Point placed, total of:" << this->SeedRepresentation->GetNumberOfSeeds()
                << "\n";
    }
  }

  svtkSeedNonUniformRepresentationCallback()
    : SeedRepresentation(nullptr)
  {
  }
  svtkSeedRepresentation* SeedRepresentation;
};

// The actual test function
int TestSeedWidgetNonUniformRepresentations(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  SVTK_CREATE(svtkSphereSource, ss);
  SVTK_CREATE(svtkPolyDataMapper, mapper);
  SVTK_CREATE(svtkActor, actor);
  SVTK_CREATE(svtkRenderer, ren);
  SVTK_CREATE(svtkRenderWindow, renWin);
  SVTK_CREATE(svtkRenderWindowInteractor, iren);
  SVTK_CREATE(svtkSeedWidget, widget);
  SVTK_CREATE(svtkSeedRepresentation, seedRep);
  SVTK_CREATE(svtkGlyphSource2D, glyphs);
  SVTK_CREATE(svtkSeedNonUniformRepresentationCallback, scbk);

  renWin->AddRenderer(ren);
  iren->SetRenderWindow(renWin);
  mapper->SetInputConnection(ss->GetOutputPort());
  actor->SetMapper(mapper);
  ren->AddActor(actor);
  ren->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(500, 500);
  widget->SetInteractor(iren);
  widget->SetRepresentation(seedRep);
  scbk->SeedRepresentation = seedRep;
  widget->AddObserver(svtkCommand::PlacePointEvent, scbk);

  iren->Initialize();
  renWin->Render();

  widget->EnabledOn();

  // Now add some seeds programmatically...

  // First, get out out of the mode where we are interactively defining seeds
  widget->CompleteInteraction();

  // Let's add a seed of type svtkOrientedPolygonalHandleRepresentation3D with
  // a triangle glyph, facing the camera.

  SVTK_CREATE(svtkOrientedPolygonalHandleRepresentation3D, handleRep1);
  glyphs->SetGlyphType(SVTK_TRIANGLE_GLYPH);
  glyphs->SetScale(.1);
  glyphs->Update();
  handleRep1->SetHandle(glyphs->GetOutput());
  handleRep1->GetProperty()->SetColor(1, 0, 0);
  handleRep1->SetLabelVisibility(1);
  handleRep1->SetLabelText("Seed-1");
  seedRep->SetHandleRepresentation(handleRep1);
  svtkHandleWidget* handleWidget1 = widget->CreateNewHandle();
  handleWidget1->SetEnabled(1);
  double p1[3] = { .3, .3, .6 };
  seedRep->GetHandleRepresentation(0)->SetWorldPosition(p1);

  // Let's add a seed of type svtkPointHandleRepresentation3D with
  // a triangle glyph, facing the camera.

  SVTK_CREATE(svtkPointHandleRepresentation3D, handleRep2);
  handleRep2->GetProperty()->SetColor(0, 1, 0);
  seedRep->SetHandleRepresentation(handleRep2);
  svtkHandleWidget* handleWidget2 = widget->CreateNewHandle();
  handleWidget2->SetEnabled(1);
  double p2[3] = { .3, -.3, .6 };
  seedRep->GetHandleRepresentation(1)->SetWorldPosition(p2);

  // Let's add a seed of type svtkOrientedPolygonalHandleRepresentation3D with
  // a triangle glyph, facing the camera.

  SVTK_CREATE(svtkOrientedPolygonalHandleRepresentation3D, handleRep3);
  glyphs->SetGlyphType(SVTK_THICKCROSS_GLYPH);
  glyphs->Update();
  handleRep3->SetHandle(glyphs->GetOutput());
  handleRep3->GetProperty()->SetColor(1, 1, 0);
  handleRep3->SetLabelVisibility(1);
  handleRep3->SetLabelText("Seed-2");
  seedRep->SetHandleRepresentation(handleRep3);
  svtkHandleWidget* handleWidget3 = widget->CreateNewHandle();
  handleWidget3->SetEnabled(1);
  double p3[3] = { -.3, .3, .6 };
  seedRep->GetHandleRepresentation(2)->SetWorldPosition(p3);

  // Let's add a seed that does not respond to user interaction now.

  SVTK_CREATE(svtkOrientedPolygonalHandleRepresentation3D, handleRep4);
  glyphs->SetGlyphType(SVTK_DIAMOND_GLYPH);
  glyphs->Update();
  handleRep4->SetHandle(glyphs->GetOutput());
  handleRep4->GetProperty()->SetColor(1, 0, 1);
  handleRep4->SetLabelVisibility(1);
  handleRep4->SetLabelText("Passive\nSeed");
  seedRep->SetHandleRepresentation(handleRep4);
  svtkHandleWidget* handleWidget4 = widget->CreateNewHandle();
  handleWidget4->SetEnabled(1);
  handleWidget4->ProcessEventsOff();
  double p4[3] = { -.3, -.3, .6 };
  seedRep->GetHandleRepresentation(3)->SetWorldPosition(p4);

  // Render..
  renWin->Render();

  iren->Start();

  return EXIT_SUCCESS;
}
