/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImplicitPlaneWidget2b.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSmartPointer.h"

#include "svtkActor.h"
#include "svtkAppendPolyData.h"
#include "svtkClipPolyData.h"
#include "svtkCommand.h"
#include "svtkConeSource.h"
#include "svtkGlyph3D.h"
#include "svtkImplicitPlaneRepresentation.h"
#include "svtkImplicitPlaneWidget2.h"
#include "svtkInteractorEventRecorder.h"
#include "svtkLODActor.h"
#include "svtkPlane.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkRendererCollection.h"
#include "svtkSphereSource.h"

static double TestImplicitPlaneWidget2bPlaneOrigins[3][3] = { { 0, 10, 0 }, { 10, 0, 0 },
  { 0, 0, 0 } };

class svtkTimerCallback : public svtkCommand
{
public:
  static svtkTimerCallback* New()
  {
    svtkTimerCallback* cb = new svtkTimerCallback;
    cb->TimerId = 0;
    cb->Count = 0;
    return cb;
  }

  void Execute(svtkObject* caller, unsigned long eventId, void* callData) override
  {
    if (svtkCommand::TimerEvent == eventId)
    {
      int tid = *static_cast<int*>(callData);

      if (tid == this->TimerId)
      {
        svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::SafeDownCast(caller);
        if (iren && iren->GetRenderWindow() && iren->GetRenderWindow()->GetRenderers())
        {
          ++this->Count;

          svtkRenderer* renderer = iren->GetRenderWindow()->GetRenderers()->GetFirstRenderer();

          svtkImplicitPlaneRepresentation* rep =
            svtkImplicitPlaneRepresentation::SafeDownCast(this->Widget->GetRepresentation());
          rep->SetOrigin(TestImplicitPlaneWidget2bPlaneOrigins[this->Count % 3]);

          double b[6];
          for (unsigned int i = 0; i < 3; i++)
          {
            b[2 * i] = TestImplicitPlaneWidget2bPlaneOrigins[this->Count % 3][i] - .625;
            b[2 * i + 1] = TestImplicitPlaneWidget2bPlaneOrigins[this->Count % 3][i] + .625;
          }
          rep->PlaceWidget(b);
          renderer->ResetCamera();
          this->Widget->Render();

          std::cout << "Origin of the widget = ("
                    << TestImplicitPlaneWidget2bPlaneOrigins[this->Count % 3][0] << " "
                    << TestImplicitPlaneWidget2bPlaneOrigins[this->Count % 3][1] << " "
                    << TestImplicitPlaneWidget2bPlaneOrigins[this->Count % 3][2] << ")"
                    << std::endl;
          std::cout << "Bounds of the widget = (" << b[0] << " " << b[1] << " " << b[2] << " "
                    << b[3] << " " << b[4] << " " << b[5] << ")" << std::endl;
        }
      }
      else if (tid == this->QuitTimerId)
      {
        svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::SafeDownCast(caller);
        if (iren)
        {
          std::cout << "Calling iren->ExitCallback()..." << std::endl;
          iren->ExitCallback();
        }
      }
    }
  }

  int Count;
  int TimerId;
  int QuitTimerId;
  svtkImplicitPlaneWidget2* Widget;
};

int TestImplicitPlaneWidget2b(int, char*[])
{
  // Create a mace out of filters.
  //
  svtkSmartPointer<svtkSphereSource> sphere = svtkSmartPointer<svtkSphereSource>::New();
  svtkSmartPointer<svtkConeSource> cone = svtkSmartPointer<svtkConeSource>::New();
  svtkSmartPointer<svtkGlyph3D> glyph = svtkSmartPointer<svtkGlyph3D>::New();
  glyph->SetInputConnection(sphere->GetOutputPort());
  glyph->SetSourceConnection(cone->GetOutputPort());
  glyph->SetVectorModeToUseNormal();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.25);
  glyph->Update();

  // The sphere and spikes are appended into a single polydata.
  // This just makes things simpler to manage.
  svtkSmartPointer<svtkAppendPolyData> apd = svtkSmartPointer<svtkAppendPolyData>::New();
  apd->AddInputConnection(glyph->GetOutputPort());
  apd->AddInputConnection(sphere->GetOutputPort());

  svtkSmartPointer<svtkPolyDataMapper> maceMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  maceMapper->SetInputConnection(apd->GetOutputPort());

  svtkSmartPointer<svtkLODActor> maceActor = svtkSmartPointer<svtkLODActor>::New();
  maceActor->SetMapper(maceMapper);
  maceActor->VisibilityOn();

  // This portion of the code clips the mace with the svtkPlanes
  // implicit function. The clipped region is colored green.
  svtkSmartPointer<svtkPlane> plane = svtkSmartPointer<svtkPlane>::New();
  svtkSmartPointer<svtkClipPolyData> clipper = svtkSmartPointer<svtkClipPolyData>::New();
  clipper->SetInputConnection(apd->GetOutputPort());
  clipper->SetClipFunction(plane);
  clipper->InsideOutOn();

  svtkSmartPointer<svtkPolyDataMapper> selectMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  selectMapper->SetInputConnection(clipper->GetOutputPort());

  svtkSmartPointer<svtkLODActor> selectActor = svtkSmartPointer<svtkLODActor>::New();
  selectActor->SetMapper(selectMapper);
  selectActor->GetProperty()->SetColor(0, 1, 0);
  selectActor->VisibilityOff();
  selectActor->SetScale(1.01, 1.01, 1.01);

  // Create the RenderWindow, Renderer and both Actors
  //
  svtkSmartPointer<svtkRenderer> ren1 = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->AddRenderer(ren1);

  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  svtkSmartPointer<svtkImplicitPlaneRepresentation> rep =
    svtkSmartPointer<svtkImplicitPlaneRepresentation>::New();
  rep->SetPlaceFactor(1.25);
  rep->PlaceWidget(glyph->GetOutput()->GetBounds());

  svtkSmartPointer<svtkImplicitPlaneWidget2> planeWidget =
    svtkSmartPointer<svtkImplicitPlaneWidget2>::New();
  planeWidget->SetInteractor(iren);
  planeWidget->SetRepresentation(rep);

  ren1->AddActor(maceActor);
  ren1->AddActor(selectActor);

  // Add the actors to the renderer, set the background and size
  //
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // render the image
  //
  renWin->SetMultiSamples(0);
  iren->Initialize();
  renWin->Render();
  planeWidget->SetEnabled(1);
  renWin->Render();

  svtkSmartPointer<svtkTimerCallback> cb = svtkSmartPointer<svtkTimerCallback>::New();
  iren->AddObserver(svtkCommand::TimerEvent, cb);
  cb->TimerId = iren->CreateRepeatingTimer(2000); // 3 seconds
  cb->Widget = planeWidget;

  // And create a one shot timer to quit after 10 seconds.
  cb->QuitTimerId = iren->CreateOneShotTimer(10000);

  iren->Start();
  return EXIT_SUCCESS;
}
