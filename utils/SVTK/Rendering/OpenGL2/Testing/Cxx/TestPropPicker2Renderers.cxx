/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImageTracerWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkActor.h>
#include <svtkAssemblyPath.h>
#include <svtkCamera.h>
#include <svtkCubeSource.h>
#include <svtkInteractorEventRecorder.h>
#include <svtkInteractorStyleTrackballCamera.h>
#include <svtkNew.h>
#include <svtkObjectFactory.h>
#include <svtkPoints.h>
#include <svtkPolyData.h>
#include <svtkPolyDataMapper.h>
#include <svtkPolyDataNormals.h>
#include <svtkPolyDataReader.h>
#include <svtkPropPicker.h>
#include <svtkProperty.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkRendererCollection.h>
#include <svtkSphereSource.h>

bool corner = true;
// bool corner = false;

double sphereColor[3] = { 0.73, 0.33, 0.83 };
double sphereColorPicked[3] = { 1.0, 1., 0.0 };
double sphereColor2[3] = { 0.33, 0.73, 0.83 };

// Handle mouse events
class MouseInteractorStyle2 : public svtkInteractorStyleTrackballCamera
{
public:
  static MouseInteractorStyle2* New();
  svtkTypeMacro(MouseInteractorStyle2, svtkInteractorStyleTrackballCamera);

  void OnLeftButtonDown() override
  {
    int* clickPos = this->GetInteractor()->GetEventPosition();

    svtkRenderWindow* renwin = this->GetInteractor()->GetRenderWindow();
    svtkRenderer* aren = this->GetInteractor()->FindPokedRenderer(clickPos[0], clickPos[1]);

    svtkNew<svtkPropPicker> picker2;
    if (0 != picker2->Pick(clickPos[0], clickPos[1], 0, aren))
    {
      svtkAssemblyPath* path = picker2->GetPath();
      svtkProp* prop = path->GetFirstNode()->GetViewProp();
      svtkActor* actor = svtkActor::SafeDownCast(prop);
      actor->GetProperty()->SetColor(sphereColorPicked);
    }
    else
      renwin->SetCurrentCursor(SVTK_CURSOR_DEFAULT);

    renwin->Render();
  }

private:
};

svtkStandardNewMacro(MouseInteractorStyle2);

void InitRepresentation(svtkRenderer* renderer)
{
  // Sphere
  svtkNew<svtkSphereSource> sphereSource;
  sphereSource->SetPhiResolution(24);
  sphereSource->SetThetaResolution(24);
  sphereSource->SetRadius(1.75);
  sphereSource->Update();

  svtkNew<svtkActor> sphere;
  svtkNew<svtkPolyDataMapper> sphereM;
  sphereM->SetInputConnection(sphereSource->GetOutputPort());
  sphereM->Update();
  sphere->SetMapper(sphereM);
  sphere->GetProperty()->BackfaceCullingOff();
  sphere->GetProperty()->SetColor(sphereColor);
  sphere->SetPosition(0, 0, 2);
  renderer->AddActor(sphere);
}

const char PropPickerEventLog[] = "# StreamVersion 1.1\n"
                                  "LeftButtonPressEvent 160 150 0 0 0 0\n"
                                  "LeftButtonReleaseEvent 160 150 0 0 0 0\n";

int TestPropPicker2Renderers(int, char*[])
{
  svtkNew<svtkRenderer> renderer0;
  renderer0->SetUseDepthPeeling(1);
  renderer0->SetMaximumNumberOfPeels(8);
  renderer0->LightFollowCameraOn();
  renderer0->TwoSidedLightingOn();
  renderer0->SetOcclusionRatio(0.0);

  renderer0->GetActiveCamera()->SetParallelProjection(1);

  svtkNew<svtkRenderWindow> renWin;
  renWin->SetAlphaBitPlanes(1);
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(renderer0);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  iren->LightFollowCameraOff();

  // Set the custom stype to use for interaction.
  svtkNew<MouseInteractorStyle2> istyle;

  iren->SetInteractorStyle(istyle);

  if (corner) // corner
  {
    svtkNew<svtkRenderer> renderer1;
    renderer1->SetViewport(0, 0, 0.1, 0.1);
    renWin->AddRenderer(renderer1);

    svtkNew<svtkSphereSource> sphereSource;
    svtkNew<svtkPolyDataMapper> mapper;
    mapper->SetInputConnection(sphereSource->GetOutputPort());
    mapper->Update();

    svtkNew<svtkActor> actor;
    actor->PickableOff();
    actor->SetMapper(mapper);
    renderer1->AddActor(actor);
  }

  {
    svtkNew<svtkCubeSource> reader;
    reader->SetXLength(80);
    reader->SetYLength(50);
    reader->SetZLength(1);
    reader->Update();

    svtkNew<svtkPolyDataNormals> norm;
    norm->SetInputConnection(reader->GetOutputPort());
    norm->ComputePointNormalsOn();
    norm->SplittingOff();
    norm->Update();

    svtkNew<svtkPolyDataMapper> mapper;
    mapper->ScalarVisibilityOff();
    mapper->SetResolveCoincidentTopologyToPolygonOffset();
    mapper->SetInputConnection(norm->GetOutputPort());
    mapper->Update();

    svtkNew<svtkActor> actor;
    actor->SetMapper(mapper);
    actor->GetProperty()->BackfaceCullingOff();
    actor->GetProperty()->SetColor(0.93, 0.5, 0.5);

    {
      renderer0->AddActor(actor);

      InitRepresentation(renderer0);

      renderer0->ResetCameraClippingRange();
      renderer0->ResetCamera();

      istyle->SetDefaultRenderer(renderer0);
    }

    actor->PickableOff();
  }
  renWin->SetSize(300, 300);

  svtkNew<svtkInteractorEventRecorder> recorder;
  recorder->SetInteractor(iren);
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(PropPickerEventLog);

  renWin->Render();
  recorder->Play();
  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  iren->Start();

  return EXIT_SUCCESS;
}
